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
}


%%{
  machine main;
  write data;
  
  access state->;
}%%
  
void initialize_parser_state(motor_state_t *state)
{
  %%{ write init; }%%
  
  RESET(state);
  
  state->feedrate = DEFAULT_FEEDRATE;  
}
  
void feed_parser(motor_state_t *state, char txt)
{
  char *p = &txt;
  char *pe = p + 1;
  char *eof = NULL;
    
  %%{
        
    action set_negative {
      if (state->current_direction) {
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
        
    action G00 {
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
    
    action set_parameter {
    }
    
    action commit {
      printf("Moving axis X %"PRIu32" steps and axis Y %"PRIu32" steps at feedrate F %"PRIu32" steps/second.\n",
       state->x_goal_steps, state->y_goal_steps, state->feedrate);
    }
    
    action reset_value {
      state->current_value_int = 0;
      state->current_value_frac = 0;
    }
    
    action reset {
      RESET(state);  
    }
    
    action cmd_err {
      RESET(state);

      puts("Command error!");
      fhold; fgoto line;
    }

    action num_err {
      RESET(state);
            
      puts("Numeric error!");
      fhold; fgoto line;
    }

    action axis_err {
      RESET(state);

      puts("Axis err");
      fhold; fgoto line;
    }
    
    action set_parameter_err {
      RESET(state);
      
      puts("Set axis err");
      fhold; fgoto line;
    }
    
    
    command_end = space* . '\r'? . '\n'; # Allow Windows and Unix style line-endings

    number = 
      (('-' @set_negative | '+' )? .    # set the integer part negative if needed
      (digit+ @accum_int) .             # accumulate integer values
      ('.' . digit{0,} @accum_frac)?)   # accumulate fractional values 
      %got_number                       # finalize 
      $err(num_err);                    # error handling

    positive_number = 
      (digit+ @accum_int) .             # accumulate integer values
      ('.' . digit{0,} @accum_frac)?   # accumulate fractional values 
      %got_number                       # finalize 
      $err(num_err);                    # error handling

# Supported axis
        
    sep = space+;
        
    axis = (
      ('X' %select_parameter_x . number ) |
      ('Y' %select_parameter_y . number ) |  
      ('F' %select_parameter_f . positive_number)
    ) >reset_value %set_parameter $err(set_parameter_err);

    opcode_00 = 'G0' . [01] %(G00) >reset . (space+ . axis)+;
        
    main := (opcode_00 @err(cmd_err) . command_end @commit)*;
    
    line := ( any* -- command_end ) . command_end @{ fgoto main; };

#   End g-codes

    write exec;
  }%%
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

