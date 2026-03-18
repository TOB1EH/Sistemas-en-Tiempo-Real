# U1_lab2 - Control de Servo Motor con Interrupciones

Laboratorio 2 de Sistemas en Tiempo Real: Control de movimiento de servo motor mediante botón con interrupciones hardware en Raspberry Pi.

## Descripción General

Este laboratorio demuestra el control de un servo motor (0-180 grados) usando:
- **Interrupciones hardware** para detectar pulsaciones de botón
- **PWM (Modulación por Ancho de Pulso)** para controlar la posición del servo
- **Patrón productor-consumidor** para separar operaciones críticas de no-críticas
- **Filtro anti-rebote por hardware** de 200ms

## Requisitos

### Hardware
- Raspberry Pi (cualquier modelo con GPIO)
- Servo motor SG90 o similar (compatible con PWM 1-2.5ms)
- Botón pulsador
- Resistencia de pull-down (10kΩ recomendado)
- Fuente de alimentación para el servo (5V)
- Cables de conexión

### Software
- Librería **pigpio** instalada
- GCC compiler
- Linux OS en Raspberry Pi

#### Instalar pigpio:
```bash
sudo apt-get update
sudo apt-get install pigpio python3-pigpio
```

## Esquema de Conexiones

```
Raspberry Pi Physical Pinout:
┌─────────────────────────┐
│ GPIO Pin    │ Función   │
├─────────────────────────┤
│ GPIO17 (Pin 11) → Botón (con pull-down interno)
│ GPIO24 (Pin 18) → Servo PWM
│ GND (Pin 6/9/14/20/25/30/34/39) → GND común
└─────────────────────────┘
```

## Especificaciones del Servo

Los servos estándar (como SG90) responden a pulsos PWM en el rango:
- **500 µs** → 0° (posición mínima)
- **1500 µs** → 90° (posición central)
- **2500 µs** → 180° (posición máxima)

En este programa usamos:
- **POSICION_MINIMA = 500 µs** (0°)
- **POSICION_MAXIMA = 2498 µs** (≈180°)
- **SALTOS_POSICION = 333 µs** (≈30° por pulsación)

## Compilación

```bash
gcc -o control_movimiento_servo control_movimiento_servo.c -lpigpio -lm
```

## Ejecución

```bash
# Ejecutar con permisos de administrador (pigpio requiere acceso directo al hardware)
sudo ./control_movimiento_servo
```

### Ejemplo de Salida
```
Botón presionado. Posición del servo: 500 microsegundos
Botón presionado. Posición del servo: 833 microsegundos
Botón presionado. Posición del servo: 1166 microsegundos
Botón presionado. Posición del servo: 1499 microsegundos
Botón presionado. Posición del servo: 1832 microsegundos
...
Botón presionado. Posición del servo: POSICION_MINIMA (reinicio)
```

## Descripción del Código

### Constantes de Configuración

```c
#define GPIO_PIN_BUTTON 17      // Pin del botón
#define GPIO_PIN_SERVO 24       // Pin del servo
#define POSICION_MINIMA 500     // Mínimo PWM (0°)
#define POSICION_MAXIMA 2498    // Máximo PWM (180°)
#define SALTOS_POSICION 333     // Incremento por pulsación (≈30°)
```

### Arquitectura de Control

**Patrón Productor-Consumidor:**
- **Productor (ISR)**: Detecta pulsación del botón e incrementa la posición
- **Consumidor (Bucle Principal)**: Imprime el estado sin bloquear la interrupción

**Variables Compartidas:**
```c
int posicion_servo;              // Posición actual del servo (0-2498 µs)
volatile int flag_imprimir_estado; // Bandera para que main imprima
volatile sig_atomic_t running;    // Control de terminación (Ctrl+C)
```

### Manejador de Interrupción (ISR)

El manejador se ejecuta en cada flanco de subida del botón:

