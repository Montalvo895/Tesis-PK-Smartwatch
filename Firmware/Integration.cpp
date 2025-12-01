// // ============================================================
// // Integration.cpp - CORE 1
// // BMI270 + FIR + EDGE IMPULSE + BLE
// // Se ejecuta en Core 1 independientemente de LVGL (Core 0)
// // ============================================================

// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/spi.h"
// #include "hardware/uart.h"
// #include "bmi270_driver/bmi2.h"
// #include "bmi270_driver/bmi270.h"
// #include "bmi270_driver/bmi2_defs.h"
// #include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// // ============================================================
// // CONFIGURACIÓN DE PINES SPI PARA BMI270
// // ============================================================
// #define USE_PIN_SET_1

// #ifdef USE_PIN_SET_1
//     #define PIN_MISO 0
//     #define PIN_CS   1
//     #define PIN_SCK  2
//     #define PIN_MOSI 3
//     #define SPI_PORT spi0
// #else
//     #define PIN_MISO 16
//     #define PIN_CS   17
//     #define PIN_SCK  18
//     #define PIN_MOSI 19
//     #define SPI_PORT spi0
// #endif

// // ============================================================
// // CONFIGURACIÓN UART PARA BLE
// // ============================================================
// #define UART_ID uart1
// #define BAUD_RATE 115200
// #define UART_TX_PIN 4
// #define UART_RX_PIN 5

// // ============================================================
// // CONFIGURACIÓN DEL CLASIFICADOR
// // ============================================================
// #define EI_WINDOW_SIZE 38
// #define EI_AXES 6
// #define EI_BUFFER_SIZE (EI_WINDOW_SIZE * EI_AXES)

// // ============================================================
// // FILTRO FIR
// // ============================================================
// #define NUM_TAPS 51

// const float fir_b[NUM_TAPS] = {
//     1.01726639929957e-19, 0.000201175858607125, 0.000408604796468194,
//     0.000238348694096093, -0.000117840315794661, 0.000460946848329137,
//     0.00268479506410267, 0.00498398313542146, 0.00433539665217539,
//     0.000388123879577121, -0.00213686414697370, 0.00183375715213004,
//     0.00928934072929410, 0.00864956109073784, -0.00739610879347813,
//     -0.0285751618069780, -0.0333704662957633, -0.0152001602452257,
//     0.000552527733305619, -0.0226677993144042, -0.0859485322235069,
//     -0.133712343198909, -0.0944839545179708, 0.0450093200168172,
//     0.207757123645979, 0.279976096805085, 0.207757123645979,
//     0.0450093200168172, -0.0944839545179708, -0.133712343198909,
//     -0.0859485322235069, -0.0226677993144042, 0.000552527733305619,
//     -0.0152001602452257, -0.0333704662957633, -0.0285751618069780,
//     -0.00739610879347813, 0.00864956109073784, 0.00928934072929410,
//     0.00183375715213004, -0.00213686414697370, 0.000388123879577121,
//     0.00433539665217539, 0.00498398313542146, 0.00268479506410267,
//     0.000460946848329137, -0.000117840315794661, 0.000238348694096093,
//     0.000408604796468194, 0.000201175858607125, 1.01726639929957e-19
// };

// static float fir_delay_x[NUM_TAPS] = {0};
// static float fir_delay_y[NUM_TAPS] = {0};
// static float fir_delay_z[NUM_TAPS] = {0};
// static float fir_delay_gx[NUM_TAPS] = {0};
// static float fir_delay_gy[NUM_TAPS] = {0};
// static float fir_delay_gz[NUM_TAPS] = {0};

// static int fir_index_x = 0, fir_index_y = 0, fir_index_z = 0;
// static int fir_index_gx = 0, fir_index_gy = 0, fir_index_gz = 0;

// static float inference_buffer[EI_BUFFER_SIZE] = {0};
// static int buffer_index = 0;

// extern const uint8_t bmi270_config_file[];
// struct bmi2_dev bmi2_dev;

