/* -*- c++ -*-
 *
 * ClientInfo.h
 */

#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include "WProgram.h"

class SensorValue
{
public:

  SensorValue();

  /* Value modified. */
  bool dirty;

  uint8_t id[16];
  size_t id_len;
  int32_t value;
};

class ClientInfo
{
public:

  ClientInfo();

  /* Client values modified. */
  bool dirty;

  uint8_t id[16];
  size_t id_len;
  uint32_t last_seqnum;
  uint32_t packetloss;

  static const int MAX_SENSORS = 5;

  SensorValue sensors[MAX_SENSORS];

  /* Look up sensor `id', `id_len'.  The method returns the sensor or
     0 if no such sensor is defined for the client info and there are
     not more space for the sensor value in the client. */
  SensorValue *lookup(const uint8_t *id, size_t id_len);

  static ClientInfo *lookup(ClientInfo *clients, int num_clients,
                            const uint8_t *id, size_t id_len);
};

#endif /* not CLIENTINFO_H */
