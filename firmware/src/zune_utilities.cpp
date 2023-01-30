#include "zune_utilities.h"
#include "HardwareSerial.h"

#define HEADER_LENGTH 4
#define CRC_LENGTH 1

uint8_t calculate_crc(uint8_t cmd_type, uint8_t * data, size_t length)
{
  uint8_t crc = length + cmd_type;
  for(int i = 0; i < length; i++){
    crc+=data[i]; // we dont care if it overflows
  }
  crc = crc ^ 0xff;
  return crc;
}

uint8_t response_buffer[256];

void send_zune_message(uint8_t cmd_type, uint8_t * data, uint8_t data_size)
  {
    uint8_t crc = calculate_crc(cmd_type, data, data_size);
    response_buffer[0] = 0x41;
    response_buffer[1] = 0x41; //from-dock, the zune sends 0x61 in this position
    response_buffer[2] = cmd_type;
    response_buffer[3] = data_size;

    memcpy(response_buffer + HEADER_LENGTH, data, data_size);
    memcpy(response_buffer + data_size + HEADER_LENGTH, &crc, CRC_LENGTH);

    uint8_t length_on_wire = (data_size + HEADER_LENGTH) + CRC_LENGTH;
    // Serial.print("to zune: ");
    // for(int i = 0; i<length_on_wire; i++)
    // {
    //   Serial.printf(" [0x%02x]0x%02x", i, response_buffer[i]);
    // }
    // Serial.println();
    Serial2.write(response_buffer, length_on_wire);
  }