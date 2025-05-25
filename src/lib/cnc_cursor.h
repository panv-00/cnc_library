#ifndef CNC_CURSOR_H
#define CNC_CURSOR_H

// using cc as shorthand for cnc_cursor

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
  size_t max_row;
  size_t max_col;

  // allow cursor to change row when col is 1 or max_col
  bool change_rows;

  size_t row;
  size_t col;

} cnc_cursor;

bool cc_move_down(cnc_cursor *cc);

bool cc_move_left(cnc_cursor *cc);

bool cc_move_right(cnc_cursor *cc);

bool cc_move_up(cnc_cursor *cc);

bool cc_set_position(cnc_cursor *cc, size_t row, size_t col);

bool cc_setup(cnc_cursor *cc, size_t max_row, size_t max_col);

#endif
