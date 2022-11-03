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
static char *heap_listp = NULL; // 힙의 시작 주소를 가리킴
static char *root = NULL;       // 명시적 가용 리스트의 첫 노드를 가리킴

int mm_init(void){
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    // void *sss = heap_listp;

// 오류1) 루트 주소에 heap_listp를 넣어줘야함.
    root = heap_listp;
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;
// 오류 2) size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; 워드 사이즈로 하면 안됨 lines outside heap
    size = (words % 2) ? (words + 1) * DSIZE : words * DSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(SUCC_LINK(bp),0);
    PUT(PRED_LINK(bp),0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

void *mm_malloc(size_t size){
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

void mm_free(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

//오류 3) 자기 자신에 정보를 지우지 않으면 안됨.
    PUT(SUCC_LINK(ptr), 0);
    PUT(PRED_LINK(ptr), 0);
    coalesce(ptr);
}
//묵시적이랑 달라지는 부분
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // CASE 1: both prev and next are allocated
    if(prev_alloc && next_alloc){
    }
    else if(prev_alloc && !next_alloc){
        // CASE 2: prev is allocatd, next is freed
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        
        //연결을 하는 행위도 열결 당하는 입장에서는 가용리스트에서 나가라는 의미랑 같네! (정리해고 같은 느낌? 난 가만히 있는데 내 일거리를 남에게 줘버림.)
        //그러니까 freenode에서 없애는 작업을 해야하는거임.
        //묵시적일 때는 가용블럭을 리스트에 넣고 따로 관리하지 않고 가용하지 않은 블럭이랑 가용한 블럭이랑 걍 섞있고 서로의 헤더 푸터만 바라보기 때문에 
        //연결을 할 때 뭐 해주는거 없이 그냥 앞 뒤 allocate야 아니야? 만 보면 됐었음.
        remove_freenode(NEXT_BLKP(bp));        // 가용 리스트에서 다음 블럭의 연결 제거

        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0)); 
    }else if(!prev_alloc && next_alloc){
        // CASE 3: prev is freed, next is allocated
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_freenode(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0)); 
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
        bp = PREV_BLKP(bp); 
    }else{
        // CASE 4: both prev and next are freed
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); 
        remove_freenode(PREV_BLKP(bp));
        remove_freenode(NEXT_BLKP(bp));

//오류 4) 아래 두 줄 순서 뒤바꾸면 뻑남 
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); //write new_size to next_block footer
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // write new_size to prev_block header
        bp = PREV_BLKP(bp); // bp is changed
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

//오류 5) 리알록을 손대지 않으면 오류남.
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
