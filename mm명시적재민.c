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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define PRED_LINK(bp) ((char *)(bp))         // free-list에서 이전 연결 블록 정보
#define SUCC_LINK(bp) ((char *)(bp) + WSIZE) // free-list에서 다음 연결 블록 정보
// PROTOTYPES
static void *extend_heap(size_t words); // 힙의 크기를 늘림
static void *coalesce(void *bp);        // 인접한 가용(free) 블록을 합침
static void *find_fit(size_t asize);    // 가용 리스트(free list) first-fit으로 탐색
static void place(void *bp, size_t asize); // find-fit으로 찾은 블록을 알맞게 위치한다.
void insert_node(char *ptr);            // free()를 통해 가용된 블록을 가용 리스트의 처음 자리에 삽입 (LIFO 정책)
void remove_freenode(char *ptr);        // 가용 리스트의 연결 포인터 수정  
// 
static char *heap_listp = NULL; // 힙의 시작 주소를 가리킴
static char *root = NULL;   

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    // void *sss = heap_listp;
    
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(SUCC_LINK(bp),0);
    PUT(PRED_LINK(bp),0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size <= 0)return NULL;
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/DSIZE);
    
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE))== NULL)return NULL;
    place(bp, asize);
    return bp;
}

// 오 이 부분이 묵시적일 때랑 비교해서 확 달라지네??!!
static void *find_fit(size_t size){
    char *addr = GET(root); //아직 선언은 안되었지만 아래 코드를 보면 addr은 bp를 가리키게 될거다.
    while(addr != NULL){ //addr이 NULL이면 탈출
        if( GET_SIZE(HDRP(addr)) >= size ){ //만약 addr이 가리키는 HDRP의 크기가 할당해야하는 size보다 크면
            return addr;//그때의 addr 리턴
        }
        addr = GET(SUCC_LINK(addr));//크기가 작으면? addr=bp=succ_link이므로 bp에 적힌 주소값을 새로운 addr로 함.
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if ((csize - asize) >= (2*DSIZE)){ 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1)); 
    }
}

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    /* implicit_find_fit */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void *coalesce(void *bp)
{
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

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

void insert_node(char *ptr){
    char *SUCC = GET(root);      // 루트가 가리기는 곳의 주소 확인
    if(SUCC != NULL){           // 루트가 없다면
	    PUT(PRED_LINK(SUCC), ptr); // free-list의 이전 블럭 연결을 현재의 블럭으로 연결
	}
    PUT(SUCC_LINK(ptr), SUCC);  // 현재 블럭의 다음 연결을 free-list의 시작 노드로 가리킴 
    PUT(root, ptr); // bp블럭을 루트가 가리키게 한다.
}

void remove_freenode(char *ptr){ 
	if(GET(PRED_LINK(ptr)) == NULL){
		if(GET(SUCC_LINK(ptr)) != NULL){
			PUT(PRED_LINK(GET(SUCC_LINK(ptr))), 0);
		}
		PUT(root, GET(SUCC_LINK(ptr)));
	}
	else{
		if(GET(SUCC_LINK(ptr)) != NULL){
			PUT(PRED_LINK(GET(SUCC_LINK(ptr))), GET(PRED_LINK(ptr)));
		}
		PUT(SUCC_LINK(GET(PRED_LINK(ptr))), GET(SUCC_LINK(ptr)));
	}
	PUT(SUCC_LINK(ptr), 0);
	PUT(PRED_LINK(ptr), 0);
}
