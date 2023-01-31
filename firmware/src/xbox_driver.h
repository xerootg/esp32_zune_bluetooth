#pragma once
#include "stdint.h"
#include "stddef.h"

void setupXbox(int dio, int _clk, int _rst);
size_t transferXbox(uint8_t * to_xbox, uint8_t * from_xbox, uint8_t len_to, uint8_t len_from);