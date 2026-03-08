#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    int seg = 0;
    int minut = 0;
    int hor = 0;

    while (1)
    {
        seg++; // Incrementa los segundos en cada iteración del bucle

        if (seg == 60)
        {
            minut++;
            seg = 0; // Reinicia los segundos cada 60 segundos, incrementando los minutos
        }
        if (minut == 60)
        {
            hor++;
            seg = 0; // Reinicia los segundos cada 60 segundos, incrementando los minutos
            minut = 0; // Reinicia los minutos cada 60 minutos, incrementando las horas
        }


        printf("\r%02d:%02d:%02d", hor, minut, seg); // Imprime la hora actual en formato HH:MM:SS, actualizando cada segundo

        fflush(stdout); // Forzar que se muestre inmediatamente y pisando el valor anterior, simulando un reloj digital en la consola

        sleep(1); // Espera 1 segundo antes de actualizar la hora, creando el efecto de un reloj digital que se actualiza cada segundo
    }
    return 0;
}
