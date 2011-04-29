/* -*- c++ -*-
 *
 * WeatherServer.pde
 *
 * Author: Markku Rossi <mtr@iki.fi>
 *
 * Copyright (c) 2011 Markku Rossi
 *
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SerialPacket.h>
#include <CommandLine.h>
#include <HomeWeather.h>
#include <ClientInfo.h>
#include <JSON.h>

/* RF transmitter pin. */
#define RF_TX_PIN 5
#define RF_RX_PIN 6

/* EEPROM addresses. */
#define EEPROM_ADDR_CONFIGURED	0
#define EEPROM_ADDR_ID		(EEPROM_ADDR_CONFIGURED + 1)
#define EEPROM_ADDR_SECRET	(EEPROM_ADDR_ID + 16)
#define EEPROM_ADDR_VERBOSE	(EEPROM_ADDR_SECRET + 16)

#define EEPROM_ADDR_MAC		(EEPROM_ADDR_VERBOSE + 1)
#define EEPROM_ADDR_IP		(EEPROM_ADDR_MAC + 6)
#define EEPROM_ADDR_GATEWAY	(EEPROM_ADDR_IP + 4)
#define EEPROM_ADDR_SUBNET	(EEPROM_ADDR_GATEWAY + 4)

SoftwareSerial rfSerial = SoftwareSerial(RF_RX_PIN, RF_TX_PIN);
SerialPacket serialPacket = SerialPacket(&rfSerial);

CommandLine cmdline = CommandLine();

uint8_t id[16];
uint8_t secret[16];

bool verbose = false;

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

uint32_t msg_seqnum;

#define MAX_CLIENTS 2

ClientInfo clients[MAX_CLIENTS];

JSON json = JSON();

Client *http_client = 0;

void
setup(void)
{
  int i;

  Serial.begin(9600);
  Serial.println("WeatherServer");

#if 1
  pinMode(RF_RX_PIN, INPUT);
  pinMode(RF_TX_PIN, OUTPUT);

  rfSerial.begin(2400);

  msg_seqnum = 0;

  /* Read configuration parameters. */

  HomeWeather::read_data(id, sizeof(id), EEPROM_ADDR_ID);
  HomeWeather::read_data(secret, sizeof(secret), EEPROM_ADDR_SECRET);

  if (EEPROM.read(EEPROM_ADDR_VERBOSE))
    verbose = true;
  else
    verbose = false;

  HomeWeather::read_data(mac, sizeof(mac), EEPROM_ADDR_MAC);
  HomeWeather::read_data(ip,	sizeof(ip), EEPROM_ADDR_IP);
  HomeWeather::read_data(gateway, sizeof(gateway), EEPROM_ADDR_GATEWAY);
  HomeWeather::read_data(subnet, sizeof(subnet), EEPROM_ADDR_SUBNET);

  HomeWeather::print_data("     id: ", id, sizeof(id));
  HomeWeather::print_data(" secret: ", secret, sizeof(secret));
  HomeWeather::print_data("    mac: ", mac, sizeof(mac));
  HomeWeather::print_data("     ip: ", ip, sizeof(ip));
  HomeWeather::print_data("gateway: ", gateway, sizeof(gateway));
  HomeWeather::print_data(" subnet: ", subnet, sizeof(subnet));

  if (verbose)
    Serial.println("Verbose");

  if (EEPROM.read(EEPROM_ADDR_CONFIGURED) != 1)
    {
      runlevel = RUNLEVEL_CONFIG;
    }
  else
    {
      runlevel = RUNLEVEL_RUN;
      Ethernet.begin(mac, ip, gateway, subnet);
    }
#endif
}

