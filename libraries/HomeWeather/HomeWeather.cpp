/*
 * HomeWeather.cpp
 *
 * Author: Markku Rossi <mtr@iki.fi>
 *
 * Copyright (c) 2011 Markku Rossi
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include "HomeWeather.h"

void
HomeWeather::print_label(int indent, const char *label)
{
  while (indent--)
    Serial.print(" ");

  Serial.print(label);

  Serial.print(": ");
}

void
HomeWeather::print_data(int indent, const char *label,
                        uint8_t *data, size_t datalen)
{
  int i;

  print_label(indent, label);

  for (i = 0; i < datalen; i++)
    Serial.print(data[i], HEX);

  Serial.println("");
}

void
HomeWeather::print_dotted(int indent, const char *label,
                          uint8_t *data, size_t datalen)
{
  int i;

  print_label(indent, label);

  for (i = 0; i < datalen; i++)
    {
      if (i > 0)
        Serial.print(".");
      Serial.print((int) data[i]);
    }

  Serial.println("");
}

void
HomeWeather::print_progstr(const prog_char str[])
{
  char c;

  if (!str)
    return;

  while ((c = pgm_read_byte(str++)))
    Serial.print(c, BYTE);
}

void
HomeWeather::print_progstr(Client *client, const prog_char str[])
{
  uint8_t c;

  if (!client || !str)
    return;

  while ((c = pgm_read_byte(str++)))
    client->write(c);
}
