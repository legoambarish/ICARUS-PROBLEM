#include <stdio.h>

void legacy_diag_dump(void) {
    char line[32];

    if (fgets(line, sizeof(line), stdin) != NULL) {
        puts(line);
    }
}
