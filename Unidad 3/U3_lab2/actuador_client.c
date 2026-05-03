#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/control_led.sock"
#define BUF_SIZE    128

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ON|OFF|STATUS>\n", argv[0]);
        return 1;
    }

    // Validar comando
    const char* cmd = argv[1];
    if (strcmp(cmd, "ON")     != 0 &&
        strcmp(cmd, "OFF")    != 0 &&
        strcmp(cmd, "STATUS") != 0) {
        fprintf(stderr, "Error: comando invalido. Use ON, OFF o STATUS\n");
        return 1;
    }

    // Crear socket
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    // Dirección del servidor
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Conectar
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect — ¿está corriendo el servidor?");
        close(fd);
        return 1;
    }

    // Enviar comando
    send(fd, cmd, strlen(cmd), 0);

    // Recibir respuesta
    char respuesta[BUF_SIZE] = {0};
    ssize_t n = recv(fd, respuesta, sizeof(respuesta) - 1, 0);
    if (n > 0) {
        printf("[RESPUESTA] %s", respuesta);
    }

    close(fd);
    return 0;
}