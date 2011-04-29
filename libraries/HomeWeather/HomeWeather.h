/* -*- c++ -*-
 *
 * HomeWeather.h
 */

#include "WProgram.h"

class HomeWeather
{
public:

  static const int MSG_CLIENT_ID	= 0;
  static const int MSG_SEQNUM		= 1;
  static const int MSG_SENSOR_ID	= 2;
  static const int MSG_SENSOR_VALUE	= 3;

  static void read_data(uint8_t *buf, size_t buflen, int eeprom_addr);
  static void print_data(const char *label, uint8_t *data, size_t datalen);
  static void parse_data(const char *val, uint8_t *buf, size_t buflen,
                         int eeprom_addr);
};
