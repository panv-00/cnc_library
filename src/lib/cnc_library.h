#ifndef CNC_LIBRARY_H
#define CNC_LIBRARY_H

// ca -> cnc_app

#include "cnc_buffer.h"
#include "cnc_cursor.h"
#include "cnc_term_token.h"
#include "cnc_term_token_width.h"
#include "cnc_terminal.h"
#include "cnc_widget.h"

typedef struct
{
  uint32_t min_term_cols;
  uint32_t min_term_rows;

  cnc_widget *cw_title_bar;
  cnc_widget *cw_display;
  cnc_widget *cw_info_bar;
  cnc_widget *cw_prompt;

  cnc_terminal *cterm;

} cnc_app;

void ca_destroy(cnc_app *ca);
int  ca_get_user_input(cnc_app *ca);
bool ca_init(cnc_app *ca, uint32_t min_term_rows, uint32_t min_term_cols);
void ca_loop(cnc_app *ca);
void ca_set_info(cnc_app *ca, const char *text);
bool ca_setup(cnc_app *ca, char *version, char *title, char *welcome_message,
              char *info_bar_text);
void ca_update(cnc_app *ca);

#endif
