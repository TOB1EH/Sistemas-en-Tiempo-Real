#!/usr/bin/env python3
"""
Este script lee datos de aceleración filtrada del MPU6050 desde stdin en un thread separado
y los grafica en tiempo real usando Matplotlib. Se espera que los datos lleguen en formato CSV (x,y,z).
"""

import sys
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import threading
import queue

# Cantidad de muestras visibles en el gráfico
VENTANA = 200

# Cola thread-safe para comunicación entre thread de lectura y animación
datos_queue = queue.Queue(maxsize=10)

# Buffers circulares para cada eje
datos_x = deque([0.0] * VENTANA, maxlen=VENTANA)
datos_y = deque([0.0] * VENTANA, maxlen=VENTANA)
datos_z = deque([0.0] * VENTANA, maxlen=VENTANA)

# Configurar la figura
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 8))
fig.suptitle('MPU6050 — Aceleración filtrada en tiempo real (100 Hz)')

linea_x, = ax1.plot(datos_x, color='red', label='Eje X', linewidth=2)
linea_y, = ax2.plot(datos_y, color='green', label='Eje Y', linewidth=2)
linea_z, = ax3.plot(datos_z, color='blue', label='Eje Z', linewidth=2)

# Configurar límites, etiquetas, leyendas y cuadrícula para cada subplot
for ax, label in zip([ax1, ax2, ax3], ['X (g)', 'Y (g)', 'Z (g)']):
    ax.set_ylim(-2, 2)
    ax.set_ylabel(label, fontsize=11)
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)

ax3.set_xlabel('Muestras', fontsize=11)

def leer_datos_stdin():
    """
    Leer datos de stdin en formato CSV (x,y,z) y enviarlos a la cola para el thread de animación.
    Se ejecuta en un thread separado para no bloquear la animación. Si la cola está llena, se
    descartan los datos nuevos para evitar bloqueos. El thread se detiene limpiamente con Ctrl+C.
    """
    try:
        # Leer línea por línea desde stdin. Se espera que cada línea tenga el formato "x,y,z"
        # con valores flotantes.
        for linea in sys.stdin:
            linea = linea.strip()
            if linea:
                try:
                    x, y, z = map(float, linea.split(','))
                    # Enviar a la cola sin bloquear (descarta si está llena)
                    datos_queue.put_nowait((x, y, z))
                except (ValueError, queue.Full):
                    pass
    except KeyboardInterrupt:
        pass

def actualizar():
    """Actualizar gráfico 20 veces por segundo (50ms interval)"""
    # Procesar TODOS los datos disponibles en la cola
    while not datos_queue.empty():
        try:
            x, y, z = datos_queue.get_nowait()
            datos_x.append(x)
            datos_y.append(y)
            datos_z.append(z)
        except queue.Empty:
            break

    linea_x.set_ydata(datos_x)
    linea_y.set_ydata(datos_y)
    linea_z.set_ydata(datos_z)

    return linea_x, linea_y, linea_z

# Iniciar thread de lectura como daemon
thread_lectura = threading.Thread(target=leer_datos_stdin, daemon=True)
thread_lectura.start()

# interval=50 ms → actualizar a 20 Hz (suficiente para 100 Hz de datos del sensor)
ani = animation.FuncAnimation(fig, actualizar,
                               interval=50,
                               blit=False,
                               repeat=True)
plt.tight_layout()
plt.show()
