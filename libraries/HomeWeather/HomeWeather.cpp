/*
 * HomeWeather.cpp
 */

#include "HomeWeather.h"
#include <EEPROM.h>
#include <CommandLine.h>

void
HomeWeather::read_data(uint8_t *buf, size_t buflen, int eeprom_addr)
{
  int i;

  for (i = 0; i < buflen; i++)
    buf[i] = EEPROM.read(eeprom_addr + i);
}

void
HomeWeather::print_data(const char *label, uint8_t *data, size_t datalen)
{
  int i;

  Serial.print(label);

  for (i = 0; i < datalen; i++)
    Serial.print(data[i], HEX);

  Serial.println("");
}

void
HomeWeather::parse_data(const char *val, uint8_t *buf, size_t buflen,
                        int eeprom_addr)
{
  int i;

  memset(buf, 0, buflen);
  CommandLine::hex_decode(val, buf, buflen);

  for (i = 0; i < buflen; i++)
    EEPROM.write(eeprom_addr + i, buf[i]);
}
