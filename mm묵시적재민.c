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
static char *heap_listp;

static void *coalesce(void *bp)
{
    //앞, 뒤 블록의 할당 여부, 사이즈를 변수화 
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // CASE 1: 앞, 뒤 꽉 참(연결 불가)
    if(prev_alloc && next_alloc)
        return bp;
    else if(prev_alloc && !next_alloc){
        // CASE 2: 뒤만 비었음

        //다음 페이로드의 헤더에서 사이즈를 가져와 지금 블록의 사이즈랑 더하고
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
        
        //지금 블록의 헤더와 푸터 사이즈를 변경하고, 할당 여부를 0으로 함
        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0));
    }else if(!prev_alloc && next_alloc){
        // CASE 3: 앞만 비었음

        //이전 페이로드의 헤더에서 사이즈를 가져와서 지금 블록 사이즈와 더하고
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
        
        //바뀐 사이즈를 현재의 푸터와 이전 블록의 헤더에 넣는다.
        PUT(FTRP(bp), PACK(size, 0)); 
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        //그리고 bp는 이제 앞에 블록의 페이로드이다.
        bp = PREV_BLKP(bp); 
    }else{
        // CASE 4: 앞, 뒤 다 비었다.

        //앞, 뒤, 현재의 사이즈를 모두 합친다.
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        //앞 블록 헤더, 뒷블록 푸터에 사이즈를 넣고
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        //bp를 앞으로 바꾼다.
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    // 더블워드 정렬 기준에 따라 16의 배수만큼 힙을 늘린다.
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 16의 배수로 늘어난 힙의 헤더와 푸터에 16/0 또는 32/0처럼 사이즈 정보와 할당 여부를 남기고 에필로그 헤더 위치를 새로 바뀐 푸터의 뒤로 옮기고 coalesce(연결) 함수를 실행한다.
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   
    // 힙 사이즈를 4워드만큼 늘리고(brk를 16바이트 증가), old_brk를 heap_list에 넣고 혹시 -1면 함수 스탑하고 리턴하라. 
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    //정렬 패딩, 프롤로그 헤더, 프롤로그 푸터, 에필로그 헤더를 넣고 heaplist를 8위치로 옮겨라.
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    //extend_heap(4)를 실행하고 혹시 null이면 -1을 리턴해라
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}


static void *find_fit(size_t size){
    void *bp;
    //할당 안 됐고, 사이즈가 요청한 사이즈보다 크거나 같은 블록의 bp(블록포인터)를 리턴
    //두 조건 중 하나가 거짓이면 다음 bp로 이동
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp= NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp))&& (size <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    //만약 find_fit에서 찾은 블록이 사용자가 요청한 크기를 떼어주고도 최소 블록 사이즈인 16보다 크다면? 
    if ((csize - asize) >= (2*DSIZE)){ 
        // 일단 사용자가 요청한 공간 떼어주고 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        //나머지는 이후에 find_fit이 찾을 수 있도록 bp를 옮겨주고 헤더정보 푸터 정보도 넣어주고 
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }else{
        // 만약 16보다 /작다면? 그냥 할당만 하고 끝.(10요청했는데 16주니까 6만큼 내부단편화)
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
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)return NULL;
    //사용자가 입력한 사이즈가 최소 공간인 16보다 작으면 쉽다. 그냥 16을 찾으면 됨.
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
    //근데 더 큰 사이즈를 요청했으면 조금 난감하다. 16의 배수로 맞추긴 해야하는데 또 너무 큰 16의 배수를 찾으면 안된다.
        //아래 식은 의미가 있어서 밑에서 따로 설명
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/DSIZE);
    
    //더블워드 정렬에 맞춰진 사이즈를 찾고, 할당한다. find_fit, place함수는 밑에서 따로 설명
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    //만약 맞는 사이즈가 힙에 없으면 새로 힙을 늘리고 늘린 힙에서 할당한다.
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE))== NULL)return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    /* implicit_find_fit */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
/*=
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size){
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

