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
// @@@@@ explicit @@@@@
#include <sys/mman.h>
#include <errno.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team 8",
    /* First member's full name */
    "D_cron",
    /* First member's email address */
    "dcron@jungle.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// Basic constants and macros
#define WSIZE 4             // 워드 = 헤더 = 풋터 사이즈(bytes)
#define DSIZE 8             // 더블워드 사이즈(bytes)
#define CHUNKSIZE (1 << 12) // heap을 이정도 늘린다(bytes)
// @@@@ explicit에서 추가 @@@@
#define MAX(x, y) ((x) > (y) ? (x) : (y))
// pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and wirte a word at address p
// p는 (void*)포인터이며, 이것은 직접 역참조할 수 없다.
#define GET(p) (*(unsigned int *)(p))              // p가 가리키는 놈의 값을 가져온다
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // p가 가리키는 포인터에 val을 넣는다

<<<<<<< HEAD
/*
 * mm_init - 초기힙을 구성하는 함수
            먼저 mem_sbrk()에 매개변수로 4 * WSIEZ를 전달하여 16 Byte만큼의 힙 공간을 늘린다.
            첫 워드(4바이트)에는 미사용 패딩 워드인 0을,
            두번째 워드에는 Prologue Header인 PACK(DSIZE, 1)을
            네번째 워드에는 Eplogue Header인 PACK(0, 1)을 넣는다.
            이 후 extend_heap()을 호출하여 CHUNKSIZE / WSIZE 만큼 힙의 크기를 늘린다. 

 *  _______________________________________                                                             ______________
 * |            |  PROLOGUE  |  PROLOGUE  |                                                            |   EPILOGUE  |
 * |   PADDING  |   HEADER   |   FOOTER   |                                                            |    HEADER   |
 * |------------|------------|------------|-----------|-----------|------------|    ...    |------------|-------------|
 * |      0     |    8 / 1   |    8 / 1   |   HEADER  |  PREDESOR |  SUCCESOR  |    ...    |   FOOTER   |    0 / 1    |
 * |------------|------------|------------|-----------|-----------|------------|    ...    |------------|-------------|
 * ^                                                                                                                 ^
 * heap_listp                                                                                                      mem_brk
 * root                                                                                                       mem_max_address
 * 
 */
int mm_init(void){
	if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){ //take 4 words from system: then initialize.
		return -1;
	}
	PUT(heap_listp, 0);                             // 미사용 패딩 워드
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // Prologue Header
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // Prologue Footer
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // Epilogue Header
	root = heap_listp;
    heap_listp += (2 * WSIZE);  // heap_listp는 힙의 시작 위치를 가리키게 한다.
	
	if(extend_heap(CHUNKSIZE / WSIZE) == NULL){ // 초기 힙의 크기를 늘린다.
		return -1;
	}
	
	return 0;
}

/*
 * extend_heap : 힙의 크기를 늘려주는 함수이다. 
            매개변수 words 를 통해 늘려주고자 하는 힙의 크기를 받는다.
            더블워드 정렬을 위해 짝수개의 DSIZE만큼의 크기를 늘린다.
            새로 추가된 힙영역은 하나의 가용 블럭이므로 
            header, footer, succesor, predesor등의 값을 초기화한다.
            이 후 coalesce()를 호출하여 전 블럭과 병합이 가능한 경우 병합을 진행한다.
 */

