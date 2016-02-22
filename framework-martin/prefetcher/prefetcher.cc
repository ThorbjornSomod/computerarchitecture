#include "interface.hh"

#define TABLE_SIZE 128

#define NEW 0
#define STEADY 1

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
	table[entry].state = NEW;
	table[entry].prevAddr = stat.mem_addr;
	table[entry].stride = 0;
	table[entry].tag = stat.pc;
    } else {
	switch(table[entry].state) {
	case NEW:
	    table[entry].stride = stat.mem_addr - table[entry].prevAddr;
	    table[entry].prevAddr = stat.mem_addr;
	    table[entry].state = STEADY;
	    break;

	case STEADY:
	    if ((stat.mem_addr - table[entry].prevAddr) == table[entry].stride) {
		if (!in_cache(stat.mem_addr + table[entry].stride)) {
		    issue_prefetch(stat.mem_addr + table[entry].stride);
		}
	    }
	    table[entry].prevAddr = stat.mem_addr;
	    table[entry].stride = stat.mem_addr - table[entry].prevAddr;
	    break;
	}
    }
}

void prefetch_complete(Addr addr) {
    /*
	 * Called when a block requested by the prefetcher has been loaded.
	 */
    }
