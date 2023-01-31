#include <Arduino.h>
#include "soc/rtc_wdt.h"

#include "xbox_driver.h"
#include "zune_driver.h"
#include "a2dp.h"

// tested, works
#define XBOX_DIO 21 //5
#define XBOX_CLK 22 //6
#define XBOX_RST 19 //7
#define ZUNE_TX 23
#define ZUNE_RX 18
#define ZUNE_AUTH 13

QueueHandle_t pt_event_queue;

TaskHandle_t watchdog_keepalive_task_handle = NULL;
TaskHandle_t audio_copy_task_handle = NULL;
TaskHandle_t zune_control_task_handle = NULL;
TaskHandle_t zune_message_receiver_handle = NULL;

void watchdog_keepalive_task(void* params)
{
  for(;;){
      rtc_wdt_feed();
  }
  taskYIELD();
}

uint8_t to_xbox[25];
uint8_t from_xbox[25];

// Arduino Setup
void setup() {

  Serial.begin(115200);

  setupXbox(XBOX_DIO, XBOX_CLK, XBOX_RST);
  setupZune(ZUNE_TX, ZUNE_RX, ZUNE_AUTH);
  //setup_audio()

  // BaseType_t xReturned = xTaskCreate(audio_copy_task, "AudioCopyTask", 1024 * 8, NULL, tskIDLE_PRIORITY, &audio_copy_task_handle);
  // if(xReturned != pdPASS) {
  //   Serial.print("error making the audio copy task. restarting.\n");
  //   ESP.restart();
  // }

  BaseType_t xReturned = xTaskCreate(watchdog_keepalive_task, "DonchaDieOnMeJim", 1024, NULL, tskIDLE_PRIORITY, &watchdog_keepalive_task_handle);
  if(xReturned != pdPASS) {
    Serial.print("error making the Watchdog Kicker. restarting.\n");
    ESP.restart();
  }

  // xReturned = xTaskCreate(avrcp_to_zune_uart_task, "ZuneUartTask", 1024 * 4, NULL, tskIDLE_PRIORITY, &zune_control_task_handle);
  // if(xReturned != pdPASS) {
  //   Serial.print("error making the Zune UART task. restarting.\n");
  //   ESP.restart();
  // }

  
  xReturned = xTaskCreate(zune_message_receiver, "ZuneUartReceiver", 1024 * 4, NULL, tskIDLE_PRIORITY, &zune_message_receiver_handle);
  if(xReturned != pdPASS) {
    Serial.print("error making the Zune UART task. restarting.\n");
    ESP.restart();
  }
}

void loop()
{
}
