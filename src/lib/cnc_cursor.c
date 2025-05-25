#include "cnc_cursor.h"

bool cc_move_down(cnc_cursor *cc)
{
  if (cc->row == 0 || cc->col == 0 || cc->row > cc->max_row ||
      cc->col > cc->max_col)
  {
    return false;
  }

  if (cc->row < cc->max_row && cc->change_rows)
  {
    ++cc->row;

    return true;
  }

  return false;
}

bool cc_move_left(cnc_cursor *cc)
{
  if (cc->row == 0 || cc->col == 0 || cc->row > cc->max_row ||
      cc->col > cc->max_col)
  {
    return false;
  }

  if (cc->col == 1)
  {
    if (cc->change_rows && cc->row > 1)
    {
      cc->col = cc->max_col;
      cc->row--;

      return true;
    }

    return false;
  }

  --cc->col;

  return true;
}

bool cc_move_right(cnc_cursor *cc)
{
  if (cc->row == 0 || cc->col == 0 || cc->row > cc->max_row ||
      cc->col > cc->max_col)
  {
    return false;
  }

  if (cc->col == cc->max_col)
  {
    if (cc->change_rows && cc->row < cc->max_row)
    {
      cc->col = 1;
      cc->row++;

      return true;
    }

    return false;
  }

  ++cc->col;
  return true;
}

bool cc_move_up(cnc_cursor *cc)
{
  if (cc->row == 0 || cc->col == 0 || cc->row > cc->max_row ||
      cc->col > cc->max_col)
  {
    return false;
  }

  if (cc->row > 1 && cc->change_rows)
  {
    --cc->row;

    return true;
  }

  return false;
}

bool cc_set_position(cnc_cursor *cc, size_t row, size_t col)
{
  if (row > 0 && col > 0 && row <= cc->max_row && col <= cc->max_col)
  {
    cc->row = row;
    cc->col = col;

    return true;
  }

  return false;
}

bool cc_setup(cnc_cursor *cc, size_t max_row, size_t max_col)
{
  if (max_row > 0 && max_col > 0)
  {
    cc->max_row = max_row;
    cc->max_col = max_col;
    cc->row     = 1;
    cc->col     = 1;

    return true;
  }

  return false;
}
