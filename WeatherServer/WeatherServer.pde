/* -*- c++ -*-
 *
 * WeatherServer.pde
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
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <SerialPacket.h>
#include <CommandLine.h>
#include <GetPut.h>
#include <HomeWeather.h>
#include <ClientInfo.h>
#include <JSON.h>

/* RF pins. */
#define RF_RX_PIN 2
#define RF_TX_PIN 3

/* OneWire bus pin. */
#define ONE_WIRE_BUS 4

#define ID_LEN 8
#define SECRET_LEN 8

#define HTTP_SERVER_LEN 32

/* EEPROM addresses. */
#define EEPROM_ADDR_CONFIGURED	0
#define EEPROM_ADDR_ID		(EEPROM_ADDR_CONFIGURED + 1)
#define EEPROM_ADDR_SECRET	(EEPROM_ADDR_ID + ID_LEN)
#define EEPROM_ADDR_VERBOSE	(EEPROM_ADDR_SECRET + SECRET_LEN)

#define EEPROM_ADDR_MAC		(EEPROM_ADDR_VERBOSE + 1)
#define EEPROM_ADDR_IP		(EEPROM_ADDR_MAC + 6)
#define EEPROM_ADDR_GATEWAY	(EEPROM_ADDR_IP + 4)
#define EEPROM_ADDR_SUBNET	(EEPROM_ADDR_GATEWAY + 4)

#define EEPROM_ADDR_HTTP_SERVER	(EEPROM_ADDR_SUBNET + 4)
#define EEPROM_ADDR_HTTP_PORT	(EEPROM_ADDR_HTTP_SERVER + HTTP_SERVER_LEN)
#define EEPROM_ADDR_PROXY_SERVER	(EEPROM_ADDR_HTTP_PORT + 2)
#define EEPROM_ADDR_PROXY_PORT	(EEPROM_ADDR_PROXY_SERVER + 4)

SoftwareSerial rfSerial = SoftwareSerial(RF_RX_PIN, RF_TX_PIN);
SerialPacket serialPacket = SerialPacket(&rfSerial);

/* Setup a OneWire instance to communicate with any OneWire devices
   (not just Maxim/Dallas temperature ICs). */
OneWire one_wire(ONE_WIRE_BUS);

/* Dallas Temperature library runnin on our OneWire bus. */
DallasTemperature sensors(&one_wire);

CommandLine cmdline = CommandLine();

uint8_t id[ID_LEN];
uint8_t secret[SECRET_LEN];

uint8_t verbose = 0;

/* Initial device configuration missing. */
#define RUNLEVEL_CONFIG	0

/* Resolving HTTP server IP address. */
#define RUNLEVEL_DNS	1

/* Normal runlevel. */
#define RUNLEVEL_RUN	2

int runlevel;

uint8_t mac[6];
uint8_t ip[4];
uint8_t gateway[4];
uint8_t subnet[4];

/* Is ethernet configured? */
bool ethernet_configured = false;

/* HTTP server DNS name. */
uint8_t http_server[HTTP_SERVER_LEN];

/* HTTP server port number. */
uint16_t http_port;

/* Should we use proxy server? */
bool use_proxy = false;

/* HTTP proxy server IP address. */
uint8_t proxy_server[4];

/* HTTP proxy server port number. */
uint16_t proxy_port;

uint32_t msg_seqnum;

#define MAX_CLIENTS 2

ClientInfo clients[MAX_CLIENTS];

char json_buffer[512];
JSON json = JSON(json_buffer, sizeof(json_buffer));

Client *http_client = 0;

const char bannerstr[] PROGMEM = "\
WeatherServer <http://www.iki.fi/mtr/HomeWeather/>\n\
Copyright (c) 2011 Markku Rossi <mtr@iki.fi>\n\
\n";

const char usagestr[] PROGMEM = "\
Available commands are:\n\
  help          print this help\n\
  set VAR VAL   sets the EEPROM variable VAR to the value VAL.  Possible\n\
                variables are:\n\
                  `id', `secret', `verbose', `configured',\n\
                  `mac', `ip', `gw', `subnet'\n\
  info          show current weather information\n";

