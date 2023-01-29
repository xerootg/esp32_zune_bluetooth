#include "zune_driver.h"
#include "HardwareSerial.h"
#include "xbox_driver.h"

int auth_pin;

uint8_t calculate_crc(uint8_t cmd_type, uint8_t * data, size_t length)
{
  uint8_t crc = length + cmd_type;
  for(int i = 0; i < length; i++){
    crc+=data[i]; // we dont care if it overflows
  }
  crc = crc ^ 0xff;
  return crc;
}

void handle_0x10(uint8_t * data, size_t size)
{
  Serial.println("welcome to the 0x10 handler!");
}

void onReceiveFunction(void) {
  // find the first byte in a message, we may have started in the middle
  uint8_t rx_buffer[4]; // four, because the first byte where we even know what we need is four bytes into a valid message
  while(true){
    if(Serial2.available())
    {
      // we are looking for the first byte
      if(Serial2.peek() != 0x41){
        Serial2.readBytes(rx_buffer,1);
        Serial.printf("Discarded: 0x%02x\n", rx_buffer[0]);
      }
      else {
        break;
      }
    }
  }
  // we have the first byte, lets get the type and length
  
  Serial2.readBytes(rx_buffer, 4);
  uint8_t type = rx_buffer[2];
  uint8_t length = rx_buffer[3];
  
  // now, we need to get the data part and the crc
  uint8_t data[length+1];
  Serial2.readBytes(data, length+1); // length + crc

  Serial.print("Packet!\n");
  Serial.printf("Type: 0x%02x\n", type);
  Serial.printf("Length: 0x%02x\n", length);
  Serial.print("Data:");
  for(int i = 0; i< length+1; i++) // a length of 0 still has data. i think.
  {
    Serial.printf(" [0x%02x]0x%02x",i,data[i]);
  }
  Serial.print("\n");

  uint8_t crc = calculate_crc(type, data, length);


  if(data[length] != crc)
  {
    Serial.println("Bad CRC!");
    return;
  }

  if(type == 0x10)
  {
    handle_0x10(data, length);
  }

}

// tx - to zune
// rx - from zune
void setupZune(int tx_pin, int rx_pin, int _auth_pin)
{
    Serial.println("hello from seattle");
    auth_pin = _auth_pin;
    // pinMode(_auth_pin, PULLDOWN); // auth is for as long as _auth_pin is low

    // 57600 was confirmed with a logic analyzer across many different device-specific handshake captures
    Serial2.begin(57600, SERIAL_8N1, rx_pin, tx_pin);
    // Serial2.onReceiveError(onRxErr);
    Serial2.onReceive(onReceiveFunction);
    Serial.println("Done setting up zune uart");
}

// the zune sends a pile of messages. we just need to respond to them.
void handle_message(uint8_t * message, size_t length)
{

}

void handle_avrcp(esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param param) {
  // key codes: https://github.com/espressif/esp-idf/blob/a82e6e63d98bb051d4c59cb3d440c537ab9f74b0/components/bt/host/bluedroid/api/include/api/esp_avrc_api.h#L44-L102
  // key states: 0 = Pressed, 1 = Released
  if(param.key_state != 0) return;

    // these are all ipod, I need to fill in the zune blanks AND initialize the correct serial port still
  switch(param.key_code) {
    case ESP_AVRC_PT_CMD_VOL_UP:
      Serial.write((uint8_t*)"\xFFU\x03\x02\x00\x02\xF9\xFFU\x03\x02\x00\x00\xFB", 14);
      break;
    case ESP_AVRC_PT_CMD_VOL_DOWN:
      Serial.write((uint8_t*)"\xFFU\x03\x02\x00\x04\xF7\xFFU\x03\x02\x00\x00\xFB", 14);
      break;
    case ESP_AVRC_PT_CMD_FORWARD:
    case ESP_AVRC_PT_CMD_FAST_FORWARD:
      Serial.write((uint8_t*)"\xFFU\x03\x02\x00\b\xF3\xFFU\x03\x02\x00\x00\xFB", 14);
      break;
    case ESP_AVRC_PT_CMD_BACKWARD:
    case ESP_AVRC_PT_CMD_REWIND:
      Serial.write((uint8_t*)"\xFFU\x03\x02\x00\x10\xEB\xFFU\x03\x02\x00\x00\xFB", 14);
      break;
    case ESP_AVRC_PT_CMD_STOP:
      Serial.write((uint8_t*)"\xFFU\x03\x02\x00\x80{\xFFU\x03\x02\x00\x00\xFB", 14);
      break;
    case ESP_AVRC_PT_CMD_PLAY:
      Serial.write((uint8_t*)"\xFFU\x04\x02\x00\x00\x01\xF9\xFFU\x03\x02\x00\x00\xFB", 15);
      break;
    case ESP_AVRC_PT_CMD_PAUSE:
      Serial.write((uint8_t*)"\xFFU\x04\x02\x00\x00\x02\xF8\xFFU\x03\x02\x00\x00\xFB", 15);
      break;
    case ESP_AVRC_PT_CMD_MUTE:
      Serial.write((uint8_t*)"\xFFU\x04\x02\x00\x00\x04\xF6\xFFU\x03\x02\x00\x00\xFB", 15);
      break;
    case ESP_AVRC_PT_CMD_ROOT_MENU:
      Serial.write((uint8_t*)"\xFFU\x05\x02\x00\x00\x00@\xB9\xFFU\x03\x02\x00\x00\xFB", 16);
      break;
    case ESP_AVRC_PT_CMD_SELECT:
      Serial.write((uint8_t*)"\xFFU\x05\x02\x00\x00\x00\x80y\xFFU\x03\x02\x00\x00\xFB", 16);
      break;
    case ESP_AVRC_PT_CMD_UP:
      Serial.write((uint8_t*)"\xFFU\x06\x02\x00\x00\x00\x00\x01\xF7\xFFU\x03\x02\x00\x00\xFB", 17);
      break;
    case ESP_AVRC_PT_CMD_DOWN:
      Serial.write((uint8_t*)"\xFFU\x06\x02\x00\x00\x00\x00\x02\xF6\xFFU\x03\x02\x00\x00\xFB", 17);
      break;
    default:
      break;
  }

  Serial.flush();
}