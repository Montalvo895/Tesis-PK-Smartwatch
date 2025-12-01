#include "LCD_1in47.h"
#include "DEV_Config.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "pico/time.h"
#include "pico/multicore.h"  // NUEVO: Para dual-core
#include "buttons.h"

// ============================================================
// CONFIGURACIÓN UART PARA BLE
// ============================================================
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5


// ============================================================
// DECLARACIÓN DE LA FUNCIÓN DEL CORE 1 (definida en Integration.cpp)
// ============================================================
extern void core1_entry(void);  // Esta función debe existir en Integration.cpp

// ============================================================
// FUNCIONES DE TIEMPO
// ============================================================
uint32_t DEV_TimeMs(void) {
    return to_ms_since_boot(get_absolute_time());
}

// ============================================================
// CONFIGURACIÓN
// ============================================================
#define DISP_BUF_SIZE (LCD_1IN47_WIDTH * 40)

// ============================================================
// VARIABLES GLOBALES
// ============================================================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[DISP_BUF_SIZE];

// Pantallas
static lv_obj_t *label_time;
static lv_obj_t *label_date;
static lv_obj_t *menu_screen = NULL;
static lv_obj_t *label_config = NULL;
static lv_obj_t *menu_btns[3] = {NULL, NULL, NULL};
static lv_obj_t *submenu_btns[2] = {NULL, NULL};

// Estado
static bool screen_on = true;
static int hh = 12, mm = 0, ss = 0, dd = 2, mo = 11, yy = 2025;
static uint32_t inactivity_timer = 0;
static uint8_t brightness = 5;
static uint32_t screen_timeout_options[] = {10000, 20000, 30000, 40000, 60000};
static uint8_t timeout_index = 2;
static bool medication_taken = false;

// Estados de navegación
typedef enum {
    STATE_MAIN = 0,
    STATE_MENU,
    STATE_SUBMENU_DATETIME,
    STATE_CONFIG_TIME,
    STATE_CONFIG_DATE,
    STATE_SUBMENU_SETTINGS,
    STATE_CONFIG_BRIGHTNESS,
    STATE_CONFIG_TIMEOUT,
    STATE_MEDICATION
} screen_state_t;

static screen_state_t screen_state = STATE_MAIN;
static uint8_t menu_index = 0;
static uint8_t submenu_index = 0;
static uint8_t selected_digit = 0;

// ============================================================
// DRIVER DE PANTALLA
// ============================================================
extern uint bl_slice_num;

void LCD_1IN47_BL(uint8_t value) {
    if (value == 0) {
        pwm_set_chan_level(bl_slice_num, PWM_CHAN_B, 0);
    } else {
        uint16_t pwm_levels[] = {
            6553, 13107, 19660, 26214, 32767,
            39321, 45874, 52428, 58981, 65535
        };
        if (value >= 1 && value <= 10) {
            pwm_set_chan_level(bl_slice_num, PWM_CHAN_B, pwm_levels[value - 1]);
        }
    }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t index = 0;
    LCD_1IN47_SetWindows(area->x1, area->y1, area->x2 + 1, area->y2 + 1);
    DEV_Digital_Write(LCD_DC_PIN, 1);
    DEV_Digital_Write(LCD_CS_PIN, 0);

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint16_t color = color_p[index].full;
            uint8_t data[2] = {color & 0xFF, color >> 8};
            DEV_SPI_Write_nByte(LCD_SPI_PORT, data, 2);
            index++;
        }
    }

    DEV_Digital_Write(LCD_CS_PIN, 1);
    lv_disp_flush_ready(disp);
}

// void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
//     uint32_t index = 0;
//     LCD_1IN47_SetWindows(area->x1, area->y1, area->x2 + 1, area->y2 + 1);
    
//     DEV_Digital_Write(LCD_DC_PIN, 1);
    
//     // LOCK SPI antes de usar el LCD
//     DEV_SPI_Lock();
//     DEV_SPI_Configure_For_LCD();
//     DEV_Digital_Write(LCD_CS_PIN, 0);

//     for (int y = area->y1; y <= area->y2; y++) {
//         for (int x = area->x1; x <= area->x2; x++) {
//             uint16_t color = color_p[index].full;
//             uint8_t data[2] = {color & 0xFF, color >> 8};
//             DEV_SPI_Write_nByte(LCD_SPI_PORT, data, 2);
//             index++;
//         }
//     }

//     DEV_Digital_Write(LCD_CS_PIN, 1);
//     DEV_SPI_Unlock(); // UNLOCK SPI
    
//     lv_disp_flush_ready(disp);
// }

