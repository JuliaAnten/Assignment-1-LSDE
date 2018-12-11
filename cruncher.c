#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils.h"

#define QUERY_FIELD_QID 0
#define QUERY_FIELD_A1 1
#define QUERY_FIELD_A2 2
#define QUERY_FIELD_A3 3
#define QUERY_FIELD_A4 4
#define QUERY_FIELD_BS 5
#define QUERY_FIELD_BE 6

Person *person_map;
unsigned int *knows_map;
unsigned short *interest_map;
unsigned long person_length, knows_length, interest_length;
FILE *outfile;

/**
* Compares result and lines up the results.
*/
int result_comparator(const void *v1, const void *v2) {
	Result *r1 = (Result *) v1;
	Result *r2 = (Result *) v2;

	if (r1->score > r2->score)
		return -1;
	else if (r1->score < r2->score)
		return +1;
	else if (r1->person_id < r2->person_id)
		return -1;
	else if (r1->person_id > r2->person_id)
		return +1;
	else if (r1->knows_id < r2->knows_id)
		return -1;
	else if (r1->knows_id > r2->knows_id)
		return +1;
	else
		return 0;
}


/**
* Calculates the interest score.
*/
unsigned char get_score(Person *person, unsigned short areltd[]) {
	long interest_offset;
	unsigned short interest;
	unsigned char score = 0;

	for (interest_offset = person->interests_first; 
		interest_offset < person->interests_first + person->interest_n; 
		interest_offset++) {

		interest = interest_map[interest_offset];
		if (areltd[0] == interest) score++;
		if (areltd[1] == interest) score++;
		if (areltd[2] == interest) score++;
		
		if (score > 2) {
			break;
		}
	}
	return score;
}


/**
* Checks if person likes the artist.
*/
char likes_artist(Person *person, unsigned short artist) {
	long interest_offset;
	unsigned short interest;
	unsigned short likesartist = 0;

	for (interest_offset = person->interests_first; 
		interest_offset < person->interests_first + person->interest_n; 
		interest_offset++) {

		interest = interest_map[interest_offset];
		if (interest == artist) {
			likesartist = 1;
			break;
		}
	}
	return likesartist;
}


/**
* Drives the search query.
*/
void query(unsigned short qid, unsigned short artist, unsigned short areltd[], unsigned short bdstart, unsigned short bdend) {
	unsigned int person_offset;
	unsigned int result_length = 0, result_idx, result_set_size = 1000;
	unsigned long knows_offset, knows_offset2;
	unsigned char score;

	char* p_score = malloc(person_length/sizeof(Person));
	Person *person, *knows, *friend;
	Result* results = malloc(result_set_size * sizeof (Result));

	// checks birthday, interest in artist and interest score
	for (person_offset = 0; person_offset < person_length/sizeof(Person); person_offset++) {
		p_score[person_offset] = 0;
		person = &person_map[person_offset];

		if (person->birthday < bdstart || person->birthday > bdend) continue; 

		if (likes_artist(person, artist))continue;

		p_score[person_offset] = get_score(person, areltd);
	}

	// runs through all the friends of person
	for (person_offset = 0; person_offset < person_length/sizeof(Person); person_offset++) {

		friend = &person_map[person_offset];

		// checks if persons friend likes the artist
		if (!likes_artist(friend, artist))continue;

		// for friends who like the artist, make offset, checks score
		for (knows_offset = friend->knows_first; 
			knows_offset < friend->knows_first + friend->knows_n; 
			knows_offset++) {

			knows_offset2 = knows_map[knows_offset];

			person = &person_map[knows_offset2];

			if(p_score[knows_offset2] < 1)continue;

			// allocates new memory if memory is too small to save results
			if (result_length >= result_set_size) {
				result_set_size *= 2;
				results = realloc(results, result_set_size * sizeof (Result));
			}

			results[result_length].person_id = knows_offset2;
			results[result_length].knows_id = person_offset;
			results[result_length].score = p_score[knows_offset2];
			result_length++;
		}
	}

	// sort results
	qsort(results, result_length, sizeof(Result), &result_comparator);

	// write output
	for (result_idx = 0; result_idx < result_length; result_idx++) {
		person_offset = results[result_idx].person_id;
		knows_offset = results[result_idx].knows_id;
		person = &person_map[person_offset];
		knows = &person_map[knows_offset];

		//Print Results to file
		for (knows_offset2 = person->knows_first;
			knows_offset2 < person->knows_first + person->knows_n;
			knows_offset2++) {
			
			if(knows_map[knows_offset2] == knows_offset) {

				fprintf(outfile, "%d|%d|%lu|%lu\n", qid, results[result_idx].score, 
					person->person_id, knows->person_id);
			}
		}

	}
}


/**
* Makes up outline for results.
*/
void query_line_handler(unsigned char nfields, char** tokens) {
	unsigned short q_id, q_artist, q_bdaystart, q_bdayend;
	unsigned short q_relartists[3];

	q_id            = atoi(tokens[QUERY_FIELD_QID]);
	q_artist        = atoi(tokens[QUERY_FIELD_A1]);
	q_relartists[0] = atoi(tokens[QUERY_FIELD_A2]);
	q_relartists[1] = atoi(tokens[QUERY_FIELD_A3]);
	q_relartists[2] = atoi(tokens[QUERY_FIELD_A4]);
	q_bdaystart     = birthday_to_short(tokens[QUERY_FIELD_BS]);
	q_bdayend       = birthday_to_short(tokens[QUERY_FIELD_BE]);
	
	query(q_id, q_artist, q_relartists, q_bdaystart, q_bdayend);
}


int main(int argc, char *argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: [datadir] [query file] [results file]\n");
		exit(1);
	}

	// memory-map files created by loader
	person_map   = (Person *)         mmapr(makepath(argv[1], "person",   "bin"), &person_length);
	interest_map = (unsigned short *) mmapr(makepath(argv[1], "interest", "bin"), &interest_length);
	knows_map    = (unsigned int *)   mmapr(makepath(argv[1], "knows",    "bin"), &knows_length);

	outfile = fopen(argv[3], "w");  
	if (outfile == NULL) {
		fprintf(stderr, "Can't write to output file at %s\n", argv[3]);
		exit(-1);
	}

	// run through queries
	parse_csv(argv[2], &query_line_handler);

	return 0;
}