// // ============================================================
// // FUNCIONES
// // ============================================================

// float fir_filter(float x, float *delay, int *index) {
//     delay[*index] = x;
//     float y = 0.0f;
//     int j = *index;

//     for (int i = 0; i < NUM_TAPS; i++) {
//         y += fir_b[i] * delay[j--];
//         if (j < 0) j = NUM_TAPS - 1;
//     }

//     (*index)++;
//     if (*index >= NUM_TAPS) *index = 0;
//     return y;
// }

// int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
//     memcpy(out_ptr, inference_buffer + offset, length * sizeof(float));
//     return 0;
// }

// void uart_init_ble() {
//     uart_init(UART_ID, BAUD_RATE);
//     gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
//     gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
// }

// void uart_send_string(const char *str) {
//     uart_write_blocking(UART_ID, (const uint8_t *)str, strlen(str));
// }

// int8_t bmi2_spi_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr) {
//     gpio_put(PIN_CS, 0);
//     uint8_t addr = reg_addr | 0x80;
//     spi_write_blocking(SPI_PORT, &addr, 1);
//     spi_read_blocking(SPI_PORT, 0x00, data, len + bmi2_dev.dummy_byte);
//     gpio_put(PIN_CS, 1);
//     return 0;
// }

// int8_t bmi2_spi_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr) {
//     uint8_t tx[len + 1];
//     tx[0] = reg_addr & 0x7F;
//     for (uint32_t i = 0; i < len; i++) tx[i + 1] = data[i];
//     gpio_put(PIN_CS, 0);
//     spi_write_blocking(SPI_PORT, tx, len + 1);
//     gpio_put(PIN_CS, 1);
//     return BMI2_OK;
// }

// void bmi2_delay_us(uint32_t period, void *intf_ptr) {
//     sleep_us(period);
// }

// // ============================================================
// // FUNCIÓN PRINCIPAL DEL CORE 1 (llamada desde main.c)
// // ============================================================
// extern "C" void core1_entry(void) {
    
//     // stdio_init_all();
//     // stdio_uart_init_full(UART_ID, BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
//     // Inicializar UART para BLE
    
//     uart_init_ble();
//     sleep_ms(100);
//     printf("[Core 1] UART BLE inicializado\n");
//     printf("[Core 1] Iniciando BMI270 + Edge Impulse...\n");
//     // Inicializar SPI
//     spi_init(SPI_PORT, 1 * 1000 * 1000);
//     gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
//     gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
//     gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
//     gpio_init(PIN_CS);
//     gpio_set_dir(PIN_CS, GPIO_OUT);
//     gpio_put(PIN_CS, 1);
    
//     // Reset SPI
//     gpio_put(PIN_CS, 0);
//     sleep_us(1000);
//     gpio_put(PIN_CS, 1);
//     sleep_us(1000);

//     printf("[Core 1] SPI inicializado\n");

//     // Configurar BMI270
//     bmi2_dev.intf = BMI2_SPI_INTF;
//     bmi2_dev.read = bmi2_spi_read;
//     bmi2_dev.write = bmi2_spi_write;
//     bmi2_dev.delay_us = bmi2_delay_us;
//     bmi2_dev.read_write_len = 32;
//     bmi2_dev.config_file_ptr = bmi270_config_file;
//     bmi2_dev.intf_ptr = NULL;
//     bmi2_dev.dummy_byte = 1;

//     // Dummy read
//     uint8_t dummy;
//     bmi2_get_regs(0x00, &dummy, 1, &bmi2_dev);

//     // Verificar CHIP ID
//     uint8_t chip_id;
//     int8_t rslt = bmi2_get_regs(BMI2_CHIP_ID_ADDR, &chip_id, 1, &bmi2_dev);
//     if (rslt == BMI2_OK && chip_id == 0x24) {
//         printf("[Core 1] ✓ BMI270 detectado (CHIP_ID: 0x%02X)\n", chip_id);
//     } else {
//         printf("[Core 1] ✗ ERROR: No se detectó BMI270 (rslt=%d, ID=0x%02X)\n", rslt, chip_id);
//         printf("[Core 1] Verifica las conexiones SPI\n");
//         while (1) tight_loop_contents();
//     }

