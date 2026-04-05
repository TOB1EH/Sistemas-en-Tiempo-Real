# U2_lab1 - Sistema de Control Multitarea con pthreads y Timers

Laboratorio 1 de la Unidad 2 de Sistemas en Tiempo Real. Implementa un sistema de control de barrera crítica con tres tareas concurrentes sobre Raspberry Pi.

## Descripción

El sistema ejecuta simultáneamente:
- **Barrido de servo** (0°↔180°) mediante ejecutivo cíclico sin `sleep()`
- **Monitoreo de botón de emergencia** con hilo POSIX de prioridad RT (`SCHED_FIFO`)
- **Telemetría periódica** exacta cada 1 segundo via `SIGALRM`

## Requisitos

- Raspberry Pi (cualquier modelo con GPIO)
- Librería **pigpio** instalada
- GCC compiler
- Permisos de root (para GPIO y `SCHED_FIFO`)

```bash
sudo apt-get install pigpio
```

## Pines GPIO

| Componente | GPIO | Pin Físico |
|---|---|---|
| LED advertencia | GPIO4 | 7 |
| Botón emergencia | GPIO17 | 11 |
| Servo motor | GPIO27 | 13 |

## Compilación y Ejecución

```bash
gcc -o sistema_control_multitarea sistema_control_multitarea.c -lpigpio -lpthread -lm
sudo ./sistema_control_multitarea
```

## Salida esperada

```
[TELEMETRIA] Posicion: 820 us | Modo: Seguro
[TELEMETRIA] Posicion: 1460 us | Modo: Seguro
[ALERTA] Parada de emergencia activada - Latencia detectada
[TELEMETRIA] Posicion: 1460 us | Modo: Alerta
```

## Conceptos demostrados

- `pthreads` con política de planificación `SCHED_FIFO`
- Timer de tiempo real con `setitimer` + `SIGALRM`
- Patrón productor-consumidor con flags volátiles
- Protección de estado compartido con `pthread_mutex`
- Ejecutivo cíclico sin `sleep()` usando `gpioTick()`