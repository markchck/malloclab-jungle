/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team 6",
    /* First member's full name */
    "Narin Jung",
    /* First member's email address */
    "nalynnshinwhwa@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* narin define */
/* Basic constant and macros*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x):(y))

/* Pack and write a word at address p */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer*/
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp)- WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp)-DSIZE)))

static void* heap_listp;

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // CASE 1: both prev and next are allocated
    if(prev_alloc && next_alloc)
        return bp;
    else if(prev_alloc && !next_alloc){
        // CASE 2: prev is allocatd, next is freed
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // new size = cur_block size + next_block size
        PUT(HDRP(bp), PACK(size, 0)); // write new size to cur_block header
        PUT(FTRP(bp), PACK(size, 0)); // copy it to the footer also.
    }else if(!prev_alloc && next_alloc){
        // CASE 3: prev is freed, next is allocated
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); // new size = cur_block size + prev_block size
        PUT(FTRP(bp), PACK(size, 0)); // write new size to cur_block footer
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // write new size to prev_block header
        bp = PREV_BLKP(bp); //bp is changed
    }else{
        // CASE 4: both prev and next are freed
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // newsize = cur_block size + prev_block size + next_block size
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // write new_size to prev_block header
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); //write new_size to next_block footer
        bp = PREV_BLKP(bp); // bp is changed
    }
    return bp;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment e*/
    size = (words % 2) ? (words + 1) * WSIZE : (words)*WSIZE;
    if((long) (bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Initialized free block head, footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    void *sss = heap_listp;
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // for making a free block
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

//first_fit: 40+ 40 = 80(implicit - short1)
//first_fit: 24 + 40 = 64(explicit_LIFO - short1)
//first_fit: 54 + 40 = 94(explicit_LIFO - short2)
static void *first_fit(size_t size){
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp= NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp))&& (size <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    
    /* explicit_first_fit(sorted by address)*/
    // char flag = 0;
    // for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    // {
    //     if(!GET_ALLOC(HDRP(bp))){
    //         flag = 1;
    //         break;
    //     }
    // }
    // if (flag){
    //     for (bp; GET_SIZE(bp + DSIZE) > 0; bp = (bp+WSIZE)){
    //         if (size <= GET_SIZE(HDRP(bp)))
    //         {
    //             return bp;
    //         }
    //     }
    // }
    
    return NULL;
}

//brute-force: 17+40 = 57
/*static void *best_fit(size_t size){
    void *bp;
    size_t min_gap = 12;
    void *best_one = NULL;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp))&& (size <= GET_SIZE(HDRP(bp)))){
            if (GET_SIZE(HDRP(bp))-size < min_gap){
                best_one = bp;
                min_gap = GET_SIZE(HDRP(bp)) - size;
            }
        }
    }
    return best_one;
}*/



static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    // 분할을 할것인가
    // 근데 왜 2*DSIZE일까?
    // 내 추측: 하나의 블록이 최소 차지할 공간이 2*DSIZE일 거 같다.
    // 왜냐하면 footer(4) + header(4) + alignment를 위한 (8)?
    if ((csize - asize) >= (2*DSIZE)){ 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }else{
        // 분할을 하지 않을 것인가
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1)); 
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	//     return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)return NULL;
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/DSIZE);
    
    if ((bp = first_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE))== NULL)return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    /* implicit_first_fit */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);

    /* explicit_first_fit (LIFO)*/
    // void *bp = heap_listp + WSIZE;
    // size_t tmp_size = GET_SIZE(HDRP(bp));
    // int flag = GET_ALLOC(bp);
    // PUT(HDRP(bp), PACK(size, 0));
    // PUT(FTRP(bp), PACK(size, 0));
    // PUT(HDRP(ptr), PACK(tmp_size, flag));
    // PUT(FTRP(ptr), PACK(tmp_size, flag));
    //coalesce(bp);

    /* explicit_first_fit (sorted by address) */
    // char flag = 0;
    // void *predecessor;
    // for (predecessor = ptr; predecessor > heap_listp; predecessor = PREV_BLKP(predecessor))
    // {
    //     if (!GET_ALLOC(HDRP(predecessor)))
    //     {
    //         size_t pre_size = GET_SIZE(predecessor);
    //         PUT(HDRP(ptr), PACK(size, 0));
    //         PUT(FTRP(ptr), PACK(size, 0));
    //         PUT(ptr+WSIZE, PACK(pre_size, 0));
    //         PUT(ptr + DSIZE, PACK(0, 0));

    //         PUT(predecessor + DSIZE, PACK(size, 0));
    //         flag = 1;
    //         break;
    //     }
    // }
    // if (!flag){
    //     PUT(HDRP(ptr), PACK(size, 0));
    //     PUT(FTRP(ptr), PACK(size, 0));
    // }
    // coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}