const char http_proxy_prefix[] PROGMEM = "POST http://";
const char http_prefix[] PROGMEM = "POST ";
const char http_suffix[] PROGMEM = "/data_api/add HTTP/1.0\r\n\
Content-Type: application/json\r\n\
Connection: close\r\n\
Host: ";
const char http_separator[] PROGMEM = "\r\n\r\n";

void
setup(void)
{
  int i;
  uint8_t buf[2];

  Serial.begin(9600);
  HomeWeather::print_progstr(bannerstr);

  /* Start temperature sensons. */
  sensors.begin();

  pinMode(RF_RX_PIN, INPUT);
  pinMode(RF_TX_PIN, OUTPUT);

  rfSerial.begin(2400);

  msg_seqnum = 0;

  /* Read configuration parameters. */

  GetPut::eeprom_read_data(id, sizeof(id), EEPROM_ADDR_ID);
  GetPut::eeprom_read_data(secret, sizeof(secret), EEPROM_ADDR_SECRET);

  verbose = EEPROM.read(EEPROM_ADDR_VERBOSE);

  GetPut::eeprom_read_data(mac, sizeof(mac), EEPROM_ADDR_MAC);
  GetPut::eeprom_read_data(ip,	sizeof(ip), EEPROM_ADDR_IP);
  GetPut::eeprom_read_data(gateway, sizeof(gateway), EEPROM_ADDR_GATEWAY);
  GetPut::eeprom_read_data(subnet, sizeof(subnet), EEPROM_ADDR_SUBNET);

  GetPut::eeprom_read_data(http_server, sizeof(http_server),
                           EEPROM_ADDR_HTTP_SERVER);
  if (http_server[0] == 0xff)
    http_server[0] = '\0';

  GetPut::eeprom_read_data(buf, sizeof(buf), EEPROM_ADDR_HTTP_PORT);
  http_port = GetPut::get_16bit(buf);

  GetPut::eeprom_read_data(proxy_server, sizeof(proxy_server),
                           EEPROM_ADDR_PROXY_SERVER);
  GetPut::eeprom_read_data(buf, sizeof(buf), EEPROM_ADDR_PROXY_PORT);
  proxy_port = GetPut::get_16bit(buf);

  HomeWeather::print_data(12,       "id", id, sizeof(id));
  HomeWeather::print_data(8,   "secret", secret, sizeof(secret));
  HomeWeather::print_data(11,      "mac", mac, sizeof(mac));
  HomeWeather::print_dotted(12,     "ip", ip, sizeof(ip));
  HomeWeather::print_dotted(12,     "gw", gateway, sizeof(gateway));
  HomeWeather::print_dotted(8, "subnet", subnet, sizeof(subnet));

  HomeWeather::print_label(3, "http-server");
  Serial.println((char *) http_server);
  HomeWeather::print_label(5, "http-port");
  Serial.println(http_port);

  HomeWeather::print_dotted(2, "proxy-server",
                            proxy_server, sizeof(proxy_server));
  HomeWeather::print_label(4, "proxy-port");
  Serial.println(proxy_port);

  HomeWeather::print_label(7, "verbose");
  Serial.println((int) verbose);

  if (EEPROM.read(EEPROM_ADDR_CONFIGURED) != 1)
    runlevel = RUNLEVEL_CONFIG;
  else
    runlevel = RUNLEVEL_DNS;

  HomeWeather::print_label(6, "runlevel");
  Serial.println((int) runlevel);
}

