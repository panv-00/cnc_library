#include "test_app.h"

void free_all_allocations(cnc_terminal *term)
{
  if (term)
  {
    cnc_terminal_destroy(term);
  }
}

int main(void)
{
  int user_input = 0;
  bool end_app = false;
  cnc_terminal *term = cnc_terminal_init(TERM_MIN_WIDTH, TERM_MIN_HEIGHT);

  if (!term)
  {
    free_all_allocations(term);
    printf("could not initialize terminal!\n");

    exit(1);
  }

  cnc_widget *titlebar = cnc_terminal_add_widget(term, WIDGET_INFO);
  cnc_widget *display = cnc_terminal_add_widget(term, WIDGET_DISPLAY);
  cnc_widget *prompt = cnc_terminal_add_widget(term, WIDGET_PROMPT);
  cnc_widget *infobar = cnc_terminal_add_widget(term, WIDGET_INFO);

  if (!titlebar || !display || !infobar || !prompt)
  {
    free_all_allocations(term);
    printf("could not add widgets!\n");

    exit(1);
  }

  if (!cnc_terminal_setup_widgets(term))
  {
    free_all_allocations(term);
    printf("could not setup widgets!\n");

    exit(1);
  }

  cnc_terminal_set_mode(term, MODE_INS);
  cnc_terminal_focus_widget(term, prompt);
  term->can_change_mode = false;
  term->can_change_focus = false;

  cnc_buffer_set_text(titlebar->data, " TEST APP ");
  cnc_buffer_set_text(display->data, "Welcome to test_app version ");
  cnc_buffer_append(display->data, APP_VERSION);
  cnc_buffer_append(display->data, "\n");
  cnc_buffer_set_text(infobar->data, " ENTER 'q' TO QUIT ");

  cnc_terminal_update_and_redraw(term);

  // loop while app has not ended
  while (!end_app)
  {
    user_input = cnc_terminal_get_user_input(term);

    // user wants to quit
    if (user_input == KEY_ENTER && cnc_buffer_equal_string(prompt->data, "q"))
    {
      end_app = true;
      cnc_widget_reset(prompt);
    }

    else if (user_input == KEY_ENTER)
    {
      cnc_buffer_set_text(display->data, prompt->data->contents);
      cnc_widget_reset(prompt);
    }

    cnc_terminal_update_and_redraw(term);
  }

  free_all_allocations(term);
  printf("No errors reported. See you next time!\n");

  return 0;
}
