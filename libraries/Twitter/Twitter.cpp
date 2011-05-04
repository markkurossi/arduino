/*
 * Twitter.cpp
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

#include "Twitter.h"

const static char hex_table[] PROGMEM = "0123456789ABCDEF";
const static char base64_table[] PROGMEM
= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const static char s_jan[] PROGMEM = "Jan";
const static char s_feb[] PROGMEM = "Feb";
const static char s_mar[] PROGMEM = "Mar";
const static char s_apr[] PROGMEM = "Apr";
const static char s_may[] PROGMEM = "May";
const static char s_jun[] PROGMEM = "Jun";
const static char s_jul[] PROGMEM = "Jul";
const static char s_aug[] PROGMEM = "Aug";
const static char s_sep[] PROGMEM = "Sep";
const static char s_oct[] PROGMEM = "Oct";
const static char s_nov[] PROGMEM = "Now";
const static char s_dec[] PROGMEM = "Dec";

Twitter::Twitter(char *buffer, size_t buffer_len)
  : buffer(buffer),
    buffer_len(buffer_len),
    server(0),
    uri(0)
{
}

void
Twitter::set_twitter_endpoint(const prog_char server[], const prog_char uri[],
                              uint8_t ip[4], uint16_t port, bool proxy)
{
  this->server = server;
  this->uri = uri;
  this->ip = ip;
  this->port = port;

  this->proxy = proxy ? 1 : 0;
}

void
Twitter::set_client_id(const prog_char consumer_key[],
                       const prog_char consumer_secret[])
{
  this->consumer_key = consumer_key;
  this->consumer_secret = consumer_secret;
}

void
Twitter::set_account_id(const prog_char access_token[],
                        const prog_char token_secret[])
{
  this->access_token.pgm = access_token;
  this->token_secret.pgm = token_secret;

  this->access_token_pgm = 1;
}

void
Twitter::set_account_id(int access_token, int token_secret)
{
  this->access_token.eeprom = access_token;
  this->token_secret.eeprom = token_secret;

  this->access_token_pgm = 0;
}

unsigned long
Twitter::get_time(uint8_t ip[4], uint16_t port, const char *server)
{
  Client http(ip, port);

  if (!http.connect())
    {
      println(PSTR("Could not connect to server"));
      return 0;
    }

  http_print(&http, PSTR("HEAD http://"));
  http.write(server);
  http_println(&http, PSTR("/ HTTP/1.1"));

  http_print(&http, PSTR("Host: "));
  http.write(server);
  http_newline(&http);

  http_println(&http, PSTR("Connection: close"));

  http_newline(&http);

  while (http.connected())
    {
      if (!read_line(&http, buffer, buffer_len))
        {
          http.stop();
          return 0;
        }

      if ((buffer[0] == 'D' || buffer[0] == 'd')
          && (buffer[1] == 'A' || buffer[1] == 'a')
          && (buffer[2] == 'T' || buffer[2] == 't')
          && (buffer[3] == 'E' || buffer[3] == 'e')
          && (buffer[4] == ':'))
        {
          http.stop();
          return parse_date(buffer + 5);
        }
    }

  http.stop();

  return 0;
}

void
Twitter::set_time(unsigned long time)
{
  timestamp = time;
}

long
Twitter::parse_date(char *date)
{
  tmElements_t tm;
  char *cp;
  char *end;

  memset(&tm, 0, sizeof(tm));

  for (cp = date; *cp && *cp != ','; cp++)
    ;

  if (*cp != ',')
    return 0;

  tm.Day = strtol(++cp, &end, 10);

  if (end == cp)
    return 0;

  cp = end;

  for (; *cp && *cp == ' '; cp++)
    ;
  if (!*cp)
    return 0;

  tm.Month = parse_month(cp, &end);
  if (tm.Month < 0)
    return 0;

  cp = end + 1;

  tm.Year = strtol(cp, &end, 10) - 1970;

  if (end == cp)
    return 0;

  cp = end + 1;

  tm.Hour = strtol(cp, &end, 10);

  if (end == cp)
    return 0;

  cp = end + 1;

  tm.Minute = strtol(cp, &end, 10);

  if (end == cp)
    return 0;

  cp = end + 1;

  tm.Second = strtol(cp, &end, 10);

  if (end == cp)
    return 0;

  return makeTime(tm) - millis() / 1000L;
}

int
Twitter::parse_month(char *str, char **end)
{
  char *cp;

  for (cp = str; *cp && *cp != ' '; cp++)
    ;

  if (!*cp)
    return 0;

  *cp = '\0';

  *end = cp;

  if (strcmp_P(str, s_jan) == 0)
    return 1;
  if (strcmp_P(str, s_feb) == 0)
    return 2;
  if (strcmp_P(str, s_mar) == 0)
    return 3;
  if (strcmp_P(str, s_apr) == 0)
    return 4;
  if (strcmp_P(str, s_may) == 0)
    return 5;
  if (strcmp_P(str, s_jun) == 0)
    return 6;
  if (strcmp_P(str, s_jul) == 0)
    return 7;
  if (strcmp_P(str, s_aug) == 0)
    return 8;
  if (strcmp_P(str, s_sep) == 0)
    return 9;
  if (strcmp_P(str, s_oct) == 0)
    return 10;
  if (strcmp_P(str, s_nov) == 0)
    return 11;
  if (strcmp_P(str, s_dec) == 0)
    return 12;

  return 0;
}

bool
Twitter::post_status(const char *message)
{
  char *cp;
  int i;

  create_nonce();
  compute_authorization(message);

  /* Post message to twitter. */

  Client http(ip, port);

  if (!http.connect())
    {
      println(PSTR("Could not connect to server"));
      return false;
    }

  http_print(&http, PSTR("POST "));

  if (proxy)
    {
      http_print(&http, PSTR("http://"));
      http_print(&http, server);
    }

  http_print(&http, uri);
  http_println(&http, PSTR(" HTTP/1.1"));

  http_print(&http, PSTR("Host: "));
  http_print(&http, server);
  http_newline(&http);

  http_println(&http,
               PSTR("Content-Type: application/x-www-form-urlencoded"));
  http_println(&http, PSTR("Connection: close"));

  /* Authorization header. */
  http_print(&http, PSTR("Authorization: OAuth oauth_consumer_key=\""));

  url_encode_pgm(buffer, consumer_key);
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_signature_method=\"HMAC-SHA1"));
  http_print(&http, PSTR("\",oauth_timestamp=\""));

  sprintf(buffer, "%ld", timestamp);
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_nonce=\""));

  hex_encode(buffer, nonce, sizeof(nonce));
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_version=\"1.0\",oauth_token=\""));

  if (access_token_pgm)
    url_encode_pgm(buffer, access_token.pgm);
  else
    url_encode_eeprom(buffer, access_token.eeprom);

  http.write(buffer);

  http_print(&http, PSTR("\",oauth_signature=\""));

  cp = base64_encode(buffer, signature, HASH_LENGTH);
  url_encode(cp + 1, buffer);

  http.write(cp + 1);

  http_println(&http, PSTR("\""));

  /* Encode content. */
  cp = url_encode(buffer, "status");
  *cp++ = '=';
  cp = url_encode(cp, message);

  int content_length = cp - buffer;
  sprintf(cp + 1, "%d", content_length);

  http_print(&http, PSTR("Content-Length: "));
  http.write(cp + 1);
  http_newline(&http);

  /* Header-body separator. */
  http_newline(&http);

  /* And finally content. */
  http.write(buffer);

  /* Read response status line. */
  if (!read_line(&http, buffer, buffer_len) || buffer[0] == '\0')
    {
      http.stop();
      return false;
    }

  int response_code;

  /* HTTP/1.1 200 Success */
  for (i = 0; buffer[i] && buffer[i] != ' '; i++)
    ;
  if (buffer[i])
    response_code = atoi(buffer + i + 1);
  else
    response_code = 0;

  bool success = (200 <= response_code && response_code < 300);

  if (!success)
    Serial.println(buffer);

  /* Skip header. */
  while (true)
    {
      if (!read_line(&http, buffer, buffer_len))
        {
          http.stop();
          return false;
        }

      if (buffer[0] == '\0')
        break;
    }

  /* Handle content. */
  while (http.connected())
    {
      while (http.available() > 0)
        {
          uint8_t byte = http.read();

          if (!success)
            Serial.print(byte, BYTE);
        }
      delay(100);
    }

  http.stop();

  if (!success)
    println(PSTR(""));

  return success;
}

