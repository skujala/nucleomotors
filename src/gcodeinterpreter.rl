#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

%%{
  machine main;
  
  write data;
}%%
  
  
char *scan(char *txt, uint16_t size)
{
  char *p = txt;
  char *pe = p + size;
  char *eof = NULL;
  
  int cs;
  
  char axis;
  
  int8_t pos = 1;
  
// Change these to more appropriate datatypes as you see necessary
  int32_t int_result = 0;
  uint32_t frac_result = 0;
  
  %%{
    write init;
        
    action set_negative {
      pos = -1;
    }
    
    action accum_int {
      int_result *= 10;
      int_result += *p - '0';
    }
    
    action accum_frac {
      frac_result *= 10;
      frac_result += *p - '0';
    }
    
    action got_number {
      int_result *= pos;
    
      // This would be perfect place to convert the number to fixed-point values      
    }
        
    action G00 {
      puts("Fast move ");
    }
        
    action select_axis {
      axis = *p;
    }
    
    action set_axis {
      printf("%c: %d.%d ", axis, int_result, frac_result);
    
      /* Reset values to their defaults */
      int_result = 0;
      frac_result = 0;
      pos = 1;
    }
    
    action commit {
      puts("Commit");
    }

    action cmd_err {
      /* Reset values to their defaults */
      int_result = 0;
      frac_result = 0;
      pos = 1;
      
      puts("Command error!");
      fhold; fgoto line;
    }

    action num_err {
      /* Reset values to their defaults */
      int_result = 0;
      frac_result = 0;
      pos = 1;
      
      puts("Numeric error!");
      fhold; fgoto line;
    }

    action axis_err {
      /* Reset values to their defaults */
      int_result = 0;
      frac_result = 0;
      pos = 1;
      
      
      puts("Axis err");
      fhold; fgoto line;
    }
    
    action set_axis_err {
      /* Reset values to their defaults */
      int_result = 0;
      frac_result = 0;
      pos = 1;
      
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

# Supported axis
        
    sep = space+;
        
    axis = ([XYF] @select_axis . number) %set_axis $err(set_axis_err);

    opcode_00 = 'G0' . [01] %(G00) . (space+ . axis)+;
        
    main := (opcode_00 @err(cmd_err) . command_end @commit)*;
    
    line := ( any* -- command_end ) . command_end @{ fgoto main; };

#   End g-codes

  }%%
  
  %%write exec;
  
  return p;
}


int main(void)
{
    char hello[] = "G00 Y5 X-50 F200 \nG01 X20\nG01 X23 Y20s\nG00 X10 Y20\n";
    
    char *p = &hello[0];
        
    p = scan(p, strlen(p));    
    return 0;
}

