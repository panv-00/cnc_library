#include "cnc_term_token.h"

bool ctt_equal(const cnc_term_token *tkn1, const cnc_term_token *tkn2)
{
  if (tkn1 == NULL || tkn2 == NULL)
  {
    return false;
  }

  if (tkn1->token.type != tkn2->token.type)
  {
    return false;
  }

  if (tkn1->token.width != tkn2->token.width)
  {
    return false;
  }

  if (tkn1->token.length != tkn2->token.length)
  {
    return false;
  }

  if (tkn1->token.value != tkn2->token.value)
  {
    return false;
  }

  for (size_t i = 0; i < tkn1->token.length; i++)
  {
    if (tkn1->seq[i] != tkn2->seq[i])
    {
      return false;
    }
  }

  return true;
}

bool ctt_is_whitespace(cnc_term_token tkn)
{
  if (tkn.token.value == C_SPC || // space character
      tkn.token.value == C_TAB)   // tab
  {
    return true;
  }

  return false;
}

bool ctt_parse_bytes(uint8_t *bytes, cnc_term_token *ctt)
{
  if (bytes == NULL || ctt == NULL)
  {
    return false;
  }

  ctt->token.type   = CTT_NONE;
  ctt->token.width  = W_NIL;
  ctt->token.length = 0;
  ctt->token.value  = 0;

  for (size_t i = 0; i < CTT_MAX_TOKEN_SIZE; i++)
  {
    ctt->seq[i] = 0;
  }

  // Printable ASCII
  if (bytes[0] >= 0x20 && bytes[0] < 0x7F)
  {
    ctt->token.type   = CTT_CHAR;
    ctt->token.width  = W_ONE;
    ctt->token.length = 1;
    ctt->token.value  = bytes[0];
    ctt->seq[0]       = bytes[0];

    return true;
  }

  // Control character without sequence
  if (bytes[0] < 0x20 || bytes[0] == 0x7F)
  {
    ctt->token.type   = CTT_CHAR;
    ctt->token.length = 1;
    ctt->token.value  = bytes[0];
    ctt->seq[0]       = bytes[0];

    return true;
  }

  // 2 bytes UTF-8
  if ((bytes[0] & 0xE0) == 0xC0) //   byte 1: 110x xxxx
  {
    if ((bytes[1] & 0xC0) != 0x80) // byte 2: 10xx xxxx
    {
      return false;
    }

    ctt->token.type   = CTT_UTF8;
    ctt->token.length = 2;

    ctt->seq[0] = bytes[0];
    ctt->seq[1] = bytes[1];

    ctt->token.value = ((ctt->seq[0] & 0x1F) << W_1) | // last 5 bits of byte 1
                       ((ctt->seq[1] & 0x3F) << W_0);  // last 6 bits of byte 2

    ctt->token.width = ctt_c_width(ctt->token.value);

    return true;
  }

  // 3 bytes UTF-8
  if ((bytes[0] & 0xF0) == 0xE0) //     byte 1: 1110 xxxx
  {
    if ((bytes[1] & 0xC0) != 0x80 || // byte 2: 10xx xxxx
        (bytes[2] & 0xC0) != 0x80)   // byte 3: 10xx xxxx
    {
      return false;
    }

    ctt->token.type   = CTT_UTF8;
    ctt->token.length = 3;

    ctt->seq[0] = bytes[0];
    ctt->seq[1] = bytes[1];
    ctt->seq[2] = bytes[2];

    ctt->token.value = ((ctt->seq[0] & 0x0F) << W_2) | // last 4 bits of byte 1
                       ((ctt->seq[1] & 0x3F) << W_1) | // last 6 bits of byte 2
                       ((ctt->seq[2] & 0x3F) << W_0);  // last 6 bits of byte 3

    ctt->token.width = ctt_c_width(ctt->token.value);

    return true;
  }

  // 4 bytes UTF-8
  if ((bytes[0] & 0xF8) == 0xF0) //     byte 1: 1111 0xxx
  {

    if ((bytes[1] & 0xC0) != 0x80 || // byte 2: 10xx xxxx
        (bytes[2] & 0xC0) != 0x80 || // byte 3: 10xx xxxx
        (bytes[3] & 0xC0) != 0x80)   // byte 4: 10xx xxxx
    {
      return false;
    }

    ctt->token.type   = CTT_UTF8;
    ctt->token.length = 4;

    ctt->seq[0] = bytes[0];
    ctt->seq[1] = bytes[1];
    ctt->seq[2] = bytes[2];
    ctt->seq[3] = bytes[3];

    ctt->token.value = ((ctt->seq[0] & 0x07) << W_3) | // last 3 bits of byte 1
                       ((ctt->seq[1] & 0x3F) << W_2) | // last 6 bits of byte 2
                       ((ctt->seq[2] & 0x3F) << W_1) | // last 6 bits of byte 3
                       ((ctt->seq[3] & 0x3F) << W_0);  // last 6 bits of byte 4

    ctt->token.width = ctt_c_width(ctt->token.value);

    return true;
  }

  return false;
}

cnc_term_token ctt_parse_value(uint32_t value)
{

  cnc_term_token token = {0};
  token.token.value    = value;

  // ANSI tokens
  size_t ansi_tokens_size =
    sizeof(ctt_ansi_tokens) / sizeof(ctt_ansi_tokens[0]);

  for (size_t i = 0; i < ansi_tokens_size; ++i)
  {
    if (ctt_ansi_tokens[i].token.value == value)
    {
      return ctt_ansi_tokens[i];
    }
  }

  token.token.length = 0;
  token.token.width  = W_NIL;
  token.token.type   = CTT_NONE;

  for (size_t i = 0; i < CTT_MAX_TOKEN_SIZE; ++i)
  {
    token.seq[i] = 0;
  }

  // ASCII character
  if (value < 0x80)
  {
    token.token.type   = CTT_CHAR;
    token.token.width  = (value < 0x20 || value == 0x7F) ? W_NIL : W_ONE;
    token.token.length = 1;
    token.seq[0]       = (uint8_t)value;
    return token;
  }

  // UTF-8 encoding
  if (value <= 0x7FF)
  {
    token.token.type   = CTT_UTF8;
    token.token.width  = W_ONE;
    token.token.length = 2;
    token.seq[0]       = 0xC0 | ((value >> 6) & 0x1F);
    token.seq[1]       = 0x80 | (value & 0x3F);
    return token;
  }

  if (value <= 0xFFFF)
  {
    token.token.type   = CTT_UTF8;
    token.token.width  = W_ONE;
    token.token.length = 3;
    token.seq[0]       = 0xE0 | ((value >> 12) & 0x0F);
    token.seq[1]       = 0x80 | ((value >> 6) & 0x3F);
    token.seq[2]       = 0x80 | (value & 0x3F);
    return token;
  }

  if (value <= 0x10FFFF)
  {
    token.token.type   = CTT_UTF8;
    token.token.width  = W_TWO; // Wide Unicode char
    token.token.length = 4;
    token.seq[0]       = 0xF0 | ((value >> 18) & 0x07);
    token.seq[1]       = 0x80 | ((value >> 12) & 0x3F);
    token.seq[2]       = 0x80 | ((value >> 6) & 0x3F);
    token.seq[3]       = 0x80 | (value & 0x3F);
    return token;
  }

  // Invalid value
  token.token.type = CTT_NONE;

  return token;
}
