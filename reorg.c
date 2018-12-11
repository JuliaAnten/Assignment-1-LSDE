#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define PERSON_FIELD_ID 0
#define PERSON_FIELD_BIRTHDAY 4
#define PERSON_FIELD_LOCATION 8
#define KNOWS_FIELD_PERSON 0
#define KNOWS_FIELD_FRIEND 1
#define INTEREST_FIELD_PERSON 0
#define INTEREST_FIELD_INTEREST 1

FILE   *person_out;
FILE   *interest_out;
FILE   *knows_out;

Person *person, *knows, *person_map_old, *person_map;

unsigned short *interest_map_old;

unsigned long person_length, knows_length, interest_length;
unsigned long knows_offset = 0;
unsigned long interest_offset = 0;

unsigned int *knows_map_old;
unsigned int person_offset = 0;

int main(int argc, char *argv[]) {

	// map the existing files into memory
	person_map_old   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
	interest_map_old = (unsigned short *) mmapr(makepath(argv[1], "interest", "bin"), &interest_length);
	knows_map_old    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

	// create paths to the original files
	char* person_output_file_old   = makepath(argv[1], "person",   "bin");
	char* interest_output_file_old = makepath(argv[1], "interest", "bin");
	char* knows_output_file_old    = makepath(argv[1], "knows",    "bin");

	// create paths to the new files
	char* person_output_file   = makepath(argv[1], "personnew",   "bin");
	char* interest_output_file = makepath(argv[1], "interests", "bin");
	char* knows_output_file    = makepath(argv[1], "knowsnew",    "bin");

    // create and map new knows file
	const char *filepath5 = knows_output_file;

	int fd5 = open(filepath5, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

	if (fd5 == -1) {
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}

    // stretch the file size to the size of the (mmapped) array of char
    size_t knowssize = knows_length; 

    if (lseek(fd5, knowssize-1, SEEK_SET) == -1) {
    	close(fd5);
    	perror("Error calling lseek() to 'stretch' the file");
    	exit(EXIT_FAILURE);
    }

    if (write(fd5, "", 1) == -1) {
    	close(fd5);
    	perror("Error writing last byte of the file");
    	exit(EXIT_FAILURE);
    }

    // file is going to be mmapped
    int *knowsmap = mmap(0, knowssize, PROT_READ | PROT_WRITE, MAP_SHARED, fd5, 0);
    if (knowsmap == MAP_FAILED) {
    	close(fd5);
    	perror("Error mmapping the file4");
    	exit(EXIT_FAILURE);
    }

    // checks locations, checks mutuality and omits if people aren't friends or don't live in same place
    int knows_offset2;
    for(person_offset = 0;  person_offset < person_length/sizeof(Person); person_offset++) {
    	person = &person_map_old[person_offset];

     	// check if friend lives in same city
    	for (knows_offset = person->knows_first; 
    		knows_offset < person->knows_first + person->knows_n; 
    		knows_offset++) {

    		knows = &person_map_old[knows_map_old[knows_offset]];

    	    if(person->location != knows->location)continue;

    	    knowsmap[knows_offset] = knows_map_old[knows_offset];
    	    
    		// // checks mutuality
    	 //    for (knows_offset2 = knows->knows_first;
      //           knows_offset2 < knows->knows_first + knows->knows_n;
      //           knows_offset2++) {

      //           if (knows_map_old[knows_offset2] == person_offset) {

      //               // add it to new file
                    
      //               break;
      //           }
      //       }
        }
    }

    // remove the old file and rename the new to the old file
    remove(knows_output_file_old);

    rename(knows_output_file, knows_output_file_old);

    // write knows now to disk
    if (msync(knowsmap, knowssize, MS_SYNC) == -1) {
    	perror("Could not sync the file to disk");
    }

    // free mapped memory
    if (munmap(knowsmap, knowssize) == -1) {
    	close(fd5);
    	perror("Error un-mmapping the file");
    	exit(EXIT_FAILURE);
    }  

    // closes the file
    close(fd5);

    return 0;

}