// ============================================================
// UTILIDADES
// ============================================================
static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int month, int year) {
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year))
        return 29;
    return days[month - 1];
}

static void reset_menu_pointers(void) {
    menu_screen = NULL;
    label_config = NULL;
    for (int i = 0; i < 3; i++) menu_btns[i] = NULL;
    for (int i = 0; i < 2; i++) submenu_btns[i] = NULL;
}

// ============================================================
// PANTALLA PRINCIPAL
// ============================================================
static void create_main_screen(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();

    label_time = lv_label_create(lv_scr_act());
    lv_label_set_text(label_time, "00:00");
    lv_obj_set_style_text_color(label_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_16, 0);
    lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -30);

    label_date = lv_label_create(lv_scr_act());
    lv_label_set_text(label_date, "2025-11-02");
    lv_obj_set_style_text_color(label_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(label_date, &lv_font_montserrat_16, 0);
    lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 20);

    if (!medication_taken) {
        lv_obj_t *med_warn = lv_label_create(lv_scr_act());
        lv_label_set_text(med_warn, LV_SYMBOL_WARNING " Medicamento");
        lv_obj_set_style_text_color(med_warn, lv_color_hex(0xFF6600), 0);
        lv_obj_set_style_text_font(med_warn, &lv_font_montserrat_16, 0);
        lv_obj_align(med_warn, LV_ALIGN_BOTTOM_MID, 0, -10);
    }
}

