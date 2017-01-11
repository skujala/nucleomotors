#ifndef GCODEINTERPRETER_H_713CE701
#define GCODEINTERPRETER_H_713CE701

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <string.h>

#define DEFAULT_FEEDRATE 1000

typedef enum {
  FORWARD = 0,
  BACKWARD
} direction_t;

typedef struct {
  // Ragel req'd
  uint16_t    cs, act;

  // Axis data -- populated by the parser state machine
  uint32_t    feedrate;
  uint32_t    x_goal_steps;
  uint32_t    y_goal_steps;

  direction_t x_direction;  
  direction_t y_direction;
  
  uint32_t    *current_parameter, current_value_int, current_value_frac;
  direction_t *current_direction;
  
  // Axis data -- populated by stepper motor driver state machines
  uint32_t    x_actual_steps;
  uint32_t    y_actual_steps;  
} motor_state_t;


// API
void initialize_parser_state(motor_state_t *state);
void feed_parser(motor_state_t *state, char txt);


#endif /* end of include guard: GCODEINTERPRETER_H_713CE701 */