char *
Twitter::url_encode(char *buffer, char ch)
{
  if ('0' <= ch && ch <= '9'
      || 'a' <= ch && ch <= 'z' || 'A' <= ch && ch <= 'Z'
      || ch == '-' || ch == '.' || ch == '_' || ch == '~')
    {
      *buffer++ = ch;
    }
  else
    {
      int val = ((int) ch) & 0xff;

      snprintf(buffer, 4, "%%%02X", val);
      buffer += 3;
    }

  *buffer = '\0';

  return buffer;
}

char *
Twitter::url_encode(char *buffer, const char *data)
{
  char ch;

  while ((ch = *data++))
    buffer = url_encode(buffer, ch);

  return buffer;
}

char *
Twitter::url_encode_pgm(char *buffer, const prog_char data[])
{
  char ch;

  while ((ch = pgm_read_byte(data++)))
    buffer = url_encode(buffer, ch);

  return buffer;
}

char *
Twitter::url_encode_eeprom(char *buffer, int address)
{
  char ch;

  while ((ch = EEPROM.read(address++)))
    buffer = url_encode(buffer, ch);

  return buffer;
}

char *
Twitter::hex_encode(char *buffer, const uint8_t *data, size_t data_len)
{
  size_t i;

  for (i = 0; i < data_len; i++)
    {
      *buffer++ = (char) pgm_read_byte(hex_table + (data[i] >> 4));
      *buffer++ = (char) pgm_read_byte(hex_table + (data[i] & 0x0f));
    }

  *buffer = '\0';

  return buffer;
}

