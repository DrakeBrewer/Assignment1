#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

void do_du(char[], int);
void print_path(dev_t, ino_t);
void show_file_info( char *, char *, int);
long int disk_usage(char *);
int is_dir( char * );

extern int errno;
extern int optind;

int main(int argc, char **argv)
{
	int opt;
	int optionProcessed = 0; // used to stop do_du from running a second time if there were options given

	// use the getopt function to check if there are any options given during runtime
	while((opt = getopt(argc, argv, ":k")) != -1) 
    { 
        switch(opt) 
        { 
            case 'k':
				if ( optind < argc )
					do_du( argv[optind], 1 );
				else
					do_du( ".", 1 );
				// do_du(argv[optind], 1);
				optionProcessed = 1;
				break;
        } 
    } 

	// no options were given at runtime
	if (optionProcessed == 0) {
		if ( argc == 1 )
			do_du( ".", 0 );
		else
			do_du( *++argv, 0 );
	} else {

	}
    
    return 0;
}


/*
	do_du
		performs a simplified version of the du command
*/
void do_du( char dirname[], int argK )
{
	DIR	*dir_ptr;		/* the directory */
	struct dirent *direntp;		/* each entry */

	if ( ( dir_ptr = opendir( dirname ) ) == NULL )
		fprintf(stderr,"duLite: cannot access '%s': %s\n", dirname, strerror(errno));
	else
	{
		 // Process regular files and subdirectories (excluding the current directory entry)
		while ((direntp = readdir(dir_ptr)) != NULL)
		{
			if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0)
			{
				continue;
			}

			// deal with memory alloc for the full path
			char *full_path;
			full_path = (char *)malloc(strlen(dirname) + strlen(direntp->d_name) + 2);
			if (full_path == NULL) {
				perror("Malloc error.");
				return;
			}

			// reformat string
			snprintf(full_path, strlen(dirname) + strlen(direntp->d_name) + 2, "%s/%s", dirname, direntp->d_name);

			if (is_dir(full_path)) {
				do_du(full_path, argK);
			} else {
				show_file_info(full_path, direntp->d_name, argK);
			}

			free(full_path);
		}

		closedir(dir_ptr);

		// Process the current directory entry after the loop
		dir_ptr = opendir(dirname);
		if (dir_ptr != NULL)
		{
			while ((direntp = readdir(dir_ptr)) != NULL)
			{
				if (strcmp(direntp->d_name, ".") == 0)
				{
					show_file_info(dirname, direntp->d_name, argK);
					break;
				}
			}
			closedir(dir_ptr);
		}
	}
}


/*
	show_file_info
		used to print the block usage and path of a file with the given path
*/
void show_file_info(char *full_path, char *filename, int argK) 
{
	long int blocks;
	// 1024 byte blocks
	if (argK == 1) {
		blocks = disk_usage(full_path) / 2;

	// 512 byte blocks
	} else {
		blocks = disk_usage(full_path);

	}

    printf("%ld ", blocks); // Print the block usage
	printf("\t%s\n", full_path); // Print the path without a trailing
}


/*
	is_dir
		uses stat to check if the target path is a directory
*/
int is_dir( char *name )
{
	struct stat info;
	return (stat( name, &info ) != -1 && S_ISDIR(info.st_mode));
}


/*
	disk_usage
		calculates the blocks used by a given file and or directory
*/
long int disk_usage(char *path)
{
	struct stat info;

	// check for stat error
	if (stat(path, &info) == -1) {
		perror(path);
		return 0;
	}
	
	if (!S_ISDIR(info.st_mode)) {
		return info.st_blocks;
	} else {

		struct dirent *direntp;
		DIR *dir_ptr;
		long int total_usage = 0;

		if ( ( dir_ptr = opendir( path ) ) == NULL ) {
			fprintf(stderr,"dulite: cannot access %s\n", path);
		}
		
		while ((direntp = readdir(dir_ptr)) != NULL) {
			// handle hidden/linked directories then skip target dir to avoid infinite loop
			if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
				total_usage += 4;
				continue;
			}

			char *new_path;

			// Update length of path string and allocate memory accordingly
			int path_len = strlen(path) + strlen(direntp->d_name) + 2;
			new_path = (char *)malloc(path_len);

			// Check for sufficient memory (malloc() returns null)
			if (new_path == NULL) {
				perror("Malloc error.");
				return 0;
			}

			snprintf(new_path, path_len, "%s/%s", path, direntp->d_name);
			total_usage += disk_usage(new_path);

			// de-allocate memory for new_path
			free(new_path);
		}

		closedir(dir_ptr);
		return total_usage;

	}
}