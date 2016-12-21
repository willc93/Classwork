

 Programming  C  CSAPP Direct Caching Simulator Lab Solution
C, Programming
CSAPP Direct Caching Simulator Lab Solution
By will
 August 30, 2016
 

/*
Sources: 
			http://www.gnu.org/software/libc/manual/html_node/Getopt.html
			https://en.wikipedia.org/wiki/C_dynamic_memory_allocation
			Computer Systems A Programmers Perspective
*/

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "cachelab.h"
#include <math.h>
#include <strings.h>
typedef unsigned long long int address;
typedef int bool;
#define true 1
#define false 0
typedef struct  // Structure for holding command line parameters given when calling the program
{
	int S, s, E, b, B;
} parameter;

typedef struct // Structure for holding hits misses evictions
{
	int e, m, h;
} cachingResult;

typedef struct // Structure representing a line within the cache
{
	address line_Tag;
	bool is_Valid;
	int used;
	char* block;
} line_Struct;

typedef struct // Structure to nest within virtual cache, representing a set of lines
{
	line_Struct* line;
} set_Struct;

typedef struct // Structure representing a cache of sets
{
	set_Struct* cache_Sets;
}virtualCache;

line_Struct structInit();
int indexReturn(int* used, int line_n, set_Struct temp_Set, bool emptyFlag);
virtualCache cacheInit(int line_n, long long block_Sz, long long set_n, virtualCache initCache);
cachingResult cacheRun(virtualCache cache, cachingResult result, address memAdd, int line_n, int temp, int b);
void printUse();

int main(int argc, char **argv) // http://www.gnu.org/software/libc/manual/html_node/Getopt.html was used
{
	cachingResult result, resultS, resultM, resultL, resultBuff; // holds results from caching for easier passing
	result.e = 0; 
	result.h = 0; 
	result.m = 0;
	virtualCache cache_t; // cache struct
	address memAdd; // memory address var
	parameter para; // parameters from command line
	FILE *trace_In; // input trace file
	char szBuff, accessType; // buffer, variable for holding I, M, L, S
	int verbosity = 0;
	int sz = 0; 
	int temp = 0;
	long long set_n = 0;
	long long block_Sz = 0;
	printf("first while");
	while((szBuff=getopt(argc, argv, "s:E:b:t:vh")) != -1){ // while szBuff equals command line argument
		switch(szBuff){
		case 's':
			temp = atoi(optarg);
			break;
		case 'E':
			para.E = atoi(optarg);
			break;
		case 'b':
			para.b = atoi(optarg);
			break;
		case 't':
			trace_In = fopen(optarg, "r"); // sets trace_In to the input trace file, read	
			break;
		case 'v':
			verbosity = true; // set verbosity equal to true if flag is set
			break;
		default:
		case 'h':
			printUse(); // prints use if h is flagged
			exit(1);
		}
	}
	if (temp == 0 || para.E == 0 || para.b == 0 || trace_In == 0){
		printf("Incorrect parameters entered.\n"); // Defaults to no parameters, inform user
        printUse();
        exit(1);
    }
	
	set_n = pow(2.0, (float)temp); // set_n = 2^s
	temp += para.b;
	cache_t = cacheInit(para.E, block_Sz, set_n, cache_t);
	if(trace_In == 0){
		printf("\nUnable to load trace.\n");
		exit(1);
	}
	 if (trace_In != NULL) {
        while (fscanf(trace_In, " %c %llx,%d", &accessType, &memAdd, &sz) == 3){	
			if(accessType != 'I'){ // if it is not a instruction cache access, per lab guideline
				if(verbosity == true){
					resultBuff = cacheRun(cache_t, resultBuff, memAdd, para.E, temp, para.b); // Increment results correspondingly 
					switch(accessType){
					case 'M':
						resultM += resultBuff;
						break;
					case 'S':
						resultS += resultBuff;
						break;
					case 'L':
						resultL += resultBuff;
						break;
					}
					result += resultBuff;
				}
				else{
					result = cacheRun(cache_t, result, memAdd, para.E, temp, para.b); // Increment results correspondingly 
				}
			}
		}
	}
	if(verbosity == true){
		printf("Results for M\n");
		printSummary(resultM.h, resultM.m, resultM.e);
		printf("Results for S\n");
		printSummary(resultS.h, resultS.m, resultS.e);
		printf("Results for L\n");
		printSummary(resultL.h, resultL.m, resultL.e);
	}
	printSummary(result.h, result.m, result.e); // print hits, misses, and evictions from result struct
	fclose(trace_In);
}

