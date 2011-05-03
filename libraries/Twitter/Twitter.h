/* -*- c++ -*-
 *
 * Twitter.h
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

#ifndef TWITTER_H
#define TWITTER_H

#include "WProgram.h"
#include <avr/pgmspace.h>
#include <sha1.h>
#include <Ethernet.h>
#include <Time.h>

class Twitter
{
public:
  Twitter(char *buffer, size_t buffer_len);

  unsigned long get_time(uint8_t ip[4], uint16_t port, const char *server);

  void set_time(unsigned long now);

  bool proxy_post(uint8_t ip[4], uint16_t port, const char *message,
                  const char *server, const char *uri,
                  const char *consumer_key, const char *consumer_secret,
                  const char *token, const char *token_secret);

  char *url_encode(char *buffer, const char *data);

  char *hex_encode(char *buffer, const uint8_t *data, size_t data_len);

  char *base64_encode(char *buffer, const uint8_t *data, size_t data_len);

  void auth_add(char ch);

  void auth_add(const char *str);

  void auth_add_pgm(const prog_char str[]);

  void auth_add_param(bool first, const char *key, const char *value,
                      char *workbuf);

private:

  void create_nonce(void);

  void compute_authorization(const char *message, const char *server,
                             const char *uri, const char *consumer_key,
                             const char *consumer_secret, const char *token,
                             const char *token_secret);

  void http_print(Client *client, const prog_char str[]);
  void http_println(Client *client, const prog_char str[]);
  void http_newline(Client *client);

  bool read_line(Client *client, char *buffer, size_t buflen);

  long parse_date(char *date);

  int parse_month(char *str, char **end);

  uint8_t nonce[8];
  unsigned long timestamp;

  /* This points to Sha1 so don't touch it before you have consumed
     the value. */
  uint8_t *signature;

  char *buffer;
  size_t buffer_len;
};

#endif /* not TWITTER_H */