#if 0
void
process_command()
{
  int argc;
  char **argv = cmdline.get_arguments(&argc);
  int i;

  if (argc < 1)
    return;

  if (strcmp(argv[0], "help") == 0)
    {
      Serial.println("Available commands are:");
      Serial.println("  help          print this help");
      Serial.println("  set VAR VAL   sets the EEPROM variable VAR to the "
                     "value VAL.  Possible\n"
                     "                variables are:"
                     " `id', `secret', `verbose', `configured'"
                     ", `mac', `ip', `gateway', `subnet'");
      Serial.println("  info          show current weather information");
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
          char *arg = argv[2];

          memset(id, 0, sizeof(id));

          if (arg[0] == '0' && arg[0] == 'x')
            {
              CommandLine::hex_decode(arg, id, sizeof(id));
            }
          else
            {
              uint32_t val = (uint32_t) CommandLine::atoi(arg);

              SerialPacket::put_32bit(id, val);
            }

          for (i = 0; i < sizeof(id); i++)
            EEPROM.write(EEPROM_ADDR_ID + i, id[i]);
        }
      else if (strcmp(argv[1], "secret") == 0)
        {
          char *arg = argv[2];

          memset(secret, 0, sizeof(secret));

          if (arg[0] == '0' && arg[1] == 'x')
            {
              CommandLine::hex_decode(arg, secret, sizeof(secret));
            }
          else
            {
              uint32_t val = (uint32_t) CommandLine::atoi(arg);

              SerialPacket::put_32bit(secret, val);
            }

          for (i = 0; i < sizeof(secret); i++)
            EEPROM.write(EEPROM_ADDR_SECRET + i, secret[i]);
        }
      else if (strcmp(argv[1], "verbose") == 0)
        {
          int val = CommandLine::atoi(argv[2]);

          if (val)
            verbose = true;
          else
            verbose = false;

          EEPROM.write(EEPROM_ADDR_VERBOSE, val ? 1 : 0);
        }
      else if (strcmp(argv[1], "configured") == 0)
        {
          int val = CommandLine::atoi(argv[2]);

          if (val)
            runlevel = RUNLEVEL_RUN;
          else
            runlevel = RUNLEVEL_CONFIG;

          EEPROM.write(EEPROM_ADDR_CONFIGURED, val ? 1 : 0);
        }
      else if (strcmp(argv[1], "mac") == 0)
        {
          HomeWeather::parse_data(argv[2], mac, sizeof(mac), EEPROM_ADDR_MAC);
        }
      else if (strcmp(argv[1], "ip") == 0)
        {
          HomeWeather::parse_data(argv[2], ip, sizeof(ip), EEPROM_ADDR_IP);
        }
      else if (strcmp(argv[1], "gateway") == 0)
        {
          HomeWeather::parse_data(argv[2], gateway, sizeof(gateway),
                                  EEPROM_ADDR_GATEWAY);
        }
      else if (strcmp(argv[1], "subnet") == 0)
        {
          HomeWeather::parse_data(argv[2], subnet, sizeof(subnet),
                                  EEPROM_ADDR_SUBNET);
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
#endif

void
loop(void)
{
  uint8_t *data;
  size_t data_len;
  uint8_t msg_type;
  uint8_t *msg_data;
  size_t msg_len;
  ClientInfo *client;
  uint32_t val;
  SensorValue *sensor = 0;
  int i, j;
  char *json_data = 0;
  byte server[] = {173, 236, 146, 80};

  runlevel = 2;

  Serial.println(".");

#if 0
  if (cmdline.read())
    process_command();
#endif

  switch (runlevel)
    {
    case RUNLEVEL_CONFIG:
      /* Just process command line arguments. */
      break;

    case RUNLEVEL_DNS:
      //http_client = new Client(server, 80);
      //http_client->connect();
      break;

    case RUNLEVEL_RUN:
      data = serialPacket.receive(&data_len);

      if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                       &data, &data_len)
          || msg_type != HomeWeather::MSG_CLIENT_ID)
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
          || msg_type != HomeWeather::MSG_SEQNUM
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
            case HomeWeather::MSG_SENSOR_ID:
              Serial.println("SENSOR");
              sensor = client->lookup(msg_data, msg_len);
              if (sensor == 0)
                {
                  Serial.println("E#values");
                  return;
                }
              Serial.println(sensor->id_len);
              break;

            case HomeWeather::MSG_SENSOR_VALUE:
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

          for (j = 0; j < ClientInfo::MAX_SENSORS; j++)
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
      Serial.println(json_data);

      break;
    }

  delay(500);
}
