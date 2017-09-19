#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "khash.h"

#include "utils.h"

#define PERSON_FIELD_ID 0
#define PERSON_FIELD_BIRTHDAY 4
#define PERSON_FIELD_LOCATION 8
#define KNOWS_FIELD_PERSON 0
#define KNOWS_FIELD_FRIEND 1
#define INTEREST_FIELD_PERSON 0
#define INTEREST_FIELD_INTEREST 1

// hash map needs long keys (large person ids), but unsigned int is enough for person offsets
KHASH_MAP_INIT_INT64(pht, unsigned int)
khash_t(pht) *person_offsets;

FILE   *person_out;
FILE   *interest_out;
FILE   *knows_out;
Person *person_map;
Person *person, *knows;
unsigned long person_length, knows_length, interest_length;

Person *person_map_old;
unsigned int *knows_map_old;
unsigned short *interest_map_old;

unsigned long person_id = 0;
unsigned long person_id_prev = 0;
unsigned long knows_id = 0;

// person offset can be smaller, we do not have so many
unsigned int person_offset = 0;
unsigned long knows_offset = 0;
unsigned long interest_offset = 0;


int main(int argc, char *argv[]) {

	// Map the existing files into memory
	person_map_old   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
	interest_map_old = (unsigned short *) mmapr(makepath(argv[1], "interest", "bin"), &interest_length);
	knows_map_old    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

	// Create paths to the original files
	char* person_output_file_old   = makepath(argv[1], "person",   "bin");
	char* interest_output_file_old = makepath(argv[1], "interest", "bin");
	char* knows_output_file_old    = makepath(argv[1], "knows",    "bin");

	// Create paths to the new files
	char* person_output_file   = makepath(argv[1], "personnew",   "bin");
	char* interest_output_file = makepath(argv[1], "interests", "bin");
	char* knows_output_file    = makepath(argv[1], "knowsnew",    "bin");


//====================================================================================Create and map new knows file



	const char *filepath5 = knows_output_file;

	int fd5 = open(filepath5, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

	if (fd5 == -1)
	{
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}

    // Stretch the file size to the size of the (mmapped) array of char
    size_t knowssize = knows_length; // + \0 null character

    if (lseek(fd5, knowssize-1, SEEK_SET) == -1)
    {
    	close(fd5);
    	perror("Error calling lseek() to 'stretch' the file");
    	exit(EXIT_FAILURE);
    }

    
    if (write(fd5, "", 1) == -1)
    {
    	close(fd5);
    	perror("Error writing last byte of the file");
    	exit(EXIT_FAILURE);
    }


    // Now the file is ready to be mmapped.
    int *knowsmap = mmap(0, knowssize, PROT_READ | PROT_WRITE, MAP_SHARED, fd5, 0);
    if (knowsmap == MAP_FAILED)
    {
    	close(fd5);
    	perror("Error mmapping the file4");
    	exit(EXIT_FAILURE);
    }



// Go through all the locations and omit if person and knows do not live in the same place
    int knows_offset2;
    for(person_offset = 0;  person_offset < person_length/sizeof(Person); person_offset++){
    	person = &person_map_old[person_offset];

     	// check if friend lives in same city and likes artist 
    	for (knows_offset = person->knows_first; 
    		knows_offset < person->knows_first + person->knows_n; 
    		knows_offset++) {

    		knows = &person_map_old[knows_map_old[knows_offset]];

    	if(person->location != knows->location)continue;


    			// friendship must be mutual
    	for (knows_offset2 = knows->knows_first;
    		knows_offset2 < knows->knows_first + knows->knows_n;
    		knows_offset2++) {


    		if (knows_map_old[knows_offset2] == person_offset) {

			// Add it to new file
    			knowsmap[knows_offset] = knows_map_old[knows_offset];

    			break;
    		}

    	}

    	// Add it to new file
    	//knowsmap[knows_offset] = knows_map_old[knows_offset];

    }

}



// Remove the old file and rename the new to the old file
remove(knows_output_file_old);

rename(knows_output_file, knows_output_file_old);



   // Write knows now to disk
if (msync(knowsmap, knowssize, MS_SYNC) == -1)
{
	perror("Could not sync the file to disk");
}

 // Free mapped memory
if (munmap(knowsmap, knowssize) == -1)
{
	close(fd5);
	perror("Error un-mmapping the file");
	exit(EXIT_FAILURE);
}

// Closing the file
close(fd5);



return 0;

}


