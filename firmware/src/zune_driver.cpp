#include "zune_driver.h"
#include "zune_utilities.h"
#include "HardwareSerial.h"
#include "xbox_driver.h"
#include "Esp.h"

static portMUX_TYPE message_copy_mux;

DMA_ATTR uint8_t zune_to_xbox[128];
DMA_ATTR uint8_t xbox_to_zune[128];
int auth_pin;

void handle_0x10(uint8_t *_data, uint8_t size)
{
  uint8_t cmd_byte = _data[0];
  uint8_t data[size - 1];        // data is all bytes after cmd_byte
  memcpy(data, _data + 1, size); // remove type
  // for(int i = 1; i<size; i++)
  // {
  //   data[i - 1] = _data[i];
  // }
  Serial.printf("Handling type 0x10 subtype: 0x%02x\n", cmd_byte);
  switch (cmd_byte)
  {
  case 0x03:
  { // HELO
    Serial.print("Sending EHLO\n");
    uint8_t ehlo[4] = {0x00, 0x03, 0x00, 0x01};
    send_zune_message(0x10, ehlo, 4);
  }
  break;
  case 0x02:
  {
    Serial.print("XboxAuthString\n");
    uint8_t XboxAuthString[178] = {
        0x00, 0x02, 0x58, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x78, 0x00,
        0x20, 0x00, 0x53, 0x00, 0x65, 0x00, 0x63, 0x00, 0x75, 0x00,
        0x72, 0x00, 0x69, 0x00, 0x74, 0x00, 0x79, 0x00, 0x20, 0x00,
        0x4D, 0x00, 0x65, 0x00, 0x74, 0x00, 0x68, 0x00, 0x6F, 0x00,
        0x64, 0x00, 0x20, 0x00, 0x33, 0x00, 0x2C, 0x00, 0x20, 0x00,
        0x56, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x69, 0x00,
        0x6F, 0x00, 0x6E, 0x00, 0x20, 0x00, 0x31, 0x00, 0x2E, 0x00,
        0x30, 0x00, 0x30, 0x00, 0x2C, 0x00, 0x20, 0x00, 0xA9, 0x00,
        0x20, 0x00, 0x32, 0x00, 0x30, 0x00, 0x30, 0x00, 0x35, 0x00,
        0x20, 0x00, 0x4D, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72, 0x00,
        0x6F, 0x00, 0x73, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x74, 0x00,
        0x20, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x70, 0x00,
        0x6F, 0x00, 0x72, 0x00, 0x61, 0x00, 0x74, 0x00, 0x69, 0x00,
        0x6F, 0x00, 0x6E, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x41, 0x00,
        0x6C, 0x00, 0x6C, 0x00, 0x20, 0x00, 0x72, 0x00, 0x69, 0x00,
        0x67, 0x00, 0x68, 0x00, 0x74, 0x00, 0x73, 0x00, 0x20, 0x00,
        0x72, 0x00, 0x65, 0x00, 0x73, 0x00, 0x65, 0x00, 0x72, 0x00,
        0x76, 0x00, 0x65, 0x00, 0x64, 0x00, 0x2E, 0x00}; //
    send_zune_message(0x10, XboxAuthString, 178);
  }
  break;
  };
  Serial.print("0x10 handler exit\n");
}

void handle_0x11(uint8_t *data, uint8_t size)
{
  Serial.println("sending to xbox chip");
  portENTER_CRITICAL(&message_copy_mux);
  memcpy(zune_to_xbox, data, size);
  portEXIT_CRITICAL(&message_copy_mux);
  // byte 5 (0x04) contains the response length, not inclusive of the header
  uint8_t response_length = (data[4]) + 5;
  // response - byte 5 (0x04) is always 0x00 - so just hack it in
  xbox_to_zune[0] = 0x00;
  // remember, we are manually setting value 0x00, so we start at 0x01 on xbox_to_zune
  transferXbox(zune_to_xbox, xbox_to_zune + 1, size, response_length);
  Serial.print("sending xbox response back to the zune\n");
  send_zune_message(0x11, xbox_to_zune, response_length + 1);
}

void zune_message_receiver(void *pvParameters)
{
  // find the first byte in a message, we may have started in the middle
  uint8_t rx_buffer[4]; // four, because the first byte where we even know what we need is four bytes into a valid message
  while (true)
  {
    while (true)
    {
      if (Serial2.available())
      {
        // we are looking for the first byte
        if (Serial2.peek() != 0x41)
        {
          Serial2.readBytes(rx_buffer, 1);
          Serial.printf("Discarded: 0x%02x\n", rx_buffer[0]);
        }
        else
        {
          break;
        }
      }
      taskYIELD();
    }
    // we have the first byte, lets get the type and length

    Serial2.readBytes(rx_buffer, 4);
    uint8_t type = rx_buffer[2];
    uint8_t length = rx_buffer[3];

    // now, we need to get the data part and the crc
    uint8_t data[length + 1];
    Serial2.readBytes(data, length + 1); // length + crc

    Serial.print("Packet! ");
    Serial.printf("Type: 0x%02x ", type);
    Serial.printf("Length: 0x%02x ", length);
    if (length > 0)
    {
      Serial.print("Data:");
      for (int i = 0; i < length; i++) // a length of 0 still has data. i think.
      {
        Serial.printf(" [0x%02x]0x%02x", i, data[i]);
      }
      Serial.print("\n");
    }

    uint8_t crc = calculate_crc(type, data, length);

    if (data[length] != crc)
    {
      Serial.println("Bad CRC!");
      return;
    }

    if (type == 0x10)
    {
      handle_0x10(data, length);
    }
    if (type == 0x11)
    {
      handle_0x11(data, length);
    }
    Serial.println("Done handling message");
    taskYIELD();
  }
}

