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
    "SWJungle_week06_10",
    /* First member's full name */
    "JongWoo Han",
    /* First member's email address */
    "jongwoo0221@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Humyung Lee",
    /* Second member's email address (leave blank if none) */
    "lhm"
};

// CONSTANTS
#define WSIZE 4     // 워드의 크기
#define DSIZE 8     // 더블 워드의 크기       
#define CHUNKSIZE (1<<12)   // 2^12 = 4KB

// MACROS
#define ALIGN(size) (((size) + (0x7) & ~0x7)    // 더블워드 정렬이기 때문에 size보다 큰 8의 배수로 올림
#define MAX(x, y) ((x) > (y) ? (x) : (y))       // x와 y 중 더 큰 값 반환
#define PACK(size, alloc) ((size) | (alloc))    // 블록의 크기와 할당 여부를 pack
#define GET(p) (*(unsigned int *)(p))               // p의 주소의 값 확인
#define PUT(p,val) (*(unsigned int *)(p) = (val))   // p의 주소에 val 값 저장
#define GET_SIZE(p) (GET(p) & ~0x7)             // 블록의 크기 반환
#define GET_ALLOC(p) (GET(p) & 0x1)             // 블록의 할당여부 반환
#define HDRP(bp) ((char *)(bp) - WSIZE)         // 블록의 footer 주소 반환
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)// 블록의 footer 주소 반환
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 이전 블록의 주소 반환
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 다음 블록의 주소 반환
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
static char *root = NULL;       // 명시적 가용 리스트의 첫 노드를 가리킴

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

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * DSIZE : words * DSIZE; // 더블워드 정렬을 위해 블록 크기 조정

    if((long)(bp = mem_sbrk(size)) == -1) // size만큼 힙을 늘릴 수 없다면 NULL 반환
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); // 추가된 free 블록의 header 정보 삽입 
    PUT(FTRP(bp), PACK(size, 0)); // 추가된 free 블록의 footer 정보 삽입
    PUT(SUCC_LINK(bp), 0);        // 추가된 free 블록의 next link 초기화
    PUT(PRED_LINK(bp), 0);        // 추가된 free 블록의 prev link 초기화
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // Epilogue header

    return coalesce(bp);          // 연속된 free 블록 합체
}

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

void mm_free(void *ptr){
	size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0)); // header 및 footer에 할당 정보 0으로 변경
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(SUCC_LINK(ptr), 0);
    PUT(PRED_LINK(ptr), 0);
    coalesce(ptr); // 이전 이후 블럭의 할당 정보를 확인하여 병합하고, free-list에 추가
}

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

void insert_node(char *bp){
    // 후입선출 기준으로 넣을거다.P830
    char *SUCC = GET(root);     // 루트가 가리키는 곳의 주소 확인
    // 루트가 존재한다면?
    if(SUCC != NULL){           

        // 후입선출이니까 지금의 루트를 갈아끼우고 낙하산으로 철현차장 자리보다 위에 꽂아야함.
        // 철현차장 선임으로 새로 연결한 블록의 payload인 bp를 넣어주면 되겠지?
	    PUT(PRED_LINK(SUCC), bp); 
	}
    //루트가 존재하지 않는다면?
    //회사에 아무도 그 일 해본 사람이 없는 경우는 새로 들어오는 bp가 '내 사수는 없어요~' 라고 말하면 같은 말이 됨.
    PUT(SUCC_LINK(bp), SUCC);  // 현재 블럭의 다음 연결을 free-list의 시작 노드로 가리킴 
    
    // 왕고(루트)자리는 bp가 차지하면 됨. 
    PUT(root, bp); // bp블럭을 루트가 가리키게 한다.
}

void remove_freenode(char *bp){
    //만약 가용리스트에서 맞는 사이즈 블록이라 할당이 된 경우라면 가용리스트 목록에서 빼줘야 할 거 아니냐 (더이상 가용하지 않게 됐으니까. 그 과정임)
    //철현(경력 20년차) - 현수(경력 10년차) - 재민(경력 3년차)가 있다고 생각하자. bp에 따라서 재민, 현수, 철현이 달라지고
    //재민, 현수, 철현 각각이 회사(가용리스트 목록)를 나오는 경우 해야할 일이 달라


    //철현처럼 사수가 없는 사람이 회사를 나가려는 상황
    if(GET(PRED_LINK(bp)) == NULL){ 
        //철현은 사수 없이 맨땅에 해딩 했음.
        if(GET(SUCC_LINK(bp)) != NULL){ //후임자는 현수가 있음.
            //그럼 철현은 현수한테 가서 현수의 사수 정보를 0으로 없애고 (야~ 나 이제 나간다~ 차장님 말고 형이라고 불러~)
            PUT(PRED_LINK(GET(SUCC_LINK(bp))), 0); 
       }
        //철현이 나가니까 이제 현수가 왕고(root)에요~.
        PUT(root, GET(SUCC_LINK(bp))); 
    }

    // 현수나 재민처럼 사수가 있는 놈이 회사를 나가려는 상황
    else{ 
         //재민, 현수 중 후임이 있는 사람이 회사를 나가려는 상황 (bp은 현수)
        if( GET(SUCC_LINK(bp)) != NULL) {
             //현수가 회사를 나오려면 재민이에게 '나이제 나가니까 모르는거 철현에게 물어봐' 니 사수는 이제 철현이야 라고 말해야함.
            PUT(PRED_LINK(GET(SUCC_LINK(bp))), GET(PRED_LINK(bp)));
        }
        // 그리고 철현에게 저 나가니 이제 재민이가 철현 부사수에요. 라고 해야할 거 아님? (철현의 후임으로 재민을 넣어야함)
        // 이 경우는 맨 뒤 재민이가 나가는 상황에도 적용 되는데 재민의 후임은 없으니까 Null이었잖아? 재민이 나가니까 이제 현수의 후임을 NUll로 바꿔주면 됨.  
        PUT(SUCC_LINK(GET(PRED_LINK(bp))), GET(SUCC_LINK(bp)));
    }

    //나간 사람도 내 사수, 후임에 대한 기억을 지워야 진정한 퇴사지!
    PUT(SUCC_LINK(bp), 0);
    PUT(PRED_LINK(bp), 0);
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


//묵시적이랑 달라지는 부분
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

/*
 * place : 지정된 크기의 블럭을 find_fit을 통해 찾은 free 블럭에 배치(할당)한다.
        만약 free 블럭에서 동적할당을 받고자하는 블럭의 크기를 제하여도
        또 다른 free블럭을 만들 수 있다면(2 * DSIZE 보다 큰 경우), free 블럭을 쪼갠다.
 */
//묵시적이랑 달라지는 부분
static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    //bp는 현재 find_fit해서 찾은 들어갈 수 있는 공간인데 왜 remove_freenode를 먼저해줄까? 이거 place함수 끝나기 바로 전에 해줘도 되나?
    //ㄴㄴ coalesce때문에 안됨.(bp를 가용리스트에서 빼줘야 새로 바뀐 연결 리스트 기준으로 coalesce가 이루어지지. freenode를 맨 뒤에 해주면 coalesce작업이 의미가 없음.)
    remove_freenode(bp);
    if ((csize - asize) >= (2*DSIZE)){ 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        
        //퇴사했으니 선임, 후임에 대한 기억을 지워야지
        PUT(SUCC_LINK(bp), 0);  
        PUT(PRED_LINK(bp), 0); 
        coalesce(bp); // 새로 생긴 블럭의 인접 블럭을 확인하여 병합
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1)); 
    }
}