#pragma once
#define MONO_BLINK 1
#define TRIPLE_BLINK 3
typedef void(*invoke_func_t)();

void init_btn(invoke_func_t ifnc);
void led_blink(int count);
void disable_blink();