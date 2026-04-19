# U2_lab3 - Sistema de Control de Clima Industrial

Laboratorio 3 de la Unidad 2 de Sistemas en Tiempo Real. Implementa un sistema de control de clima usando el sensor de temperatura AHT10 (I2C), tres hilos POSIX concurrentes y una máquina de estados para controlar un actuador de ventilación.

## Hardware requerido

| Componente | GPIO | Pin Físico |
|---|---|---|
| Sensor AHT10 (SDA) | GPIO2 | 3 |
| Sensor AHT10 (SCL) | GPIO3 | 5 |
| Ventilador/Actuador | GPIO17 | 11 |

> Verificar que I2C esté habilitado: `sudo raspi-config` → Interface Options → I2C → Enable  
> Confirmar que el sensor aparece en el bus: `i2cdetect -y 1` (debe mostrar `0x38`)

## Arquitectura de hilos

| Hilo | Prioridad | Función |
|---|---|---|
| Tarea A — Adquisición | SCHED_FIFO / 80 | Lee el AHT10 cada 1 segundo y actualiza la variable global |
| Tarea B — Control lógico | SCHED_FIFO / 40 | Evalúa la máquina de estados cada 500ms |
| Tarea C — Diagnóstico | SCHED_OTHER | Imprime temperatura y estado cada 5 segundos |

## Máquina de estados

```
REPOSO ──(T > 30°C)──→ ALERTA ──(60 seg con T > 30°C)──→ VENTILACION
  ↑                      |                                      |
  └──(T < 30°C)──────────┘        (T < 25°C o 120 seg) ────────┘
```

> Los umbrales de temperatura son ajustables en el código según el ambiente de prueba.

## Compilación y ejecución

```bash
gcc -o control_clima control_de_clima.c -lpigpio -lpthread -lm
sudo ./control_clima
```

## Ejemplo de salida

```
[DIAGNOSTICO] Temperatura actual: 23.00 °C | Estado: REPOSO
[CONTROL] Estado: ALERTA - Temperatura = 24.07 °C
[DIAGNOSTICO] Temperatura actual: 24.19 °C | Estado: ALERTA
[CONTROL] Estado: VENTILACION - Temperatura sigue alta = 24.80 °C
[DIAGNOSTICO] Temperatura actual: 24.80 °C | Estado: VENTILACION
[CONTROL] Estado: REPOSO - Temperatura bajó = 23.99 °C
```

## Conceptos demostrados

- Comunicación I2C con `i2cOpen`, `i2cWriteDevice`, `i2cReadDevice`
- Tres hilos POSIX con prioridades RT mixtas (`SCHED_FIFO` y `SCHED_OTHER`)
- Mutex para proteger variables compartidas sin bloquear operaciones I2C
- Máquina de estados con temporización precisa mediante `gpioTick()`