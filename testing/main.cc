#include "interface.hh"

int main() {
    prefetch_init();
    prefetch_access(create(10, 10, 0, 0));
    prefetch_access(create(10, 15, 0, 0));
    prefetch_access(create(10, 20, 0, 0));
    prefetch_access(create(10, 25, 0, 0));
    prefetch_access(create(10, 30, 0, 0));
}
