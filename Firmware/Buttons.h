#ifndef BUTTONS_H
#define BUTTONS_H

#pragma once
#include <stdbool.h>
#include <stdint.h>

#define BTN_UP     26
#define BTN_DOWN   29
#define BTN_MENU   28
#define BTN_SELECT 27

void buttons_init(void);
bool read_button(uint8_t gpio);
uint32_t DEV_TimeMs(void);  // para debounce

#endif