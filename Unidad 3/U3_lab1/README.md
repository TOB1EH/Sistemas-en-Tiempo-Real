# Procesamiento Concurrente de Señales Inerciales - U3_lab1

---

## Objetivo

Implementar un sistema de **adquisición, filtrado y visualización en tiempo real** de vibraciones/movimiento utilizando:
- Hilos concurrentes (pthreads)
- Colas de mensajes POSIX (mqueue)
- Tuberías de Linux (pipes)
- Sincronización con I2C

El sistema monitorea la **estabilidad de una plataforma** mediante un sensor **MPU6050** (acelerómetro de 3 ejes) y presenta los datos de forma fluida en una estación terrestre.

---

## Arquitectura del Sistema

```
┌─────────────────────────────────────────────────┐
│          SENSOR MPU6050 (I2C Bus)               │
│          Aceleración: X, Y, Z                   │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
        ┌──────────────────────┐
        │ Hilo Productor       │
        │ • Lee raw @ 100 Hz   │
        │ • I2C cada 10ms      │
        │ • mq_send()          │
        └──────────┬───────────┘
                   │ (mqueue)
                   ▼
        ┌──────────────────────┐
        │ Cola de Mensajes     │
        │ POSIX (mqueue)       │
        │ Max 10 mensajes      │
        └──────────┬───────────┘
                   │
                   ▼
        ┌──────────────────────┐
        │ Hilo Consumidor      │
        │ • Recibe datos       │
        │ • Filtra (media móvil)│
        │ • CSV a stdout       │
        └──────────┬───────────┘
                   │
          ┌────────┴────────┐
          ▼                 ▼
     [stdout pipe]   [visualización]
          │
          ▼
    ┌──────────────────────┐
    │ Python plotter       │
    │ • Matplotlib         │
    │ • 3 subplots (X,Y,Z) │
    │ • Tiempo real 100 Hz │
    └──────────────────────┘
```

---

## Componentes del Sistema

### **1. Hilo Productor (Adquisición Cruda)**

**Archivo**: `procesamiento_seniales.c` - `hilo_productor()`

**Función**: Leer datos **raw** del MPU6050 a través del bus I2C

**Características**:
- Frecuencia: **100 Hz** (cada 10 ms)
- Sincronización con `gpioTick()` (microsegundos)
- Lectura de 3 ejes: X, Y, Z en gravedad (g)
- Envío a cola mediante `mq_send()`
- Manejo de desbordamiento de cola (descarta muestra si cola llena)

**Protocolo I2C**:
```c
- Dispositivo: MPU6050 (dirección 0x68)
- Registro PWR_MGMT_1 (0x6B): Despertar sensor
- Registro ACCEL (0x3B): Leer 6 bytes (3 ejes × 2 bytes)
- Conversión: raw_value / 16384.0f = aceleración en g
```

### **2. Hilo Consumidor (Procesamiento y Despacho)**

**Archivo**: `procesamiento_seniales.c` - `hilo_consumidor()`

**Función**: Procesar datos crudos y enviar por stdout en formato CSV

**Características**:
- **Filtro de media móvil**: Ventana deslizante de 10 muestras por eje
- **Un filtro independiente** para cada eje (X, Y, Z)
- **Cola no bloqueante** para evitar bloqueos de CPU
- **Salida CSV**: `x_filtrado,y_filtrado,z_filtrado\n`
- **Stderr para logs**: Errores y mensajes van a stderr, no rompen pipe

**Algoritmo de Filtro**:
```c
buffer[indice] = nuevo_valor
suma += nuevo_valor - valor_viejo
promedio = suma / N_muestras
```

### **3. Visualización en Tiempo Real (Python)**

**Archivo**: `plotter_3ejes.py`

**Función**: Graficar datos filtrados en 3 subplots simultáneos

**Características**:
- **Matplotlib**: Gráficos interactivos
- **3 subplots**: Un gráfico por eje (X, Y, Z)
- **Buffer circular**: Últimas 200 muestras visibles
- **Actualización 100 Hz**: Sincronizada con sensor
- **Rango**: ±2g (rango nativo del MPU6050)
- **Entrada**: Pipe desde stdout del programa C

---

## Requisitos

