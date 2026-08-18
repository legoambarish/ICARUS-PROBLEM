#ifndef UTILS_ALLOCATOR_GUARD_H
#define UTILS_ALLOCATOR_GUARD_H

#include <stddef.h>

void checked_free(void *ptr);
void allocator_guard_set_static_target(void *ptr);

#endif
