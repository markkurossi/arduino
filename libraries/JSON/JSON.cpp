/*
 * JSON.cpp
 */

#include "JSON.h"

JSON::JSON()
  : buffer_pos(0),
    stack_pos(0)
{
}

void
JSON::clear(void)
{
  buffer_pos = 0;
  stack_pos = 0;
}

bool
JSON::add_object(void)
{
  if (!obj_separator())
    return false;

  if (!push('o'))
    return false;

  return append("{");
}

bool
JSON::add(const char *key, int32_t value)
{
  if (!is_object())
    return false;

  if (!obj_separator())
    return false;

  return append("\"") && append(key) && append("\":") && append(value);
}

bool
JSON::add(const char *key, const char *value)
{
  if (!is_object())
    return false;

  if (!obj_separator())
    return false;

  return (append("\"") && append(key) && append("\":\"")
          && append(value) && append("\""));
}

bool
JSON::add(const char *key, const uint8_t *data, size_t data_len)
{
  size_t i;
  char buf[4];

  if (!is_object())
    return false;

  if (!obj_separator())
    return false;

  if (!append("\"") || !append(key) || !append("\":\""))
    return false;

  for (i = 0; i < data_len; i++)
    {
      snprintf(buf, sizeof(buf), "%02x", data[i]);
      if (!append(buf))
        return false;
    }

  return append("\"");
}

bool
JSON::add_array(const char *key)
{
  if (!obj_separator())
    return false;

  if (!push('a'))
    return false;

  return append("\"") && append(key) && append("\":") && append("[");
}

bool
JSON::pop(void)
{
  const char *str = "";

  if (stack_pos <= 0)
    return false;

  switch (stack[--stack_pos])
    {
    case 'o':
      str = "}";
      break;

    case 'a':
      str = "]";
      break;

    default:
      return false;
      break;
    }

  return append(str);
}

char *
JSON::finish(void)
{
  while (stack_pos > 0)
    if (!pop())
      return 0;

  if (!append("x"))
    return false;

  buffer[buffer_pos - 1] = '\0';

  return buffer;
}

bool
JSON::push(char type)
{
  if (stack_pos >= STACK_SIZE)
    return false;

  stack[stack_pos++] = type;

  return true;
}

bool
JSON::append(const char *value)
{
  size_t len = strlen(value);

  if (buffer_pos + len > sizeof(buffer))
    return false;

  memcpy(buffer + buffer_pos, value, len);
  buffer_pos += len;

  return true;
}

bool
JSON::append(int32_t value)
{
  char buf[16];

  snprintf(buf, sizeof(buf), "%d", value);

  return append(buf);
}

bool
JSON::is_object()
{
  return stack_pos > 0 && stack[stack_pos - 1] == 'o';
}

bool
JSON::obj_separator()
{
  if (buffer_pos <= 0)
    return true;

  switch (buffer[buffer_pos - 1])
    {
    case '{':
    case '[':
      return true;
      break;

    default:
      break;
    }

  return append(",");
}
