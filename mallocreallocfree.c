/* Sources: Computer Systems a Programmers Perspective
            The C Programming Language
            http://danluu.com/malloc-tutorial/
            http://g.oswego.edu/dl/html/malloc.html
            https://www.cs.cmu.edu/~fp/courses/15213-s05/code/18-malloc/malloc.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
/*                        
    malloc, realloc, and free implementation in C.
        
    The list is searched in a first fit manner, where the first fit that is 
    found is returned. This is done recursively.

    The word size is set to four, it is aligned to multiples of eight, and the
    minimum block size twenty four. 

    Coalescing checks adjacent blocks for allocation bit, then merges if
    nearby and unallocated.
*/
team_t team = {
};                      
////////////////////////////////////////////////////////////////////////////////
// Integer constant macros

// Minimum size of a block
#define MINBLKSZ         24
// Size of a single word
#define WORDSIZE         4
// Size of a double word, 2x WORDSIZE 
#define DOUBLESIZE       8
// Set alignment; idest, currently multiples of 8
#define ALIGNMENT        8
// Account for information fields     
#define INFOFIELD        16 
// Sets error read out to true or false - ¡SIGNIFICANTLY AFFECTS THROUGHPUT!
#define VERBOSITY        0
#define CHECKVERB        0
////////////////////////////////////////////////////////////////////////////////
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(ptr) (((size_t)(ptr) + (ALIGNMENT-1)) & ~0x7)
////////////////////////////////////////////////////////////////////////////////
// Set an allocated bit (free or not) in addition to filling
#define FILLSET(size, alloc)  ((size) | (alloc))
////////////////////////////////////////////////////////////////////////////////
// Read and write a word at address p
#define GET(ptr)       (*(int *)(ptr))
#define PUT(ptr, val)  (*(int *)(ptr) = (val))
////////////////////////////////////////////////////////////////////////////////
// Return the size from address ptr
#define SIZE(ptr)      (GET(ptr) & ~0x7)
////////////////////////////////////////////////////////////////////////////////
// Footer interfacing
#define FOOTSET(ptr, val) (PUT(RETURNFOOT(ptr), (int)val)) 
#define FOOTSIZE(ptr)  (GET(RETURNFOOT(ptr)) & ~0x7)
#define FOOTALLOC(ptr) (GET(RETURNFOOT(ptr)) & 0x1)
#define RETURNFOOT(ptr)  ((void *)(ptr) + HEADSIZE(ptr) - DOUBLESIZE)
////////////////////////////////////////////////////////////////////////////////
// Returns next or previous pointer, or sets next or previous of a passed block
#define NEXTPTR(ptr)  (*(void **)(ptr + WORDSIZE))
#define PREVPTR(ptr)  (*(void **)(ptr))
#define NEXTSET(block, ptr) (NEXTPTR(block) = ptr)
#define PREVSET(block, ptr) (PREVPTR(block) = ptr)
////////////////////////////////////////////////////////////////////////////////
// Gives a pointer to the next or previous block
#define NEXTBLOCK(ptr)  ((void *)(ptr) + HEADSIZE(ptr))
#define PREVBLOCK(ptr)  ((void *)(ptr) - SIZE((void *)(ptr) - DOUBLESIZE))
////////////////////////////////////////////////////////////////////////////////
// Header interfacing
#define RETURNHEAD(ptr)  ((void *)(ptr) - WORDSIZE)
#define HEADSET(ptr, val) (PUT(RETURNHEAD(ptr), (int)val)) 
#define HEADSIZE(ptr)  (GET(RETURNHEAD(ptr)) & ~0x7)
#define HEADALLOC(ptr) (GET(RETURNHEAD(ptr)) & 0x1)
////////////////////////////////////////////////////////////////////////////////
// heapstart is initialized and set to the start of the heap for referencing
static char *heapstart = NULL; 
// listPointer is used for pointing to the list while retaining a heapstart for
// referencing
static char *listPointer = NULL;
////////////////////////////////////////////////////////////////////////////////
static void *heapAddend(size_t words);
static void move(void *ptr, size_t adjusted);
static void *firstFit(size_t adjusted);
static void *mm_coalesce(void *ptr);
static void *fitRecurs(void *ptr, size_t adjusted);
static void freeAdd(void *ptr); 
static void freeRem(void *ptr); 
static void mm_check();
////////////////////////////////////////////////////////////////////////////////
static size_t returnGreater(size_t a, size_t b){
    if(a > b){
        
        return a;
    }

    else{
        
        return b;
    }
}
////////////////////////////////////////////////////////////////////////////////
/*
 * freeAdd - Adds a passed pointer to the free list
 */
