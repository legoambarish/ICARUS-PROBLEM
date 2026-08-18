#include <math.h>

double orbital_period(void) {
    return 5400.0;
}

double solar_incidence(int tick) {
    return cos((double)tick * 6.283185307179586 / 150.0);
}

int verify_flag(unsigned int input) {
    return input == 0xb6f27e32;
}
