# U2_lab2 - Simulador de Sistema de Control de Vuelo

Laboratorio 2 de la Unidad 2 de Sistemas en Tiempo Real. Demuestra el comportamiento del planificador de Linux bajo distintas políticas de scheduling mediante tres hilos concurrentes que simulan tareas de un avión.

## Descripción

El programa lanza tres hilos que simulan tareas críticas de vuelo:

| Hilo | Tarea | Prioridad |
|---|---|---|
| Crítico | Control de estabilidad | 80 |
| Navegación | Procesamiento de ruta | 40 |
| Telemetría | Log a tierra (activado por timer) | 10 |

Cada hilo ejecuta trabajo pesado durante **10 segundos** y al finalizar se imprime una tabla comparativa de iteraciones completadas.

## Requisitos

- Linux (cualquier distribución)
- GCC compiler
- Permisos de root para `SCHED_FIFO` y prioridades RT

## Compilación

```bash
gcc -o simulador simulador.c -lpthread -lm
```

## Ejecución

```bash
# Caso A — sin prioridades RT (no requiere sudo)
./simulador

# Caso B y C — con SCHED_FIFO (requiere sudo)
sudo ./simulador
```

## Casos de Ensayo

El programa tiene un `#define` para cambiar entre políticas sin recompilar el comportamiento:

```c
// En simulador.c, línea 7 — alternar entre casos:
#define SCHEMA_EJECUCION SCHED_OTHER   // Caso A
#define SCHEMA_EJECUCION SCHED_FIFO    // Caso B y C
```

### Caso A — SCHED_OTHER (planificador por defecto)

Todos los hilos usan la política normal de Linux. El SO distribuye CPU equitativamente.

```bash
# Compilar con SCHEMA_EJECUCION = SCHED_OTHER
gcc -o simulador simulador.c -lpthread -lm
./simulador
```

**Resultados obtenidos:**
```
Hilo                 Prioridad    Iteraciones
Crítico              80           813.344.548
Navegación           40           796.511.168
Telemetría           10           20
```

**Observación:** Los hilos Crítico y Navegación obtienen cantidades similares de CPU. La Telemetría ejecuta exactamente 20 veces (una por cada disparo del timer de 500ms en 10 segundos).

---

### Caso B — SCHED_FIFO con prioridades RT

Los hilos usan planificación de tiempo real FIFO. Un hilo de mayor prioridad desaloja a los de menor prioridad.

```bash
# Compilar con SCHEMA_EJECUCION = SCHED_FIFO
gcc -o simulador simulador.c -lpthread -lm
sudo ./simulador
```

**Resultados obtenidos:**
```
Hilo                 Prioridad    Iteraciones
Crítico              80           724.138.771
Navegación           40           765.363.480
Telemetría           10           21
```

**Observación:** Los resultados son similares al Caso A porque la Raspberry Pi tiene 4 cores — cada hilo corre en un core distinto en paralelo real. El efecto de SCHED_FIFO sería dramático en un sistema de un solo core. La Telemetría sigue con ~20 iteraciones controladas por el timer.

---

### Caso C — Inversión de Prioridad

Se introduce un mutex compartido entre Crítico (p80) y Telemetría (p10). Telemetría toma el mutex primero y lo mantiene durante un trabajo largo, bloqueando al Crítico mientras Navegación corre libremente.

> **Importante:** Se deben descomentar las lineas dedicadas a este inciso correspondientes al uso del mutex

**Ejecución:**

```bash
sudo ./simulador
```

**Resultados obtenidos:**
```
Hilo                 Prioridad    Iteraciones
Crítico              80           165.273.573    ← cayó 77%
Navegación           40           1.624.707.849  ← subió 2x
Telemetría           10           20
```

**Observación:** Este es el bug de inversión de prioridad. El hilo de prioridad 40 ejecuta ~10 veces más iteraciones que el de prioridad 80. La secuencia del problema es:

```
1. Telemetría (p10) toma el mutex al recibir SIGALRM
2. Crítico (p80) intenta tomar el mutex → queda BLOQUEADO
3. Navegación (p40) no necesita el mutex → corre LIBREMENTE
4. Resultado: p40 adelanta a p80 mientras p10 tiene el recurso
```

---

## Resumen comparativo

| Caso | Política | Crítico | Navegación | Telemetría |
|---|---|---|---|---|
| A — Normal | SCHED_OTHER | 813M | 796M | 20 |
| B — Tiempo Real | SCHED_FIFO | 724M | 765M | 21 |
| C — Inversión | SCHED_FIFO + mutex | **165M** | **1.624M** | 20 |

## Conceptos demostrados

- `pthreads` con tres hilos concurrentes y distintas prioridades
- Política `SCHED_FIFO` vs `SCHED_OTHER`
- Timer periódico con `setitimer` + `sigwait` para activación exacta
- Inversión de prioridad con mutex compartido entre hilos de distinta prioridad
- Impacto del multi-core en el comportamiento del planificador RT