//     rslt = bmi270_init(&bmi2_dev);
//     if (rslt != BMI2_OK) {
//         printf("[Core 1] ✗ ERROR: bmi270_init falló\n");
//         while (1) tight_loop_contents();
//     }

//     printf("[Core 1] BMI270 inicializado correctamente\n");

//     // Configurar sensor
//     uint8_t pwr_ctrl = 0x0E;
//     bmi2_set_regs(0x7D, &pwr_ctrl, 1, &bmi2_dev);
//     uint8_t acc_conf = 0xA7, gyr_conf = 0xA7;
//     bmi2_set_regs(0x40, &acc_conf, 1, &bmi2_dev);
//     bmi2_set_regs(0x42, &gyr_conf, 1, &bmi2_dev);
//     uint8_t pwr_conf = 0x02;
//     bmi2_set_regs(0x7C, &pwr_conf, 1, &bmi2_dev);
//     uint8_t acc_range = 0x01, gyr_range = 0x04;
//     bmi2_set_regs(0x41, &acc_range, 1, &bmi2_dev);
//     bmi2_set_regs(0x43, &gyr_range, 1, &bmi2_dev);

//     printf("[Core 1] Configuración del sensor completada\n");

//     // Constantes de conversión
//     const float ACC_LSB_TO_G = 0.000122f * 9.81f;
//     const float GYR_LSB_TO_DPS = 1.0f / 262.4f;
    
//     char ble_buffer[128];
//     char pred_buffer[128];

//     ei_impulse_result_t result = {0};
//     int sample_count = 0;

//     printf("[Core 1] Entrando en loop principal...\n");

//     // ============================================================
//     // LOOP PRINCIPAL - CORE 1
//     // ============================================================
//     while (1) {
//         uint8_t sensor_data[12] = {0};
//         rslt = bmi2_get_regs(0x0C, sensor_data, 12, &bmi2_dev);

//         if (rslt == BMI2_OK) {
//             // Leer datos crudos
//             int16_t ax = (int16_t)((sensor_data[1] << 8) | sensor_data[0]);
//             int16_t ay = (int16_t)((sensor_data[3] << 8) | sensor_data[2]);
//             int16_t az = (int16_t)((sensor_data[5] << 8) | sensor_data[4]);
//             int16_t gx = (int16_t)((sensor_data[7] << 8) | sensor_data[6]);
//             int16_t gy = (int16_t)((sensor_data[9] << 8) | sensor_data[8]);
//             int16_t gz = (int16_t)((sensor_data[11] << 8) | sensor_data[10]);

//             // Convertir a unidades físicas
//             float acc_x_raw = ax * ACC_LSB_TO_G;
//             float acc_y_raw = ay * ACC_LSB_TO_G;
//             float acc_z_raw = az * ACC_LSB_TO_G;
//             float gyr_x_raw = gx * GYR_LSB_TO_DPS;
//             float gyr_y_raw = gy * GYR_LSB_TO_DPS;
//             float gyr_z_raw = gz * GYR_LSB_TO_DPS;

//             // Aplicar filtro FIR
//             float acc_x_filt = fir_filter(acc_x_raw, fir_delay_x, &fir_index_x);
//             float acc_y_filt = fir_filter(acc_y_raw, fir_delay_y, &fir_index_y);
//             float acc_z_filt = fir_filter(acc_z_raw, fir_delay_z, &fir_index_z);
//             float gyr_x_filt = fir_filter(gyr_x_raw, fir_delay_gx, &fir_index_gx);
//             float gyr_y_filt = fir_filter(gyr_y_raw, fir_delay_gy, &fir_index_gy);
//             float gyr_z_filt = fir_filter(gyr_z_raw, fir_delay_gz, &fir_index_gz);

