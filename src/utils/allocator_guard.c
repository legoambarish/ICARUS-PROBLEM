#include <stdlib.h>

#include "utils/allocator_guard.h"

void allocator_guard_set_static_target(void *ptr) {
    (void) ptr;
}

void checked_free(void *ptr) {
    free(ptr);
}
