#include "cnc_buffer.h"

// private functions declaration
static void _cb_scroll(cnc_buffer *cb);

// private functions definition
static void _cb_scroll(cnc_buffer *cb)
{
  if (cb == NULL || cb->data == NULL || cb->size == 0)
  {
    return;
  }

  size_t shift = cb->capacity / CB_SCROLL_FACTOR;

  if (shift == 0 || shift >= cb->size)
  {
    return;
  }

  memmove(cb->data, cb->data + shift,
          (cb->size - shift) * sizeof(cnc_term_token));

  cb->size -= shift;
}

// main functions
bool cb_append_buf(cnc_buffer *dst, const cnc_buffer *src)
{
  if (dst == NULL || src == NULL || dst->data == NULL || src->data == NULL)
  {
    return false;
  }

  for (size_t i = 0; i < src->size; i++)
  {
    if (cb_push(dst, src->data[i]) == false)
    {
      return false;
    }
  }

  return true;
}

bool cb_append_txt(cnc_buffer *cb, const char *text)
{
  if (cb == NULL || text == NULL)
  {
    return false;
  }

  while (*text)
  {
    cnc_term_token token = {0};

    // Get the length of the next character (in UTF-8 bytes)
    unsigned char *start = (unsigned char *)text;

    // Pass the next `utf8_len` bytes to `ctt_parse_bytes()`
    bool status = ctt_parse_bytes(start, &token);

    if (status != true)
    {
      return false;
    }

    // push the token to the buffer
    if (!cb_push(cb, token))
    {
      return false;
    }

    text += token.token.length;
  }

  return true;
}

void cb_clear(cnc_buffer *cb)
{
  if (cb == NULL || cb->data == NULL)
  {
    return;
  }

  memset(cb->data, 0, cb->size * sizeof(cnc_term_token));
  cb->size = 0;
}

size_t cb_data_length(cnc_buffer *cb, size_t start_index, size_t count)
{
  if (cb == NULL || cb->data == NULL)
  {
    return 0;
  }

  if (start_index >= cb->size)
  {
    return 0;
  }

  size_t real_count = count > cb->size ? cb->size : count;

  size_t length = 0;

  for (size_t i = 0; i < real_count; i++)
  {
    length += cb->data[i + start_index].token.length;
  }

  return length;
}

size_t cb_data_width(cnc_buffer *cb, size_t start_index, size_t count)
{
  if (cb == NULL || cb->data == NULL)
  {
    return 0;
  }

  if (start_index >= cb->size)
  {
    return 0;
  }

  size_t real_count =
    count > (cb->size - start_index) ? (cb->size - start_index) : count;

  size_t width = 0;

  for (size_t i = 0; i < real_count; i++)
  {
    if (cb->data[i + start_index].token.type == CTT_CHAR ||
        cb->data[i + start_index].token.type == CTT_UTF8)
    {
      width += cb->data[i + start_index].token.width;
    }
  }

  return width;
}

void cb_destroy(cnc_buffer *cb)
{
  if (cb == NULL)
  {
    return;
  }

  free(cb->data);
  cb->data = NULL;

  cb->size     = 0;
  cb->capacity = 0;
}

bool cb_equal(cnc_buffer *cb, cnc_buffer *match)
{
  if (cb == NULL || cb->data == NULL || match == NULL || match->data == NULL)
  {
    return false;
  }

  if (cb->size != match->size)
  {
    return false;
  }

  for (size_t i = 0; i < cb->size; ++i)
  {
    if (ctt_equal(&cb->data[i], &match->data[i]) == false)
    {
      return false;
    }
  }

  return true;
}

bool cb_equal_c_str(cnc_buffer *cb, char *str)
{
  if (cb == NULL || str == NULL)
  {
    return false;
  }

  cnc_buffer cb_str;

  if (cb_init(&cb_str, strlen(str)) == false)
  {
    return false;
  }

  if (cb_set_txt(&cb_str, str) == false)
  {
    cb_destroy(&cb_str);

    return false;
  }

  bool return_value = cb_equal(cb, &cb_str);
  cb_destroy(&cb_str);

  return return_value;
}

const cnc_term_token *cb_get(const cnc_buffer *cb, size_t index)
{
  if (cb == NULL || cb->data == NULL || index >= cb->size)
  {
    return NULL;
  }

  return &cb->data[index];
}