void printUse()
{
	printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
	printf("-h              Print this help message.\n");
	printf("-v              Optional verbose flag.\n");
	printf("-s <num>        Number of set index bits.\n");
	printf("-E <num>        Number of lines per set.\n");
	printf("-b <num>        Number of block offset bits.\n");
	printf("-t <file>       Trace file.\n");
}

virtualCache cacheInit(int line_n, long long block_Sz, long long set_n, virtualCache initCache){
	set_Struct temp_Set;
	line_Struct temp_Line;
	int i, j; // i = index of line, j = index of set
	initCache.cache_Sets = (set_Struct *) malloc(sizeof(set_Struct) * set_n);
	for (j = 0; j < set_n; j++){
		temp_Set.line = (line_Struct *) malloc(sizeof(line_Struct) * line_n);
		initCache.cache_Sets[j] = temp_Set; 
		for (i = 0; i < line_n; i++){
			temp_Line.used = 0;
			temp_Line.line_Tag = 0;
			temp_Line.is_Valid = false;
			temp_Set.line[i] = temp_Line;
		}
	}
	return initCache;
}

int indexReturn(int* used, int line_n, set_Struct temp_Set, bool emptyFlag){
	int i;
	int most_Freq = temp_Set.line[0].used; // declaring variables here, since if the else statement is taken they are unnecessary
	int least_Freq = most_Freq;
	int szBuff, j = 0;
	for(i = 1; i < line_n; i++){
		szBuff = temp_Set.line[i].used;
		if(most_Freq < szBuff){
			most_Freq = szBuff;
		}
		if (least_Freq > szBuff){
			j = i;
			least_Freq = szBuff;
		}
	}

	used[1] = most_Freq;
	used[0] = least_Freq;
	if(emptyFlag){
		return j;
	}


	for(i = 0; i < line_n; i++){
		if (!temp_Set.line[i].is_Valid){
			return i;
		}
	}
	return 0;
}

cachingResult cacheRun(virtualCache cache, cachingResult result, address memAdd, int line_n, int temp, int b){
	int i,j; // used for indexing
	int tag_Sz = (64-temp); // calculates tag size based off of param s/b and 64 bit
	int missCheck = result.h; // for comparison to increment misses
	bool spaceCheck = true;
	address tag_In = memAdd >> temp;
	int index = ((memAdd << tag_Sz) >> (tag_Sz + b));
	set_Struct temp_Set = cache.cache_Sets[index]; // sets temp_Set equal to the passed cache's cache_Sets set index

	for (i = 0; i < line_n; i++){// increment thsrough line indexing
		line_Struct temp_Line = temp_Set.line[i];
		if (temp_Line.is_Valid){
			if(tag_In == temp_Line.line_Tag){ // if it is valid and the tag is the same
				temp_Line.used += 1;
				temp_Set.line[i] = temp_Line; // move used value over to cache
				result.h++; // increment hit
			}
		}
		else if (!temp_Line.is_Valid && spaceCheck) {
			spaceCheck = false; // there are empty lines
		}
	}
	
	if (result.h == missCheck){ // If they are the same, negating will equal 0 and ! on 0 is true; i.e, true equals no change between previous and now
		result.m++; // miss is found
	}
	else{ // a hit was triggered, progress to next cacheRun w/in the while loop
		return result;
	}
	int* used = (int* ) malloc(sizeof(int)*2); // variables are declared here due to the branch - they do not need to be declared unless missed
	j = indexReturn(used, line_n, temp_Set, spaceCheck); // index of empty line, or least recently used line; determined by branch off of emptyFlag bool
	temp_Set.line[j].used = used[1]+1;
	temp_Set.line[j].line_Tag = tag_In;
	
	if(spaceCheck){
		result.e++; // Since it is full, increment eviction
	}	
	else{
		printf("Space_Check\n");
		temp_Set.line[j].is_Valid = true; // if not empty, it is valid
	}
	return result;
}
