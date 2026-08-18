#ifndef MIDDLEWARE_COMMAND_QUEUE_H
#define MIDDLEWARE_COMMAND_QUEUE_H

struct AppState;

int command_queue_depth(void);
void cmd_bind_state(struct AppState *state);
void cmd_set_actuators(double temp_pwm, double wheel_rpm);

#endif