void
process_command()
{
  int argc;
  char **argv = cmdline.get_arguments(&argc);
  int i;
  uint8_t buf[2];

  if (argc < 1)
    return;

  if (strcmp(argv[0], "help") == 0)
    {
      HomeWeather::print_progstr(usagestr);
    }
  else if (strcmp(argv[0], "set") == 0)
    {
      if (argc != 3)
        {
          Serial.println("Invalid amount of arguments for command `set'");
          return;
        }

      if (strcmp(argv[1], "id") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], id, sizeof(id), EEPROM_ADDR_ID);
        }
      else if (strcmp(argv[1], "secret") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], secret, sizeof(secret),
                                    EEPROM_ADDR_SECRET);
        }
      else if (strcmp(argv[1], "verbose") == 0)
        {
          verbose = (uint8_t) atoi(argv[2]);
          EEPROM.write(EEPROM_ADDR_VERBOSE, verbose);
        }
      else if (strcmp(argv[1], "configured") == 0)
        {
          int val = atoi(argv[2]);

          if (val)
            runlevel = RUNLEVEL_DNS;
          else
            runlevel = RUNLEVEL_CONFIG;

          EEPROM.write(EEPROM_ADDR_CONFIGURED, val ? 1 : 0);
        }
      else if (strcmp(argv[1], "mac") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], mac, sizeof(mac), EEPROM_ADDR_MAC);
        }
      else if (strcmp(argv[1], "ip") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], ip, sizeof(ip), EEPROM_ADDR_IP);
        }
      else if (strcmp(argv[1], "gw") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], gateway, sizeof(gateway),
                                    EEPROM_ADDR_GATEWAY);
        }
      else if (strcmp(argv[1], "subnet") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], subnet, sizeof(subnet),
                                    EEPROM_ADDR_SUBNET);
        }
      else if (strcmp(argv[1], "http-server") == 0)
        {
          for (i = 0; argv[2][i] && i < HTTP_SERVER_LEN - 1; i++)
            http_server[i] = argv[2][i];
          http_server[i] = '\0';

          GetPut::eeprom_write_data(http_server, sizeof(http_server),
                                    EEPROM_ADDR_HTTP_SERVER);
        }
      else if (strcmp(argv[1], "http-port") == 0)
        {
          http_port = strtol(argv[2], 0, 10);

          GetPut::put_16bit(buf, http_port);
          GetPut::eeprom_write_data(buf, sizeof(buf), EEPROM_ADDR_HTTP_PORT);
        }
      else if (strcmp(argv[1], "proxy-server") == 0)
        {
          GetPut::eeprom_parse_data(argv[2], proxy_server, sizeof(proxy_server),
                                    EEPROM_ADDR_PROXY_SERVER);
        }
      else if (strcmp(argv[1], "proxy-port") == 0)
        {
          proxy_port = strtol(argv[2], 0, 0);

          GetPut::put_16bit(buf, proxy_port);
          GetPut::eeprom_write_data(buf, sizeof(buf), EEPROM_ADDR_PROXY_PORT);
        }
      else
        {
          Serial.print("Unknown variable `");
          Serial.print(argv[1]);
          Serial.println("'");
        }
    }
  else if (strcmp(argv[0], "info") == 0)
    {
    }
  else
    {
      Serial.print("Unknown command `");
      Serial.print(argv[0]);
      Serial.println("'");
    }
}

static void
resolve_dns(void)
{
  if (!ethernet_configured)
    {
      /* Configure our ethernet interface. */

      if (verbose)
        Serial.println("Configuring ethernet");

      Ethernet.begin(mac, ip, gateway, subnet);
      ethernet_configured = true;
    }

  if (proxy_port != 0xffff)
    {
      /* We have a HTTP proxy server configuration.  No need to
         resolve server DNS. */

      if (verbose)
        Serial.println("Using proxy server");

      use_proxy = true;
      runlevel = RUNLEVEL_RUN;
      return;
    }

  Serial.println("Resolving server IP");

  //http_client = new Client(server, 80);
  //http_client->connect();
}

