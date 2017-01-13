#include "gcodeinterpreter.h"

#define RESET(x) { \
  x->x_goal_steps = 0; \
  x->y_goal_steps = 0; \
  x->x_direction = FORWARD; \
  x->y_direction = FORWARD; \
  x->current_parameter = 0; \
  x->current_value_int = 0; \
  x->current_value_frac = 0; \
  x->current_direction = 0; \
  x->ret = OK; \
}


%%{
  machine main;
  write data;
  
  access state->;
}%%
  
void initialize_parser_state(motor_state_t *state, uint32_t default_feedrate)
{
  %%{ write init; }%%
  
  RESET(state);
  
  state->feedrate = default_feedrate;
}
  
parser_error_t feed_parser(motor_state_t *state, char txt)
{
  char *p = &txt;
  char *pe = p + 1;
  char *eof = NULL;
      
  %%{
        
    action set_negative {
      if (state->current_direction != 0) {
        *(state->current_direction) = BACKWARD;
      }
    }
    
    action accum_int {
      state->current_value_int *= 10;
      state->current_value_int += *p - '0';
    }
    
    action accum_frac {
      state->current_value_frac *= 10;
      state->current_value_frac += *p - '0';
    }
    
    action got_number {      
      *(state->current_parameter) = state->current_value_int;
    
      // This would be perfect place to convert the number to fixed-point values if needed     
    }
    
    action scale_feedrate {    
      *(state->current_parameter) = state->current_value_int;
    
      // This would be perfect place to convert the number to fixed-point values if needed     
    }
        
    action select_parameter_x {
      state->current_parameter = &state->x_goal_steps;
      state->current_direction = &state->x_direction;      
    }

    action select_parameter_y {
      state->current_parameter = &state->y_goal_steps;
      state->current_direction = &state->y_direction;
    }

    action select_parameter_f {
      state->current_parameter = &state->feedrate;
      state->current_direction = 0;
    }
    
    action commit {      
      state->y_direction == FORWARD ? gpio_clear(GPIOA, GPIO8) : gpio_set(GPIOA, GPIO8);
      state->x_direction == FORWARD ? gpio_clear(GPIOB, GPIO5) : gpio_set(GPIOB, GPIO5);

      // calculate the feedrates / axis
      
      if (state->x_goal_steps > 0 || state->y_goal_steps > 0) {
        double inv_time = state->feedrate / hypot(state->x_goal_steps, state->y_goal_steps);

        double vx, vy;
      
        vx = state->x_goal_steps * inv_time; 
        vy = state->y_goal_steps * inv_time; 

        state->x_actual_steps = 0;
        state->y_actual_steps = 0;
        
        uint32_t inv_vx, inv_vy;
        
        inv_vx = 1000000/vx;
        inv_vy = 1000000/vy;

        timer_set_period(TIM2, inv_vx);
        timer_set_oc_value(TIM2, TIM_OC2, inv_vx >> 1);
        timer_generate_event(TIM2, TIM_EGR_UG);

        timer_set_period(TIM3, inv_vy);
        timer_set_oc_value(TIM3, TIM_OC2, inv_vy >> 1);        
        timer_generate_event(TIM3, TIM_EGR_UG);
        
        if (state->x_goal_steps != 0) {
          state->axes_state |= X_MOVING;
          timer_enable_counter(TIM2);
        }
        
        if (state->y_goal_steps != 0){
          state->axes_state |= Y_MOVING;
          timer_enable_counter(TIM3);
        }
      }      
      
      state->ret = WORKING;
    }
    
    action reset_value {
      state->current_value_int = 0;
      state->current_value_frac = 0;
      state->current_direction = NULL;
      state->current_parameter = NULL;
    }
    
    action reset {      
      timer_disable_counter(TIM2);
      timer_disable_counter(TIM3);
      
      
      RESET(state);  
    }
    
    action cmd_err {
      RESET(state);

      state->ret = SYNTAX_ERROR;
      fhold; fgoto line;
    }

    action num_err {
      RESET(state);
            
      state->ret = NUMERIC_ERROR;
      fhold; fgoto line;
    }

    action axis_err {
      RESET(state);

      state->ret = AXIS_ERROR;
      fhold; fgoto line;
    }
    
    action set_parameter_err {
      RESET(state);
      
      state->ret = SET_AXIS_ERROR;
      fhold; fgoto line;
    }
    
    sep = [ \t]+;
    
    command_end = sep* . ('\n'|'\r'){1,}; # Allow Windows and Unix style line-endings

    number = 
      (('-' @set_negative | '+' )? .    # set the integer part negative if needed
      (digit+ @accum_int) .             # accumulate integer values
      ('.' . digit{0,} @accum_frac)?)   # accumulate fractional values 
      %got_number                       # finalize 
      $err(num_err);                    # error handling

    positive_number = 
      (digit+ @accum_int) .             # accumulate integer values
      ('.' . digit{0,} @accum_frac)?    # accumulate fractional values 
      %scale_feedrate                   # finalize 
      $err(num_err);                    # error handling

# Supported axis
        
        
    axis = (
      ('X' @select_parameter_x . number ) |
      ('Y' @select_parameter_y . number ) |  
      ('F' @select_parameter_f . positive_number)
    ) >reset_value $err(set_parameter_err);

    opcode_00 = 'G0' >reset . [01] . (sep+ . axis)+;
        
    main := (opcode_00 @err(cmd_err) . command_end @commit)* ; 
    
    line := ( any* -- command_end ) . command_end @{ fgoto main; };

#   End g-codes

    write exec;
  }%%
    
  return(state->ret);
}

// This is for testing
// int main(void)
// {
//   motor_state_t state;
//   
//   initialize_parser_state(&state);
//     
//   char hello[] = "G01 F1400\nG01 Y5 X-50 F200   \r\nG01 X20\nG01 X23 Y20s\nG00 X10 Y20\n";
//       
//   for (uint16_t i = 0; i < strlen(hello); i++) {
//     feed_parser(&state, hello[i]);
//   }
//   
//   return 0;
// }

