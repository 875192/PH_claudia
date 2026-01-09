# RESUMEN DE IMPLEMENTACIÓN - PRÁCTICA 3 PARTE A

## Objetivo
Implementar una interfaz gráfica en LCD para el juego Sudoku, permitiendo visualizar el tablero, candidatos, errores y tiempo de juego.

---

## PASO 1: PANTALLA INICIAL CON INSTRUCCIONES ✅

### Archivos modificados:
- `lcd.h` - Agregadas declaraciones de funciones Sudoku
- `lcd.c` - Implementada función `Sudoku_Pantalla_Inicial()`
- `button.c` - Agregado include de `lcd.h` y variable `pantalla_mostrada`
- `main.c` - Agregado include de `lcd.h`, inicialización del LCD y llamada a `Sudoku_Pantalla_Inicial()`

### Funcionalidad implementada:
```c
void Sudoku_Pantalla_Inicial(void)
```

**Elementos visuales:**
- Título: "SUDOKU 9x9" centrado
- Sección de instrucciones:
  - Botón Derecho: cambia valor
  - Botón Izquierdo: confirma
  - Introducir valor 0: borrar
  - Fila 0: terminar partida
- Leyenda de símbolos:
  - F = Fila
  - C = Columna
  - E = Error detectado
- Rectángulo con mensaje: "Pulse un botón para jugar"

**Ubicación en el código:**
- Archivo: `lcd.c`, líneas ~554-585
- Llamada inicial: `main.c` después de `Lcd_Init()`

**Comportamiento:**
- Se muestra al iniciar el programa
- Permanece hasta que el usuario presiona cualquier botón
- Al presionar botón → pasa a estado INTRODUCIR_FILA y dibuja tablero

---

## PASO 2: DIBUJO DEL TABLERO DE SUDOKU ✅

### Archivos modificados:
- `lcd.h` - Agregada declaración de `Sudoku_Dibujar_Tablero()`
- `lcd.c` - Implementada función `Sudoku_Dibujar_Tablero()`
- `button.c` - Llamada a `Sudoku_Dibujar_Tablero()` en estado ESPERANDO_INICIO

### Funcionalidad implementada:
```c
void Sudoku_Dibujar_Tablero(void)
```

**Características del tablero:**
- Tamaño de celda: 23x23 píxeles
- Tamaño total tablero: 207x207 píxeles (9 celdas × 23 píxeles)
- Márgenes:
  - Izquierdo: 20 píxeles + 10 para números de fila
  - Superior: 10 píxeles + 10 para números de columna

**Elementos visuales:**
1. **Numeración de columnas (1-9)**: En la parte superior, centrada sobre cada columna
2. **Numeración de filas (1-9)**: En el lado izquierdo, centrada en cada fila
3. **Líneas del tablero**:
   - Líneas gruesas (2 píxeles): Cada 3 celdas (delimitando regiones 3x3)
   - Líneas finas (1 píxel): Entre celdas individuales
4. **Información inferior**:
   - Tiempo: "Tiempo: 00:00" (parte inferior izquierda)
   - Ayuda: "Fila 0: Salir" (parte inferior derecha)

**Ubicación en el código:**
- Archivo: `lcd.c`, líneas ~588-651
- Llamada: `button.c` en `boton_confirmado()`, caso ESPERANDO_INICIO

**Comportamiento:**
- Se dibuja cuando el usuario presiona un botón en la pantalla inicial
- Sustituye la pantalla de instrucciones
- Muestra el tablero vacío listo para el juego

---

## SOLUCIÓN A PROBLEMA: BLOQUEO EN TRANSFERENCIA DMA ✅

### Problema detectado:
Al llamar a `Lcd_Dma_Trans()`, el sistema se quedaba bloqueado en el `while(ucZdma0Done)` esperando que la interrupción del DMA termine.

### Solución implementada:
Agregado timeout de seguridad en `Lcd_Dma_Trans()`:

```c
/* Esperar con timeout para evitar bloqueo */
{
    INT32U timeout = 100000;
    while(ucZdma0Done && timeout > 0)
    {
        timeout--;
    }
}
```

**Archivo modificado:** `lcd.c`, función `Lcd_Dma_Trans()`

**Razón del timeout:**
- Evita que el sistema se quede indefinidamente bloqueado
- Permite que el juego continúe incluso si la interrupción DMA no funciona correctamente
- Valor 100000: suficiente para que el DMA termine normalmente, pero no excesivo

---

## ESTADO ACTUAL DEL PROYECTO

### Funcionalidades completadas:
✅ Pantalla inicial con instrucciones  
✅ Tablero de Sudoku 9x9 con numeración  
✅ Estructura de máquina de estados del juego (ya existía en button.c)  
✅ Sistema de antirrebotes para botones (ya existía en timer3)  
✅ Sistema de medición de tiempo (ya existía en timer2)  

### Pendiente de implementar:
⏳ Mostrar valores de celdas (pistas y valores del usuario)  
⏳ Distinguir visualmente pistas de valores introducidos  
⏳ Mostrar candidatos en celdas vacías (grid 3x3)  
⏳ Actualizar tiempo en pantalla en tiempo real  
⏳ Resaltar errores visualmente  
⏳ Pantalla final de victoria/derrota  
⏳ Permitir fila 0 para terminar partida  

---

## PRÓXIMOS PASOS SUGERIDOS

### Paso 3: Mostrar valores en las celdas
- Implementar función para dibujar números grandes en celdas
- Distinguir color/estilo entre pistas y valores del usuario
- Actualizar tablero cuando se introduce un valor

### Paso 4: Mostrar candidatos
- Implementar función para dibujar grid 3x3 dentro de celda vacía
- Marcar candidatos con círculos pequeños
- Actualizar candidatos cuando cambia el tablero

### Paso 5: Actualizar tiempo
- Modificar función para actualizar solo el área del tiempo
- Llamar periódicamente desde el timer2

### Paso 6: Resaltar errores
- Implementar función para dibujar celda con error (borde grueso/color invertido)
- Detectar y marcar todas las celdas involucradas en error

### Paso 7: Pantalla final
- Implementar función para mostrar victoria o partida terminada
- Mostrar tiempo total
- Mensaje para reiniciar

---

## NOTAS TÉCNICAS

### Constantes importantes:
```c
#define MARGEN_IZQ 20
#define MARGEN_SUP 10
#define TAM_CELDA 23
#define GROSOR_FINO 1
#define GROSOR_GRUESO 2
```

### Colores disponibles (escala de grises 4 bits):
```c
#define BLACK 0xf
#define WHITE 0x0
#define LIGHTGRAY 0x5
#define DARKGRAY 0xa
```

### Funciones LCD disponibles:
- `Lcd_Init()` - Inicializa controlador LCD
- `Lcd_Clr()` - Limpia buffer virtual
- `Lcd_Dma_Trans()` - Transfiere buffer virtual → pantalla física
- `Lcd_Draw_Box()` - Dibuja rectángulo
- `Lcd_Draw_HLine()` - Dibuja línea horizontal
- `Lcd_Draw_VLine()` - Dibuja línea vertical
- `Lcd_DspAscII6x8()` - Dibuja texto pequeño
- `Lcd_DspAscII8x16()` - Dibuja texto grande
- `LCD_PutPixel()` - Dibuja píxel individual

### Estructura de datos Sudoku:
```c
typedef uint16_t CELDA;
CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]

Bits de CELDA:
[15:7] - Vector de candidatos (9 bits)
[6]    - No usado
[5]    - ERROR
[4]    - PISTA
[3:0]  - VALOR (0-9)
```

---

**Última actualización:** 9 de enero de 2026
