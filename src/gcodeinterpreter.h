#ifndef GCODEINTERPRETER_H_713CE701
#define GCODEINTERPRETER_H_713CE701

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>


#include <stdint.h>
#include <inttypes.h>
#include <math.h>

typedef enum {
  FORWARD = 1,
  BACKWARD = 2
} direction_t;

typedef enum {
  OK = 0,
  WORKING, 
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
  volatile uint16_t    cs, act;
  volatile parser_error_t ret;
  

  // Axis data -- populated by the parser state machine
  volatile uint32_t    feedrate;
  volatile uint32_t    x_goal_steps;
  volatile uint32_t    y_goal_steps;

  volatile direction_t x_direction;  
  volatile direction_t y_direction;
  
  volatile uint32_t    *current_parameter, current_value_int, current_value_frac;
  volatile direction_t *current_direction;
  
  // Axis data -- populated by stepper motor driver state machines
  volatile uint32_t    x_actual_steps;
  volatile uint32_t    y_actual_steps;
  
  volatile uint8_t     axes_state;
} motor_state_t;

  


// API
void initialize_parser_state(motor_state_t *state, uint32_t default_feedrate);
parser_error_t feed_parser(motor_state_t *state, char txt);


#endif /* end of include guard: GCODEINTERPRETER_H_713CE701 */
