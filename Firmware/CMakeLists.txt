# cmake_minimum_required(VERSION 3.13)

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# # == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
# if(WIN32)
#     set(USERHOME $ENV{USERPROFILE})
# else()
#     set(USERHOME $ENV{HOME})
# endif()
# set(sdkVersion 2.2.0)
# set(toolchainVersion RISCV_ZCB_RPI_2_2_0_3)
# set(picotoolVersion 2.2.0-a4)
# set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# if (EXISTS ${picoVscode})
#     include(${picoVscode})
# endif()
# # ====================================================================================

# set(PICO_BOARD pico2 CACHE STRING "Board type")

# # Pull in Raspberry Pi Pico SDK (must be before project)
# include(pico_sdk_import.cmake)

# if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
#   message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
# endif()

# project(Integration_final C CXX ASM)

# # Initialise the Raspberry Pi Pico SDK
# pico_sdk_init()

# # ============================================================
# # DEFINICIONES PARA EDGE IMPULSE
# # ============================================================
# if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk")
#     add_definitions(
#         -DEIDSP_QUANTIZE_FILTERBANK=0
#         -DEI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0
#         -DARM_MATH_LOOPUNROLL
#     )
#     message(STATUS "✓ Edge Impulse SDK encontrado - Habilitando soporte ML")
#     set(EI_ENABLED TRUE)
# else()
#     message(STATUS "✗ Edge Impulse SDK no encontrado - Compilando solo LVGL")
#     set(EI_ENABLED FALSE)
# endif()

# # ============================================================
# # CONFIGURACIÓN LVGL - FORZAR USO DEL lv_conf.h DE LA RAÍZ
# # ============================================================
# set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
# set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
# set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)

# # Definir macro de compilación para que LVGL use nuestro lv_conf.h
# add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)

# # Asegurar que el include path correcto esté primero
# include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# # ============================================================
# # SUBDIRECTORIOS LVGL
# # ============================================================
# add_subdirectory(./lib/Config)
# add_subdirectory(./lib/LCD)
# add_subdirectory(./lib/lvgl)

# # ============================================================
# # INCLUDES
# # ============================================================
# include_directories(
#     ${CMAKE_CURRENT_LIST_DIR}
#     ./lib/Config
#     ./lib/LCD
#     ./lib/lvgl
# )

# # Agregar includes de Integration si existen
# if(EI_ENABLED)
#     include_directories(
#         ./bmi270_driver
#         ./edge-impulse-sdk
#         ./model-parameters
#         ./tflite-model
#     )
# endif()

# # ============================================================
# # UTILIDADES EDGE IMPULSE
# # ============================================================
# if(EI_ENABLED)
#     include(${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/cmake/utils.cmake)
    
#     RECURSIVE_FIND_FILE(EI_CPP "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk" "*.cpp")
#     RECURSIVE_FIND_FILE(EI_CC  "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk" "*.cc")
#     RECURSIVE_FIND_FILE(EI_C   "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk" "*.c")
#     RECURSIVE_FIND_FILE(EI_MODEL "${CMAKE_CURRENT_LIST_DIR}/tflite-model" "*.cpp")
    
#     list(APPEND EI_SOURCES ${EI_CPP} ${EI_CC} ${EI_C} ${EI_MODEL})
    
#     message(STATUS "✓ Edge Impulse: Archivos fuente encontrados")
# endif()

# # ============================================================
# # EJECUTABLE PRINCIPAL
# # ============================================================
# if(EI_ENABLED)
#     # Compilar con Integration (Dual-Core)
#     add_executable(Integration_final
#         main.c
#         Buttons.c
#         Integration.cpp
#         bmi270_driver/bmi2.c
#         bmi270_driver/bmi270.c
#         ${EI_SOURCES}
#     )
#     message(STATUS ">>> Compilando DUAL-CORE: Core0=LVGL | Core1=BMI270+ML")
# else()
#     # Compilar solo LVGL (Single-Core)
#     add_executable(Integration_final
#         main.c
#         Buttons.c
#     )
#     message(STATUS ">>> Compilando SINGLE-CORE: Solo LVGL")
# endif()

# pico_set_program_name(Integration_final "Integration_final")
# pico_set_program_version(Integration_final "1.0")

# # Modify the below lines to enable/disable output over UART/USB
# pico_enable_stdio_uart(Integration_final 1)
# pico_enable_stdio_usb(Integration_final 0)

# # ============================================================
# # OPTIMIZACIÓN
# # ============================================================
# add_compile_options(-O2)

