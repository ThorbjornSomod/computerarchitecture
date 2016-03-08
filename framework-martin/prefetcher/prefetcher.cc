#include "interface.hh"

#define TABLE_SIZE 128 //The size of the DCPTEntry table
#define DELTAS_SIZE 11 //The size of the circular buffer used for the deltas

#define BUFFER_ENTRY uint16_t //The data type of the deltas

/* The circular buffer data structure:
 * head: Specifies the position of the oldest element in the buffer
 * tail: Specifies the position of the newest element in the buffer
 * cap: The total capacity of the buffer
 * size: The current size of the buffer
 */
typedef struct {
    uint8_t head;
    uint8_t tail;
    uint8_t cap;
    uint8_t size;
    BUFFER_ENTRY buffer[DELTAS_SIZE];
} CircBuffer;


/* The DCPTEntry datastructure:
 * pc: Program Counter of the entry
 * lastAddr: The most recently referenced address in a load/store instruction
 * lastPrefetch: The most recently prefetched address
 * deltas: The circular buffer of address deltas
 */
typedef struct {
    Addr pc;
    Addr lastAddr;
    Addr lastPrefetch;
    CircBuffer deltas;
} DCPTEntry;


/* Initializes the buffer. Can be called several times to reset the buffer */
void init(CircBuffer* buffer) {
    buffer->head = 0;
    buffer->tail = 0;
    buffer->cap = DELTAS_SIZE;
    buffer->size = 0;
}


/* prev and next:
 * References the previous and next _position_ in the buffer.
 * Why not just do (+-) 1? Trouble when the buffer wraps around its size and 0.
 */
inline int prev(CircBuffer* buffer, int entry) {
    return entry == 0 ? buffer->size-1 : entry-1;
}


inline int next(CircBuffer* buffer, int entry) {
    return (entry + 1) % buffer->size;
}

/* Self explanatory */
inline int min(int a, int b) {
    return a > b ? b : a;
}

/* Pushes a new element onto the buffer. If the buffer is full (size == cap).
 * The oldest element is evicted
 */
void push(CircBuffer* buffer, BUFFER_ENTRY elem) {
    if (buffer->size > 0) {
	buffer->tail = (buffer->tail + 1) % buffer->cap;
    }
    if (buffer->size == buffer->cap) {
	buffer->head = (buffer->head + 1) % buffer->cap;
    }

    buffer->buffer[buffer->tail] = elem;
    buffer->size = min(buffer->size+1, buffer->cap);
}


static DCPTEntry table[TABLE_SIZE];


void prefetch_init(void) {
}


void prefetch_access(AccessStat stat) {
    int entry = stat.pc % TABLE_SIZE;
    CircBuffer* buffer = &table[entry].deltas;

    /* Checks whether there is an entry corresponding to the pc.
     * If not, it creates one, possibly deleting an older entry.
     */
    if (table[entry].pc != stat.pc) {
	init(buffer);
	table[entry].pc = stat.pc;
    }

    /* Pushes the new delta onto the buffer and updates lastAddr */
    push( buffer, stat.mem_addr - table[entry].lastAddr );
    table[entry].lastAddr = stat.mem_addr;


    /* This for loop checks for pattern matches corresponding to
     * the two most recent deltas in the buffer.
     * If found, it breaks and saves the position where they were found
     * in the variable it.
     */
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

    /*
     * If there was a pattern match earlier,
     * We prefetch _all_ adresses matching the pattern found
     * for future addresses.
     * The point is that we hope the pattern will repeat itself.
     * We also save the value for the last prefetch to avoid
     * duplicate prefetches on subsequent calls of this function.
     */
    if ( hit ) {
	Addr addr = table[entry].lastAddr;
	do {
	    addr += buffer->buffer[it];

	    if ( table[entry].lastPrefetch < addr &&
		 !in_cache(addr) &&
		 !in_mshr_queue(addr) ) {
		issue_prefetch(addr);
		table[entry].lastPrefetch = addr;
	    }
	    it = next(buffer, it);
	    
	} while (it != secLast);
    }
}


void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
