#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"
static void place(void *, size_t );
static void *find_fit(size_t );
static void *coalesce(void *);
static void *extend_heap(size_t );
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
    ""
};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 기본 단위인 word, double word, 새로 할당받는 힙의 크기 CHUNKSIZE를 정의한다 */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

// 최댓값 구하는 함수 매크로
#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* header 및 footer 값(size + allocated) 리턴 */
#define PACK(size, alloc) ((size) | (alloc))

/* 주소 p에서의 word를 읽어오거나 쓰는 함수 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* header or footer에서 블록의 size, allocated field를 읽어온다 */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 블록 포인터 bp를 인자로 받아 블록의 header와 footer의 주소를 반환한다 */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 블록 포인터 bp를 인자로 받아 이후, 이전 블록의 주소를 리턴한다 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// 항상 prologue block을 가리키는 정적 전역 변수 설정
static char *heap_listp;

// next-fit 방식을 위한 next_listp 변수
static char *next_listp;

/**
 * 
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void * temp = bp;

    if (prev_alloc && next_alloc){
        /* CASE 1. 이전 블록과 이후 블록 모두 할당되어 있을 때 */

        return bp;
    } else if (prev_alloc && !next_alloc){
        /* CASE 2. 이전 블록은 할당 상태이고 다음 블록은 가용 상태일 때*/

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc){
        /* CASE 3. 이전 블록은 가용 상태 다음 블록은 할당 상태*/
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        if (temp == next_listp) {
            next_listp = bp;
        }
    }else {
        /* CASE 4. 이전 이후 모두 가용 상태 */

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        if (temp == next_listp) {
            next_listp = bp;
        }
    }

    next_listp = bp;
    return bp;
}

/**
 * 
 */
static void *extend_heap(size_t words) {
    size_t size;
    char *bp;

    // 더블 워드 정렬을 유지하기 위해서 홀수면 + 1을 해주고 byte로 변경해준다.
    // 짝수면 그냥 byte로 변경
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    // size를 전달해서 힙 영역을 확장한다.
    // 현재 bp에는 확장된 가용 블록의 첫 주소가 들어있다.
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    // 헤더와 풋터 그리고 에필로그 헤더를 수정한다.
    
    // 새로 추가된 가용 블록에 헤더 추가
    PUT(HDRP(bp), PACK(size, 0));
    
    // 새로 추가된 가용 블록에 풋터 추가
    PUT(FTRP(bp), PACK(size, 0));

    // 에필로그 헤더를 설정
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    
    // 연결이 필요하다면 연결
    return coalesce(bp);
}

/**
 * 초기화
 * 
*/
int mm_init(void) {
    // (void *) - 1 => 2^64
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) -1) {
        return -1;
    }

    // 패딩 설정 (for double word alignment)
    PUT(heap_listp, 0);

    // 헤더를 설정한다.
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    // 풋터를 설정한다.
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    // 에필로그 헤더를 설정한다
    PUT(heap_listp + (3*WSIZE), PACK(0,1));

    // prologue block을 가리키도록 더블 워드 만큼 이동
    heap_listp += (2 * WSIZE);
    
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
        
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    /* size: byte단위로 입력받아 메모리를 동적으로 할당 받는다. */

    size_t asize;       // 조절된 블록 크기
    size_t extendsize;  // 할당할 수 있는 메모리 공간이 없다면 늘려야하는 힙 영역의 크기
    
    char *bp;

    if (size == 0) {
        return NULL;
    }

    // 더블 워드 정렬을 만족시키기 위해서, 요청한 블록 크기를 조절한다.
    if (size <= DSIZE) {
        // 최소 16바이트 크기의 블록을 구성한다.
        // 추가되는 8바이트(2word)는 헤더, 풋터 그리고 오버헤드 포함한다.
        asize = 2 * DSIZE;
    }else {
        // 일반적인 규칙에 따라 오버헤드 바이트를 추가하고 
        // 인접 8의 배수로 반올림한다.
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    // 요청한 사이즈의 가용 블록을 발견 했을 때 맞는 블록을 찾으면 배치.
    if ((bp = find_fit(asize)) != NULL) {
        // 초과 부분을 분할한다.
        place(bp, asize);
        return bp;
    }else {
        // 요청한 사이즈의 가용 블록을 발견하지 못했을 때
        extendsize = MAX(asize, CHUNKSIZE);
        // 힙을 새로운 가용 블록으로 확장하고 , 
        // 요청한 블록을 새 가용 블록에 배치한다.
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
            return NULL;
        }

        // 필요한 경우에 블록을 분할한다.
        place(bp, asize);

        // 새롭게 할당한 블록의 포인터를 반환한다.
        return bp;
    }
}

/**
 * first fit 방식으로 빈 가용 블록 find
 */
static void *find_fit(size_t asize) {
    void *bp;

    if (next_listp == NULL) {
        next_listp = heap_listp;
    }

    for (bp = next_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            next_listp = bp;
            return bp;
        }
    }

    for (bp = heap_listp; HDRP(bp) != HDRP(next_listp); bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }        
    }

    /* 맞는 fit이 존재하지 않는다면 */
    return NULL;
}

/**
 * 
 */
static void place(void *bp, size_t asize) {
    // 현재 가용 블록의 크기
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)){
        // 요청한 크기의 가용 블록을 뺀 나머지 부분의 크기가 최소 블록의 크기(4워드 = 16 = 4칸)와 같거나 크다면?

        // 분리한 가용 블록의 헤더와 풋터를 새롭게 설정 (할당 되었으므로 1)
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);

        // 나머지 가용 블록의 헤더와 풋터를 새롭게 설정
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }else {
        // 요청한 크기의 가용 블록을 뺀 나머지 부분의 크기가 최소 블록의 크기(4워드 = 16 = 4칸)보다 작다면?

        // 해당 블록을 통째로 사용한다.
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    newptr = mm_malloc(size);

    if (newptr == NULL) {
      return NULL;
    }

    copySize = GET_SIZE(HDRP(oldptr));
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    
    if (size < copySize) {
      copySize = size;
    }

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}