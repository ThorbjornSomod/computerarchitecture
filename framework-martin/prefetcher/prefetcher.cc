#include "interface.hh"

#define TABLE_SIZE 128

#define STATE_INIT 0
#define STATE_TRANSIENT 1
#define STATE_STEADY 2
#define STATE_NO_PREDICTION 3

typedef struct {
    Addr tag;
    Addr prevAddr;
    Addr stride;
    uint8_t state;
} RPTEntry;

RPTEntry table[TABLE_SIZE];

void prefetch_init(void) {
    DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_access(AccessStat stat)
{
    int entry = stat.pc % TABLE_SIZE;
    if (table[entry].tag != stat.pc) {
	table[entry].prevAddr = stat.mem_addr;
	table[entry].stride = 0;
	table[entry].tag = stat.pc;
	table[entry].state = STATE_INIT;

    } else {
	int correct = (stat.mem_addr - table[entry].prevAddr) == table[entry].stride;

	switch(table[entry].state) {
	case STATE_INIT:
	    if (!correct) {
		table[entry].stride = stat.mem_addr - table[entry].prevAddr;
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_TRANSIENT;
	    } else {
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_STEADY;
	    }
	    break;

	case STATE_TRANSIENT:
	    if (!correct) {
		table[entry].stride = stat.mem_addr - table[entry].prevAddr;
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_NO_PREDICTION;
	    } else {
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_STEADY;
	    }
	    break;

	case STATE_NO_PREDICTION:
	    if (!correct) {
		table[entry].stride = stat.mem_addr - table[entry].prevAddr;
		table[entry].prevAddr = stat.mem_addr;
	    } else {
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_TRANSIENT;
	    }
	    break;

	case STATE_STEADY:
	    if (!correct) {
		table[entry].prevAddr = stat.mem_addr;
		table[entry].state = STATE_INIT;
	    } else {
		table[entry].prevAddr = stat.mem_addr;
	    }
	    break;
	}

	int prefetchAddr = table[entry].prevAddr + table[entry].stride;
	if ( table[entry].state == STATE_STEADY &&
	     correct &&
	     !in_cache(prefetchAddr) ) {
	    issue_prefetch( prefetchAddr );
	}
    }
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
