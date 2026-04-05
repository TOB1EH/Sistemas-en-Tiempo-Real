#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    char texto[1000];
    int contador = 0;
    char respuesta;
    char *palabra;

    do
    {
        contador = 0; // Reiniciar el contador para cada nueva frase

        printf("Ingrese un texto para contar la cantidad de palabras: ");
        fgets(texto, sizeof(texto), stdin);

        /*
        * Contar palabras: La función char palabra = strtok(texto, " \n");
        * toma el texto ingresado y lo divide en "tokens" (subcadenas) usando
        * los delimitadores espacio (" ") y salto de línea ("\n"). Es decir,
        * separa el texto en palabras, y la variable palabra apunta a la primera
        * palabra encontrada.
        * */
        palabra = strtok(texto, ". \n");

        /*
        * El ciclo while (palabra != NULL) se ejecuta mientras 
        * que haya palabras para contar. En cada iteración, se 
        * incrementa el contador y se obtiene la siguiente palabra 
        * usando strtok(NULL, " \n"), que continúa dividiendo el 
        * texto a partir de la última posición.
        */
        while (palabra != NULL)
        {
            contador++;
            // Obtener la siguiente palabra
            palabra = strtok(NULL, " \n");
        }

        printf("Número de palabras: %d\n", contador);

        printf("\n¿Desea contar otra frase? (s/n): ");
        scanf(" %c", &respuesta);

        // Limpiar el buffer de entrada para evitar problemas con fgets
        while (getchar() != '\n'); // Limpiar el buffer de entrada
        if (respuesta != 's' && respuesta != 'S') {
            break;
        }
    } while (respuesta == 's' || respuesta == 'S');
    return 0;
}