static void freeAdd(void *ptr){
    
    if(VERBOSITY == 1){
        
        printf("\nAdding %x to free list", (unsigned int)ptr);
    }
    NEXTSET(ptr, listPointer); 
    PREVSET(listPointer, ptr); 
    PREVSET(ptr, NULL); 
    listPointer = ptr;
    
    if(CHECKVERB == 1){
    
        mm_check();
    } 
}
////////////////////////////////////////////////////////////////////////////////
/*
 * freeRem - Removes a passed pointer from the free list
 */
static void freeRem(void *ptr){
    
    if(VERBOSITY == 1){
    
        printf("\nRemoving %x from free list", (unsigned int)ptr);
    }
    // If previous pointer
    if (PREVPTR(ptr) != 0){ 

        NEXTSET(PREVPTR(ptr), NEXTPTR(ptr));
    }
    // Previous is zero
    else{ 
    
        listPointer = NEXTPTR(ptr);
    }       
    
    PREVSET(NEXTPTR(ptr), PREVPTR(ptr));
    
    if(CHECKVERB == 1){
    
        mm_check();
    }
}
////////////////////////////////////////////////////////////////////////////////
/*
 *  fitRecurs - Uses recursion to search for ptr that is greater or equal to
 *              passed adjusted size, and returns. If no fit is found through
 *              bound checking, return NULL for flagging.
 */
static void *fitRecurs(void *ptr, size_t adjusted){
    /* Utilitzes mem_heap_foo functions to check for heap boundaries, if outside
       return NULL for flagging */ 
    if(ptr > mem_heap_hi() || ptr < mem_heap_lo()){
        
        if(VERBOSITY == 1){
        
            printf("\n%x is outside of heap limits", (unsigned int)ptr);
        }
        return NULL; 
    } 
    // If the passed adjusted size is smaller or equal to ptr & its unset
    if(HEADALLOC(ptr) == 0 && (adjusted <= (size_t)HEADSIZE(ptr))){
        
        if(VERBOSITY == 1){
        
            printf("\nFit found at %x for %x", (unsigned int)ptr, adjusted);
        }
        return ptr;
    }
    // Else, it is inside limits, but no fit. Recursion!
    else{
        
        ptr = NEXTPTR(ptr);
        return fitRecurs(ptr, adjusted);
    }
}
////////////////////////////////////////////////////////////////////////////////
/*
 *  firstFit - returns the first fit ptr, while traversing through the list    
 *             from start by using fitRecurs and passed listPointer.
 */ 
static void *firstFit(size_t adjusted){
    //Set ptr to the start of the list
    void *ptr = listPointer;
    void *buff;

    if(VERBOSITY == 1){
        
        printf("\nSearching for fit for %x", adjusted);
    }
    /* fitRecurs recursively iterates through list, returns NULL when outside
       heap limits */
    buff = fitRecurs(ptr, adjusted);
    
    // If it is outside limits, then a fit was not found.
    if(buff == NULL){
        
        if(VERBOSITY == 1){
        
            printf("\nFit not found for size %x", adjusted);
        }
        return NULL;
    }
    // A fit was found, return pointer
    else{
        
        if(VERBOSITY == 1){
            printf("\nFit found at %x",(unsigned int)ptr);
        }
        return buff;
    }
}
////////////////////////////////////////////////////////////////////////////////
/*
 * mm_init - initializes memory, from CMU example, initializes memory
 */