// tx - to zune
// rx - from zune
void setupZune(int tx_pin, int rx_pin, int _auth_pin)
{
  spinlock_initialize(&message_copy_mux);
  Serial.println("hello from seattle");
  auth_pin = _auth_pin;
  // pinMode(_auth_pin, PULLDOWN); // auth is for as long as _auth_pin is low

  // 57600 was confirmed with a logic analyzer across many different device-specific handshake captures
  Serial2.setRxBufferSize(SOC_UART_FIFO_LEN * 4);
  Serial2.begin(57600, SERIAL_8N1, rx_pin, tx_pin);
  // Serial2.onReceiveError(onRxErr);
  Serial.println("Done setting up zune uart");
  digitalWrite(_auth_pin, 0);
  delay(10);
  digitalWrite(_auth_pin, 1);
  delay(10);
  digitalWrite(_auth_pin, 0);
}

void handle_avrcp(esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param param)
{
  // key codes: https://github.com/espressif/esp-idf/blob/a82e6e63d98bb051d4c59cb3d440c537ab9f74b0/components/bt/host/bluedroid/api/include/api/esp_avrc_api.h#L44-L102
  // key states: 0 = Pressed, 1 = Released
  if (param.key_state != 0)
    return;

  // these are all ipod, I need to fill in the zune blanks AND initialize the correct serial port still
  switch (param.key_code)
  {
  case ESP_AVRC_PT_CMD_VOL_UP:
    Serial.write((uint8_t *)"\xFFU\x03\x02\x00\x02\xF9\xFFU\x03\x02\x00\x00\xFB", 14);
    break;
  case ESP_AVRC_PT_CMD_VOL_DOWN:
    Serial.write((uint8_t *)"\xFFU\x03\x02\x00\x04\xF7\xFFU\x03\x02\x00\x00\xFB", 14);
    break;
  case ESP_AVRC_PT_CMD_FORWARD:
  case ESP_AVRC_PT_CMD_FAST_FORWARD:
    Serial.write((uint8_t *)"\xFFU\x03\x02\x00\b\xF3\xFFU\x03\x02\x00\x00\xFB", 14);
    break;
  case ESP_AVRC_PT_CMD_BACKWARD:
  case ESP_AVRC_PT_CMD_REWIND:
    Serial.write((uint8_t *)"\xFFU\x03\x02\x00\x10\xEB\xFFU\x03\x02\x00\x00\xFB", 14);
    break;
  case ESP_AVRC_PT_CMD_STOP:
    Serial.write((uint8_t *)"\xFFU\x03\x02\x00\x80{\xFFU\x03\x02\x00\x00\xFB", 14);
    break;
  case ESP_AVRC_PT_CMD_PLAY:
    Serial.write((uint8_t *)"\xFFU\x04\x02\x00\x00\x01\xF9\xFFU\x03\x02\x00\x00\xFB", 15);
    break;
  case ESP_AVRC_PT_CMD_PAUSE:
    Serial.write((uint8_t *)"\xFFU\x04\x02\x00\x00\x02\xF8\xFFU\x03\x02\x00\x00\xFB", 15);
    break;
  case ESP_AVRC_PT_CMD_MUTE:
    Serial.write((uint8_t *)"\xFFU\x04\x02\x00\x00\x04\xF6\xFFU\x03\x02\x00\x00\xFB", 15);
    break;
  case ESP_AVRC_PT_CMD_ROOT_MENU:
    Serial.write((uint8_t *)"\xFFU\x05\x02\x00\x00\x00@\xB9\xFFU\x03\x02\x00\x00\xFB", 16);
    break;
  case ESP_AVRC_PT_CMD_SELECT:
    Serial.write((uint8_t *)"\xFFU\x05\x02\x00\x00\x00\x80y\xFFU\x03\x02\x00\x00\xFB", 16);
    break;
  case ESP_AVRC_PT_CMD_UP:
    Serial.write((uint8_t *)"\xFFU\x06\x02\x00\x00\x00\x00\x01\xF7\xFFU\x03\x02\x00\x00\xFB", 17);
    break;
  case ESP_AVRC_PT_CMD_DOWN:
    Serial.write((uint8_t *)"\xFFU\x06\x02\x00\x00\x00\x00\x02\xF6\xFFU\x03\x02\x00\x00\xFB", 17);
    break;
  default:
    break;
  }

  Serial.flush();
}