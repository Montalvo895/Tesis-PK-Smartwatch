#include "buttons.h"
#include "pico/stdlib.h"
#include "DEV_Config.h"   // para DEV_TimeMs()

// arreglo de último tiempo (archivo-scope)
static uint32_t last_press[4] = {0,0,0,0};

void buttons_init(void) {
    gpio_init(BTN_MENU);   gpio_set_dir(BTN_MENU, GPIO_IN); gpio_pull_up(BTN_MENU);
    gpio_init(BTN_UP);     gpio_set_dir(BTN_UP, GPIO_IN); gpio_pull_up(BTN_UP);
    gpio_init(BTN_DOWN);   gpio_set_dir(BTN_DOWN, GPIO_IN); gpio_pull_up(BTN_DOWN);
    gpio_init(BTN_SELECT); gpio_set_dir(BTN_SELECT, GPIO_IN); gpio_pull_up(BTN_SELECT);
}

bool read_button(uint8_t gpio)
{
    uint8_t index = 0;
    if (gpio == BTN_UP)     index = 0;
    else if (gpio == BTN_DOWN)   index = 1;
    else if (gpio == BTN_MENU)   index = 2;
    else if (gpio == BTN_SELECT) index = 3;
    else return false;

    uint32_t now = DEV_TimeMs();

    // LOW = presionado (pull-up)
    if (!gpio_get(gpio)) {
        if (now - last_press[index] > 250) { // debounce 150ms
            last_press[index] = now;
            return true;
        }
    }
    return false;
}
