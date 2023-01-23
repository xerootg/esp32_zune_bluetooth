#include <Arduino.h>
#define USE_A2DP
#define AI_THINKER_ES8388_VOLUME_HACK 0

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"
#include "AudioLibs/AudioA2DP.h"

#include "esp_avrc_api.h"

AudioKitStream kit; // Access I2S as stream
A2DPStream out = A2DPStream::instance();
StreamCopy copier(out, kit);  

QueueHandle_t pt_event_queue;


TaskHandle_t audio_copy_task_handle = NULL;
TaskHandle_t ipod_control_task_handle = NULL;

void passthrough_callback(esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param param) {
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

void audio_copy_task(void* pvParameters) {
  for( ;; ) copier.copy();
}

void avrcp_to_uart_task(void* pvParameters) {
  esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param event;
  esp_bt_gap_cb_param_t result;

  for( ;; ) {
    if(pt_event_queue) {
      if(xQueueReceive(pt_event_queue, &event, 0) == pdTRUE) {
        if(event.key_code != 0) passthrough_callback(event);
      }
    }
  }
  taskYIELD();
}

// Arduino Setup
void setup() {
    // configure the input stream
    auto cfg = kit.defaultConfig(RX_MODE);
    cfg.sd_active = false;
    cfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2;
    cfg.sample_rate = AUDIO_HAL_44K_SAMPLES;

    auto config = out.defaultConfig();
    config.auto_reconnect = true;
    config.mode = TX_MODE;
    pt_event_queue = config.passthrough_event_queue;
    out.begin(config);
    while(!out.a2dp_source->is_connected())
    {
        
    }

    kit.begin(cfg);
    kit.setVolume(1);
    out.notifyBaseInfo(44100);

      // step 5: start the tasks that will copy the audio data and handle the ipod control
  BaseType_t xReturned = xTaskCreate(audio_copy_task, "AudioCopyTask", 1024 * 8, NULL, tskIDLE_PRIORITY, &audio_copy_task_handle);
  if(xReturned != pdPASS) {
    Serial.print("error making the audio copy task. restarting.\n");
    ESP.restart();
  }

  xReturned = xTaskCreate(avrcp_to_uart_task, "AVRCPToUartTask", 1024 * 4, NULL, tskIDLE_PRIORITY, &ipod_control_task_handle);
  if(xReturned != pdPASS) {
    Serial.print("error making the AVRCP translation task. restarting.\n");
    ESP.restart();
  }
}

void loop(){}


