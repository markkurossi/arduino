/* -*- c++ -*-
 *
 * CommandLine.h
 */

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "WProgram.h"

class CommandLine
{
public:

  CommandLine();

  bool read(void);

  char **get_arguments(int *argc_return);

  static int32_t atoi(const char *input);

  static size_t hex_decode(const char *input, uint8_t *buffer,
                           size_t buffer_len);

private:

  bool split_arguments(void);

  static int atoh(uint8_t ch);

  uint8_t buffer[256];
  size_t buffer_pos;

  static const int MAX_ARGS = 20;

  char *argv[MAX_ARGS];
  int argc;
};

#endif /* not COMMANDLINE_H */
