#include "stdint.h"
#include "strings.h"

void send_zune_message(uint8_t cmd_type, uint8_t * data, uint8_t data_size);
uint8_t calculate_crc(uint8_t cmd_type, uint8_t * data, size_t length);