static void *extend_heap(size_t words){
=======
// Read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~0x7) // ~0x00000111 -> 0x11111000(얘와 and연산하면 size나옴)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //헤더+데이터+풋터 -(헤더+풋터)

// Given block ptr bp, compute address of next and previous blocks
// 현재 bp에서 WSIZE를 빼서 header를 가리키게 하고, header에서 get size를 한다.
// 그럼 현재 블록 크기를 return하고(헤더+데이터+풋터), 그걸 현재 bp에 더하면 next_bp나옴
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// @@@@ explicit에서 추가 @@@@
#define PRED_FREEP(bp) (*(void **)(bp))
#define SUCC_FREEP(bp) (*(void **)(bp + WSIZE))

static void *heap_listp = NULL; // heap 시작주소 pointer
static void *free_listp = NULL; // free list head - 가용리스트 시작부분

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
// @@@@ explicit에서 추가 @@@@
void removeBlock(void *bp);
void putFreeBlock(void *bp);

int mm_init(void)
{                              // @@@@ explicit에서 추가 @@@@
    heap_listp = mem_sbrk(24); // 24byte를 늘려주고, 함수의 시작부분을 가리키는 주소를 반환,mem_brk는 끝에 가있음
    if (heap_listp == (void *)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0);                       // Unused padding
    PUT(heap_listp + WSIZE, PACK(16, 1));     // 프롤로그 헤더 16/1
    PUT(heap_listp + 2 * WSIZE, NULL);        // 프롤로그 PRED 포인터 NULL로 초기화
    PUT(heap_listp + 3 * WSIZE, NULL);        // 프롤로그 SUCC 포인터 NULL로 초기화
    PUT(heap_listp + 4 * WSIZE, PACK(16, 1)); // 프롤로그 풋터 16/1
    PUT(heap_listp + 5 * WSIZE, PACK(0, 1));  // 에필로그 헤더 0/1
    void * sss = heap_listp;
    free_listp = heap_listp + DSIZE; // free_listp를 PRED 포인터 가리키게 초기화
    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) // word가 몇개인지 확인해서 넣으려고(DSIZE로 나눠도 됨)
        return -1;
    return 0;
}
//연결
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); //이전 블록이 할당되었는지 아닌지 0 or 1
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); //다음 블록이 할당되었는지 아닌지 0 or 1
    size_t size = GET_SIZE(HDRP(bp));                   //현재 블록 사이즈
    // @@@@ explicit에서 추가 @@@@
    // case 1 - 가용블록이 없으면 조건을 추가할 필요 없다. 맨 밑에서 freelist에 넣어줌
    // case 2
    if (prev_alloc && !next_alloc)
    {
        removeBlock(NEXT_BLKP(bp)); // @@@@ explicit에서 추가 @@@@
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0)); // header가 바뀌었으니까 size도 바뀐다!
    }
    // case 3
    else if (!prev_alloc && next_alloc)
    {
        removeBlock(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
    
    
        PUT(FTRP(bp), PACK(size, 0));
        // PUT(FTRP(bp), PACK(size,0));  // @@@@ explicit에서 추가 @@@@ - 여기 다르긴함
        // PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        // bp = PREV_BLKP(bp); //bp를 prev로 옮겨줌
    }
    // case 4
    else if (!prev_alloc && !next_alloc)
    {
        removeBlock(PREV_BLKP(bp));
        removeBlock(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); // bp를 prev로 옮겨줌
    }
    putFreeBlock(bp); // 연결이 된 블록을 free list 에 추가
    return bp;
}

// extend_heap은 두 가지 경우에 호출된다.
// 1. heap이 초기화될 때 다음 블록을 CHUNKSIZE만큼 미리 할당해준다.
// 2. mm_malloc이 적당한 맞춤(fit)을 찾지 못했을 때 CHUNKSIZE만큼 할당해준다.
//                        - - -
// heap을 CHUNKSIZE byte로 확장하고 초기 가용 블록을 생성한다.
// 여기까지 진행되면 할당기는 초기화를 완료하고, application으로부터
// 할당과 반환 요청을 받을 준비를 완료하였다.
static void *extend_heap(size_t words)
{
>>>>>>> 35d56bc7aa75c054ebb2badca7ea3b539ec3eb38
    char *bp;
    size_t size;

    // Allocate an even number of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if (((bp = mem_sbrk(size)) == (void *)-1)) // size를 불러올 수 없으면
        return NULL;

    // Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));         // Free block header
    PUT(FTRP(bp), PACK(size, 0));         // Free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // New epilogue header

    // Coalesce(연결후 합침)
    return coalesce(bp);
}
<<<<<<< HEAD

