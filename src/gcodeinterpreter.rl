#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <string.h>




typedef enum {
  FORWARD = 0,
  BACKWARD
} direction_t;

typedef struct {
  // Ragel req'd
  uint16_t    cs, act;

  // Axis data
  uint32_t    feedrate;
  uint32_t    x_goal_steps;
  uint32_t    y_goal_steps;

  direction_t x_direction;  
  direction_t y_direction;
  
  uint32_t    x_actual_steps;
  uint32_t    y_actual_steps;
  
  uint32_t    *current_parameter, current_value_int, current_value_frac;
  direction_t *current_direction;  
} motor_state_t;

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



#define DEFAULT_FEEDRATE 1000

%%{
  machine main;
  
  write data;
  
  access state->;
}%%

  
char *scan(motor_state_t *state, char *txt, uint16_t size)
{
  char *p = txt;
  char *pe = p + size;
  char *eof = NULL;
    
  %%{
    write init;
        
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
    
      // This would be perfect place to convert the number to fixed-point values      
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

  }%%
  
  %%write exec;
  
  return p;
}


int main(void)
{
  motor_state_t state;
  
  state.feedrate = DEFAULT_FEEDRATE;

  
  char hello[] = "G01 F1400\nG01 Y5 X-50 F200   \nG01 X20\nG01 X23 Y20s\nG00 X10 Y20\n";
    
  char *p = &hello[0];
        
  p = scan(&state, p, strlen(p));    
  return 0;
}