//             // Almacenar en buffer de inferencia
//             inference_buffer[buffer_index++] = acc_x_filt;
//             inference_buffer[buffer_index++] = acc_y_filt;
//             inference_buffer[buffer_index++] = acc_z_filt;
//             inference_buffer[buffer_index++] = gyr_x_filt;
//             inference_buffer[buffer_index++] = gyr_y_filt;
//             inference_buffer[buffer_index++] = gyr_z_filt;

//             sample_count++;

//             // Enviar datos filtrados por BLE
//             // snprintf(ble_buffer, sizeof(ble_buffer),
//             //     "SENS:AX=%.3f,AY=%.3f,AZ=%.3f,GX=%.3f,GY=%.3f,GZ=%.3f\n",
//             //     acc_x_filt, acc_y_filt, acc_z_filt,
//             //     gyr_x_filt, gyr_y_filt, gyr_z_filt);
//             // uart_send_string(ble_buffer);
//             snprintf(ble_buffer, sizeof(ble_buffer),
//                 "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
//                 acc_x_filt, acc_y_filt, acc_z_filt,
//                 gyr_x_filt, gyr_y_filt, gyr_z_filt);
//             uart_send_string(ble_buffer);
//             printf("[Core 1] %s", ble_buffer);

//             // Cuando el buffer está lleno, ejecutar clasificador
//             if (buffer_index >= EI_BUFFER_SIZE) {
//                 buffer_index = 0;
//                 sample_count = 0;

//                 // Crear señal para el clasificador
//                 signal_t features_signal;
//                 features_signal.total_length = EI_BUFFER_SIZE;
//                 features_signal.get_data = &raw_feature_get_data;

//                 // Ejecutar inferencia
//                 EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
                
//                 if (res == EI_IMPULSE_OK) {
//                     // Encontrar la clase con mayor probabilidad
//                     float max_value = 0.0f;
//                     int max_index = 0;
//                     for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
//                         if (result.classification[i].value > max_value) {
//                             max_value = result.classification[i].value;
//                             max_index = i;
//                         }
//                     }
                    
//                     // Enviar predicción por BLE
//                     // snprintf(pred_buffer, sizeof(pred_buffer),
//                     //     "PRED:%s,%.2f\n",
//                     //     ei_classifier_inferencing_categories[max_index],
//                     //     max_value * 100.0f);
//                     // uart_send_string(pred_buffer);
//                     // printf("[Core 1] %s", pred_buffer);
//                 }
//             }
//         }

//         sleep_ms(20);
//     }
// }

// ============================================================
// Integration.cpp - CORE 1
// BMI270 + FIR + EDGE IMPULSE + BLE
// Se ejecuta en Core 1 independientemente de LVGL (Core 0)
// Versión MEJORADA con control de buffer y ventana deslizante
// ============================================================

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "bmi270_driver/bmi2.h"
#include "bmi270_driver/bmi270.h"
#include "bmi270_driver/bmi2_defs.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// ============================================================
// CONFIGURACIÓN DE PINES SPI PARA BMI270
// ============================================================
#define USE_PIN_SET_1

#ifdef USE_PIN_SET_1
    #define PIN_MISO 0
    #define PIN_CS   1
    #define PIN_SCK  2
    #define PIN_MOSI 3
    #define SPI_PORT spi0
#else
    #define PIN_MISO 16
    #define PIN_CS   17
    #define PIN_SCK  18
    #define PIN_MOSI 19
    #define SPI_PORT spi0
#endif

// ============================================================
// CONFIGURACIÓN UART PARA BLE
// ============================================================
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// ============================================================
// CONFIGURACIÓN DEL CLASIFICADOR
// ============================================================
#define EI_WINDOW_SIZE 38
#define EI_AXES 6
#define EI_BUFFER_SIZE (EI_WINDOW_SIZE * EI_AXES)  // 38 × 6 = 228

// ============================================================
// FILTRO FIR
// ============================================================
#define NUM_TAPS 51

