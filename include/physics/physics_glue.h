#ifndef PHYSICS_PHYSICS_GLUE_H
#define PHYSICS_PHYSICS_GLUE_H

struct AppState;

double physics_orbital_period(void);
double physics_solar_incidence(int tick);
int physics_verify_flag(unsigned int input);
void physics_update_actuators(struct AppState *state, int current_tick);

#endif
