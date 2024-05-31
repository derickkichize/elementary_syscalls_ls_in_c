#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

/* syscall numbers for OpenBSD XXX: "/usr/include/sys/syscall.h" */
#define BSD_SYS_GETDENTS  99
#define BSD_SYS_CHDIR	  12 
#define BSD_SYS_OPEN  	  0x5
#define BSD_SYS_CLOSE	  0x6
#define BSD_SYS_STAT	  0x26

/* flags for syscall options */
#define X_RDONLY	  0x0000
#define X_CLOEXEC 	  0x10000
#define X_DIRECTORY	  0x20000

/* definition for maximum directory entry names */
#define MAXNAMELEN	  255

/* permissions */
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100

#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010

#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001

/* option flags for priting directory entries */
#define SIMPLE_ENTRIES 	  0b00000001
#define DETAIL_ENTRIES 	  0b00000010
#define INODE_ENTRIES  	  0b00000100
#define HIDDEN_ENTRIES 	  0b00001000
#define RECURSIVE_ENTRIES 0b00010000

/* custom dirent struct */
struct x_dirent {
	ino_t 	 d_fileno;  	 	 /* file number entry */
	off_t 	 d_off;     	 	 /* offset after this entry */
	uint16_t d_reclen;  	 	 /* length of this record */
	uint8_t  d_type;    	 	 /* file type */
	uint8_t	 d_namelen; 	 	 /* length of string in d_name */
	uint8_t	 __d_padding[4]; 	 /* supress padding after d_name */
	char 	 d_name[MAXNAMELEN + 1]; /*name must be no longer than this */
};

/* custom file stat struct */
struct x_stat {
	mode_t    st_mode;
	dev_t     st_dev;
	ino_t     st_ino;
	nlink_t   st_nlink;
	uid_t 	  st_uid;
	gid_t 	  st_gid;
	dev_t 	  st_rdev;
	time_t 	  st_atime;
	long   	  st_atimensec;
	time_t 	  st_mtime;
	long   	  st_mtimensec;
	time_t 	  st_ctime;
	long   	  st_ctimensec;
	off_t  	  st_size;
	blkcnt_t  st_blocks;
	blksize_t st_blksize;
	u_int32_t st_flags;
	u_int32_t st_gen;
	time_t	  __st_birthtime;
	long	  __st_birthtimensec;
};

typedef struct x_stat stat_t;
typedef struct x_dirent dirent_t;

/*buf sizes and constants */
#define DIRBUFSIZE   512
#define CURDIR 	     "."
#define DT_DIR 	     0x4
#define MAXENTRYLINE 4096

/* function prototypes */
void print_dir_entries 	    (int opt, const char* dirname);
void print_entries          (int opt, dirent_t*   dir);
void print_detailed_entries (int opt, char* 	  name);

/* prints detailed file entries */
void
print_detailed_entries (int opt, char* name)
{
	stat_t sb;
	char* modtime;

	char* filetype[] = {
		"?","p","c","?",
		"d","?","b","?",
		"-","?","l","?","s"
	};

	if ((name[0] == '.'
	    || strcmp(name, "."  ) == 0
	    || strcmp(name, ".." ) == 0) && !(opt & HIDDEN_ENTRIES))
		return;

	if (syscall (BSD_SYS_STAT, 
		     name, &sb) == -1) {
		perror (name);
		exit (EXIT_FAILURE);
	}

	if (opt & INODE_ENTRIES)  
		printf("%lld ", sb.st_ino);

	printf("%s", filetype[(sb.st_mode >> 12) & 017]);

	printf("%c%c%c%c%c%c%c%c%c",
	       (sb.st_mode & S_IRUSR) ? 'r':'-',
	       (sb.st_mode & S_IWUSR) ? 'w':'-',
	       (sb.st_mode & S_IXUSR) ? 'x':'-',
	       (sb.st_mode & S_IRGRP) ? 'r':'-',
	       (sb.st_mode & S_IWGRP) ? 'w':'-',
	       (sb.st_mode & S_IXGRP) ? 'x':'-',
	       (sb.st_mode & S_IROTH) ? 'r':'-',
	       (sb.st_mode & S_IWOTH) ? 'w':'-',
	       (sb.st_mode & S_IXOTH) ? 'x':'-');

	printf("%lld", sb.st_size);

	modtime = ctime (&sb.st_mtime);
	modtime[strlen (modtime) - 1] = '\0';

	printf ("  %s  ", modtime);
	printf ("%s\n", name);
}


/* prints simple directory entries */
void
print_entries (int opt, dirent_t* dir)
{
	if ((dir->d_name[0] == '.'
	    || strcmp(dir->d_name, ".")  == 0
	    || strcmp(dir->d_name, "..") == 0) && !(opt & HIDDEN_ENTRIES))
		return;
	else if (opt & INODE_ENTRIES) 
		printf("%lld ", dir->d_fileno);

	printf("%s\n", dir->d_name);
}



/* prints directory entries info based on options */
void
print_dir_entries (int opt, const char* dirname)
{
	int  fd, n_entries = 0;
	char buf [DIRBUFSIZE], 
	     line [MAXENTRYLINE];

	memset(line, 0, MAXENTRYLINE);

	if (syscall (BSD_SYS_CHDIR, dirname) == -1) {
		perror("chdir");
		exit (EXIT_FAILURE);
	}

	if ((fd = syscall (BSD_SYS_OPEN, CURDIR,
			   X_RDONLY  |
			   X_CLOEXEC |
			   X_DIRECTORY)) == -1) {
		perror("open");
		exit (EXIT_FAILURE);
	}

	while ((n_entries = syscall ( BSD_SYS_GETDENTS, 
				      fd, buf, DIRBUFSIZE)) > 0) {
		int bufpos = 0;
		while (bufpos < n_entries) {
			dirent_t* entry = (dirent_t*)(buf + bufpos);

			if (opt & DETAIL_ENTRIES) 
				print_detailed_entries (opt, entry->d_name);
			else if (opt & SIMPLE_ENTRIES || opt & HIDDEN_ENTRIES) 
				print_entries (opt, entry);
			else if (opt & INODE_ENTRIES) {
				print_entries (opt, entry);
			}

			bufpos += entry->d_reclen;
		}

	}

	if (n_entries == -1) {
		perror ("getdents");
		exit (EXIT_FAILURE);
	}


	if ((syscall (BSD_SYS_CLOSE, fd) == -1)) {
		perror ("close");
		exit (EXIT_FAILURE);
	}
}


void
__usage(const char* progname)
{
	fprintf(stderr, "%s - [la] <folder>.\n", progname);
	exit (EXIT_FAILURE);
}

int 
main (int argc, char* argv[])
{
	int opt = 0, c;
	char* dirname;

	while ((c = getopt(argc, argv, "liar")) != -1) {
		switch (c) {
		case 'r':
		case 'R':
			opt |= RECURSIVE_ENTRIES;
			break;
		case 'i':
			opt |= INODE_ENTRIES;
			break;
		case 'l':
			opt |= DETAIL_ENTRIES;
			break;
		case 'a':
			opt |= HIDDEN_ENTRIES;
			break;
		default:
			__usage(argv[0]);
		}
	}

	if (opt == 0) opt |= SIMPLE_ENTRIES;

	dirname = (optind < argc && argv[optind] != NULL) ?
		argv[optind] : CURDIR;

	print_dir_entries(opt, dirname);
	return 0;
}
