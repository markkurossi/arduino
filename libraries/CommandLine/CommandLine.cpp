/*
 * CommandLine.cpp
 */

#include "CommandLine.h"

#define ISSPACE(ch)     \
((ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')

CommandLine::CommandLine()
  : buffer_pos(0),
    argc(0)
{
}

bool
CommandLine::read(void)
{
  while (Serial.available() > 0)
    {
      uint8_t byte = Serial.read();

      if (byte == '\n')
        {
          buffer[buffer_pos] = '\0';
          return split_arguments();
        }

      /* Reserver space for the trailing null-character. */
      if (buffer_pos + 2 < sizeof(buffer))
        buffer[buffer_pos++] = byte;
    }

  return false;
}

char **
CommandLine::get_arguments(int *argc_return)
{
  *argc_return = argc;

  /* Prepare for the next line. */
  buffer_pos = 0;
  argc = 0;

  return argv;
}

int32_t
CommandLine::atoi(const char *input)
{
  int32_t val = 0;
  int i;

  for (i = 0; input[i]; i++)
    {
      char ch  = input[i];

      if ('0' <= ch && ch <= '9')
        {
          val *= 10;
          val += ch - '0';
        }
      else
        {
          break;
        }
    }

  return val;
}

size_t
CommandLine::hex_decode(const char *input, uint8_t *buffer, size_t buffer_len)
{
  int len;
  int i;
  int pos = 0;
  int val, v;

  if (input[0] == '0' && input[1] == 'x')
    input += 2;

  len = strlen(input);

  if ((len % 2) == 0)
    {
      i = 0;
    }
  else
    {
      i = 1;

      val = atoh(buffer[0]);
      if (val < 0 || pos >= buffer_len)
        return pos;

      buffer[pos++] = (uint8_t) val;
    }

  for (; i < len; i += 2)
    {
      val = atoh(input[i]);
      v = atoh(input[i + 1]);

      if (val < 0 || v < 0 || pos >= buffer_len)
        return pos;

      val <<= 4;
      val |= v;

      buffer[pos++] = (uint8_t) val;
    }

  return pos;
}

bool
CommandLine::split_arguments(void)
{
  size_t i = 0;

  for (argc = 0; argc < MAX_ARGS; argc++)
    {
      /* Skip leading whitespace. */
      for (; i < buffer_pos && ISSPACE(buffer[i]); i++)
        ;
      if (i >= buffer_pos)
        break;

      argv[argc] = (char *) (buffer + i);

      /* Find the end of this argument. */
      for (; i < buffer_pos && !ISSPACE(buffer[i]); i++)
        ;
      if (i >= buffer_pos)
        {
          argc++;
          break;
        }

      /* Make argument null-terminated. */
      buffer[i++] = '\0';
    }

  return argc > 0;
}

int
CommandLine::atoh(uint8_t ch)
{
  if ('0' <= ch && ch <= '9')
    return ch - '0';
  if ('a' <= ch && ch <= 'f')
    return 10 + ch - 'a';
  if ('A' <= ch && ch <= 'F')
    return 10 + ch - 'A';

  return -1;
}
