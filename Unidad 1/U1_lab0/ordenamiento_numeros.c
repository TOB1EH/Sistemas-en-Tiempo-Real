#include <stdio.h>

int main(int argc, char const *argv[])
{
    int numeros[100]; // Arreglo para almacenar hasta 100 números
    int n;            // Cantidad de números a ingresar
    int opcion;       // 1 = ascendente, 2 = descendente
    int temp;         // Variable auxiliar para el intercambio

    // Solicitar la cantidad de números
    printf("¿Cuántos números desea ingresar? ");
    scanf("%d", &n);

    // Ingresar los números
    for (int i = 0; i < n; i++)
    {
        printf("Número %d: ", i + 1);
        scanf("%d", &numeros[i]);
    }

    // Elegir tipo de ordenamiento
    printf("\n¿Cómo desea ordenarlos?\n");
    printf("1. Ascendente\n");
    printf("2. Descendente\n");
    printf("Opción: ");
    scanf("%d", &opcion);

    // Ordenamiento burbuja: compara pares adyacentes y los intercambia
    // si están en el orden incorrecto. Se repite hasta que todo esté ordenado.
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - 1 - i; j++)
        {
            // Condición de intercambio según la opción elegida
            // Ascendente: si el actual es mayor que el siguiente, intercambiar
            // Descendente: si el actual es menor que el siguiente, intercambiar
            if ((opcion == 1 && numeros[j] > numeros[j + 1]) ||
                (opcion == 2 && numeros[j] < numeros[j + 1]))
            {
                // Intercambio usando variable auxiliar
                temp = numeros[j];
                numeros[j] = numeros[j + 1];
                numeros[j + 1] = temp;
            }
        }
    }

    // Mostrar el resultado
    printf("\nNúmeros ordenados: ");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", numeros[i]);
    }
    printf("\n");

    return 0;
}
