/* -*- c++ -*-
 *
 * Twitter.pde
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

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <sha1.h>
#include <Time.h>
#include <Twitter.h>

/* OneWire bus pin. */
#define ONE_WIRE_BUS 4

OneWire one_wire(ONE_WIRE_BUS);
DallasTemperature sensors(&one_wire);

/* Local network configuration. */
uint8_t mac[6] =     {0xC4, 0x2C, 0x3A, 0x3B, 0xC5};
uint8_t ip[4] =      {172, 21, 23, 59};
uint8_t gateway[4] = {172, 21, 23, 1};
uint8_t subnet[4] =  {255, 255, 255, 0};

/* Connect via HTTP proxy. */
uint8_t twitter_ip[4] = {172, 16, 99, 10};
uint16_t twitter_port = 8080;

unsigned long basetime = 0;
unsigned long last_tweet = 0;

#define TWEET_DELTA (5L * 60L)

/* XXX check the size */
char buffer[512];

char *consumer_key = "sxRMVcaaruRhi6hpJgyg";
char *consumer_secret = "NEBmeBtIr5uUTPpIDpeQkKG2GDHSE10XTfzkfweUlv0";
char *token = "124707650-7sXFtUAWMYIuyPW49mGRzOlHgOVtTrXVfuLrJxzF";
char *token_secret = "sSTaef42ZXB3Bzvh14IKv05QuVjCtyT0xveZdGZK4Y";

Twitter twitter(buffer, sizeof(buffer));

void
setup()
{
  Serial.begin(9600);
  Serial.println("Twitter demo");

  sensors.begin();

  Ethernet.begin(mac, ip, gateway, subnet);

  delay(500);
}

void
loop()
{
  int i;

  if (basetime == 0)
    {
      /* Resolve base time. */
      basetime = twitter.get_time(twitter_ip, twitter_port, "api.twitter.com");

      Serial.print("Time is ");
      Serial.println(basetime);
    }
  else
    {
      unsigned long now = basetime + millis() / 1000L;

      if (now > last_tweet + TWEET_DELTA)
        {
          Serial.println("Posting to Twitter");

          last_tweet = now;
          twitter.set_time(now);

          sensors.requestTemperatures();

          float temp = sensors.getTempCByIndex(0);

          if (temp != DEVICE_DISCONNECTED)
            {
              char msg[32];
              long val = temp * 100L;

              sprintf(msg, "Office temperature is %ld.%02ld",
                      val / 100L, val % 100L);

              Serial.println(msg);

              if (true)
                twitter.proxy_post(twitter_ip, twitter_port, msg,
                                   "api.twitter.com",
                                   "/1/statuses/update.json",
                                   consumer_key, consumer_secret, token,
                                   token_secret);
            }
        }
    }

  delay(5000);
}
