#include "lab4.h"
#include <getopt.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
/*
    HEADER
    File: csim.c
    Author: Cameron J. Skidmore
    M#: M13455725

*/

/* 
 * printSummary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded. 
 */

//Sets up global definitions for largest that cache could be
#define MAX_NUM_SETS 32
#define MAX_NUM_LINES 5
#define MAX_NUM_BYTES 32

//Global variables for hits, misses, evictions
int hits = 0;
int misses = 0;
int evictions = 0;

//A structure for each line in each set
typedef struct {
    int valid;
    int tag;
    int dirty;
    char data[MAX_NUM_BYTES];
} cache_line;

//A structure for each set
typedef struct {
    cache_line blocks[MAX_NUM_LINES];
} cache_set;

//The master structure for each cache
typedef struct {
    int num_sets;
    int associativity;
    int block_size;
    cache_set sets[MAX_NUM_SETS];
} cache;

//Initializes the cache. Sets everything as 0s
void init_cache(cache *c,int num_sets,int associativity,int block_size) {
    c->associativity = associativity;
    c->block_size = block_size;
    c->num_sets = num_sets;
    
    int i, j;
    for (i = 0; i < c->num_sets; i++) {
        for (j = 0; j < c->associativity; j++) {
            c->sets[i].blocks[j].valid = 0;
            c->sets[i].blocks[j].tag = 0;
            c->sets[i].blocks[j].dirty = 0;
            //memset(c->sets[i].blocks[j].data, 0, sizeof(c->sets[i].blocks[j].data));
        }
    }
}

//The main section of the simulator,
//Decodes the address and instruction of valgrind trace,
// Checks that place in the cache and tallies hits, misses, and evictions
void access_cache(cache *c, char type, unsigned long long address, int size) {
    //Gets the seperate bit sections from the address
    int set_index = address << 27;
    set_index = (int)((unsigned int)set_index >> 30);

    //printf("Set index: %i\n", set_index);
    int tag = address / (c->block_size * c->num_sets);
    //printf("set: %x, tag: %x\t", set_index, tag);

    //printf("Tag: %i\n", tag);
    //printf("Type: %c\n", type);
    
    //Initializes variables
    int i;

    //Gets target set from the cache
    cache_set *set = &(c->sets[set_index]);
    for (i = 0; i < c->associativity; i++) {
        cache_line *block = &(set->blocks[i]);
        //If the tag is found
        if (block->tag == tag) {
            //If loading, miss or hit
            if(type == 'L'){
                ////printf("cas 3\n");
                if(block->valid){
                    hits++;
                  //printf("hit\n");
                } else {
                    misses++;
                   //printf("miss\n");
                    block->valid = 1;
                }
                return;
            }
            //If modifying
            else if(type == 'M'){
                //If hit
                if(block->valid){
                    //printf("cas 0\n");
                    hits++;
                    hits++;
                   //printf("hit hit\n");
                    //memcpy(block->data, &type, size);
                    return;

                }
                //If modifying empty value, misses, writes, hits
                else {
                    //printf("cas 1\n");
                    misses++;
                   //printf("miss hit\n");
                    //memcpy(block->data, &type, size);
                    block->valid = 1;
                    hits++;
                    return;
                }
            }
            //If storing
            else {
                //printf("cas4\n");
                hits++;
               //printf("hit\n");
                //memcpy(block->data, &type, size);
                block->valid = 1;
                return;
            }
        }
        //If current line is not the tag
        else {
            //If directly mapped cache
            if(c->associativity == 1){
                //If modyifying
                if(type == 'M'){
                    misses++;
                    evictions++;
                    hits++;
                    //memcpy(block->data, &type, size);
                    block->tag = tag;
                    block->valid = 1;
                    //hits++;
                   //printf("miss evict hit\n");
                    return;
                }
                //If setting
                else if(type == 'S'){
                    misses++;
                    if(block->valid == 1){
                        evictions++;
                        //printf("valid bit\n");
                       //printf("miss evict\n");
                    } else {
                       //printf("miss\n");
                    }
                    block->tag = tag;
                    block->valid = 1;
                    return;
                }
                //If loading 
                else {
                    misses++;
                    evictions++;
                    //memcpy(block->data, &type, size);
                    block->tag = tag;
                    block->valid = 1;
                   //printf("miss evict\n");
                    return;

                }
            }
            //If not directly mapped, checks we've reached the end of lines, if so, misses, evicts LRU and replaces
            else{
                if(i == c->associativity){
                    misses++;
                    evictions++;
                }
                //Does nothing and checks next line
                else{
                    ;
                }
            }
        }
    }
}


