#include <Arduino.h>
#define USE_A2DP
#define AI_THINKER_ES8388_VOLUME_HACK 0

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"
#include "AudioLibs/AudioA2DP.h"

#include "esp_avrc_api.h"

#include "xbox_driver.h"
#include "zune_driver.h"

// esp32-a1s header
// gnd,  0, rst, tx0, rx0, 3v3, 3v3
//  21, 22,  19,  23,  18,   5, gnd
// 0, rst, tx0, rx0 are for programming interface, so the top row is useless

// tested, works
#define XBOX_CLK 22
#define XBOX_DIO 21
#define XBOX_RST 19

AudioKitStream kit; // Access I2S as stream
A2DPStream out = A2DPStream::instance();
StreamCopy copier(out, kit);  

QueueHandle_t pt_event_queue;


TaskHandle_t audio_copy_task_handle = NULL;
TaskHandle_t zune_control_task_handle = NULL;

void audio_copy_task(void* pvParameters) {
  for( ;; ) copier.copy();
}

void avrcp_to_uart_task(void* pvParameters) {
  esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param event;
  esp_bt_gap_cb_param_t result;

  for( ;; ) {
    if(pt_event_queue) {
      if(xQueueReceive(pt_event_queue, &event, 0) == pdTRUE) {
        if(event.key_code != 0) handle_avrcp(event);
      }
    }
  }
  taskYIELD();
}

uint8_t to_xbox[25];
uint8_t from_xbox[25];

// Arduino Setup
void setup() {
    Serial.begin(115200);
    setupZune();
    setupXbox(XBOX_DIO, XBOX_CLK, XBOX_RST);
    uint8_t out[5] = 
    {
      0x09, 0x5b, 0x00, 0x00, 0x17
    };

    uint8_t in[29];

    transferXbox(out, in, sizeof(out), sizeof(in));

    Serial.print("out: ");

    for(int i = 0; i<5; i++)
    {
      Serial.printf("0x%02x ", out[i]);
    }


    Serial.print("\nIn: ");

    for(int i = 0; i<29; i++)
    {
      Serial.printf("0x%02x ", in[i]);
    }    

  //   // setup xbox uart, rst, clk
  //   // set pin 18 high, then low
  //   // wait for hello


  //   // configure the input stream
  //   auto cfg = kit.defaultConfig(RX_MODE);
  //   cfg.sd_active = false;
  //   cfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2;
  //   cfg.sample_rate = AUDIO_HAL_44K_SAMPLES;

  //   auto config = out.defaultConfig();
  //   config.auto_reconnect = true;
  //   config.mode = TX_MODE;
  //   pt_event_queue = config.passthrough_event_queue;
  //   out.begin(config);
  //   while(!out.a2dp_source->is_connected())
  //   {
        
  //   }

  //   kit.begin(cfg);
  //   kit.setVolume(1);
  //   out.notifyBaseInfo(44100);

  //     // step 5: start the tasks that will copy the audio data and handle the ipod control
  // BaseType_t xReturned = xTaskCreate(audio_copy_task, "AudioCopyTask", 1024 * 8, NULL, tskIDLE_PRIORITY, &audio_copy_task_handle);
  // if(xReturned != pdPASS) {
  //   Serial.print("error making the audio copy task. restarting.\n");
  //   ESP.restart();
  // }

  // xReturned = xTaskCreate(avrcp_to_uart_task, "AVRCPToUartTask", 1024 * 4, NULL, tskIDLE_PRIORITY, &zune_control_task_handle);
  // if(xReturned != pdPASS) {
  //   Serial.print("error making the AVRCP translation task. restarting.\n");
  //   ESP.restart();
  // }
}

void loop()
{
  delay(100);
}