/*
 * mm_malloc : <stdlib.h>에 있는 malloc()를 explicit 방식으로 구현했다.
            동적 메모리를 할당하는 함수이다. 
            asize는 adjust size로 더블워드 정렬을 위해 크기를 조정한다. 
 */

void *mm_malloc(size_t size){   
    size_t asize; // adjusted size.
    size_t esize; // extend size. if it doesn't fits, extend it to CHUNKSIZE.
    char *bp;
    
	if(size <= 0){  // 할당 사이즈가 0보다 같거나 작으면 NULL을 반환한다.
		return NULL;
	} 

    if(size <= DSIZE){  // 할당할 사이즈가 8보다 작으면 asize를 16 Byte으로 한다.
        asize = 2 * DSIZE;  
    }
    else{
        asize = DSIZE * ((size + DSIZE + DSIZE-1) / DSIZE); // 더블 워드 정렬을 위해 size보다 크거나 같은 8의 배수로 크기를 재조정
    }
    if((bp = find_fit(asize)) != NULL){ // free-list에서 size보다 큰 리스트 탐색 
        place(bp, asize);               // 탐색에 성공하면 place()를 통해 할당
        return bp;
    }

    // 할당할 가용 블럭을 찾지 못하면 힙의 크기를 늘린다.
    esize = MAX(asize, CHUNKSIZE);      // 할당할 크기와 미리 선언된 CHUNSIZE를 비교하여 더 큰 크기 만큼 힙의 크기를 늘린다.
    if((bp = extend_heap(esize / DSIZE)) == NULL){ 
        return NULL;
    }
    place(bp, asize); // 확장된 힙의 블럭에 할당한다.
    return bp;
}

/*
 * mm_free : 할당되었던 블록을 가용 블럭으로 만든다.
            ptr에 지정된 블럭에 할당된 메모리 해제 및 free-list에 가용블럭 삽입한다.
            반환할 블록의 헤더를 통해 해당 블록의 크기를 확인하고
            PACK(size, 0)을 통해 할당 여부에 가용으로 나타낸다.
            free과정은 블록의 데이터를 지우지 않고, 가용 여부만 업데이트 한다.
            그 이유는 이 후 에 기용여부만 확인 후, 다시 할당이 되었을 때 
            새로 데이터의 값을 초기화하면 되기 때문이다. 따로 데이터는 지우지 않는다. 
            
 */

void mm_free(void *ptr){
    	
	size_t size = GET_SIZE(HDRP(ptr));
   
    PUT(HDRP(ptr), PACK(size, 0)); // header 및 footer에 할당 정보 0으로 변경
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(SUCC_LINK(ptr), 0);
    PUT(PRED_LINK(ptr), 0);
    coalesce(ptr); // 이전 이후 블럭의 할당 정보를 확인하여 병합하고, free-list에 추가
}

