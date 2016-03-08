#include "interface.hh"

#include <stdlib.h>

#define TABLE_SIZE 128
#define DELTAS_SIZE 4

#define BUFFER_ENTRY Addr

typedef struct {
    uint8_t initialized;
    int head;
    int tail;
    int cap;
    int size;
    BUFFER_ENTRY* buffer;
} CircBuffer;


typedef struct {
    Addr pc;
    Addr lastAddr;
    Addr lastPrefetch;
    CircBuffer deltas;
} DCPTEntry;


void init(CircBuffer* buffer, int cap) {
    buffer->initialized = 1;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->buffer = (BUFFER_ENTRY*) malloc( sizeof(BUFFER_ENTRY) * cap );
    buffer->cap = cap;
    buffer->size = 0;

    for (int i = 0; i < buffer->cap; i++) {
	buffer->buffer[i] = 0;
    }
}


int prev(CircBuffer* buffer, int entry) {
    return entry == 0 ? buffer->size-1 : entry-1;
}


int next(CircBuffer* buffer, int entry) {
    return (entry + 1) % buffer->size;
}


void destroy(CircBuffer* buffer) {
    if ( buffer->initialized ) {
	free( buffer->buffer );
    }
    buffer->initialized = 0;
}


int min(int a, int b) {
    return a > b ? b : a;
}


void push(CircBuffer* buffer, BUFFER_ENTRY elem) {
    if (buffer->size > 0) {
	buffer->tail = (buffer->tail + 1) % buffer->cap;
    }

    buffer->buffer[buffer->tail] = elem;

    if (buffer->size == buffer->cap) {
	buffer->head = (buffer->head + 1) % buffer->cap;
    }
    buffer->size = min(buffer->size+1, buffer->cap);
}


static DCPTEntry table[TABLE_SIZE];


void prefetch_init(void) {
}


void prefetch_access(AccessStat stat) {
    int entry = stat.pc % TABLE_SIZE;
    CircBuffer* buffer = &table[entry].deltas;

    if (table[entry].pc != stat.pc) {
	destroy(buffer);
	init(buffer, DELTAS_SIZE);
	table[entry].pc = stat.pc;
    }

    push( buffer, stat.mem_addr - table[entry].lastAddr );
    table[entry].lastAddr = stat.mem_addr;

    int last = buffer->tail;
    int secLast = prev(buffer, last);

    int it;
    int hit = 0;

    for (it = buffer->head; it != secLast; it = next(buffer, it)) {
	if (buffer->buffer[it] == buffer->buffer[secLast] &&
	    buffer->buffer[next(buffer, it)] == buffer->buffer[last]) {
	    hit = 1;
	    break;
	}
    }

    if ( hit ) {
	Addr addr = table[entry].lastAddr;
	do {
	    addr += buffer->buffer[it];

	    if ( !in_cache(addr) &&
		 table[entry].lastPrefetch < addr) {
		issue_prefetch(addr);
		table[entry].lastPrefetch = addr;
	    }
	    it = next(buffer, it);
	    
	} while (it != buffer->head);
    }
}


void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