1. **Verifica la posición actual**
2. **Incrementa la posición** en saltos de ~30°
3. **Reinicia a 0°** cuando alcanza el máximo
4. **Envía PWM al servo** mediante `gpioServo()`
5. **Levanta una bandera** para que main imprima (sin printf en ISR)

```c
void manejador_interrupcion_boton(int gpio, int level, uint32_t tick)
{
    if (level == 1) // Flanco de subida (botón liberado)
    {
        // Incrementar posición o reiniciar
        if (posicion_servo >= POSICION_MAXIMA)
            posicion_servo = POSICION_MINIMA;
        else
            posicion_servo += SALTOS_POSICION;

        // Enviar comando PWM al servo
        gpioServo(GPIO_PIN_SERVO, posicion_servo);
        
        // Activar bandera para que main imprima
        flag_imprimir_estado = 1;
    }
}
```

### Bucle Principal

El bucle principal realiza tareas no-críticas:
- Verifica la bandera de impresión
- Imprime el estado actual del servo
- Duerme 10ms para no saturar la CPU

```c
while(running)
{
    if (flag_imprimir_estado == 1)
    {
        printf("Botón presionado. Posición del servo: %d microsegundos\n", posicion_servo);
        flag_imprimir_estado = 0;
    }
    usleep(10000); // Dormir 10ms
}
```

### Filtro Anti-Rebote

Se utiliza filtro de glitch por hardware de 200ms:

```c
gpioSetGlitchFilter(pin_button, 200000); // 200,000 µs = 200ms
```

Esto evita que los rebotes mecánicos del botón causen múltiples interrupciones falsas.

## Características Técnicas

| Característica | Valor |
|---|---|
| **Tipo de Control** | Interrupciones hardware |
| **Precisión PWM** | Microsegundos |
| **Rango de Servo** | 0° a 180° |
| **Incremento por Pulsación** | ~30° |
| **Filtro Anti-Rebote** | 200ms (hardware) |
| **Consumo de CPU Idle** | Muy bajo (el servo es controlado por hardware) |
| **Respuesta a Eventos** | < 1ms típicamente |

## Optimizaciones Implementadas

1. **ISR No-Bloqueante**: Solo realiza operaciones de microsegundos
2. **Bandera de Impresión**: printf delegado al bucle principal (no en ISR)
3. **Filtro Hardware**: Anti-rebote a nivel de hardware, no por software
4. **Sleep Inteligente**: `usleep(10000)` reduce uso de CPU
5. **PWM Hardware**: `gpioServo()` maneja PWM directamente en hardware

## Posibles Mejoras

1. **Control Análogo de Posición**: Usar potenciómetro en lugar de botón
2. **Velocidad Variable**: Implementar aceleración/desaceleración suave
3. **LCD/Display**: Mostrar ángulo en grados en lugar de microsegundos
4. **Múltiples Servos**: Controlar varios servos simultáneamente
5. **Guardado de Estado**: Recordar última posición entre reinicios

## Troubleshooting

### El servo no responde
- Verificar que pigpio esté corriendo: `sudo pigpiod&`
- Comprobar conexión física GPIO24 y GND
- Revisar que la fuente de alimentación sea suficiente (500mA mínimo)

### Comportamiento errático del servo
- Aumentar el filtro anti-rebote: `gpioSetGlitchFilter(pin_button, 300000)`
- Agregar capacitor (0.1µF) entre botón y GND
- Verificar que no haya interferencia electromagnética

### Programa no compila
- Asegurarse que pigpio esté instalado: `sudo apt-get install libpigpio-dev`
- Compilar con flags correctos: `-lpigpio -lm`

## Referencias

- [pigpio Documentation](http://abyz.me.uk/rpi/pigpio/cif.html)
- [Raspberry Pi GPIO Pinout](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
- [Servo Motor Control](https://en.wikipedia.org/wiki/Servo_control)
- [PWM (Pulse Width Modulation)](https://en.wikipedia.org/wiki/Pulse-width_modulation)

## Autor

Sistemas en Tiempo Real - Ingeniería Informática

**Fecha**: Marzo 2026
