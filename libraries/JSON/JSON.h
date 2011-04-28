/* -*- c++ -*-
 * JSON.h
 */

#ifndef JSON_H
#define JSON_H

#include "WProgram.h"

class JSON
{
public:

  JSON();

  void clear(void);

  bool add_object(void);

  bool add(const char *key, int32_t value);
  bool add(const char *key, const char *value);
  bool add(const char *key, const uint8_t *data, size_t data_len);
  bool add_array(const char *key);

  bool pop(void);
  char *finish(void);

private:

  bool push(char type);
  bool append(const char *value);
  bool append(int32_t value);
  bool is_object();
  bool obj_separator();

  char *buffer;
  size_t buffer_len;
  size_t buffer_pos;

  static const size_t STACK_SIZE = 16;

  char stack[STACK_SIZE];
  size_t stack_pos;
};

#endif /* not JSON_H */