const float fir_b[NUM_TAPS] = {
    1.01726639929957e-19, 0.000201175858607125, 0.000408604796468194,
    0.000238348694096093, -0.000117840315794661, 0.000460946848329137,
    0.00268479506410267, 0.00498398313542146, 0.00433539665217539,
    0.000388123879577121, -0.00213686414697370, 0.00183375715213004,
    0.00928934072929410, 0.00864956109073784, -0.00739610879347813,
    -0.0285751618069780, -0.0333704662957633, -0.0152001602452257,
    0.000552527733305619, -0.0226677993144042, -0.0859485322235069,
    -0.133712343198909, -0.0944839545179708, 0.0450093200168172,
    0.207757123645979, 0.279976096805085, 0.207757123645979,
    0.0450093200168172, -0.0944839545179708, -0.133712343198909,
    -0.0859485322235069, -0.0226677993144042, 0.000552527733305619,
    -0.0152001602452257, -0.0333704662957633, -0.0285751618069780,
    -0.00739610879347813, 0.00864956109073784, 0.00928934072929410,
    0.00183375715213004, -0.00213686414697370, 0.000388123879577121,
    0.00433539665217539, 0.00498398313542146, 0.00268479506410267,
    0.000460946848329137, -0.000117840315794661, 0.000238348694096093,
    0.000408604796468194, 0.000201175858607125, 1.01726639929957e-19
};

static float fir_delay_x[NUM_TAPS] = {0};
static float fir_delay_y[NUM_TAPS] = {0};
static float fir_delay_z[NUM_TAPS] = {0};
static float fir_delay_gx[NUM_TAPS] = {0};
static float fir_delay_gy[NUM_TAPS] = {0};
static float fir_delay_gz[NUM_TAPS] = {0};

static int fir_index_x = 0, fir_index_y = 0, fir_index_z = 0;
static int fir_index_gx = 0, fir_index_gy = 0, fir_index_gz = 0;

static float inference_buffer[EI_BUFFER_SIZE] = {0};
static int buffer_index = 0;

extern const uint8_t bmi270_config_file[];
struct bmi2_dev bmi2_dev;

// ============================================================
// FUNCIONES
// ============================================================

float fir_filter(float x, float *delay, int *index) {
    delay[*index] = x;
    float y = 0.0f;
    int j = *index;

    for (int i = 0; i < NUM_TAPS; i++) {
        y += fir_b[i] * delay[j--];
        if (j < 0) j = NUM_TAPS - 1;
    }

    (*index)++;
    if (*index >= NUM_TAPS) *index = 0;
    return y;
}

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, inference_buffer + offset, length * sizeof(float));
    return 0;
}

void uart_init_ble() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void uart_send_string(const char *str) {
    uart_write_blocking(UART_ID, (const uint8_t *)str, strlen(str));
}

int8_t bmi2_spi_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr) {
    gpio_put(PIN_CS, 0);
    uint8_t addr = reg_addr | 0x80;
    spi_write_blocking(SPI_PORT, &addr, 1);
    spi_read_blocking(SPI_PORT, 0x00, data, len + bmi2_dev.dummy_byte);
    gpio_put(PIN_CS, 1);
    return 0;
}

int8_t bmi2_spi_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr) {
    uint8_t tx[len + 1];
    tx[0] = reg_addr & 0x7F;
    for (uint32_t i = 0; i < len; i++) tx[i + 1] = data[i];
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, tx, len + 1);
    gpio_put(PIN_CS, 1);
    return BMI2_OK;
}

void bmi2_delay_us(uint32_t period, void *intf_ptr) {
    sleep_us(period);
}

