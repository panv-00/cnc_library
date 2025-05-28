#include "cnc_library.h"

static void _ca_refresh_info(cnc_app *ca);

static void _ca_refresh_info(cnc_app *ca)
{
  if (ca == NULL)
  {
    return;
  }

  cnc_buffer b;

  if (cb_init(&b, INFO_BUFFER_SIZE) == false)
  {
    return;
  }

  for (size_t i = 23; i < ca->cw_info_bar->buffer.size; i++)
  {
    cb_push(&b, ca->cw_info_bar->buffer.data[i]);
  }

  ca_set_info(ca, "");
  cb_append_buf(&ca->cw_info_bar->buffer, &b);

  cb_destroy(&b);
}

void ca_destroy(cnc_app *ca)
{
  if (ca)
  {
    ct_destroy(ca->cterm);
  }
}

int ca_get_user_input(cnc_app *ca)
{
  if (ca->cterm == NULL)
  {
    return 0;
  }

  int return_value = ct_get_user_input(ca->cterm);
  ct_update(ca->cterm);

  return return_value;
}

bool ca_init(cnc_app *ca, uint32_t min_term_rows, uint32_t min_term_cols)
{
  if (ca == NULL)
  {
    ca_destroy(ca);

    return false;
  }

  ca->min_term_rows = min_term_rows;
  ca->min_term_cols = min_term_cols;

  ca->cterm = ct_init(min_term_rows, min_term_cols);

  if (ca->cterm == NULL)
  {
    ca_destroy(ca);
    printf("Could not initialize terminal! ");

    return false;
  }

  return true;
}

void ca_set_info(cnc_app *ca, const char *text)
{
  if (ca == NULL || text == NULL)
  {
    return;
  }

  char buf[14];

  sprintf(buf, "[%5zu/%5zu]", ca->cw_display->buffer.size,
          ca->cw_display->buffer.capacity);

  cb_set_text(&ca->cw_info_bar->buffer, buf);

  if (ca->cw_prompt->has_focus)
  {
    cb_append_txt(&ca->cw_info_bar->buffer, " PROMPT : ");
  }

  else if (ca->cw_display->has_focus)
  {
    cb_append_txt(&ca->cw_info_bar->buffer, " DISPLAY: ");
  }

  else
  {
    cb_append_txt(&ca->cw_info_bar->buffer, "          ");
  }

  cb_append_txt(&ca->cw_info_bar->buffer, text);
}

bool ca_setup(cnc_app *ca, char *version, char *title, char *welcome_message,
              char *info_bar_text)
{
  if (ca == NULL)
  {
    ca_destroy(ca);
    printf("App instance not initialized! ");

    return false;
  }

  if (ca->cterm == NULL)
  {
    ca_destroy(ca);
    printf("Terminal not initialized! ");

    return false;
  }

  if (version == NULL)
  {
    ca_destroy(ca);
    printf("App version not set to a value! ");

    return false;
  }

  if (title == NULL)
  {
    ca_destroy(ca);
    printf("App title not set to a value! ");

    return false;
  }

  if (welcome_message == NULL)
  {
    ca_destroy(ca);
    printf("App welcome message not set to a value! ");

    return false;
  }

  if (info_bar_text == NULL)
  {
    ca_destroy(ca);
    printf("App info text not set to a value! ");

    return false;
  }

  ca->cw_title_bar = ct_add_widget(ca->cterm, WIDGET_TITLE);
  ca->cw_display   = ct_add_widget(ca->cterm, WIDGET_DISPLAY);
  ca->cw_info_bar  = ct_add_widget(ca->cterm, WIDGET_INFO);
  ca->cw_prompt    = ct_add_widget(ca->cterm, WIDGET_PROMPT);

  if (ca->cw_title_bar == NULL || ca->cw_display == NULL ||
      ca->cw_info_bar == NULL || ca->cw_prompt == NULL)
  {
    ca_destroy(ca);
    printf("Could not add widget(s)! ");

    return false;
  }

  if (ct_setup_widgets(ca->cterm) == false)
  {
    ca_destroy(ca);
    printf("Could not setup widget(s)! ");

    return false;
  }

  ct_set_mode(ca->cterm, MODE_INS);
  ct_focus_widget(ca->cterm, ca->cw_prompt);

  ca->cterm->can_change_mode  = false;
  ca->cterm->can_change_focus = false;

  cb_set_text(&(ca->cw_title_bar->buffer), title);
  cb_append_txt(&(ca->cw_title_bar->buffer), " (v. ");
  cb_append_txt(&(ca->cw_title_bar->buffer), version);
  cb_append_txt(&(ca->cw_title_bar->buffer), ")\n");
  cb_set_text(&(ca->cw_display->buffer), welcome_message);
  ca_set_info(ca, info_bar_text);

  ca_update(ca);

  return true;
}

void ca_update(cnc_app *ca)
{
  _ca_refresh_info(ca);
  ct_update(ca->cterm);
}