int mm_init() {
    if(VERBOSITY == 1){
        
        printf("\nmm_init executing");
    
    }
    
    if ((heapstart = mem_sbrk(48)) == NULL){
        
        if(VERBOSITY == 1){
    
            printf("\nHeap allocation error");
    
        }
        return -1;
    }

    PUT(heapstart, 0);                               
    PUT(heapstart + (1 * WORDSIZE), FILLSET(MINBLKSZ, 1));
    PUT(heapstart + (DOUBLESIZE), 0);                  
    PUT(heapstart + (WORDSIZE + DOUBLESIZE), 0);                   
    PUT(heapstart + MINBLKSZ, FILLSET(MINBLKSZ, 1));    
    PUT(heapstart + MINBLKSZ + WORDSIZE, FILLSET(0, 1)); 
    listPointer = heapstart + DOUBLESIZE; 

    if (heapAddend(MINBLKSZ) == 0){ 
        
        if(VERBOSITY == 1){
            
            printf("\nAddending to heap");
        }
        return -1;
    }
    
    if(VERBOSITY == 1){
    
        printf("\nmm_init CS");
    
        if(CHECKVERB == 1){
    
            mm_check();
        }
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////////

/*
 * mm_malloc - Uses returnGreater to return whichever is greater, aligned
 *             size or the MINBLKSZ, and attempts to allocate based on given
 *             size.
 */
void *mm_malloc(size_t size) {
    /* Set size passed to an aligned size that is at least MINBLKSZ if it is
       larger than the passed size */
    size_t adjusted = returnGreater(ALIGN(size) + ALIGNMENT, MINBLKSZ);
    size_t extended = returnGreater(adjusted, MINBLKSZ);
    char *ptr;
    
    if(VERBOSITY == 1){
    
        printf("\nCalling malloc with size %x",size);
    }
    // If the size requested is less than 0, it is not allocatable
    if (size <= 0){
    
        if(VERBOSITY == 1){
    
            printf("\nErroneous size requested for mm_malloc");
        }
        return NULL;
    }
    // If firstFit returns a non null ptr, then a fit was found
    if (((ptr = firstFit(adjusted)) != NULL)) {
    
        move(ptr, adjusted);
        
        if(CHECKVERB == 1){
        
            mm_check();
        }
        return ptr;
    }
    // heapAddend returns NULL if unable to allocate
    else if ((ptr = heapAddend(extended)) == NULL) {
        
        return NULL;
    }
    // Else, move into position and return moved pointer
    else{
        
        move(ptr, adjusted);
        
        if(CHECKVERB == 1){
        
            mm_check();
        }
        return ptr;
    }
} 

//
////////////////////////////////////////////////////////////////////////////////
/*
 *  mm_free - Sets passed ptr as free, and checks adjacent for coalescing
 *            by using the mm_coalesce function
 */
void mm_free(void *ptr){
    size_t size = HEADSIZE(ptr);
    
    if(VERBOSITY == 1){
       
        printf("\nSetting %x to free", (unsigned int)ptr);
    }
    // if passed NULL ptr, ignore
    if(ptr == NULL){
       
        return;
    }  
    else{
    
        FOOTSET(ptr, FILLSET(size, 0));
        HEADSET(ptr, FILLSET(size, 0)); 
        mm_coalesce(ptr);
    
        if(CHECKVERB == 1){
    
            mm_check();
        }
        return;
    } 
}
////////////////////////////////////////////////////////////////////////////////
/*
 * mm_realloc - Align and just passed size, then return a pointer to an area
 *              in memory where it is able to fit, hopefully by reallocation
 */
void *mm_realloc(void *ptr, size_t size){
    void *temp;
    size_t currSize = HEADSIZE(ptr); 
    size_t sizeNew = ALIGN(size + INFOFIELD);
    size_t adjusted;
    size_t next_set;
    size_t buff;
    
    if(VERBOSITY == 1){
    
        printf("\nCalling mm_realloc on %x, size of %x", 
            (unsigned int)ptr, size);
    }   
    
    // If the size is less than 0, than the size passed is erroneous
    // free the associated pointer, and return NULL for cleanup
    if(size <= 0){ 
    
        mm_free(ptr); 
        return NULL; 
    }
    // If ptr is NULL, then malloc and return the created pointer
    else if(ptr == NULL){
    
        return (ptr = mm_malloc(size));
    } 
    
    // 
    else if(sizeNew <= currSize){  
    
        return ptr; 
    } 
    else { 
        next_set = HEADALLOC(NEXTBLOCK(ptr));
        // Case 1: the size of the next block and the size of the current block
        // combined is greater than the sizeNew, and next is unallocated        
        if(next_set == 0 && (sizeNew <= (buff = currSize + HEADSIZE(NEXTBLOCK(ptr))))){ if(VERBOSITY == 1){ printf("\nRealloc Case 1 for %x", (unsigned int)ptr); } freeRem(NEXTBLOCK(ptr)); HEADSET(ptr, FILLSET(buff, 1)); FOOTSET(ptr, FILLSET(buff, 1)); if(CHECKVERB == 1){ mm_check(); } return ptr; } // Case 2: The next block is set free, and it is the last before // the end else if(next_set == 0 && ((HEADSIZE(NEXTBLOCK(NEXTBLOCK(ptr)))) == 0)){ if(VERBOSITY == 1){ printf("\nRealloc Case 2 for %x", (unsigned int)ptr); } temp = heapAddend(sizeNew - currSize + HEADSIZE( NEXTBLOCK(ptr))); adjusted = (HEADSIZE(temp) + currSize); HEADSET(ptr, FILLSET(adjusted, 1)); FOOTSET(ptr, FILLSET(adjusted, 1)); if(CHECKVERB == 1){ mm_check(); } return ptr; } // Case 3: It is last before the end else if(HEADSIZE(NEXTBLOCK(ptr)) == 0){ if(VERBOSITY == 1){ printf("\nRealloc Case 3 for %x", (unsigned int)ptr); } temp = heapAddend(sizeNew - currSize); adjusted = currSize + HEADSIZE(temp); HEADSET(ptr, FILLSET(adjusted, 1)); FOOTSET(ptr, FILLSET(adjusted, 1)); if(CHECKVERB == 1){ mm_check(); } return ptr; } // Case 4: Malloc with the new size and make it fit else { if(VERBOSITY == 1){ printf("\nRealloc Case 4 for %x", (unsigned int)ptr); } temp = mm_malloc(sizeNew); move(temp, sizeNew); memcpy(temp, ptr, sizeNew); mm_free(ptr); if(CHECKVERB == 1){ mm_check(); } return temp; } } return NULL; } //////////////////////////////////////////////////////////////////////////////// /* * mm_check - Checks for pointers that evaded coalescing, if previous is greater than current, for allocation */ static void mm_check(void){ int error = 0; void *ptr = listPointer; void *prev = NULL; size_t prevSet = FOOTALLOC(PREVBLOCK(ptr)) || PREVBLOCK(ptr) == ptr; size_t nextSet = HEADALLOC(NEXTBLOCK(ptr)); // While ptr is within limits while(ptr > mem_heap_hi() || ptr < mem_heap_lo()){ // if it is allocated if(HEADALLOC(ptr)){ printf("%x is allocated but on the free list",(unsigned int)ptr); ++error; if(VERBOSITY == 1){ printf("\n%d errors now.", error); } } // if the previous is greater than the current if(prev != 0 && prev > ptr){
            
            printf("%x's previous is greater", (unsigned int)ptr);
            ++error;
            
            if(VERBOSITY == 1){
            
                printf("\n%d errors now.", error);
            }
        }
        // If the previous, next, and current are all unallocated
        if(prevSet == 0 && nextSet == 0 && HEADALLOC(ptr) == 0){
            
            printf("\n%x escaped ternary coalescing with previous and next",
                (unsigned int)ptr);
            ++error;
            
            if(VERBOSITY == 1){
            
                printf("\n%d errors now.", error);
            }
        }
        // If the previous and current are unallocated
        if(prevSet == 0 && HEADALLOC(ptr) == 0){
            
            printf("\n%x was not coalesced with previous",(unsigned int)ptr);
            ++error;
            
            if(VERBOSITY == 1){
            
                printf("\n%d errors now.", error);
            }
        }
        // If the next and the current are unallocated
        if(nextSet == 0 && HEADALLOC(ptr) == 0){
            
            printf("\n%x was not coalesced with next",(unsigned int)ptr);
            ++error;
            
            if(VERBOSITY == 1){
            
                printf("\n%d errors now.", error);
            }
        }
        // Set prev equal to current for advancement
        prev = ptr;
        // Set ptr equal to the next for advancing
        ptr = NEXTPTR(ptr); 
        // Reset the allocated bits
        prevSet = FOOTALLOC(PREVBLOCK(ptr)) || PREVBLOCK(ptr) == ptr;
        nextSet = HEADALLOC(NEXTBLOCK(ptr));
    }
    // If the error flag has been incremented, errors occurred
    if(error != 0){
        
        printf("\n%d errors have occurred", error);
    }
    // Else, no errors occurred and the check passed
    else{
        
        printf("\n No errors have occurred!");
    }

}
////////////////////////////////////////////////////////////////////////////////
/*
 * heapAddend - adds a block to the end of the heap, coalesces and merges
 *              if neighbor is free
 */ 
static void *heapAddend(size_t word) {
    char *ptr;
    size_t size;
    
    if(VERBOSITY == 1){
    
        printf("\nAddending %x", word);
    }
    // If passed size_t is even, as proven by modulus
    if(word % 2 == 0){
    
        size = word * WORDSIZE;
    }
    // increment to make it even
    else{
    
        size = (++word * WORDSIZE);
    }
    // If size is less than the minimum, make it the minimum
    if (size < MINBLKSZ){ size = MINBLKSZ; } // use sbrk func to allocate if ((int)(ptr = mem_sbrk(size)) == -1){ return NULL; } // Free footer, free head, since addended shift epi HEADSET(ptr, FILLSET(size, 0)); FOOTSET(ptr, FILLSET(size, 0)); HEADSET(NEXTBLOCK(ptr), FILLSET(0, 1)); if(CHECKVERB == 1){ mm_check(); } if(VERBOSITY == 1){ printf("\n%x addended to heap", word); } // Return while coalescing return mm_coalesce(ptr); } //////////////////////////////////////////////////////////////////////////////// /* * move - moves a block of adjusted size to ptr; * if the size is not cohesive and is at least MINBLKSZ, split */ static void move(void *ptr, size_t adjusted){ size_t size = HEADSIZE(ptr); if(VERBOSITY == 1){ printf("Moving %x size block to start of block %x", adjusted, (unsigned int)ptr); } // Checking against MINBLKSZ if ((size - adjusted) >= MINBLKSZ) {
    
        HEADSET(ptr, FILLSET(adjusted, 1));
        FOOTSET(ptr, FILLSET(adjusted, 1));
        freeRem(ptr);
        ptr = NEXTBLOCK(ptr);
        HEADSET(ptr, FILLSET(size-adjusted, 0));
        FOOTSET(ptr, FILLSET(size-adjusted, 0));
        mm_coalesce(ptr);
    }
    else {
    
        HEADSET(ptr, FILLSET(size, 1));
        FOOTSET(ptr, FILLSET(size, 1));
        freeRem(ptr);
    }
    if(CHECKVERB == 1){
    
        mm_check();
    }
}
////////////////////////////////////////////////////////////////////////////////
/* 
 * mm_coalesce - Coalesces adjacent blocks; if either are set free, they are
 *               merged with ptr and ptr is returned.
 */