/*
 * coalesce : 현재 bp가 가리키는 블록의 이전 블록과 다음 블록의 할당 여부를 
            확인하여 가용 블럭(free)이 있다면 현재 블록과 인접 가용 블럭을 
            하나의 가용 블럭으로 합친다.
            이 때 LIFO 정책의 명시적 가용 리스트에서 새로 추가된 가용 블럭은 
            항상 가용 리스트의 제일 처음으로 연결되므로 remove_freenode()를 
            호출하여 명시적 가용 리스트에서 인접 가용 블럭의 연결을 끊어준다.
 */
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록의 할당 여부 확인 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 1 : 할당된 블록, 0 : 가용 블록
    size_t size = GET_SIZE(HDRP(bp)); // 현재 블럭의 크기 확인

    if(prev_alloc && next_alloc){ // CASE 1: 이전, 다음 블럭 모두 할당되어 있으면 합치지 않음
	}
	
    else if(prev_alloc && !next_alloc){ // CASE 2: 다음 블럭이 가용 블럭(free) 일 때
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 현재 블럭의 크기에 다음 블럭의 크기를 더함
        remove_freenode(NEXT_BLKP(bp));        // 가용 리스트에서 다음 블럭의 연결 제거
        PUT(HDRP(bp), PACK(size, 0));          // 현재 블럭의 header 및 footer에 크기 및 할당 여부 업데이트
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc){ // CASE 3: 이전 블럭이 가용 블럭(free) 일 때
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); // CASE 2와 유사
        remove_freenode(PREV_BLKP(bp));
        PUT(FTRP(bp),PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else{  // CASE 4: 이전 블럭 및 다음 블럭 모두 가용 블럭(free) 일 때 
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        remove_freenode(PREV_BLKP(bp));
        remove_freenode(NEXT_BLKP(bp));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_node(bp); // CASE에 따라 만들어진 블럭을 가용 리스트(free-list)의 가장 앞에 삽입
    return bp;
}

void *mm_realloc(void *ptr, size_t size){
    if(size <= 0){ //equivalent to mm_free(ptr).
        mm_free(ptr);
        return 0;
    }

    if(ptr == NULL){
        return mm_malloc(size); //equivalent to mm_malloc(size).
    }

    void *newp = mm_malloc(size); //new pointer.

    if(newp == NULL){
        return 0;
    }
    
    size_t oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize){
    	oldsize = size; //shrink size.
	} 
    memcpy(newp, ptr, oldsize); //cover.
    mm_free(ptr);
    return newp;
}

/*
 * insert_node : free된 가용 블럭을 free-list의 제일 앞에 삽입한다. 
 */

void insert_node(char *ptr){
    char *SUCC = GET(root);      // 루트가 가리기는 곳의 주소 확인
    if(SUCC != NULL){           // 루트가 없다면
	    PUT(PRED_LINK(SUCC), ptr); // free-list의 이전 블럭 연결을 현재의 블럭으로 연결
	}
    PUT(SUCC_LINK(ptr), SUCC);  // 현재 블럭의 다음 연결을 free-list의 시작 노드로 가리킴 
    PUT(root, ptr); // bp블럭을 루트가 가리키게 한다.
}

/*
 * remove_freenode : malloc()에서 가용 블럭을 할당할 때 혹은 
                coalesce()에서 가용 블록끼리의 병합을 진핼할 때 
                명시적 가용 리스트에서 해당 블럭의 연결을 끊고 
                이중 연결 리스트의 이전, 이후 연결을 문제 없게 이어주는 함수이다. 
 */

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

/*
 * find_fit : 할당할 블록을 최초 할당 방식으로 찾는 함수
            명시적 가용 리스트의 처음부터 마지막부분에 도달할 때까지
            가용 리스트를 탐색하면서 사이즈가 asize보다 크거나 같은 블럭을
            찾으면 그 블럭의 주소를 반환한다. 
            만약 해당 블럭을 찾지 못했다면 NULL을 반환하고 힙을 혹장한다.
 */

static void *find_fit(size_t asize){ 
    char *addr = GET(root);
    while(addr != NULL){
        if(GET_SIZE(HDRP(addr)) >= asize){
        	return addr;
		}
        addr = GET(SUCC_LINK(addr));
    }
    return NULL;
}

/*
 * place : 지정된 크기의 블럭을 find_fit을 통해 찾은 free 블럭에 배치(할당)한다.
        만약 free 블럭에서 동적할당을 받고자하는 블럭의 크기를 제하여도
        또 다른 free블럭을 만들 수 있다면(2 * DSIZE 보다 큰 경우), free 블럭을 쪼갠다.
 */
