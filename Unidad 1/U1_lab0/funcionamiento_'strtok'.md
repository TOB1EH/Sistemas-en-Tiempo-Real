# Contador de palabras usando `strtok()`

## Cómo funciona `strtok` internamente

`strtok` mantiene un **puntero interno estático** que recuerda dónde se quedó la última vez. Por eso:

- **Primera llamada** → `strtok(texto, " \n")` — le pasás el texto. Busca el primer token y guarda internamente la posición donde terminó.
- **Siguientes llamadas** → `strtok(NULL, " \n")` — le pasás `NULL` para decirle "seguí desde donde te quedaste". Busca el siguiente token a partir de esa posición guardada.

---

### Ejemplo concreto

Supongamos que el texto es `"Hola mundo cruel\n"`:

    Memoria: ['H','o','l','a',' ','m','u','n','d','o',' ','c','r','u','e','l','\n','\0']


**Llamada 1:** `strtok(texto, " \n")`

- Encuentra "Hola", reemplaza el espacio por '\0'
- Retorna un puntero al inicio: "Hola"
- Internamente guarda la posición después del '\0' (donde empieza "mundo")

**Llamada 2:** `strtok(NULL, " \n")`

- Continúa desde donde se quedó
- Encuentra "mundo", reemplaza el espacio por '\0'
- Retorna puntero a: "mundo"
- Guarda la posición después (donde empieza "cruel")

**Llamada 3:** `strtok(NULL, " \n")`

- Continúa desde donde se quedó
- Encuentra "cruel", reemplaza '\n' por '\0'
- Retorna puntero a: "cruel"

**Llamada 4:** `strtok(NULL, " \n")`

- No hay más tokens
- Retorna NULL → el while termina

---

### ¿Qué es `palabra` entonces?

`palabra` **no almacena una copia** de la palabra. Es un **puntero** que apunta a una posición dentro del array `texto` original. `strtok` modifica `texto` reemplazando los delimitadores por `\0` (fin de cadena), así cada porción se lee como una cadena independiente.

```
Antes:  "Hola mundo cruel\n"
Después: "Hola\0mundo\0cruel\0"
              ↑      ↑      ↑
          llamada1  llamada2  llamada3
          palabra   palabra   palabra
          apunta    apunta    apunta
          aquí      aquí      aquí
```

---

### Resumen

| Concepto | Detalle |
|----------|---------|
| `strtok(texto, delim)` | Primera llamada: empieza desde el inicio de `texto` |
| `strtok(NULL, delim)` | Siguientes: continúa desde la posición interna guardada |
| `palabra` | Es un puntero que apunta **dentro** de `texto`, no una copia |
| `strtok` modifica `texto` | Reemplaza delimitadores por `\0` para "cortar" las palabras |
| Retorna `NULL` | Cuando ya no quedan más tokens |