static void *mm_coalesce(void *ptr){
    size_t size = HEADSIZE(ptr);
    size_t prevSet = FOOTALLOC(PREVBLOCK(ptr)) || PREVBLOCK(ptr) == ptr;
    size_t nextSet = HEADALLOC(NEXTBLOCK(ptr));
    if(VERBOSITY == 1){
    
        printf("\nCalling mm_coalesce on %x", (unsigned int)ptr);
    }
    // If both neighbors are free ternary coalesce
    if (prevSet == 0 && nextSet == 0) {              
    
        if(VERBOSITY == 1){
    
            printf("\nBlocks before and after %x were mm_coalesced",
                (unsigned int)ptr);
        }              
        size += HEADSIZE(PREVBLOCK(ptr)) + HEADSIZE(NEXTBLOCK(ptr));
        freeRem(PREVBLOCK(ptr));
        freeRem(NEXTBLOCK(ptr));
        ptr = PREVBLOCK(ptr);
        HEADSET(ptr, FILLSET(size, 0));
        FOOTSET(ptr, FILLSET(size, 0));
    }
    // If the following block is not set allocated, coalesce
    else if (nextSet == 0) {                                    
    
        if(VERBOSITY == 1){
    
            printf("\nCoalescing block after %x",(unsigned int)ptr);
        }
        size += HEADSIZE(NEXTBLOCK(ptr));
        freeRem(NEXTBLOCK(ptr));
        HEADSET(ptr, FILLSET(size, 0));
        FOOTSET(ptr, FILLSET(size, 0));
    }
    // If the previous block is not set allocated, coalesce
    else if (prevSet == 0) {                               
    
        if(VERBOSITY == 1){
    
            printf("\nCoalescing block before %x",(unsigned int)ptr);
        }
        size += HEADSIZE(PREVBLOCK(ptr));
        ptr = PREVBLOCK(ptr);
        freeRem(ptr);
        HEADSET(ptr, FILLSET(size, 0));
        FOOTSET(ptr, FILLSET(size, 0));
    }
    freeAdd(ptr);
    if(CHECKVERB == 1){
    
        mm_check();   
    }
    return ptr;
}
////////////////////////////////////////////////////////////////////////////////