static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    remove_freenode(bp);
    
    // free 블럭에서 동적할당을 받고자하는 블럭의 크기를 제하여도
    // 또 다른 free블럭을 만들 수 있다면, free 블럭을 쪼갠다.
	if((csize - asize) >= (2 * DSIZE)){ 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp); 
        PUT(HDRP(bp), PACK(csize - asize, 0)); // header 및 footer 초기화
        PUT(FTRP(bp), PACK(csize - asize, 0)); 
        PUT(SUCC_LINK(bp), 0);  // 연결 정보 초기화
        PUT(PRED_LINK(bp), 0); 
        coalesce(bp); // 새로 생긴 블럭의 인접 블럭을 확인하여 병합
    }
    else{ // split하더라도 여유롭지 않으므로 그대로 사용
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
=======
// find_fit함수, frist-fit
static void *find_fit(size_t asize)
{
    void *bp;
    // @@@@ explicit에서 추가 @@@@
    // 가용리스트 내부의 유일한 할당블록인 프롤로그 블록을 만나면 종료
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) != 1; bp = SUCC_FREEP(bp))
    {
        if (GET_SIZE(HDRP(bp)) >= asize)
        {
            return bp;
        }
    }
    return NULL; // No fit
}
// place 함수
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    //할당블록은 freelist에서 지운다
    removeBlock(bp);
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1)); //현재 크기를 헤더에 집어넣고
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        putFreeBlock(bp); // free list 첫번째에 분할된 블럭을 넣는다.
>>>>>>> 35d56bc7aa75c054ebb2badca7ea3b539ec3eb38
    }
    else
    {
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
    size_t asize; //할당할 블록 사이즈
    size_t extendsize;
    void *bp; //얘 char *bp였는데 왜 바뀌었지?

    // Ignore spurious requests - size가 0이면 할당x
    if (size <= 0) // == 대신 <=
        return NULL;

    // Adjust block size to include overhead and alignment reqs.
    if (size <= DSIZE)     // size가 8byte보다 작다면,
        asize = 2 * DSIZE; // 최소블록조건인 16byte로 맞춤
    else                   // size가 8byte보다 크다면
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // Search the free list for a fit - 적절한 가용(free)블록을 가용리스트에서 검색
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize); //가능하면 초과부분 분할
        return bp;
    }

    // No fit found -> Get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

// LIFO 방식으로 새로 반환되거나 생성된 가용 블록을 가용리스트 맨 앞에 추가
void putFreeBlock(void *bp)
{
    SUCC_FREEP(bp) = free_listp;
    PRED_FREEP(bp) = NULL;
    PRED_FREEP(free_listp) = bp;
    free_listp = bp;
}
// free list 맨 앞에 프롤로그 블록이 존재
void removeBlock(void *bp)
{
    // 첫 번째 블록을 없앨 때
    if (bp == free_listp)
    {
        PRED_FREEP(SUCC_FREEP(bp)) = NULL;
        free_listp = SUCC_FREEP(bp);
    }
    else
    {
        SUCC_FREEP(PRED_FREEP(bp)) = SUCC_FREEP(bp);
        PRED_FREEP(SUCC_FREEP(bp)) = PRED_FREEP(bp);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    if (size <= 0)
    {
        mm_free(bp);
        return 0;
    }
    if (bp == NULL)
    {
        return mm_malloc(size);
    }
    void *newp = mm_malloc(size);
    if (newp == NULL)
    {
        return 0;
    }
    size_t oldsize = GET_SIZE(HDRP(bp));
    if (size < oldsize)
    {
        oldsize = size;
    }
    // 메모리의 특정한 부분으로부터 얼마까지의 부분을 다른 메모리 영역으로
    // 복사해주는 함수(bp로부터 oldsize만큼의 문자를 newp로 복사해라)
    memcpy(newp, bp, oldsize);
    mm_free(bp);
    return newp;
}