// ============================================================
// FUNCIÓN PRINCIPAL DEL CORE 1 (llamada desde main.c)
// ============================================================
extern "C" void core1_entry(void) {
    
    // Inicializar UART para BLE
    uart_init_ble();
    sleep_ms(100);
    printf("[Core 1] ✓ UART BLE inicializado\n");
    printf("[Core 1] Iniciando BMI270 + Edge Impulse...\n");
    
    // Inicializar SPI
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    
    // Reset SPI
    gpio_put(PIN_CS, 0);
    sleep_us(1000);
    gpio_put(PIN_CS, 1);
    sleep_us(1000);

    printf("[Core 1] ✓ SPI inicializado\n");

    // Configurar BMI270
    bmi2_dev.intf = BMI2_SPI_INTF;
    bmi2_dev.read = bmi2_spi_read;
    bmi2_dev.write = bmi2_spi_write;
    bmi2_dev.delay_us = bmi2_delay_us;
    bmi2_dev.read_write_len = 32;
    bmi2_dev.config_file_ptr = bmi270_config_file;
    bmi2_dev.intf_ptr = NULL;
    bmi2_dev.dummy_byte = 1;

    // Dummy read
    uint8_t dummy;
    bmi2_get_regs(0x00, &dummy, 1, &bmi2_dev);

    // Verificar CHIP ID
    uint8_t chip_id;
    int8_t rslt = bmi2_get_regs(BMI2_CHIP_ID_ADDR, &chip_id, 1, &bmi2_dev);
    if (rslt == BMI2_OK && chip_id == 0x24) {
        printf("[Core 1] ✓ BMI270 detectado (CHIP_ID: 0x%02X)\n", chip_id);
    } else {
        printf("[Core 1] ✗ ERROR: No se detectó BMI270 (rslt=%d, ID=0x%02X)\n", rslt, chip_id);
        printf("[Core 1] Verifica las conexiones SPI\n");
        while (1) tight_loop_contents();
    }

    rslt = bmi270_init(&bmi2_dev);
    if (rslt != BMI2_OK) {
        printf("[Core 1] ✗ ERROR: bmi270_init falló\n");
        while (1) tight_loop_contents();
    }

    printf("[Core 1] ✓ BMI270 inicializado correctamente\n");

    // Configurar sensor
    uint8_t pwr_ctrl = 0x0E;
    bmi2_set_regs(0x7D, &pwr_ctrl, 1, &bmi2_dev);
    uint8_t acc_conf = 0xA7, gyr_conf = 0xA7;
    bmi2_set_regs(0x40, &acc_conf, 1, &bmi2_dev);
    bmi2_set_regs(0x42, &gyr_conf, 1, &bmi2_dev);
    uint8_t pwr_conf = 0x02;
    bmi2_set_regs(0x7C, &pwr_conf, 1, &bmi2_dev);
    uint8_t acc_range = 0x01, gyr_range = 0x04;
    bmi2_set_regs(0x41, &acc_range, 1, &bmi2_dev);
    bmi2_set_regs(0x43, &gyr_range, 1, &bmi2_dev);

    printf("[Core 1] ✓ Configuración del sensor completada\n");

    // Constantes de conversión
    const float ACC_LSB_TO_G = 0.000122f * 9.81f;
    const float GYR_LSB_TO_DPS = 1.0f / 262.4f;
    
    char ble_buffer[128];
    char pred_buffer[128];

    ei_impulse_result_t result = {0};

    printf("[Core 1] ✓ Sistema iniciado - Recolectando datos...\n");

    // ============================================================
    // LOOP PRINCIPAL - CORE 1
    // ============================================================
    while (1) {
        uint8_t sensor_data[12] = {0};
        rslt = bmi2_get_regs(0x0C, sensor_data, 12, &bmi2_dev);

        if (rslt == BMI2_OK) {
            // Leer datos crudos
            int16_t ax = (int16_t)((sensor_data[1] << 8) | sensor_data[0]);
            int16_t ay = (int16_t)((sensor_data[3] << 8) | sensor_data[2]);
            int16_t az = (int16_t)((sensor_data[5] << 8) | sensor_data[4]);
            int16_t gx = (int16_t)((sensor_data[7] << 8) | sensor_data[6]);
            int16_t gy = (int16_t)((sensor_data[9] << 8) | sensor_data[8]);
            int16_t gz = (int16_t)((sensor_data[11] << 8) | sensor_data[10]);

            // Convertir a unidades físicas
            float acc_x_raw = ax * ACC_LSB_TO_G;
            float acc_y_raw = ay * ACC_LSB_TO_G;
            float acc_z_raw = az * ACC_LSB_TO_G;
            float gyr_x_raw = gx * GYR_LSB_TO_DPS;
            float gyr_y_raw = gy * GYR_LSB_TO_DPS;
            float gyr_z_raw = gz * GYR_LSB_TO_DPS;

            // Aplicar filtro FIR
            float acc_x_filt = fir_filter(acc_x_raw, fir_delay_x, &fir_index_x);
            float acc_y_filt = fir_filter(acc_y_raw, fir_delay_y, &fir_index_y);
            float acc_z_filt = fir_filter(acc_z_raw, fir_delay_z, &fir_index_z);
            float gyr_x_filt = fir_filter(gyr_x_raw, fir_delay_gx, &fir_index_gx);
            float gyr_y_filt = fir_filter(gyr_y_raw, fir_delay_gy, &fir_index_gy);
            float gyr_z_filt = fir_filter(gyr_z_raw, fir_delay_gz, &fir_index_gz);

            // ✅ PROTECCIÓN: Verificar que no exceda el buffer
            if (buffer_index + EI_AXES <= EI_BUFFER_SIZE) {
                inference_buffer[buffer_index++] = acc_x_filt;
                inference_buffer[buffer_index++] = acc_y_filt;
                inference_buffer[buffer_index++] = acc_z_filt;
                inference_buffer[buffer_index++] = gyr_x_filt;
                inference_buffer[buffer_index++] = gyr_y_filt;
                inference_buffer[buffer_index++] = gyr_z_filt;
            }

            // Enviar datos filtrados por BLE
            // snprintf(ble_buffer, sizeof(ble_buffer),
            //     "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
            //     acc_x_filt, acc_y_filt, acc_z_filt,
            //     gyr_x_filt, gyr_y_filt, gyr_z_filt);
            // uart_send_string(ble_buffer);
            // printf("[Core 1] %s", ble_buffer);

            // Cuando el buffer está lleno, ejecutar clasificador
            if (buffer_index >= EI_BUFFER_SIZE) {
                // Crear señal para el clasificador
                signal_t features_signal;
                features_signal.total_length = EI_BUFFER_SIZE;
                features_signal.get_data = &raw_feature_get_data;

                // Ejecutar inferencia
                EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
                
                if (res == EI_IMPULSE_OK) {
                    // Encontrar la clase con mayor probabilidad
                    float max_value = 0.0f;
                    int max_index = 0;
                    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                        if (result.classification[i].value > max_value) {
                            max_value = result.classification[i].value;
                            max_index = i;
                        }
                    }
                    
                    // Enviar predicción por BLE
                    snprintf(pred_buffer, sizeof(pred_buffer),
                        "PRED:%s,%.2f\n",
                        ei_classifier_inferencing_categories[max_index],
                        max_value * 100.0f);
                    uart_send_string(pred_buffer);
                    printf("[Core 1] %s", pred_buffer);
                }

                // ✅ SOLUCIÓN ROBUSTA: Desplazar buffer de forma segura (ventana deslizante)
                // Desplazar 50% (19 muestras = 114 valores)
                const int SHIFT_SAMPLES = EI_WINDOW_SIZE / 2;  // 19 muestras
                const int SHIFT_VALUES = SHIFT_SAMPLES * EI_AXES;  // 114 valores (19 × 6)
                
                // Mover datos: copiar últimas 19 muestras al inicio
                for (int i = 0; i < (EI_BUFFER_SIZE - SHIFT_VALUES); i++) {
                    inference_buffer[i] = inference_buffer[i + SHIFT_VALUES];
                }
                
                // Resetear índice para continuar desde donde quedó
                buffer_index = EI_BUFFER_SIZE - SHIFT_VALUES;  // 114
            }
        }

        sleep_ms(10);  // ✅ 100Hz → Predicciones cada ~380ms (mejor que 20ms)
    }
}



