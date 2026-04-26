import sys
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Cantidad de muestras visibles en el gráfico
VENTANA = 200

# Buffers circulares para cada eje
datos_x = deque([0.0] * VENTANA, maxlen=VENTANA)
datos_y = deque([0.0] * VENTANA, maxlen=VENTANA)
datos_z = deque([0.0] * VENTANA, maxlen=VENTANA)

# Configurar la figura
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 6))
fig.suptitle('MPU6050 — Aceleración filtrada en tiempo real')

linea_x, = ax1.plot(datos_x, color='red',   label='Eje X')
linea_y, = ax2.plot(datos_y, color='green',  label='Eje Y')
linea_z, = ax3.plot(datos_z, color='blue',   label='Eje Z')

for ax, label in zip([ax1, ax2, ax3], ['X (g)', 'Y (g)', 'Z (g)']):
    ax.set_ylim(-2, 2)
    ax.set_ylabel(label)
    ax.legend(loc='upper right')
    ax.grid(True)

ax3.set_xlabel('Muestras')

def actualizar(frame):
    # Leer una línea de stdin (viene del pipe)
    linea = sys.stdin.readline().strip()
    if not linea:
        return linea_x, linea_y, linea_z

    try:
        x, y, z = map(float, linea.split(','))
        datos_x.append(x)
        datos_y.append(y)
        datos_z.append(z)
    except ValueError:
        pass  # ignorar líneas malformadas

    linea_x.set_ydata(datos_x)
    linea_y.set_ydata(datos_y)
    linea_z.set_ydata(datos_z)

    return linea_x, linea_y, linea_z

# interval=10 ms → intenta actualizar a 100 Hz
ani = animation.FuncAnimation(fig, actualizar,
                               interval=10,
                               blit=True,
                               cache_frame_data=False)
plt.tight_layout()
plt.show()