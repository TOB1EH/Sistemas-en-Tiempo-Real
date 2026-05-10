# Compilación y prueba
```bash
# Compilar los dos programas por separado
gcc -o actuador_server actuador_server.c -lpthread
gcc -o actuador_client actuador_client.c
```

## Terminal 1 — arrancar el servidor
```bash
./actuador_server
```

## Terminal 2 — enviar comandos
```bash
./actuador_client ON
./actuador_client STATUS
./actuador_client OFF
./actuador_client STATUS
```

## Ver el log generado
```bash
cat /tmp/alarma.log
```

La salida esperada en la Terminal 2:
```bash
[RESPUESTA] LED_OK: ON
[RESPUESTA] ESTADO: ON | Ultimo cambio: 14:32:05
[RESPUESTA] LED_OK: OFF
[RESPUESTA] ESTADO: OFF | Ultimo cambio: 14:32:07
```

Y en /tmp/alarma.log:
```bash
[2026-05-03 14:32:05] ALARMA ACTIVADA
[2026-05-03 14:32:07] ALARMA DESACTIVADA
```

Para probar múltiples clientes simultáneos
```bash
# Lanzar 5 clientes al mismo tiempo
for i in 1 2 3 4 5; do
    ./actuador_client ON &
done
wait
./actuador_client STATUS
```

Esto verifica que el mutex funciona correctamente bajo concurrencia.