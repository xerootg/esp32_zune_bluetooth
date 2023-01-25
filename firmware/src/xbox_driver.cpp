#include "xbox_driver.h"
#include "hal/uart_types.h"
#include "HardwareSerial.h"
#include "esp32-hal-ledc.h"
#include "driver/rmt.h"
#include "Esp.h"


// Here's the math for the constants below, as observed in packet dumps of transactions with xbox chip
// 1/baud = bit duration = .0000260416666 seconds or 26.042 µs	
// 1 s = 1_000_000us
// bit duration/16 = clk duration = 1.627604166*10^-6 or 0.000001627604166s or 1.627604166us
// Frequency(hz)/time(second) = 1 (second)
// x(hz) * 0.000001627604166(s) = 1
// 614400(hz)*0.000001627604166(s) = 1
const int baudrate = 38400;
// const int clk_hz = 614400;
const float one_bit_ms = (1.0/baudrate)*1000.0; // time for a single 'bit' in a byte, converted to microseconds
const int one_bit_us = one_bit_ms*1000;
const float clk_duration_ns = (one_bit_ms/16)*100000.0; //bit duration/16 = clk duration
const float rmt_sample_ns = 12.5; // this is the finest resolution we have
int dio;
int clk;
int rst;

rmt_data_t clk_data;
rmt_obj_t* rmt_send = NULL;
rmt_config_t config;

void setupXbox(int _dio, int _clk, int _rst){
    pinMode(_clk, OUTPUT);
    pinMode(_rst, PULLUP);
    pinMode(dio, PULLUP);
    clk = _clk;
    dio = _dio;

    //traces indicate the parity bit is set. even/odd doesnt matter, 8 bits is always adds up to the same parity
    Serial1.begin(baudrate, SERIAL_8E1);    
    
};

void startClock()
{
    if ((rmt_send = rmtInit(clk, RMT_TX_MODE, RMT_MEM_64)) == NULL)
    {
        Serial.println("init sender failed\n");
    }
    // 52/51 = 615khz, target is 614.4
    clk_data.level0 = 1;
    clk_data.duration0 = 52;
    clk_data.level1 = 0;
    clk_data.duration1 = 51;
    float realTick = rmtSetTick(rmt_send, rmt_sample_ns);
    Serial.printf("real tick set to: %fns\n", realTick);

    rmtLoop(rmt_send, &clk_data, sizeof(clk_data));
}

void stopClock()
{
    rmtDeinit(rmt_send);
}

void transferXbox(uint8_t * to_xbox, uint8_t * from_xbox, uint8_t len_to, uint8_t len_from)
{
    Serial.print("out: ");

    for(int i = 0; i<5; i++)
    {
      Serial.printf("%x", to_xbox[i]);
    }
    Serial.print("\n");

    //start clock
    Serial1.setPins(-1,dio);
    Serial.println("begin xbox tx");
    startClock();
    Serial1.write(to_xbox, len_to);
    Serial1.flush();
    delayMicroseconds(one_bit_us); // there are 16 clock cycles _after_ the stop bit, not the best but this works
    stopClock();
    Serial.println("end xbox tx");
    
    delayMicroseconds(2000);

    Serial1.setPins(dio,-1);
    // empty the buffer, we are essentially in loopback
    while(Serial1.available())
    {
        Serial1.read(); // flush the rx cache, since we 
    }
    Serial.println("begin xbox rx");
    if(len_from>0)
    {
        delayMicroseconds(30);
        startClock();
        Serial1.readBytes(from_xbox, len_from); // possibly could timeout;
        stopClock();
    }
    Serial.println("end xbox rx");
}
