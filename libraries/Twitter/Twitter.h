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
#include <EEPROM.h>
#include <sha1.h>
#include <Ethernet.h>
#include <Time.h>

class Twitter
{
public:

  /* Constructs a new Twitter instance with the work buffer `buffer',
     `buffer_len'. */
  Twitter(char *buffer, size_t buffer_len);

  /* Set the Twitter API endpoint configuration.

     The argument `server' specifies the fully qualified server name
     (e.g. `api.twitter.com').  The argument `uri' contains the API
     URI at the server (e.g. `/1/statuses/update.json').

     The arguments `ip', `port' specify the HTTP connection end-point.
     If the argument `proxy' is false, the connection end-point must
     be the Twitter server.  If the argument `proxy' is true, the
     connection end-point must be specify you HTTP proxy server to use
     for the operation.  The `ip' array must remain valid as long as
     this twitter instance is used; the method does not copy the `ip'
     argument but stores a pointer to the provided value. */
  void set_twitter_endpoint(const prog_char server[], const prog_char uri[],
                            uint8_t ip[4], uint16_t port, bool proxy);

  /* Set the client application identification to `consumer_key',
     `consumer_secret'. */
  void set_client_id(const prog_char consumer_key[],
                     const prog_char consumer_secret[]);

  /* Set the twitter account identification to program memory data
     `access_token', `token_secret. */
  void set_account_id(const prog_char access_token[],
                      const prog_char token_secret[]);

  /* Set the twitter account identification to EEPROM memory data
     `access_token', `token_secret'. */
  void set_account_id(int access_token, int token_secret);

  unsigned long get_time(uint8_t ip[4], uint16_t port, const char *server);

  void set_time(unsigned long now);

  /* Post status message `message' to twitter.  The message must be
     UTF-8 encoded.  The method returns true if the status message was
     posted and false on error. */
  bool post_status(const char *message);

  char *url_encode(char *buffer, char ch);
  char *url_encode(char *buffer, const char *data);
  char *url_encode_pgm(char *buffer, const prog_char data[]);
  char *url_encode_eeprom(char *buffer, int address);

  char *hex_encode(char *buffer, const uint8_t *data, size_t data_len);

  char *base64_encode(char *buffer, const uint8_t *data, size_t data_len);

  void auth_add(char ch);

  void auth_add(const char *str);

  void auth_add_pgm(const prog_char str[]);

  void auth_add_param(const char *key, const char *value, char *workbuf);

  void auth_add_param_separator(void);
  void auth_add_value_separator(void);

private:

  void create_nonce(void);

  void compute_authorization(const char *message);

  void http_print(Client *client, const prog_char str[]);
  void http_println(Client *client, const prog_char str[]);
  void http_newline(Client *client);

  /* Print the argument program memory string to serial output. */
  void println(const prog_char str[]);

  bool read_line(Client *client, char *buffer, size_t buflen);

  long parse_date(char *date);

  int parse_month(char *str, char **end);

  /* Flags. */

  /* Is the provided HTTP end point a HTTP proxy or the Twitter
     server? */
  unsigned int proxy : 1;

  /* Is access token in PGM or in EEPROM? */
  unsigned int access_token_pgm : 1;

  uint8_t nonce[8];
  unsigned long timestamp;

  /* This points to Sha1 so don't touch it before you have consumed
     the value. */
  uint8_t *signature;

  char *buffer;
  size_t buffer_len;

  /* Twitter server host name. */
  const prog_char *server;

  /* Twitter API URI at the server. */
  const prog_char *uri;

  /* An IP address to connnect to. */
  uint8_t *ip;

  /* TCP port number to connnect to. */
  uint16_t port;

  /* Application consumer key. */
  const prog_char *consumer_key;

  /* Application consumer secret. */
  const prog_char *consumer_secret;

  /* Twitter account access token. */
  union
  {
    const prog_char *pgm;
    int eeprom;
  } access_token;

  /* Twitter account access token secret. */
  union
  {
    const prog_char *pgm;
    int eeprom;
  } token_secret;
};

#endif /* not TWITTER_H */