// ============================================================
// MENÚ PRINCIPAL
// ============================================================
static void show_main_menu(void) {
    if (menu_screen == NULL) {
        lv_obj_clean(lv_scr_act());
        reset_menu_pointers();
        
        menu_screen = lv_list_create(lv_scr_act());
        lv_obj_set_size(menu_screen, 240, 160);
        lv_obj_center(menu_screen);
        lv_obj_set_style_bg_color(menu_screen, lv_color_hex(0x1A1A1A), LV_PART_MAIN);

        lv_obj_t *title = lv_list_add_text(menu_screen, "MENU PRINCIPAL");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        
        menu_btns[0] = lv_list_add_btn(menu_screen, LV_SYMBOL_CLOCK, "Fecha y Hora");
        menu_btns[1] = lv_list_add_btn(menu_screen, LV_SYMBOL_SETTINGS, "Ajustes");
        menu_btns[2] = lv_list_add_btn(menu_screen, LV_SYMBOL_PLUS, "Medicacion");
        
        for (int i = 0; i < 3; i++) {
            lv_obj_set_style_text_color(menu_btns[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
    }

    for (int i = 0; i < 3; i++)
        lv_obj_set_style_bg_color(menu_btns[i], lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(menu_btns[menu_index], lv_color_hex(0x0066CC), LV_PART_MAIN);
}

// ============================================================
// SUBMENÚ FECHA Y HORA
// ============================================================
static void show_submenu_datetime(void) {
    if (menu_screen == NULL) {
        lv_obj_clean(lv_scr_act());
        reset_menu_pointers();
        
        menu_screen = lv_list_create(lv_scr_act());
        lv_obj_set_size(menu_screen, 240, 140);
        lv_obj_center(menu_screen);
        lv_obj_set_style_bg_color(menu_screen, lv_color_hex(0x1A1A1A), LV_PART_MAIN);

        lv_obj_t *title = lv_list_add_text(menu_screen, "FECHA Y HORA");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        
        submenu_btns[0] = lv_list_add_btn(menu_screen, LV_SYMBOL_CLOCK, "Ajustar Hora");
        submenu_btns[1] = lv_list_add_btn(menu_screen, LV_SYMBOL_LIST, "Ajustar Fecha");
        
        for (int i = 0; i < 2; i++) {
            lv_obj_set_style_text_color(submenu_btns[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
    }

    for (int i = 0; i < 2; i++)
        lv_obj_set_style_bg_color(submenu_btns[i], lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(submenu_btns[submenu_index], lv_color_hex(0x0066CC), LV_PART_MAIN);
}

// ============================================================
// SUBMENÚ AJUSTES
// ============================================================
static void show_submenu_settings(void) {
    if (menu_screen == NULL) {
        lv_obj_clean(lv_scr_act());
        reset_menu_pointers();
        
        menu_screen = lv_list_create(lv_scr_act());
        lv_obj_set_size(menu_screen, 240, 140);
        lv_obj_center(menu_screen);
        lv_obj_set_style_bg_color(menu_screen, lv_color_hex(0x1A1A1A), LV_PART_MAIN);

        lv_obj_t *title = lv_list_add_text(menu_screen, "AJUSTES");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        
        submenu_btns[0] = lv_list_add_btn(menu_screen, LV_SYMBOL_IMAGE, "Brillo");
        submenu_btns[1] = lv_list_add_btn(menu_screen, LV_SYMBOL_POWER, "Duracion");
        
        for (int i = 0; i < 2; i++) {
            lv_obj_set_style_text_color(submenu_btns[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
    }

    for (int i = 0; i < 2; i++)
        lv_obj_set_style_bg_color(submenu_btns[i], lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(submenu_btns[submenu_index], lv_color_hex(0x0066CC), LV_PART_MAIN);
}

// ============================================================
// CONFIG HORA
// ============================================================
static void show_time_config(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();
    
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "AJUSTAR HORA");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    label_config = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label_config, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0);
    lv_obj_align(label_config, LV_ALIGN_CENTER, 0, 0);
    
    char buf[10];
    if (selected_digit == 0)
        snprintf(buf, sizeof(buf), "[%02d]:%02d", hh, mm);
    else
        snprintf(buf, sizeof(buf), "%02d:[%02d]", hh, mm);
    lv_label_set_text(label_config, buf);
    
    lv_obj_t *hint = lv_label_create(lv_scr_act());
    lv_label_set_text(hint, "SELECT=cambiar | UP/DOWN=ajustar");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ============================================================
// CONFIG FECHA
// ============================================================
static void show_date_config(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();
    
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "AJUSTAR FECHA");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    label_config = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label_config, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0);
    lv_obj_align(label_config, LV_ALIGN_CENTER, 0, 0);
    
    char buf[20];
    if (selected_digit == 0)
        snprintf(buf, sizeof(buf), "[%02d]/%02d/%04d", dd, mo, yy);
    else if (selected_digit == 1)
        snprintf(buf, sizeof(buf), "%02d/[%02d]/%04d", dd, mo, yy);
    else
        snprintf(buf, sizeof(buf), "%02d/%02d/[%04d]", dd, mo, yy);
    lv_label_set_text(label_config, buf);
    
    lv_obj_t *hint = lv_label_create(lv_scr_act());
    lv_label_set_text(hint, "SELECT=cambiar | UP/DOWN=ajustar");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ============================================================
// CONFIG BRILLO
// ============================================================
static void show_brightness_config(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();
    
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "AJUSTAR BRILLO");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    label_config = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label_config, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0);
    lv_obj_align(label_config, LV_ALIGN_CENTER, 0, 0);
    
    char buf[10];
    snprintf(buf, sizeof(buf), "%d / 10", brightness);
    lv_label_set_text(label_config, buf);
    
    lv_obj_t *bar_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bar_bg, 200, 10);
    lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(bar_bg, 0, 0);
    lv_obj_align(bar_bg, LV_ALIGN_CENTER, 0, 40);
    
    lv_obj_t *bar_fill = lv_obj_create(bar_bg);
    lv_obj_set_size(bar_fill, (brightness * 200) / 10, 10);
    lv_obj_set_style_bg_color(bar_fill, lv_color_hex(0x00CC66), 0);
    lv_obj_set_style_border_width(bar_fill, 0, 0);
    lv_obj_align(bar_fill, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t *hint = lv_label_create(lv_scr_act());
    lv_label_set_text(hint, "UP/DOWN=ajustar");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ============================================================
// CONFIG TIMEOUT
// ============================================================
static void show_timeout_config(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();
    
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "DURACION ENCENDIDO");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    label_config = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label_config, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0);
    lv_obj_align(label_config, LV_ALIGN_CENTER, 0, 0);
    
    char buf[10];
    const char *labels[] = {"10s", "20s", "30s", "40s", "1m"};
    snprintf(buf, sizeof(buf), "%s", labels[timeout_index]);
    lv_label_set_text(label_config, buf);
    
    lv_obj_t *hint = lv_label_create(lv_scr_act());
    lv_label_set_text(hint, "UP/DOWN=ajustar");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ============================================================
// MEDICACIÓN
// ============================================================
static void show_medication_screen(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    reset_menu_pointers();
    
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "MEDICACION");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *question = lv_label_create(lv_scr_act());
    lv_label_set_text(question, "Ya tomo el\nmedicamento?");
    lv_obj_set_style_text_color(question, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(question, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(question, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(question, LV_ALIGN_CENTER, 0, -20);

    label_config = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0);
    lv_obj_align(label_config, LV_ALIGN_CENTER, 0, 30);
    
    if (medication_taken) {
        lv_label_set_text(label_config, "SI");
        lv_obj_set_style_text_color(label_config, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(label_config, "NO");
        lv_obj_set_style_text_color(label_config, lv_color_hex(0xFF0000), 0);
    }
    
    lv_obj_t *hint = lv_label_create(lv_scr_act());
    lv_label_set_text(hint, "UP/DOWN = cambiar");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// ============================================================
// RELOJ
// ============================================================
static void clock_update_cb(lv_timer_t *t) {
    if (screen_state != STATE_MAIN || label_time == NULL || label_date == NULL)
        return;
        
    ss++;
    if (ss == 60) { ss=0; mm++; }
    if (mm == 60) { mm=0; hh++; }
    if (hh == 24) hh = 0;

    char hbuf[6];
    snprintf(hbuf, sizeof(hbuf), "%02d:%02d", hh, mm);
    lv_label_set_text(label_time, hbuf);

    char dbuf[12];
    snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d", yy, mo, dd);
    lv_label_set_text(label_date, dbuf);
}

// ============================================================
// ACTUALIZACIÓN DE PANTALLA
// ============================================================
static void update_screen(void) {
    switch (screen_state) {
        case STATE_MAIN: create_main_screen(); break;
        case STATE_MENU: show_main_menu(); break;
        case STATE_SUBMENU_DATETIME: show_submenu_datetime(); break;
        case STATE_CONFIG_TIME: show_time_config(); break;
        case STATE_CONFIG_DATE: show_date_config(); break;
        case STATE_SUBMENU_SETTINGS: show_submenu_settings(); break;
        case STATE_CONFIG_BRIGHTNESS: show_brightness_config(); break;
        case STATE_CONFIG_TIMEOUT: show_timeout_config(); break;
        case STATE_MEDICATION: show_medication_screen(); break;
    }
}

static void on_user_activity(void) {
    inactivity_timer = 0;
    if (!screen_on) {
        LCD_1IN47_BL(brightness);
        screen_on = true;
    }
}

static void go_to_idle_mode(void) {
    if (screen_on) {
        LCD_1IN47_BL(0);
        screen_on = false;
    }
}

// ============================================================
// MAIN - CORE 0
// ============================================================
int main(void) {
    stdio_init_all();
    // stdio_uart_init_full(UART_ID, BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    // Inicializar hardware
    if (DEV_Module_Init() != 0)
        return -1;

    LCD_1IN47_Init(VERTICAL);
    LCD_1IN47_Clear(BLACK);
    LCD_1IN47_BL(brightness);

    buttons_init();

    // Inicializar LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_BUF_SIZE);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 172;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    create_main_screen();
    lv_timer_create(clock_update_cb, 1000, NULL);

    // ============================================================
    // LANZAR CORE 1 (BMI270 + Edge Impulse)
    // ============================================================
    printf("[Core 0] Iniciando Core 1 para BMI270/Edge Impulse...\n");
    multicore_launch_core1(core1_entry);
    printf("[Core 0] Core 1 lanzado exitosamente\n");

    // ============================================================
    // LOOP PRINCIPAL - CORE 0 (LVGL)
    // ============================================================
    uint32_t last_time = DEV_TimeMs();

    while (1) {
        lv_timer_handler();
        lv_tick_inc(5);
        DEV_Delay_ms(5);

        uint32_t now = DEV_TimeMs();
        inactivity_timer += (now - last_time);
        last_time = now;

        bool menu_pressed   = read_button(BTN_MENU);
        bool up_pressed     = read_button(BTN_UP);
        bool down_pressed   = read_button(BTN_DOWN);
        bool select_pressed = read_button(BTN_SELECT);

        if (!screen_on) {
            if (menu_pressed || up_pressed || down_pressed || select_pressed)
                on_user_activity();
            continue;
        }

        if (menu_pressed) {
            on_user_activity();
            if (screen_state == STATE_MAIN) {
                screen_state = STATE_MENU;
                menu_index = 0;
            } else {
                screen_state = STATE_MAIN;
            }
            update_screen();
        }

        switch (screen_state) {
            case STATE_MENU:
                if (up_pressed && menu_index > 0) { 
                    menu_index--; 
                    on_user_activity(); 
                    show_main_menu(); 
                }
                if (down_pressed && menu_index < 2) { 
                    menu_index++; 
                    on_user_activity(); 
                    show_main_menu(); 
                }
                if (select_pressed) {
                    on_user_activity();
                    if (menu_index == 0) { 
                        screen_state = STATE_SUBMENU_DATETIME; 
                        submenu_index = 0;
                        reset_menu_pointers();
                    }
                    else if (menu_index == 1) { 
                        screen_state = STATE_SUBMENU_SETTINGS; 
                        submenu_index = 0;
                        reset_menu_pointers();
                    }
                    else if (menu_index == 2) {
                        screen_state = STATE_MEDICATION;
                        reset_menu_pointers();
                    }
                    update_screen();
                    DEV_Delay_ms(200);
                }
                break;

            case STATE_SUBMENU_DATETIME:
                if (up_pressed && submenu_index > 0) { 
                    submenu_index--; 
                    on_user_activity(); 
                    show_submenu_datetime(); 
                }
                if (down_pressed && submenu_index < 1) { 
                    submenu_index++; 
                    on_user_activity(); 
                    show_submenu_datetime(); 
                }
                if (select_pressed) {
                    on_user_activity();
                    selected_digit = 0;
                    screen_state = (submenu_index == 0) ? STATE_CONFIG_TIME : STATE_CONFIG_DATE;
                    reset_menu_pointers();
                    update_screen();
                    DEV_Delay_ms(200);
                }
                break;

            case STATE_SUBMENU_SETTINGS:
                if (up_pressed && submenu_index > 0) { 
                    submenu_index--; 
                    on_user_activity(); 
                    show_submenu_settings(); 
                }
                if (down_pressed && submenu_index < 1) { 
                    submenu_index++; 
                    on_user_activity(); 
                    show_submenu_settings(); 
                }
                if (select_pressed) {
                    on_user_activity();
                    screen_state = (submenu_index == 0) ? STATE_CONFIG_BRIGHTNESS : STATE_CONFIG_TIMEOUT;
                    reset_menu_pointers();
                    update_screen();
                    DEV_Delay_ms(200);
                }
                break;

            case STATE_CONFIG_TIME:
                if (select_pressed) { selected_digit ^= 1; on_user_activity(); show_time_config(); }
                if (up_pressed) {
                    on_user_activity();
                    if (selected_digit == 0) { hh++; if (hh > 23) hh = 0; }
                    else { mm++; if (mm > 59) mm = 0; }
                    ss = 0;
                    show_time_config();
                }
                if (down_pressed) {
                    on_user_activity();
                    if (selected_digit == 0) { hh--; if (hh < 0) hh = 23; }
                    else { mm--; if (mm < 0) mm = 59; }
                    ss = 0;
                    show_time_config();
                }
                break;

            case STATE_CONFIG_DATE:
                if (select_pressed) { 
                    selected_digit = (selected_digit + 1) % 3; 
                    on_user_activity(); 
                    show_date_config(); 
                }
                if (up_pressed) {
                    on_user_activity();
                    if (selected_digit == 0) {
                        dd++; 
                        if (dd > days_in_month(mo, yy)) dd = 1;
                    } else if (selected_digit == 1) {
                        mo++; 
                        if (mo > 12) mo = 1;
                        if (dd > days_in_month(mo, yy)) dd = days_in_month(mo, yy);
                    } else {
                        yy++;
                    }
                    show_date_config();
                }
                if (down_pressed) {
                    on_user_activity();
                    if (selected_digit == 0) {
                        dd--; 
                        if (dd < 1) dd = days_in_month(mo, yy);
                    } else if (selected_digit == 1) {
                        mo--; 
                        if (mo < 1) mo = 12;
                        if (dd > days_in_month(mo, yy)) dd = days_in_month(mo, yy);
                    } else {
                        yy--;
                        if (yy < 2025) yy = 2025;
                    }
                    show_date_config();
                }
                break;

            case STATE_CONFIG_BRIGHTNESS:
                if (up_pressed) {
                    on_user_activity();
                    if (brightness < 10) {
                        brightness++;
                        LCD_1IN47_BL(brightness);
                    }
                    show_brightness_config();
                }
                if (down_pressed) {
                    on_user_activity();
                    if (brightness > 1) {
                        brightness--;
                        LCD_1IN47_BL(brightness);
                    }
                    show_brightness_config();
                }
                break;

            case STATE_CONFIG_TIMEOUT:
                if (up_pressed) {
                    on_user_activity();
                    if (timeout_index < 4) timeout_index++;
                    show_timeout_config();
                }
                if (down_pressed) {
                    on_user_activity();
                    if (timeout_index > 0) timeout_index--;
                    show_timeout_config();
                }
                break;

            case STATE_MEDICATION:
                if (up_pressed || down_pressed) {
                    on_user_activity();
                    medication_taken = !medication_taken;
                    show_medication_screen();
                }
                break;
        }

        if (inactivity_timer >= screen_timeout_options[timeout_index]) {
            go_to_idle_mode();
            inactivity_timer = 0;
        }
    }
}