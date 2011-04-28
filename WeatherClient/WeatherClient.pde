/* -*- c++ -*-
 *
 * WeatherClient.pde
 *
 */

#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <SerialPacket.h>
#include <CommandLine.h>
#include <HomeWeather.h>

/* Temperature sensor data wire is plugged into port 2 on the
   Arduino. */
#define ONE_WIRE_BUS 2

/* RF transmitter pin. */
#define RF_TX_PIN 4
#define RF_RX_PIN 5

#define RX_LED_PIN 6
#define OW_LED_PIN 7

/* EEPROM addresses. */
#define EEPROM_ADDR_ID		0
#define EEPROM_ADDR_SECRET	(EEPROM_ADDR_ID + 16)
#define EEPROM_ADDR_VERBOSE	(EEPROM_ADDR_SECRET + 16)

SoftwareSerial rfSerial = SoftwareSerial(RF_RX_PIN, RF_TX_PIN);
SerialPacket serialPacket = SerialPacket(&rfSerial);

CommandLine cmdline = CommandLine();

uint8_t client_id[16];
uint8_t client_secret[16];

bool verbose = false;

uint32_t msg_seqnum;

uint8_t buffer[255];

/*
 * Setup a oneWire instance to communicate with any OneWire devices
 * (not just Maxim/Dallas temperature ICs). */
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup(void)
{
  int i;

  // start serial port
  Serial.begin(9600);
  Serial.println("WeatherClient");

  // Start up the library
  sensors.begin();

  /* Start RF transmitter. */
  pinMode(RF_RX_PIN, INPUT);
  pinMode(RF_TX_PIN, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  pinMode(OW_LED_PIN, OUTPUT);

  rfSerial.begin(2400);

  msg_seqnum = 0;

  /* Read configuration parameters.*/

  for (i = 0; i < sizeof(client_id); i++)
    client_id[i] = EEPROM.read(EEPROM_ADDR_ID + i);
  for (i = 0; i < sizeof(client_secret); i++)
    client_secret[i] = EEPROM.read(EEPROM_ADDR_SECRET + i);

  if (EEPROM.read(EEPROM_ADDR_VERBOSE))
    verbose = true;
  else
    verbose = false;

  Serial.print("     id: ");
  for (i = 0; i < sizeof(id); i++)
    Serial.print(id[i], HEX);
  Serial.println("");

  Serial.print(" secret: ");
  for (i = 0; i < sizeof(client_secret); i++)
    Serial.print(client_secret[i], HEX);
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

          memset(client_id, 0, sizeof(client_id));

          if (arg[0] == '0' && arg[0] == 'x')
            {
              CommandLine::hex_decode(arg, client_id, sizeof(client_id));
            }
          else
            {
              uint32_t id = (uint32_t) CommandLine::atoi(arg);

              SerialPacket::put_32bit(client_id, id);
            }

          for (i = 0; i < sizeof(client_id); i++)
            EEPROM.write(EEPROM_ADDR_ID + i, client_id[i]);
        }
      else if (strcmp(argv[1], "secret") == 0)
        {
          char *arg = argv[2];

          memset(client_secret, 0, sizeof(client_secret));

          if (arg[0] == '0' && arg[1] == 'x')
            {
              CommandLine::hex_decode(arg, client_secret,
                                      sizeof(client_secret));
            }
          else
            {
              uint32_t secret = (uint32_t) CommandLine::atoi(arg);

              SerialPacket::put_32bit(client_secret, secret);
            }

          for (i = 0; i < sizeof(client_secret); i++)
            EEPROM.write(EEPROM_ADDR_SECRET + i, client_secret[i]);
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
  if (cmdline.read())
    process_command();

  // Call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus

  if (verbose)
    Serial.print("Requesting temperatures...");

  sensors.requestTemperatures();

  if (verbose)
    Serial.println("DONE");

  /* Construct message containing all sensor readings. */

  DeviceAddress addr;
  int count = sensors.getDeviceCount();
  int i;

  serialPacket.clear();

  serialPacket.add_message(HomeWeather::MSG_CLIENT_ID,
                           client_id, sizeof(client_id));
  serialPacket.add_message(HomeWeather::MSG_SEQNUM, msg_seqnum++);

  for (i = 0; i < count; i++)
    {
      if (!sensors.getAddress(addr, i))
        continue;

      float temp = sensors.getTempC(addr);

      if (temp == DEVICE_DISCONNECTED)
        continue;

      if (verbose)
        {
          Serial.print("Temperature ");
          Serial.print(i);
          Serial.print(" is: ");
          Serial.println(temp);
        }

      serialPacket.add_message(HomeWeather::MSG_SENSOR_ID, addr, sizeof(addr));
      serialPacket.add_message(HomeWeather::MSG_SENSOR_VALUE,
                               (uint32_t) (temp * 100));
    }

  serialPacket.send();

  delay(500);
}
