# U1_lab1 - Programas de Tiempo Real con Raspberry Pi

Laboratorio 1 de Sistemas en Tiempo Real usando GPIO y pigpio en Raspberry Pi.

## Requisitos

- Raspberry Pi (cualquier modelo)
- Librería **pigpio** instalada
- GCC compiler

Instalar pigpio:
```bash
sudo apt-get install pigpio
```

## Programas

### 1. blink_led.c
Parpadea un LED cada 500ms (0.5 segundos) sin usar interrupciones.

**Compilación:**
```bash
gcc -o blink_led blink_led.c -lpigpio -lm
```

**Ejecución:**
```bash
sudo ./blink_led
```

---

### 2. boton_con_interrupciones.c
Control de LED mediante un botón usando interrupciones por flanco (pull-up).

**Compilación:**
```bash
gcc -o boton_con_interrupciones boton_con_interrupciones.c -lpigpio -lm
```

**Ejecución:**
```bash
sudo ./boton_con_interrupciones
```

---

### 3. analisis_latencia_y_determinismo.c
Mide latencia y jitter en respuesta a presiones de botón. Realiza 20 mediciones y analiza determinismo.

**Compilación:**
```bash
gcc -o analisis_latencia_y_determinismo analisis_latencia_y_determinismo.c -lpigpio -lm
```

**Ejecución:**
```bash
sudo ./analisis_latencia_y_determinismo
```

---

## Pines GPIO Utilizados

| Componente | GPIO | Pin Físico |
|-----------|------|-----------|
| LED       | GPIO4 | 7         |
| Botón     | GPIO17 | 11        |

## Notas

- **Sudo requerido**: Todos los programas requieren permisos de root (`sudo`)
- **Flag `-lm`**: Enlaza la librería matemática (requerida por pigpio)
- **Flag `-lpigpio`**: Enlaza la librería pigpio
