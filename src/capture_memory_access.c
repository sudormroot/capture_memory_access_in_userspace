//
//
// all are POSIX interfaces
//
//
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <ucontext.h>

static int access_count=0;

static void *mem=NULL;


#define mfence() do {\
	asm volatile("" ::: "memory");\
} while(0)

static void pagefault_handler(int sig, siginfo_t *info, void *ctx)
{
	void *ptr = (void *) info->si_addr;

    printf("PageFault at address: %p\n", ptr);

	//we can also get CPU's registers from (ucontext_t*)ctx

	access_count++;

	//hold CPU memory access for 10S
	//sleep(10);
	
	//enable current access
	mprotect(mem, 4096, PROT_READ|PROT_WRITE);

	//after return, the CPU hardware will automatically retry.
}


int main(int argc, char **argv)
{
	struct sigaction act;
	int *ptr;
	int val = 0;

	//
	// Capture hardware interrupt (segment-fault) at userspace
	//
	// 
	// CPU hardware trigger page-segment-fault
	// --> OS handler --> deliver to userspace through sigaction interface (POSIX)
	//
	//
	
	//flush stdout immediately
	setbuf(stdout, NULL);

	//install segment-fault handler
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = pagefault_handler;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;

	//handle Segment-fault from CPU hardware
    sigaction(SIGSEGV, &act, NULL);


	//handle other memory errors
	sigaction(SIGBUS, &act, NULL);
	sigaction(SIGTRAP, &act, NULL);


	//int pagesize = getpagesize()
	//int len = pageize;

	//
	// allocate 4 pages
	//
	
	if(posix_memalign(&mem, 4096, 4096) < 0) {
		exit(-1);
	}

	memset(mem, 0, 4096);

	//try to read memory
	ptr = mem;

	*ptr = 1234;

	//mfence();

	//we capture all memory STORE instructions at hardware level
	
	//set page to write-only
	if (mprotect(mem, 4096, PROT_NONE) == -1) {
		exit(-1);
	}

	
	//test 1
	//printf("#1 before read memory at %p\n", ptr);
	for(int i =0; i < 5;i++){
		printf("#%d before read - ptr = %p\n", i, ptr);
		val = *ptr;
		printf("#%d after read  - ptr = %p\n\n", i, ptr);

		ptr += sizeof(int);
		//mfence();
		mprotect(mem, 4096, PROT_NONE);
	}

	printf("access_count=%d\n", access_count);

	return 0;
}