static void
poll_rf_clients(void)
{
  uint8_t *data;
  size_t data_len;
  uint8_t msg_type;
  uint8_t *msg_data;
  size_t msg_len;
  uint32_t val;
  ClientInfo *client;
  SensorValue *sensor = 0;

  data = serialPacket.receive(&data_len);

  if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                   &data, &data_len)
      || msg_type != MSG_CLIENT_ID)
    {
      Serial.print("E1");
      delay(500);
      return;
    }

  client = ClientInfo::lookup(clients, MAX_CLIENTS, msg_data, msg_len);
  if (client == 0)
    {
      Serial.println("E#clients");
      return;
    }

  if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                   &data, &data_len)
      || msg_type != MSG_SEQNUM
      || msg_len != 4)
    {
      Serial.println("E2");
      delay(500);
      return;
    }

  val = SerialPacket::get_32bit(msg_data);

  if (val > client->last_seqnum)
    client->packetloss += val - client->last_seqnum - 1;

  client->last_seqnum = val;
  client->dirty = true;

  /* Process all info messages. */
  while (data_len > 0)
    {
      if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                       &data, &data_len))
        {
          Serial.println("E3");
          delay(500);
          return;
        }

      switch (msg_type)
        {
        case MSG_SENSOR_ID:
          Serial.println("SENSOR");
          sensor = client->lookup(msg_data, msg_len);
          if (sensor == 0)
            {
              Serial.println("E#values");
              return;
            }
          Serial.println(sensor->id_len);
          break;

        case MSG_SENSOR_VALUE:
          Serial.println("VALUE");
          if (sensor == 0 || msg_len != 4)
            {
              Serial.println("E4");
              delay(500);
              return;
            }

          sensor->value = (int32_t) SerialPacket::get_32bit(msg_data);
          sensor->dirty = true;
          Serial.println(sensor->value);
          sensor = 0;
          break;

        default:
          Serial.println("E5");
          delay(500);
          return;
          break;
        }
    }
}

static void
poll_local_sensors(void)
{
  DeviceAddress addr;
  int count;
  int i;

  sensors.requestTemperatures();

  count = sensors.getDeviceCount();
  for (i = 0; i < count; i++)
    {
      if (!sensors.getAddress(addr, i))
        continue;

      float temp = sensors.getTempC(addr);

      if (temp == DEVICE_DISCONNECTED)
        continue;

      if (verbose)
        {
          Serial.print("Temp ");
          Serial.print(i);
          Serial.print(" is ");
          Serial.println(temp);
        }
    }
}

static void
post_json_to_server(void)
{
  ClientInfo *client;
  SensorValue *sensor;
  int i, j;
  char *json_data = 0;

  /* Post data to server. */

  json.clear();
  json.add_object();

  json.add("id", id, sizeof(id));
  json.add("ts", millis());
  json.add("sn", msg_seqnum++);

  json.add_array("c");

  for (i = 0; i < MAX_CLIENTS; i++)
    {
      client = &clients[i];

      if (client->id_len == 0 || !client->dirty)
        continue;

      json.add_object();

      json.add("id", client->id, client->id_len);
      json.add("pkt", client->last_seqnum);
      json.add("loss", client->packetloss);

      json.add_array("s");

      for (j = 0; j < CLIENT_INFO_MAX_SENSORS; j++)
        {
          sensor = &client->sensors[j];

          if (sensor->id_len == 0 || !sensor->dirty)
            continue;

          json.add_object();

          json.add("id", sensor->id, sensor->id_len);
          json.add("v", sensor->value);

          json.pop();

          sensor->dirty = false;
        }

      /* Finish sensors array. */
      json.pop();

      /* Finish client object. */
      json.pop();

      client->dirty = false;
    }

  json_data = json.finish();
  if (verbose > 1)
    Serial.println(json_data);

  uint8_t *server = proxy_server;
  uint16_t port = proxy_port;

  Client http_client(server, port);

  if (!http_client.connect())
    {
      if (verbose)
        Serial.println("Failed to connect to server");
      return;
    }

  if (use_proxy)
    {
      HomeWeather::print_progstr(&http_client, http_proxy_prefix);
      http_client.write((const char *) http_server);
    }
  else
    {
      HomeWeather::print_progstr(&http_client, http_prefix);
    }

  HomeWeather::print_progstr(&http_client, http_suffix);
  http_client.write((const char *) http_server);
  HomeWeather::print_progstr(&http_client, http_separator);

  http_client.write(json_data);

  /* Read response. */
  while (http_client.connected())
    {
      while (http_client.available() > 0)
        {
          uint8_t byte = http_client.read();
          Serial.print((int) byte, BYTE);
        }
      delay(100);
    }

  http_client.stop();
}

void
loop(void)
{
#if 1
  if (cmdline.read())
    process_command();
#endif

  switch (runlevel)
    {
    case RUNLEVEL_CONFIG:
      /* Just process command line arguments. */
      break;

    case RUNLEVEL_DNS:
      resolve_dns();
      break;

    case RUNLEVEL_RUN:
      // poll_rf_clients();
      poll_local_sensors();
      post_json_to_server();
      break;
    }

  delay(500);
}