bool cb_init(cnc_buffer *cb, size_t max_capacity)
{
  if (cb == NULL || max_capacity == 0)
  {
    return false;
  }

  // if max cap is less than defined initial capacity,
  // use it as initial capacity

  size_t initial_capacity =
    max_capacity <= CB_INIT_CAP ? max_capacity : CB_INIT_CAP;

  cb->data = malloc(initial_capacity * sizeof(*cb->data));

  if (cb->data == NULL)
  {
    return false;
  }

  memset(cb->data, 0, initial_capacity * sizeof(*cb->data));

  cb->size         = 0;
  cb->capacity     = initial_capacity;
  cb->max_capacity = max_capacity;

  return true;
}

bool cb_insert(cnc_buffer *cb, const cnc_term_token token, size_t index)
{
  if (cb == NULL || cb->data == NULL || index > cb->size)
  {
    return false;
  }

  // if Buffer is full
  if (cb->size >= cb->capacity)
  {
    // Can the buffer grow?
    if (cb->capacity < cb->max_capacity)
    {
      size_t new_capacity = cb->capacity * 2;

      if (new_capacity > cb->max_capacity)
      {
        new_capacity = cb->max_capacity;
      }

      void *new_data = realloc(cb->data, new_capacity * sizeof(cnc_term_token));

      if (new_data == NULL)
      {
        return false;
      }

      cb->data     = new_data;
      cb->capacity = new_capacity;
    }

    // can't grow, let's scroll (drop oldest element)
    else
    {
      memmove(&cb->data[0], &cb->data[1],
              (cb->size - 1) * sizeof(cnc_term_token));

      cb->size--;

      if (index > 0)
      {
        index--;
      }
    }
  }

  // Shift elements to make space at index
  if (index < cb->size)
  {
    memmove(&cb->data[index + 1], &cb->data[index],
            (cb->size - index) * sizeof(cnc_term_token));
  }

  cb->data[index] = token;
  cb->size++;

  return true;
}

bool cb_locate_buffer(cnc_buffer *cb, cnc_buffer *search, size_t *location)
{
  if (cb == NULL || cb->data == NULL || search == NULL ||
      search->data == NULL || location == NULL)
  {
    return false;
  }

  size_t buffer_length = cb->size;
  size_t search_length = search->size;

  if (buffer_length == 0 || buffer_length < search_length)
  {
    return false;
  }

  if (search_length == 0)
  {
    *location = 0;

    return true;
  }

  for (size_t i = 0; i <= buffer_length - search_length; i++)
  {
    size_t j;

    for (j = 0; j < search_length; j++)
    {
      if (ctt_equal(&cb->data[i + j], &search->data[j]) == false)
      {
        break;
      }
    }

    if (j == search_length)
    {
      *location = i;
      return true;
    }
  }

  return false;
}

bool cb_locate_c_str(cnc_buffer *cb, const char *str, size_t *location)
{
  if (cb == NULL || cb->data == NULL || str == NULL || location == NULL)
  {
    return false;
  }

  cnc_buffer search;

  if (cb_init(&search, strlen(str)) == false)
  {
    return false;
  }

  if (cb_set_txt(&search, str) == false)
  {
    return false;
  }

  bool return_value = cb_locate_buffer(cb, &search, location);

  cb_destroy(&search);

  return return_value;
}

bool cb_overwrite(cnc_buffer *dst, size_t dst_start, size_t length,
                  cnc_buffer *src, size_t src_start)
{
  if (dst == NULL || src == NULL || dst->data == NULL || src->data == NULL)
  {
    return false;
  }

  if (dst_start >= dst->size || src_start >= src->size)
  {
    return false;
  }

  // Calculate how much we can actually copy
  size_t max_src_len   = src->size - src_start;
  size_t max_dst_len   = dst->size - dst_start;
  size_t actual_length = length;

  if (actual_length > max_src_len)
  {
    actual_length = max_src_len;
  }

  if (actual_length > max_dst_len)
  {
    actual_length = max_dst_len;
  }

  if (actual_length == 0)
  {
    return false;
  }

  memcpy(&dst->data[dst_start], &src->data[src_start],
         actual_length * sizeof(cnc_term_token));

  return true;
}

