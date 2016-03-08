#include "interface.hh"

#include <stdio.h>

AccessStat create(Addr pc, Addr mem_addr, Tick time, int miss) {
    return {.pc = pc,
	    .mem_addr = mem_addr,
	    .time = time,
	    .miss = miss};
}

void issue_prefetch(Addr addr) {
    printf("Prefetch issued at %lu\n", addr);
}


void get_prefetch_bit(Addr addr) {
    printf("Get prefetch bit at %lu\n", addr);
}


void set_prefetch_bit(Addr addr) {
    printf("Set prefetch bit at %lu\n", addr);
}


int in_cache(Addr addr) {
    printf("In cache called at %lu\n", addr);
    return 0;
}


int in_mshr_queue(Addr addr) {
    printf("In mshr queue called at %lu\n", addr);
    return 1;
}


int current_queue_size(void) {
    printf("Current queue size called\n");
    return 0;
}
