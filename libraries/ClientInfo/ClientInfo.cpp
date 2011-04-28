/*
 * ClientInfo.cpp
 */

#include "ClientInfo.h"

SensorValue::SensorValue()
  : dirty(false),
    id_len(0),
    value(0)
{
}

ClientInfo::ClientInfo()
  : dirty(false),
    id_len(0),
    last_seqnum((uint32_t) -1),
    packetloss(0)
{
}

SensorValue *
ClientInfo::lookup(const uint8_t *id, size_t id_len)
{
  int i;

  for (i = 0; i < 5; i++)
    {
      SensorValue *value = &sensors[i];

      if (value->id_len == 0)
        break;

      if (value->id_len == id_len && memcmp(id, value->id, id_len) == 0)
        return value;
    }

  if (i >= 5)
    return 0;

  return &sensors[i];
}

ClientInfo *
ClientInfo::lookup(ClientInfo *clients, int num_clients,
                   const uint8_t *id, size_t id_len)
{
  int i;
  ClientInfo *empty = 0;

  for (i = 0; i < 5; i++)
    {
      ClientInfo *client = &clients[i];

      if (client->id_len == 0)
        {
          if (!empty)
            empty = client;
        }
      else
        {
          if (client->id_len == id_len && memcmp(client->id, id, id_len) == 0)
            return client;
        }
    }

  if (!empty)
    return 0;

  memcpy(empty->id, id, id_len);
  empty->id_len = id_len;

  empty->last_seqnum = (uint32_t) -1;
  empty->packetloss = 0;

  return empty;
}
