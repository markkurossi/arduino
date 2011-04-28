/* -*- c++ -*-
 *
 * SerialPacket.h
 */

#ifndef SERIALPACKET_H
#define SERIALPACKET_H

#include "WProgram.h"
#include <SoftwareSerial.h>

class SerialPacket
{
 public:

  SerialPacket(SoftwareSerial *serial);

  /* Sends the packet `data', `data_len'.  The method returns true if
     the packet was sent and false on error. */
  bool send(uint8_t *data, size_t data_len);

  /* Receives a packet from the serial port. */
  uint8_t *receive(size_t *len_return);

  /* Clears the packet's buffer and prepare for new message
     construction. */
  void clear(void);

  bool add_message(uint8_t type, const uint8_t *data, size_t data_len);

  bool add_message(uint8_t type, uint32_t value);

  /* Sends the message that has been currenlty constructed using the
     clear() and add_message() methods. */
  bool send(void);

  static bool parse_message(uint8_t *type_return, uint8_t **msg_return,
                            size_t *msg_len_return,
                            uint8_t **datap, size_t *data_lenp);

  static void put_32bit(uint8_t *buf, uint32_t val);

  static uint32_t get_32bit(uint8_t *buf);

  /* The number of packets received. */
  uint32_t num_packets;

  /* The number of errors received. */
  uint32_t num_errors;

 private:

  SoftwareSerial *serial;

  uint8_t buffer[256];
  size_t bufpos;
};

#endif /* not SERIALPACKET_H */
