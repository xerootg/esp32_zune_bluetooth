#include "zune_driver.h"
#include "HardwareSerial.h"
#include "xbox_driver.h"

int auth_pin;

void setupZune(int from_zune, int to_zune, int _auth_pin)
{
    auth_pin = _auth_pin;
    pinMode(_auth_pin, PULLDOWN); // auth is for as long as _auth_pin is low

    Serial2.begin(57600, SERIAL_8N1, from_zune, to_zune);
}

uint8_t calculate_crc(uint8_t * packet, size_t length)
{

}

void doAuthHandshake()
{
    //41 41 10 04 00 03 00 01 E7
    Serial2.write()
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