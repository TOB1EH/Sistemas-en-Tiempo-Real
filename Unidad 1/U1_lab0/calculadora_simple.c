#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    float num1, num2, resultado;
    char operador;
    char respuesta;

    do
    {
        printf("\n==============================\n");
        printf("||    Calculadora Simple    ||\n");
        printf("==============================\n\n");
        printf("Ingrese el primer número: ");
        scanf("%f", &num1);
        printf("Ingrese el segundo número: ");
        scanf("%f", &num2);
        printf("Ingrese el operador (+, -, *, /): ");
        scanf(" %c", &operador);

        switch (operador)
        {    case '+':
            resultado = num1 + num2;
            printf("\nResultado: %.2f + %.2f = %.2f\n", num1, num2, resultado);
            break;
        case '-':
            resultado = num1 - num2;
            printf("\nResultado: %.2f - %.2f = %.2f\n", num1, num2, resultado);
            break;
        case '*':
            resultado = num1 * num2;
            printf("\nResultado: %.2f * %.2f = %.2f\n", num1, num2, resultado);
            break;
        case '/':
            if (num2 != 0) {
                resultado = num1 / num2;
                printf("\nResultado: %.2f / %.2f = %.2f\n", num1, num2, resultado);
            } else {
                printf("\nError: No se puede dividir por cero.\n");
            }
            break;
        default:
            printf("\nError: Operador no válido.\n");
            break;
        }
        printf("\n¿Desea realizar otra operación? (s/n): ");
        scanf(" %c", &respuesta);
        if (respuesta != 's' && respuesta != 'S') {
            break;
        }
    } while (respuesta == 's' || respuesta == 'S');

    return 0;
}