# # ============================================================
# # LIBRERÍAS
# # ============================================================
# if(EI_ENABLED)
#     target_link_libraries(Integration_final
#         LCD 
#         LVGL
#         Config
#         pico_stdlib
#         pico_multicore      # CRÍTICO para dual-core
#         pico_sync           # ⭐ NUEVO: Para mutex entre cores
#         hardware_spi
#         hardware_i2c
#         hardware_dma
#         hardware_uart
#         hardware_gpio
#     )
# else()
#     target_link_libraries(Integration_final
#         LCD 
#         LVGL
#         Config
#         pico_stdlib
#         pico_sync           # ⭐ NUEVO: También en modo single-core
#         hardware_spi
#         hardware_i2c
#         hardware_dma
#     )
# endif()

# # Add the standard include files to the build
# target_include_directories(Integration_final PRIVATE
#     ${CMAKE_CURRENT_LIST_DIR}
# )

# pico_add_extra_outputs(Integration_final)

# Generated Cmake Pico project file - INTEGRADO LVGL + BMI270/EdgeImpulse
# Versión mejorada con GLOB_RECURSE para Edge Impulse

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion RISCV_ZCB_RPI_2_2_0_3)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(Integration_final C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ============================================================
# DEFINICIONES PARA EDGE IMPULSE
# ============================================================
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk")
    add_definitions(
        -DEIDSP_QUANTIZE_FILTERBANK=0
        -DEI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0
        -DARM_MATH_LOOPUNROLL
    )
    message(STATUS "✓ Edge Impulse SDK encontrado - Habilitando soporte ML")
    set(EI_ENABLED TRUE)
else()
    message(STATUS "✗ Edge Impulse SDK no encontrado - Compilando solo LVGL")
    set(EI_ENABLED FALSE)
endif()

# ============================================================
# CONFIGURACIÓN LVGL - FORZAR USO DEL lv_conf.h DE LA RAÍZ
# ============================================================
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)

# Definir macro de compilación para que LVGL use nuestro lv_conf.h
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)

# Asegurar que el include path correcto esté primero
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================
# SUBDIRECTORIOS LVGL
# ============================================================
add_subdirectory(./lib/Config)
add_subdirectory(./lib/LCD)
add_subdirectory(./lib/lvgl)

# ============================================================
# INCLUDES
# ============================================================
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ./lib/Config
    ./lib/LCD
    ./lib/lvgl
)

# Agregar includes de Integration si existen
if(EI_ENABLED)
    include_directories(
        ./bmi270_driver
        ./edge-impulse-sdk
        ./model-parameters
        ./tflite-model
    )
endif()

# ============================================================
# RECOLECCIÓN DE ARCHIVOS EDGE IMPULSE CON GLOB_RECURSE
# ============================================================
if(EI_ENABLED)
    # Buscar recursivamente todos los archivos fuente de Edge Impulse
    file(GLOB_RECURSE EI_CPP "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/*.cpp")
    file(GLOB_RECURSE EI_CC  "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/*.cc")
    file(GLOB_RECURSE EI_C   "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/*.c")
    file(GLOB_RECURSE EI_MODEL "${CMAKE_CURRENT_LIST_DIR}/tflite-model/*.cpp")
    
    # Combinar todas las fuentes
    list(APPEND EI_SOURCES ${EI_CPP} ${EI_CC} ${EI_C} ${EI_MODEL})
    
    # Contar archivos encontrados
    list(LENGTH EI_SOURCES EI_SOURCE_COUNT)
    message(STATUS "✓ Edge Impulse: ${EI_SOURCE_COUNT} archivos fuente encontrados")
endif()

# ============================================================
# EJECUTABLE PRINCIPAL
# ============================================================
if(EI_ENABLED)
    # Compilar con Integration (Dual-Core)
    add_executable(Integration_final
        main.c
        Buttons.c
        Integration.cpp
        bmi270_driver/bmi2.c
        bmi270_driver/bmi270.c
        ${EI_SOURCES}
    )
    message(STATUS ">>> Compilando DUAL-CORE: Core0=LVGL | Core1=BMI270+ML")
else()
    # Compilar solo LVGL (Single-Core)
    add_executable(Integration_final
        main.c
        Buttons.c
    )
    message(STATUS ">>> Compilando SINGLE-CORE: Solo LVGL")
endif()

pico_set_program_name(Integration_final "Integration_final")
pico_set_program_version(Integration_final "1.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Integration_final 1)
pico_enable_stdio_usb(Integration_final 0)

# ============================================================
# OPTIMIZACIÓN
# ============================================================
add_compile_options(-O2)

# ============================================================
# LIBRERÍAS
# ============================================================
if(EI_ENABLED)
    target_link_libraries(Integration_final
        LCD 
        LVGL
        Config
        pico_stdlib
        pico_multicore      # CRÍTICO para dual-core
        pico_sync           # Para mutex entre cores
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_uart
        hardware_gpio
    )
else()
    target_link_libraries(Integration_final
        LCD 
        LVGL
        Config
        pico_stdlib
        pico_sync           # También en modo single-core
        hardware_spi
        hardware_i2c
        hardware_dma
    )
endif()

# Add the standard include files to the build
target_include_directories(Integration_final PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(Integration_final)