### Hardware
- **Raspberry Pi** (o compatible con pigpio)
- **Sensor MPU6050** conectado a I2C (GPIO 2=SDA, GPIO 3=SCL)
- **Bus I2C habilitado** (`raspi-config` → Interface Options → I2C)

### Software

#### Linux
```bash
# Librerías de desarrollo
sudo apt-get update
sudo apt-get install -y python3-pip python3-matplotlib pigpio libpigpio-dev build-essential

# Python
pip3 install matplotlib
```

#### Verificar pigpio
```bash
# Iniciar daemon pigpio (requerido para funcionar)
sudo pigpiod

# O en una sesión: pigpiod -s 1 (para modo de debugging)
```

---

## Compilación y Ejecución

### **Paso 1: Compilar el programa C**

```bash
cd Unidad\ 3/
gcc -Wall -Wextra -std=c99 -O2 \
    -o procesamiento_seniales procesamiento_seniales.c \
    -lpigpio -lpthread -lrt -lm
```

### **Paso 2: Ejecutar con visualización en tiempo real**

```bash
# Terminal 1: Iniciar daemon pigpio
sudo pigpiod

# Terminal 2: Ejecutar con visualización
sudo ./procesamiento_seniales | python3 plotter_3ejes.py
```

### **Alternativa: Ejecución sin visualización**

```bash
# Solo ver datos en consola (CSV)
sudo ./procesamiento_seniales
```

**Salida esperada**:
```
0.0234,0.0156,-0.9823
0.0245,0.0167,-0.9834
0.0256,0.0178,-0.9845
...
```

---

## Ejemplo de Sesión

```bash
$ sudo ./procesamiento_seniales | python3 plotter_3ejes.py

# [Ventana Matplotlib se abre]
# [Tres gráficos con líneas rojas, verdes, azules]
# [Se actualizan en tiempo real 100 veces por segundo]

# Presionar Ctrl+C para detener
^C
# Limpieza automática de recursos (cola, I2C, GPIO)
```

---

## Sincronización y Seguridad

### **Variables Compartidas**

```c
volatile int running = 1;  /* Bandera SIGINT */
```

- **volatile**: Evita optimizaciones del compilador
- **SIGINT handler**: Permite salida limpia con Ctrl+C

### **Cola de Mensajes POSIX**

```c
struct mq_attr {
    .mq_maxmsg  = 10,                   /* Max 10 mensajes */
    .mq_msgsize = sizeof(Aceleracion),  /* ~12 bytes */
    .mq_flags   = 0                     /* Bloqueante en productor */
};
```

**Ventajas**:
- Sincronización automática entre hilos
- Capacidad limitada (evita memory bloat)
- Persistencia en `/dev/mqueue/` (para debugging)

### **Protección I2C**

El bus I2C es compartido. En futuras extensiones con múltiples sensores:
```c
pthread_mutex_t mutex_i2c;
/* Proteger mpu6050_leer() y mpu6050_init() */
```

---

## Especificaciones Técnicas

### MPU6050 (Acelerómetro de 3 Ejes)

| Parámetro | Valor |
|-----------|-------|
| Rango | ±2g (configurable) |
| Resolución | 16-bit signed |
| Escala | 1g = 16,384 LSB |
| Bus I2C | Dirección 0x68 |
| Frecuencia Max | 1000 Hz (limitado a 100 Hz en código) |

### Sincronización Temporal

```c
uint32_t ultimo_tick = gpioTick();  /* microsegundos */

if ((ahora - ultimo_tick) >= 10000) { /* 10ms = 10,000 µs */
    /* Leer sensor */
    ultimo_tick = ahora;
}
```

- **Precisión**: Microsegundos (µs)
- **Jitter**: Mínimo (<1ms típicamente)
- **Determinismo**: Mejor que sleep() + syscall overhead

### Filtro de Media Móvil

```
N_MUESTRAS = 10

Ejemplo:
  Entrada raw:    [1.0, 1.1, 0.9, 1.2, 0.8, 1.1, 0.9, 1.0, 1.2, 0.8]
  Promedio:       1.0 g (suavizado)
  
  Nueva muestra:  1.5 → [1.1, 0.9, 1.2, 0.8, 1.1, 0.9, 1.0, 1.2, 0.8, 1.5]
  Nuevo promedio: 1.05 g
```

