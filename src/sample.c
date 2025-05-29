#include "sample.h"

int main(void)
{
  int  user_input = 0;
  bool end_app    = false;
  bool first_run  = true;

  cnc_app sample;

  ca_init(&sample, MIN_TERM_HEIGHT, MIN_TERM_WIDTH);
  ca_setup(&sample, APP_VERSION, " SAMPLE APP", "Welcome to sample app!",
           " enter 'q' to exit!");

  cnc_terminal *term    = sample.cterm;
  cnc_widget   *title   = sample.cw_title_bar;
  cnc_widget   *display = sample.cw_display;
  // cnc_widget   *info    = sample.cw_info_bar;
  cnc_widget *prompt = sample.cw_prompt;

  // set main display widget
  term->main_display_widget = display;

  cb_append_txt(&display->buffer, "\ntesting a new red line .. ");

  // call CTT_PV(C_COL) -> next token sets color info
  cb_push(&display->buffer, CTT_PV(C_COL));
  cb_push(&display->buffer, CTT_PV(KS_WHI_BG));
  cb_push(&display->buffer, CTT_PV(C_COL));
  cb_push(&display->buffer, CTT_PV(KS_RED_FG));
  cb_append_txt(&display->buffer, " .. testing a new red line ");

  cb_append_txt(&display->buffer, "\n This would be a normal line...");

  term->can_change_focus = true;
  term->can_change_mode  = true;

  // uptade app before run loop
  ct_update(sample.cterm);

  // loop while app has not ended
  while (!end_app)
  {
    if ((user_input = ca_get_user_input(&sample)) == C_ENT)
    {
      // user wants to quit
      if (cb_equal_c_str(&prompt->buffer, "q"))
      {
        end_app = true;
      }

      if (cb_equal_c_str(&prompt->buffer, "exit"))
      {
        title->bg = CTT_PV(KS_RED_BG);
        title->fg = CTT_PV(KS_BLA_FG);
      }

      else
      {
        title->bg = CTT_PV(KS_NON___);
        title->fg = CTT_PV(KS_NON___);
      }

      if (first_run)
      {
        first_run = false;
        cw_reset(display);
      }

      else
      {
        cb_append_txt(&display->buffer, "\n");
      }

      cb_append_txt(&display->buffer, "--> ");
      cb_append_buf(&display->buffer, &prompt->buffer);

      if (display->data_index >= display->frame.height)
      {
        display->index++;
      }

      cw_reset(prompt);
    }

    ca_update(&sample);
  }

  ca_destroy(&sample);

  printf("No errors reported!\n");

  return 0;
}