bool cb_push(cnc_buffer *cb, const cnc_term_token token)
{
  if (cb == NULL || cb->data == NULL)
  {
    return false;
  }

  // If there is space, append the token
  if (cb->size < cb->capacity)
  {
    cb->data[cb->size++] = token;

    return true;
  }

  // if Buffer is full, but can grow
  if (cb->capacity < cb->max_capacity)
  {
    // Calculate the new capacity
    size_t new_capacity = cb->capacity * 2;

    if (new_capacity > cb->max_capacity)
    {
      new_capacity = cb->max_capacity;
    }

    cnc_term_token *new_data =
      realloc(cb->data, new_capacity * sizeof(cnc_term_token));

    if (new_data == NULL)
    {
      return false;
    }

    cb->data     = new_data;
    cb->capacity = new_capacity;
  }

  // If buffer is full but cannot grow: scroll
  else if (cb->size >= cb->capacity)
  {
    _cb_scroll(cb);
  }

  cb->data[cb->size++] = token;

  return true;
}

bool cb_remove(cnc_buffer *cb, size_t index)
{
  if (cb == NULL || cb->data == NULL || index >= cb->size)
  {
    return false;
  }

  if (index < cb->size - 1)
  {
    memmove(&cb->data[index], &cb->data[index + 1],
            (cb->size - index - 1) * sizeof(cnc_term_token));
  }

  cb->size--;

  return true;
}

bool cb_replace(cnc_buffer *cb, const cnc_term_token *match,
                const cnc_term_token *replacement)
{
  if (cb == NULL || cb->data == NULL || match == NULL || replacement == NULL)
  {
    return false;
  }

  bool replaced = false;

  for (size_t i = 0; i < cb->size; i++)
  {
    if (ctt_equal(&cb->data[i], match))
    {
      cb->data[i] = *replacement;
      replaced    = true;
    }
  }

  return replaced;
}

bool cb_resize(cnc_buffer *cb, size_t new_capacity)
{
  if (cb == NULL || cb->data == NULL || new_capacity == 0)
  {
    return false;
  }

  if (new_capacity > cb->max_capacity)
  {
    return false;
  }

  if (new_capacity == cb->capacity)
  {
    return true; // Nothing to do
  }

  cnc_term_token *new_data = malloc(new_capacity * sizeof(*new_data));

  if (new_data == NULL)
  {
    return false;
  }

  // Copy existing elements (truncate if needed)
  size_t to_copy = (cb->size < new_capacity) ? cb->size : new_capacity;

  memcpy(new_data, cb->data, to_copy * sizeof(cnc_term_token));

  free(cb->data);

  cb->data     = new_data;
  cb->capacity = new_capacity;
  cb->size     = to_copy;

  return true;
}

bool cb_set(cnc_buffer *cb, const cnc_term_token token, size_t index)
{
  if (cb == NULL || cb->data == NULL)
  {
    return false;
  }

  cb->data[index] = token;

  return true;
}

bool cb_set_buf(cnc_buffer *dst, cnc_buffer *src)
{
  if (dst == NULL || dst->data == NULL || src == NULL || src->data == NULL)
  {
    return false;
  }

  cb_clear(dst);

  return cb_append_buf(dst, src);
}

bool cb_set_txt(cnc_buffer *cb, const char *text)
{
  if (cb == NULL || cb->data == NULL || text == NULL)
  {
    return false;
  }

  cb_clear(cb);

  return cb_append_txt(cb, text);
}

bool cb_set_c_str(cnc_buffer *cb, char *dst, size_t dst_size)
{
  if (cb == NULL || cb->data == NULL || dst == NULL)
  {
    return false;
  }

  size_t dst_index = 0;

  for (size_t i = 0; i < cb->size; i++)
  {
    if (cb->data[i].token.type == CTT_CHAR ||
        cb->data[i].token.type == CTT_UTF8)
    {
      size_t len = cb->data[i].token.length;

      if (dst_index + len >= dst_size)
      {
        return false;
      }

      for (size_t j = 0; j < len; j++)
      {
        dst[dst_index++] = cb->data[i].seq[j];
      }
    }
  }

  if (dst_index >= dst_size)
  {
    return false;
  }

  dst[dst_index] = C_NUL;

  return true;
}
