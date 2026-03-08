#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    time_t t;              // Almacena el tiempo en segundos desde el 1 de enero de 1970 (epoch Unix)
    struct tm *tm_info;    // Puntero a una estructura que descompone el tiempo en horas, minutos, segundos, etc.

    while (1)
    {
        time(&t); // Obtiene el tiempo actual en segundos desde el epoch Unix y lo almacena en 't'
        tm_info = localtime(&t); // Convierte 't' a la hora local descompuesta (hora, minuto, segundo, etc.) y devuelve un puntero a esa estructura que se almacena en 'tm_info'

        // Imprime la hora actual en formato HH:MM:SS usando los campos de la estructura tm_info
        printf("\r%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

        fflush(stdout); // Forzar que se muestre inmediatamente y pisando el valor anterior, simulando un reloj digital en la consola

        sleep(1); // Espera 1 segundo antes de actualizar la hora, creando el efecto de un reloj digital que se actualiza cada segundo
    }

    return 0;
}