//Function for decoding the current line and calling access_cache()
void decodeLine(char line[15], cache *c){
    //printf("Line: %c\n", line[1]);
    //If loading an instruction, do nothing
    if(line[0] == 'I'){
        ;
    }
    //Everything else
    else{
        //Parses the pieces of the line
        char type = line[1];
        char address[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
        int i = 3;
        int j = 0;
        while(line[i] != ','){
            address[j] = line[i];
            //printf("%c", line[i]);
            i++;
            j++;
        }
        //printf("ADDRESS: 0x%s\n", address);
        i++;

        char *sizeStr = &line[i];
        int size = atoi(sizeStr);
        unsigned long long hexAddr = (int)strtol(address, NULL, 16);
        //printf("Type: %c, Address: %x, Size: %i\n", type, hexAddr, size);

        access_cache(c, type, hexAddr, size);
    }
}

int main(int argc, char **argv){
    /*       GetOpt Section           */
    //NOTE: This section for getting the option arguments is sourced
    // from: https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    //Sets up the variables
    char *svalue = NULL;
    char *Evalue = NULL;
    char *bvalue = NULL;
    char *tvalue = NULL;
    int index;
    int x;

    //Sets error value to 0
    opterr = 0;

    //Loop to go through all arguments with a switch case to set each value dependent on which flag it uses
    while ((x = getopt (argc, argv, "s:E:b:t:")) != -1)
        switch (x){
            case 's':
                //printf("S set: %s\n", optarg);
                svalue = optarg;
                break;
            case 'E':
                Evalue = optarg;
                break;
            case 'b':
                bvalue = optarg;
                break;
            case 't':
                tvalue = optarg;
                break;
            case ':':
               //printf("Option -s%c requires an argument\n", optopt);
                return 1;
            case '?':
                if ((optopt == 's') || (optopt == 'E') || (optopt == 'b') || (optopt == 't')){
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                }
                else if (isprint (optopt)){
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                } else{
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                    return 1;
                }
            default:
                abort ();
            }
    //printf ("s = %s, E = %s, b = %s, t = %s\n", svalue, Evalue, bvalue, tvalue);
    for (index = optind; index < argc; index++)
       printf ("Non-option argument %s\n", argv[index]);

    //Creates the variables and converts the inputs to the right formats
    //Calculates the correct blocksize and number of set
    int blockBits = atoi(bvalue);
    double blockSize = pow(2, blockBits);
    int setBits = atoi(svalue);
    double sets = pow(2, setBits);
    int lines = atoi(Evalue);
    //int cacheSize = sets * lines * blockSize;
    //printf("Block Size: %f, Sets: %f, Cache Size: %i\n", blockSize, sets, cacheSize);


    /*      CREATES THE CACHE       */
    //Creates the cache
    cache cache1;

    //Initializes the cache
    init_cache(&cache1, sets, lines, blockSize);



    /*       SECTION FOR DECODING TRACE FILE          */
    //Gets the filename as a string and creates variable for the values    
    char *filename = tvalue;
    char c;

    // Opens the file
    FILE *fptr;  
    fptr = fopen(filename, "r");
    
    //Rejects if file does not exist
    if (fptr == NULL){
       //printf("Cannot open file \n");
        exit(0);
    }
  
    // Read contents from file
    c = fgetc(fptr);
    while (c != EOF){
        //Creates a char array for each line
        char line[15] = "               ";
        int i = 0;
        //Scans through and saves each line to the array
        while((c != '\n') && (c != EOF)){
            line[i] = c;
            i++;
            //Gets the next char
            c = fgetc(fptr);
        }
        //printf("Line: %s\n", line);
        decodeLine(line, &cache1);

        //Starts the next line
        c = fgetc(fptr);
        
    }
    fclose(fptr);
   //printf("\n");
    printSummary(hits, misses, evictions);
    return 0;
}
