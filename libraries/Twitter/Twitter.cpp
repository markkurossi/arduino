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
    buffer_len(buffer_len)
{
}

unsigned long
Twitter::get_time(uint8_t ip[4], uint16_t port, const char *server)
{
  Client http(ip, port);

  if (!http.connect())
    {
      Serial.println("E*connect");
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

  Serial.println(date);

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
Twitter::proxy_post(uint8_t ip[4], uint16_t port, const char *message,
                    const char *server, const char *uri,
                    const char *consumer_key, const char *consumer_secret,
                    const char *token, const char *token_secret)
{
  char *cp;

#if 1
  create_nonce();
#else
  memcpy(nonce, "\xf0\xba\xce\x2f\x79\x8e\xa9\xd1", sizeof(nonce));
  timestamp = 1304409531L;
#endif

  compute_authorization(message, server, uri, consumer_key, consumer_secret,
                        token, token_secret);

  /* Post message to twitter. */

  Client http(ip, port);

  if (!http.connect())
    {
      Serial.println("E*connect");
      return false;
    }

  http_print(&http, PSTR("POST http://"));
  http.write(server);
  http.write(uri);
  http_println(&http, PSTR(" HTTP/1.1"));

  http_print(&http, PSTR("Host: "));
  http.write(server);
  http_newline(&http);

  http_println(&http,
               PSTR("Content-Type: application/x-www-form-urlencoded"));
  http_println(&http, PSTR("Connection: close"));

  /* Authorization header. */
  http_print(&http, PSTR("Authorization: OAuth oauth_consumer_key=\""));

  url_encode(buffer, consumer_key);
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_signature_method=\"HMAC-SHA1"));
  http_print(&http, PSTR("\",oauth_timestamp=\""));

  sprintf(buffer, "%ld", timestamp);
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_nonce=\""));

  hex_encode(buffer, nonce, sizeof(nonce));
  http.write(buffer);

  http_print(&http, PSTR("\",oauth_version=\"1.0\",oauth_token=\""));

  url_encode(buffer, token);
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

  while (http.connected())
    {
      while (http.available() > 0)
        {
          uint8_t byte = http.read();

          Serial.print(byte, BYTE);
        }
      delay(100);
    }

  http.stop();
  Serial.println("");

  return true;
}

char *
Twitter::url_encode(char *buffer, const char *data)
{
  for (; *data; data++)
    {
      char cp = *data;

      if ('0' <= cp && cp <= '9'
          || 'a' <= cp && cp <= 'z' || 'A' <= cp && cp <= 'Z'
          || cp == '-' || cp == '.' || cp == '_' || cp == '~')
        {
          *buffer++ = cp;
        }
      else
        {
          snprintf(buffer, 4, "%%%02X", cp);
          buffer += 3;
        }
    }

  *buffer = '\0';

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
Twitter::auth_add_param(bool first, const prog_char key[], const char *value,
                        char *workbuf)
{
  if (!first)
    auth_add_pgm(PSTR("%26"));

  auth_add_pgm(key);

  auth_add_pgm(PSTR("%3D"));

  url_encode(workbuf, value);
  auth_add(workbuf);
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
Twitter::compute_authorization(const char *message, const char *server,
                               const char *uri, const char *consumer_key,
                               const char *consumer_secret, const char *token,
                               const char *token_secret)
{
  char *cp = buffer;

  /* Compute key and init HMAC. */

  cp = url_encode(buffer, consumer_secret);
  *cp++ = '&';
  cp = url_encode(cp, token_secret);

  Sha1.initHmac((uint8_t *) buffer, cp - buffer);

  auth_add_pgm(PSTR("POST&http%3A%2F%2F"));
  auth_add(server);

  url_encode(buffer, uri);
  auth_add(buffer);

  auth_add('&');

  auth_add_param(true,  PSTR("oauth_consumer_key"), consumer_key, buffer);

  cp = hex_encode(buffer, nonce, sizeof(nonce));

  auth_add_param(false, PSTR("oauth_nonce"), buffer, cp + 1);

  auth_add_param(false, PSTR("oauth_signature_method"), "HMAC-SHA1", buffer);

  sprintf(buffer, "%ld", timestamp);
  auth_add_param(false, PSTR("oauth_timestamp"), buffer, cp + 1);

  auth_add_param(false, PSTR("oauth_token"), token, buffer);
  auth_add_param(false, PSTR("oauth_version"), "1.0", buffer);

  cp = url_encode(buffer, message);
  auth_add_param(false, PSTR("status"), buffer, cp + 1);

  signature = Sha1.resultHmac();

  base64_encode(buffer, signature, HASH_LENGTH);

  Serial.print("Signature: ");
  Serial.println(buffer);
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
