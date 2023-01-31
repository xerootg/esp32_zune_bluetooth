#include "xbox_driver.h"
#include "hal/uart_types.h"
#include "HardwareSerial.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-rmt.h"
#include "soc/rmt_struct.h"
#include "driver/rmt.h"
#include "Esp.h"

// Here's the math for the constants below, as observed in packet dumps of transactions with xbox chip
// 1/baud = bit duration = .0000260416666 seconds or 26.042 µs
// 1 s = 1_000_000us
// bit duration/16 = clk duration = 1.627604166*10^-6 or 0.000001627604166s or 1.627604166us
// Frequency(hz)/time(second) = 1 (second)
// x(hz) * 0.000001627604166(s) = 1
// 614400(hz)*0.000001627604166(s) = 1
const int baudrate = 19200;
// const int clk_hz = 614400;
const float one_bit_ms = (1.0 / baudrate) * 1000.0; // time for a single 'bit' in a byte, converted to microseconds
const int one_bit_us = one_bit_ms * 1000;
const float clk_duration_ns = (one_bit_ms / 16) * 100000.0; // bit duration/16 = clk duration
const float rmt_sample_ns = 100;                            // 100; // this is the finest resolution we have
int dio;
int clk;
int rst;

#define RMT_BIT_LENGTH 208
rmt_data_t one_byte[RMT_BIT_LENGTH];
rmt_obj_t *rmt_send = NULL;
rmt_channel_t *channel;
rmt_config_t config;

static portMUX_TYPE xbox_mux;

void setupXbox(int _dio, int _clk, int _rst)
{
    spinlock_initialize(&xbox_mux);
    pinMode(_clk, OUTPUT);
    digitalWrite(_rst, 0);
    pinMode(_rst, PULLUP);
    pinMode(dio, PULLUP);
    clk = _clk;
    dio = _dio;

    // traces indicate the parity bit is set. even/odd doesnt matter, 8 bits is always adds up to the same parity
    Serial1.begin(baudrate, SERIAL_8E1);

    // PREAMBLE FOR SENDING
    // clk bytes 0-14,16-30 3.08us high, 4.95
    for (int index = 0; index < 31; index++)
    {
        one_byte[index].duration0 = 30; // 3.08us
        one_byte[index].level0 = 1;
        one_byte[index].duration1 = 50; // 4.95us
        one_byte[index].level1 = 0;
    }
    // clk bytes 15,31 3.08us high, 12.45 low
    one_byte[15].duration0 = 30; // 3.08us
    one_byte[15].level0 = 1;
    one_byte[15].duration1 = 125; // 4.95us
    one_byte[15].level1 = 0;
    // clk bytes 15,31 3.08us high, 12.45 low
    one_byte[31].duration0 = 30; // 3.08us
    one_byte[31].level0 = 1;
    one_byte[31].duration1 = 125; // 45us padding _into_ the start byte
    one_byte[31].level1 = 0;

    // bit 1 of 11 in uart frame, start, 8 bits, parity, stop - 176, plus 33
    for (int bit = 0; bit < 11; bit++)
    {
        uint8_t base = 31 + (bit * 16);
        for (int i = 1; i <= 16; i++)
        {
            uint8_t index = base + i;
            if (i == 0 || i == 16)
            {
                one_byte[index].duration0 = 50;
            }
            else
            {
                one_byte[index].duration0 = 15;
            }
            one_byte[index].level0 = 0;
            one_byte[index].duration1 = 15;
            one_byte[index].level1 = 1;
        }
    }
    // this becomes the 209'th clk cycle, we don't want this.
    one_byte[207].level1 = 0;
    one_byte[207].duration1 = 1000;

    if ((rmt_send = rmtInit(clk, RMT_TX_MODE, RMT_MEM_512)) == NULL)
    {
        Serial.println("init sender failed\n");
        abort();
    }
    float realTick = rmtSetTick(rmt_send, rmt_sample_ns);
    Serial.printf("real tick set to: %fns\n", realTick);
    digitalWrite(_rst, 0);
}
volatile bool inProcess = false;
size_t transferXbox(uint8_t *to_xbox, uint8_t *from_xbox, uint8_t len_to, uint8_t len_from)
{
    size_t toRet = 0;
    Serial1.setRxTimeout(190);
    Serial.println("sending to xbox.");
    if(inProcess) return toRet;
    // portENTER_CRITICAL(&xbox_mux);
    inProcess = true;
    Serial1.setPins(14, dio);
    // Serial.println("begin xbox tx");
    for (int byte = 0; byte < len_to; byte++)
    {
        rmtWrite(rmt_send, one_byte, RMT_BIT_LENGTH);
        delayMicroseconds(248); // attempting to sync the uart bytes from the wakeup bytes
        Serial1.write(to_xbox[byte]);
        Serial1.flush(true);
        // digitalWrite(dio, 1);
    }
    rmtWrite(rmt_send, NULL, 0); // turn RMT off

    Serial1.setPins(dio,14);
    // empty the buffer, we are essentially in loopback
    if(len_from>0)
    {
        rmtLoop(rmt_send, one_byte, RMT_BIT_LENGTH);
        toRet = Serial1.read(from_xbox, len_from); // possibly could timeout;
    }
    rmtWrite(rmt_send, NULL, 0); // turn RMT off
    // portEXIT_CRITICAL(&xbox_mux);
    inProcess = false;
    Serial.println("end xbox rx");
    return toRet;
}