**Beneficios**:
- Elimina ruido high-frequency
- Mantiene cambios lógicos (bajo lag)
- Implementación O(1) con buffer circular

---

## Debugging

### Ver cola en tiempo real
```bash
# Terminal nueva
ls -la /dev/mqueue/
cat /dev/mqueue/imu_queue  # Ver atributos de cola
```

### Monitoreo de procesos
```bash
# En otra terminal
ps aux | grep procesamiento_seniales
lsof -p <PID>  # Ver recursos abiertos

# Ver threads
ps -eLf | grep procesamiento
```

### Logs a stderr
```bash
# Capturar solo stderr para diagnosticar
sudo ./procesamiento_seniales 2> errores.log | python3 plotter_3ejes.py
```

---

## Requerimiento Opcional: Control de Servo (SG90)

**Funcionalidad**: Un tercer hilo actuaría el eje X del sensor replicando movimiento

**Pseudocódigo**:
```c
void* hilo_actuador(void* arg) {
    while (running) {
        pthread_mutex_lock(&mutex_eje_x);
        float angulo = eje_x_filtrado;  /* Leer eje X */
        pthread_mutex_unlock(&mutex_eje_x);
        
        /* Convertir aceleración a ángulo */
        int pulso = 1500 + (angulo * 100);  /* 1000-2000 µs */
        gpioServo(GPIO_PIN, pulso);
        
        usleep(10000);  /* 100 Hz */
    }
}
```

**Estado**: No implementado en esta versión (requerimiento opcional)

---

## Formato de Datos

### CSV (stdout)
```
x_filtrado,y_filtrado,z_filtrado
-0.0234,0.0156,0.9823
-0.0245,0.0167,0.9834
-0.0256,0.0178,0.9845
```

**Rango esperado**: ±2.0g  
**Precisión**: 4 decimales  
**Frecuencia**: 100 Hz (10ms/línea)

---

## 🧹 Limpieza de Recursos

El programa limpia automáticamente:

```c
mq_close(cola);           /* Cerrar descriptor */
mq_unlink(NOMBRE_COLA);   /* Eliminar cola del sistema */
i2cClose(handle);         /* Cerrar I2C */
gpioTerminate();          /* Terminar pigpio */
pthread_join(...);        /* Esperar hilos */
```

**Manual** (si quedó basura):
```bash
# Limpiar cola remanente
rm /dev/mqueue/imu_queue

# Terminar daemon pigpio
sudo killall pigpiod
```

---

## Problemas Comunes

| Problema | Causa | Solución |
|----------|-------|----------|
| "Error al abrir I2C" | Bus I2C no habilitado | `sudo raspi-config` → Interface Options → I2C |
| "Error al crear cola" | Permisos insuficientes | Ejecutar con `sudo` |
| "Error al inicializar pigpio" | Daemon no corriendo | `sudo pigpiod` en otra terminal |
| Gráfico no se actualiza | stdin bloqueado | Verificar pipe con `\|` |
| Datos irregulares | Ruido en sensor | Aumentar N_MUESTRAS en filtro |

---

## Conceptos Clave

### Hilos (pthreads)
- **Productor**: Lectura I2C (10ms crítico)
- **Consumidor**: Procesamiento (sin timing fijo)
- **Sincronización**: Cola de mensajes (no spin-wait)

### Colas POSIX
- **Productor → Cola**: `mq_send()` (bloqueante si llena)
- **Cola → Consumidor**: `mq_receive()` (no bloqueante con O_NONBLOCK)
- **Ventaja**: Desacoplamiento de frecuencias (100Hz productor → variable consumidor)

### Pipes
- **Productor**: stdout del programa C
- **Consumidor**: stdin del script Python
- **Ventaja**: Simplicidad, testing independiente

---

## Referencias

- **POSIX Signals & Threads**: `man pthread`, `man mq_open`
- **Pigpio Library**: http://abyz.me.uk/rpi/pigpio/
- **MPU6050 Datasheet**: `MPU-6000.PDF` (en directorio)
- **Matplotlib Animation**: https://matplotlib.org/stable/api/animation_api.html

---

## 👨Autor

**Realizado por**: Tobias Funes  
**Materia**: Sistemas de Tiempo Real  
**Fecha**: Abril 2026
