#include "a2dp.h"

#define USE_A2DP
#define AI_THINKER_ES8388_VOLUME_HACK 0

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"
#include "AudioLibs/AudioA2DP.h"

#include "esp_avrc_api.h"

#include "zune_driver.h"

// This is in ZuneBT.cpp, we will grab it from there.
extern QueueHandle_t pt_event_queue;

AudioKitStream kit; // Access I2S as stream
A2DPStream out = A2DPStream::instance();
StreamCopy copier(out, kit);  

void audio_copy_task(void* pvParameters) {
  for( ;; ) copier.copy();
}

void avrcp_to_zune_uart_task(void* pvParameters) {
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

void setup_audio()
{
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
}