char *
Twitter::base64_encode(char *buffer, const uint8_t *data, size_t data_len)
{
  int i, len;

  for (i = 0, len = data_len; len > 0; i += 3, len -= 3)
    {
      unsigned long val;

      val = (unsigned long) data[i] << 16;
      if (len > 1)
        val |= (unsigned long) data[i + 1] << 8;
      if (len > 2)
        val |= data[i + 2];

      *buffer++ = (char) pgm_read_byte(base64_table + (val >> 18));
      *buffer++ = (char) pgm_read_byte(base64_table + ((val >> 12) & 0x3f));
      *buffer++ = (char) pgm_read_byte(base64_table + ((val >> 6) & 0x3f));
      *buffer++ = (char) pgm_read_byte(base64_table + (val & 0x3f));
    }

  for (; len < 0; len++)
    buffer[len] = '=';

  *buffer = '\0';

  return buffer;
}

void
Twitter::auth_add(char ch)
{
  Sha1.write(ch);
}

void
Twitter::auth_add(const char *str)
{
  Sha1.print(str);
}

void
Twitter::auth_add_pgm(const prog_char str[])
{
  uint8_t c;

  while ((c = pgm_read_byte(str++)))
    Sha1.print(c);
}

void
Twitter::auth_add_param(const prog_char key[], const char *value, char *workbuf)
{
  /* Add separator.  We know that this method is not used to add the
     first parameter. */
  auth_add_param_separator();

  auth_add_pgm(key);

  auth_add_value_separator();

  url_encode(workbuf, value);
  auth_add(workbuf);
}

void
Twitter::auth_add_param_separator(void)
{
  auth_add_pgm(PSTR("%26"));
}

void
Twitter::auth_add_value_separator(void)
{
  auth_add_pgm(PSTR("%3D"));
}

void
Twitter::create_nonce(void)
{
  int i;

  srandom(millis());

  for (i = 0; i < sizeof(nonce); i++)
    nonce[i] = (uint8_t) random();
}

void
Twitter::compute_authorization(const char *message)
{
  char *cp = buffer;

  /* Compute key and init HMAC. */

  cp = url_encode_pgm(buffer, consumer_secret);
  *cp++ = '&';

  if (access_token_pgm)
    cp = url_encode_pgm(cp, token_secret.pgm);
  else
    cp = url_encode_eeprom(cp, token_secret.eeprom);

  Sha1.initHmac((uint8_t *) buffer, cp - buffer);

  auth_add_pgm(PSTR("POST&http%3A%2F%2F"));
  auth_add_pgm(server);

  url_encode_pgm(buffer, uri);
  auth_add(buffer);

  auth_add('&');

  auth_add_pgm(PSTR("oauth_consumer_key"));
  auth_add_value_separator();
  url_encode_pgm(buffer, consumer_key);
  auth_add(buffer);

  cp = hex_encode(buffer, nonce, sizeof(nonce));

  auth_add_param(PSTR("oauth_nonce"), buffer, cp + 1);

  auth_add_param(PSTR("oauth_signature_method"), "HMAC-SHA1", buffer);

  sprintf(buffer, "%ld", timestamp);
  auth_add_param(PSTR("oauth_timestamp"), buffer, cp + 1);

  auth_add_param_separator();

  auth_add_pgm(PSTR("oauth_token"));
  auth_add_value_separator();
  if (access_token_pgm)
    url_encode_pgm(buffer, access_token.pgm);
  else
    url_encode_eeprom(buffer, access_token.eeprom);
  auth_add(buffer);

  auth_add_param(PSTR("oauth_version"), "1.0", buffer);

  cp = url_encode(buffer, message);
  auth_add_param(PSTR("status"), buffer, cp + 1);

  signature = Sha1.resultHmac();

  base64_encode(buffer, signature, HASH_LENGTH);
}

void
Twitter::http_print(Client *client, const prog_char str[])
{
  uint8_t c;

  if (!client || !str)
    return;

  while ((c = pgm_read_byte(str++)))
    client->write(c);
}

void
Twitter::http_println(Client *client, const prog_char str[])
{
  http_print(client, str);
  http_newline(client);
}

void
Twitter::http_newline(Client *client)
{
  client->write('\r');
  client->write('\n');
}

void
Twitter::println(const prog_char str[])
{
  uint8_t c;

  if (!str)
    return;

  while ((c = pgm_read_byte(str++)))
    Serial.write(c);

  Serial.write('\r');
  Serial.write('\n');
}

bool
Twitter::read_line(Client *client, char *buffer, size_t buflen)
{
  size_t pos = 0;

  while (client->connected())
    {
      while (client->available() > 0)
        {
          uint8_t byte = client->read();

          if (byte == '\n')
            {
              /* EOF found. */
              if (pos < buflen)
                {
                  if (pos > 0 && buffer[pos - 1] == '\r')
                    pos--;

                  buffer[pos] = '\0';
                }
              else
                {
                  buffer[buflen - 1] = '\0';
                }

              return true;
            }

          if (pos < buflen)
            buffer[pos++] = byte;
        }

      delay(100);
    }

  return false;
}
