Thesis PK Smartwatch
Sistema wearable para monitoreo de síntomas motores en pacientes con Parkinson.

Este repositorio contiene el firmware del reloj inteligente desarrollado con el módulo RP2350A, el sensor inercial BMI270, un módulo BLE JDY-10 y una pantalla LCD SPI. Incluye además la aplicación móvil construida en Flutter, encargada de recibir los datos, procesarlos y almacenarlos en Firebase, así como mostrar el estado del paciente en tiempo real.

------------------------------------------------------------
Estructura general del repositorio
------------------------------------------------------------

1. /Firmware/
   - Lectura del sensor inercial BMI270 (acelerómetro y giroscopio)
   - Envío de datos vía BLE usando el módulo JDY-10
   - Control de la pantalla LCD SPI
   - Implementación del modelo de Edge Impulse
   - Gestión de energía y modos de bajo consumo

2. /mobile_app/
   - Conexión BLE y adquisición de datos en tiempo real
   - Decodificación y visualización del estado del paciente
   - Panel de gráficos y métricas clínicas
   - Sincronización con Firebase (Auth, Firestore, Storage)
   - Arquitectura basada en Provider y patrones limpios

------------------------------------------------------------
Arquitectura general del sistema
------------------------------------------------------------

Dispositivo (Smartwatch):
   - Captura aceleración y velocidad angular
   - Ejecuta inferencia local con Edge Impulse
   - Envía datos al smartphone vía BLE

Aplicación móvil:
   - Recibe datos BLE
   - Muestra resultados en tiempo real
   - Gestiona sesiones de usuario
   - Sincroniza historial clínico

Backend (Firebase):
   - Almacena sesiones, estados e inferencias

------------------------------------------------------------
Tecnologías utilizadas
------------------------------------------------------------

- RP2350A (Raspberry Pi)
- Sensor IMU BMI270
- BLE JDY-10
- LCD SPI
- C / C++
- Edge Impulse (TinyML)
- Flutter (Dart)
- Firebase (Auth, Firestore, Storage)

------------------------------------------------------------
Cómo ejecutar
------------------------------------------------------------

Firmware:
   1. Abrir el proyecto en VS Code
   2. Compilar para RP2350A
   3. Flashear el binario
   4. Activar modo de pruebas (BMI270 + BLE)

Aplicación móvil:
   1. Instalar Flutter 3.0+
   2. Ejecutar:
      flutter pub get
      flutter run
   3. Emparejar con el reloj y visualizar datos

------------------------------------------------------------
Estado del proyecto
------------------------------------------------------------

Actualmente en fase de validación experimental dentro del trabajo de grado.
