#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"

Person *person_output_file;
unsigned int *knows_output_file;
unsigned short *interest_output_file;
unsigned long person_length, knows_length, interest_length;


// void remove_element(array_type *array, int index, int array_length)
// {
//    int i;
//    for(i = index; i < array_length - 1; i++) array[i] = array[i + 1];
// }



int main(int argc, char *argv[]) {
	unsigned int person_offset;
	unsigned long knows_offset, knows_offset2;

	Person *person, *knows;
	unsigned char score;
	unsigned int result_length = 0, result_idx, result_set_size = 1000;

	person_output_file   = (Person *)         mmaprw(makepath(argv[1], "person",   "bin"), &person_length);
	interest_output_file = (unsigned short *) mmaprw(makepath(argv[1], "interest", "bin"), &interest_length);
	knows_output_file    = (unsigned int *)   mmaprw(makepath(argv[1], "knows",    "bin"), &knows_length);


int personsize = person_length/sizeof(Person);
int size;

	//printf("Size persons: %d\n", personsize);
/** In this for loop we will get all the persons and maybe order them in another fasion? **/
	for (person_offset = 0; person_offset < personsize; person_offset++) {

		person = &person_output_file[person_offset];
		 


    /** In this for loop we can check if a friend lives in another city and add them together within the same persons bin file **/
    // check if friend lives in same city and likes artist 
		for (knows_offset = person->knows_first;
			knows_offset < person->knows_first + person->knows_n; 
			knows_offset++) {

			knows = &person_output_file[knows_output_file[knows_offset]];

		if (person->location != knows->location){
			printf("Person: %d does not know: %d and lives in: %d, %d\n", person->person_id, knows->person_id, person->location, knows->location );

				knows = &person_output_file[knows_output_file[knows_offset+1]];
				continue;
			 
			// person_output_file[person_offset]= person_output_file[personsize+1];


				//Update the bin file or write a new one with the correct results?
				//Maybe with MMAP updates? Don't know how...
				//Or writing a new file with the correct records. We will need to recreate loader.c


			    // if (fp != 0)
			    // {
			    //     fprintf(fp, "%d,%d,%d,%d,%d,%d,%d;\n", person->person_id, person->birthday, person->location, person->knows_first, person->knows_n, person->interests_first, person->interest_n);

			    // }

		} 
	}

}

if(mmapwritefile(person_output_file, 0, MS_SYNC) == 0){
	printf("File written back successfully");
}
else{
	printf("Error written back Failed");
}


return 0;
}

