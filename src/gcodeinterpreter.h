#ifndef GCODEINTERPRETER_H_713CE701
#define GCODEINTERPRETER_H_713CE701

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include <stdint.h>
#include <inttypes.h>
#include <math.h>

typedef enum {
  FORWARD = 0,
  BACKWARD
} direction_t;

typedef enum {
  OK = 0,
  SYNTAX_ERROR,
  NUMERIC_ERROR,
  AXIS_ERROR,
  SET_AXIS_ERROR
} parser_error_t;

typedef enum {
  ALL_DONE = 0,
  X_MOVING = 1,
  Y_MOVING = 2
} axis_states_t;

typedef struct {
  // Ragel req'd
  uint16_t    cs, act;
  parser_error_t ret;
  

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
  
  uint8_t     axes_state;
} motor_state_t;

  


// API
void initialize_parser_state(motor_state_t *state, uint32_t default_feedrate);
parser_error_t feed_parser(motor_state_t *state, char txt);


#endif /* end of include guard: GCODEINTERPRETER_H_713CE701 */
