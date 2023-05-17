/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */
static char *mem_start_brk;  // 힙 영역의 첫 바이트의 주소를 가리키는 변수
static char *mem_brk;        // 힙 영역의 마지막 바이트의 주소를 가리키는 변수
static char *mem_max_addr;   // 힙의 최대크기(20MB)의 마지막 바이트의 주소를 가리키는 변수

// 즉 할당된 가상 메모리는 mem_start_brk ~ mem_brk 사이에 위치.
// 미할당된 가상 메모리는 mem_brk 이후에 위치.


/* 
 * mem_init - initialize the memory system model
 */
void mem_init(void)
{
    /* allocate the storage we will use to model the available VM */
    // 20MB의 MAX_HEAP(힙의 최대 크기)을 malloc으로 동적 할당해온다
    // 성공 시 mem_start_brk는 할당받은 힙 영역의 첫 바이트 주소를 가리킨다.
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
        // 만약 메모리를 불러오는데 실패했다면 에러를 띄우고 프로그램을 종료.
	    fprintf(stderr, "mem_init_vm: malloc error\n");
	    exit(1);
    }

    // 힙의 최대크기(20MB)의 마지막 바이트의 주소는 힙의 첫 바이트 + MAX_HEAP이다.
    mem_max_addr = mem_start_brk + MAX_HEAP;
    // 아직 힙 영역은 비어있으므로 mem_brk = mem_start_brk이다.
    mem_brk = mem_start_brk;
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int incr) 
{
    /* incr: 인수로 입력받은 바이트 크기 만큼 힙을 늘려줌. */

    // 힙 영역을 늘리기 전 끝 포인터를 저장한다.
    char *old_brk = mem_brk;

    // 힙이 줄어들거나 최대 힙 사이즈를 초과하면 메모리 부족으로 에러 처리 & -1을 리턴한다.
    if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
        /* incr < 0 : 힙 사이즈가 줄어듬을 의미 */
        /* (mem_brk) + incr > mem_max_addr : 최대 힙 사이즈를 초과함을 의미 */
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)-1;
    }

    mem_brk += incr;        // mem_brk에 늘릴 힙 공간만큼을 더해주어 힙을 늘려준다.
    return (void *)old_brk; // 늘어난 새로운 메모리를 사용하기 위해 늘리기 전 끝 포인터를 리턴한다.
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
