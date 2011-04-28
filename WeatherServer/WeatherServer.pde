/* -*- c++ -*-
 *
 * WeatherServer.pde
 *
 * Author: Markku Rossi <mtr@iki.fi>
 *
 * Copyright (c) 2011 Markku Rossi
 *
 */

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
#define EEPROM_ADDR_ID		0
#define EEPROM_ADDR_SECRET	(EEPROM_ADDR_ID + 16)
#define EEPROM_ADDR_VERBOSE	(EEPROM_ADDR_SECRET + 16)

SoftwareSerial rfSerial = SoftwareSerial(RF_RX_PIN, RF_TX_PIN);
SerialPacket serialPacket = SerialPacket(&rfSerial);

CommandLine cmdline = CommandLine();

uint8_t id[16];
uint8_t secret[16];

bool verbose = false;

uint32_t msg_seqnum;

#define MAX_CLIENTS 5

ClientInfo clients[MAX_CLIENTS];

JSON json = JSON();

void setup(void)
{
  int i;

  Serial.begin(9600);
  Serial.println("WeatherServer");

  pinMode(RF_RX_PIN, INPUT);
  pinMode(RF_TX_PIN, OUTPUT);

  rfSerial.begin(2400);

  msg_seqnum = 0;

  /* Read configuration parameters. */

  for (i = 0; i < sizeof(id); i++)
    id[i] = EEPROM.read(EEPROM_ADDR_ID + i);
  for (i = 0; i < sizeof(secret); i++)
    secret[i] = EEPROM.read(EEPROM_ADDR_SECRET + i);

  if (EEPROM.read(EEPROM_ADDR_VERBOSE))
    verbose = true;
  else
    verbose = false;

  Serial.print("     id: ");
  for (i = 0; i < sizeof(id); i++)
    Serial.print(id[i], HEX);
  Serial.println("");

  Serial.print(" secret: ");
  for (i = 0; i < sizeof(secret); i++)
    Serial.print(secret[i], HEX);
  Serial.println("");

  Serial.print("verbose: ");
  Serial.println(verbose ? "true" : "false");
}

void process_command()
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
                     "                variables are: `id', `secret', and "
                     " `verbose'");
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

void loop(void)
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

  if (cmdline.read())
    process_command();

  data = serialPacket.receive(&data_len);

  if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                   &data, &data_len)
      || msg_type != HomeWeather::MSG_CLIENT_ID)
    goto malformed_packet;

  client = ClientInfo::lookup(clients, 5, msg_data, msg_len);
  if (client == 0)
    {
      Serial.println("Too many clients");
      return;
    }

  if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                   &data, &data_len)
      || msg_type != HomeWeather::MSG_SEQNUM
      || data_len != 4)
    goto malformed_packet;

  val = SerialPacket::get_32bit(data);

  if (val > client->last_seqnum)
    client->packetloss += val - client->last_seqnum - 1;

  client->last_seqnum = val;
  client->dirty = true;

  /* Process all info messages. */
  while (data_len > 0)
    {
      if (!SerialPacket::parse_message(&msg_type, &msg_data, &msg_len,
                                       &data, &data_len))
        goto malformed_packet;

      switch (msg_type)
        {
        case HomeWeather::MSG_SENSOR_ID:
          sensor = client->lookup(msg_data, msg_len);
          if (sensor == 0)
            {
              Serial.println("Too many sensor values");
              return;
            }
          break;

        case HomeWeather::MSG_SENSOR_VALUE:
          if (sensor == 0 || msg_len != 0)
            goto malformed_packet;

          sensor->value = (int32_t) SerialPacket::get_32bit(msg_data);
          sensor->dirty = true;
          sensor = 0;
          break;

        default:
          goto malformed_packet;
          break;
        }
    }

  /* Post data to server. */

  json.clear();
  json.add_object();

  json.add("id", id, sizeof(id));
  json.add("timestamp", millis());
  json.add("seqnum", msg_seqnum++);

  json.add_array("clients");

  for (i = 0; i < MAX_CLIENTS; i++)
    {
      client = &clients[i];

      if (client->id_len == 0 || !client->dirty)
        continue;

      json.add_object();

      json.add("id", client->id, client->id_len);
      json.add("packets", client->last_seqnum);
      json.add("packetloss", client->packetloss);

      json.add_array("sensors");

      for (j = 0; j < ClientInfo::MAX_SENSORS; j++)
        {
          sensor = &client->sensors[j];

          if (sensor->id_len == 0 || !sensor->dirty)
            continue;

          json.add_object();

          json.add("id", sensor->id, sensor->id_len);
          json.add("value", sensor->value);

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

  return;


  /*
   * Error handling.
   */

 malformed_packet:

  Serial.println("Received malformed packet");
  return;
}
