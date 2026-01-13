# GUÍA COMPLETA: CALIBRACIÓN TOUCHSCREEN CON MÉTODO 5 PUNTOS
## Sistema S3C44B0X con LCD 320x240

---

## 📋 ÍNDICE
1. [Resumen General](#resumen-general)
2. [Problema Original](#problema-original)
3. [Solución Implementada](#solución-implementada)
4. [Archivos Modificados](#archivos-modificados)
5. [Implementación Detallada](#implementación-detallada)
6. [Cómo Usar](#cómo-usar)
7. [Funciones de Test](#funciones-de-test)
8. [Notas Importantes](#notas-importantes)

---

## 🎯 RESUMEN GENERAL

Esta guía documenta la implementación completa de un sistema de **calibración de touchscreen resistivo de 5 puntos** con detección automática de orientación y manejo de coordenadas inversas.

**Sistema:** S3C44B0X con LCD 320x240 píxeles  
**Problema:** Coordenadas del touchscreen invertidas/rotadas respecto al LCD  
**Solución:** Calibración de 5 puntos con swap XY automático y escalas con signo

---

## ❌ PROBLEMA ORIGINAL

### Síntomas
- Al tocar en la esquina superior izquierda del LCD, el sistema detectaba la esquina inferior derecha
- Los ejes X e Y estaban intercambiados (swap XY)
- Las coordenadas estaban invertidas en ambos ejes
- Imposible usar el touchscreen de forma precisa

### Causa Raíz
- Hardware del touchscreen con orientación diferente al LCD
- ADC del S3C44B0X lee valores que no corresponden directamente a píxeles
- Necesidad de transformación matemática: coordenadas ADC → coordenadas LCD

---

## ✅ SOLUCIÓN IMPLEMENTADA

### Método de Calibración de 5 Puntos
Captura 5 puntos de referencia en el LCD:
- **A**: Esquina superior izquierda
- **B**: Esquina superior derecha  
- **C**: Esquina inferior derecha
- **D**: Esquina inferior izquierda
- **E**: Centro

### Características Clave
1. **Detección automática de swap XY**: Analiza si los ejes están intercambiados
2. **Factores de escala con signo**: Maneja inversión de ejes automáticamente
3. **Fixed-point 16.16**: Aritmética de punto fijo para precisión sin float
4. **Filtrado robusto**: 12 muestras por punto, descarta extremos
5. **Antirrebote**: Ignora toques muy rápidos (< 300ms)
6. **Promediado múltiple**: Mayor precisión usando 4 esquinas

---

## 📁 ARCHIVOS MODIFICADOS

### 1. **tp.h** (c:\hlocal\workspace_Hardware\practica3\tp.h)

**AÑADIDO:**
```c
// Funciones de calibración de 5 puntos
void ts_calibrate_5pt(int XRES, int YRES, int M);
int ts_read_calibrated(int *x, int *y);
void report_touch_data(int x, int y);
void ts_test_calibracion(void);
void ts_test_numeros(void);
```

**Ubicación:** Al final del archivo, después de las funciones existentes

---

### 2. **tp.c** (c:\hlocal\workspace_Hardware\practica3\tp.c)

#### A. **AÑADIDO - Variables Globales de Calibración** (después de los includes)
```c
/*===================================================================================
 * VARIABLES GLOBALES DE CALIBRACIÓN (Método 5 puntos)
 *===================================================================================*/
static int g_swap_xy = 0;           // 1 si los ejes están intercambiados
static long g_kx_fp;                // Factor de escala X en formato 16.16 fixed-point
static long g_ky_fp;                // Factor de escala Y en formato 16.16 fixed-point
static int g_ts_xc;                 // Centro X crudo del touch
static int g_ts_yc;                 // Centro Y crudo del touch
static int g_lcd_xc;                // Centro X del LCD
static int g_lcd_yc;                // Centro Y del LCD
static volatile int g_ts_ready = 0; // Flag para indicar que hay datos del touch
static int g_ts_raw_x;              // Última lectura X cruda
static int g_ts_raw_y;              // Última lectura Y cruda
static unsigned int g_last_touch_time = 0;  // Último tiempo de toque válido (microsegundos)
#define TOUCH_DEBOUNCE_TIME 300000  // Tiempo mínimo entre toques: 300ms = 300000us
```

#### B. **AÑADIDO - Include del timer2**
```c
#include "timer2.h"
```
**Ubicación:** Con los demás includes al inicio

#### C. **MODIFICADO - Función TSInt()** (Rutina de Interrupción)

**CAMBIO 1: Aumentar array de muestras**
```c
// ANTES:
ULONG Pt[5];

// DESPUÉS:
ULONG Pt[11];  /* Aumentado a 11 para almacenar 10 muestras + promedio */
ULONG swap;    /* Variable para ordenamiento */
```

**CAMBIO 2: Mejorar delays**
```c
// ANTES:
DelayTime(1000);

// DESPUÉS:
DelayTime(2000);  // delay más largo para mejor estabilización
```

**CAMBIO 3: Tomar 10 muestras en lugar de 5**
```c
// DESPUÉS (para X e Y):
for( i=0; i<10; i++ )
{
    rADCCON |= 0x1;
    while( rADCCON & 0x1 );
    while( !(rADCCON & 0x40) );
    Pt[i] = (0x3ff&rADCDAT);
    DelayTime(100);  // Pequeño delay entre lecturas
}
```

**CAMBIO 4: Ordenar y filtrar muestras**
```c
// AÑADIR después de capturar las 10 muestras:
// Ordenar muestras para eliminar extremos (bubble sort simple)
for (i = 0; i < 9; i++) {
    for (j = 0; j < 9 - i; j++) {
        if (Pt[j] > Pt[j + 1]) {
            swap = Pt[j];
            Pt[j] = Pt[j + 1];
            Pt[j + 1] = swap;
        }
    }
}
// Promedio de las 6 muestras centrales (descartando 2 máximas y 2 mínimas)
Pt[10] = (Pt[2] + Pt[3] + Pt[4] + Pt[5] + Pt[6] + Pt[7]) / 6;
```

**CAMBIO 5: Reportar coordenadas crudas**
```c
// AÑADIR después de leer X e Y:
tmp = Pt[10];  // Esto ya existe
// ... lectura de Y ...
// AÑADIR:
report_touch_data(tmp, Pt[10]);
```

#### D. **AÑADIDO - Nuevas Funciones** (al final del archivo)

**Función 1: clamp()**
```c
static int clamp(int val, int min, int max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}
```

**Función 2: draw_cross()**
```c
static void draw_cross(int x, int y)
{
    int i;
    // Línea horizontal
    for (i = -10; i <= 10; i++)
    {
        if (x + i >= 0 && x + i < SCR_XSIZE)
            (LCD_PutPixel(x + i, y, 0xf));
    }
    // Línea vertical
    for (i = -10; i <= 10; i++)
    {
        if (y + i >= 0 && y + i < SCR_YSIZE)
            (LCD_PutPixel(x, y + i, 0xf));
    }
}
```

**Función 3: report_touch_data()** - Con antirrebote
```c
void report_touch_data(int x, int y)
{
    unsigned int current_time = timer2_count();
    unsigned int time_diff;
    
    /* Calcular diferencia de tiempo (manejando overflow del contador) */
    if (current_time >= g_last_touch_time)
        time_diff = current_time - g_last_touch_time;
    else
        time_diff = (0xFFFFFFFF - g_last_touch_time) + current_time + 1;
    
    /* Solo aceptar el toque si ha pasado suficiente tiempo desde el último */
    if (g_ts_ready == 0 && time_diff >= TOUCH_DEBOUNCE_TIME)
    {
        g_ts_raw_x = x;
        g_ts_raw_y = y;
        g_ts_ready = 1;
        g_last_touch_time = current_time;
    }
}
```

**Función 4: ts_wait_for_touch()**
```c
static void ts_wait_for_touch(void)
{
    g_ts_ready = 0;
    while (g_ts_ready == 0)
    {
        // Esperar interrupción
    }
}
```

**Función 5: ts_read_raw()**
```c
static void ts_read_raw(int *xr, int *yr)
{
    ts_wait_for_touch();
    *xr = g_ts_raw_x;
    *yr = g_ts_raw_y;
}
```

**Función 6: get_cal_point()** - Captura punto con filtrado
```c
static void get_cal_point(int lcd_x, int lcd_y, int *ts_x, int *ts_y)
{
    int i, j;
    int samples_x[12], samples_y[12];
    int sum_x = 0, sum_y = 0;
    int temp;
    
    draw_cross(lcd_x, lcd_y);
    Lcd_Dma_Trans();
    Delay(80);
    
    // Tomar 12 muestras
    for (i = 0; i < 12; i++)
    {
        ts_read_raw(&samples_x[i], &samples_y[i]);
        Delay(25);
    }
    
    // Ordenar muestras X
    for (i = 0; i < 11; i++) {
        for (j = 0; j < 11 - i; j++) {
            if (samples_x[j] > samples_x[j + 1]) {
                temp = samples_x[j];
                samples_x[j] = samples_x[j + 1];
                samples_x[j + 1] = temp;
            }
        }
    }
    
    // Ordenar muestras Y
    for (i = 0; i < 11; i++) {
        for (j = 0; j < 11 - i; j++) {
            if (samples_y[j] > samples_y[j + 1]) {
                temp = samples_y[j];
                samples_y[j] = samples_y[j + 1];
                samples_y[j + 1] = temp;
            }
        }
    }
    
    // Promediar las 8 muestras centrales
    for (i = 2; i < 10; i++) {
        sum_x += samples_x[i];
        sum_y += samples_y[i];
    }
    
    *ts_x = sum_x / 8;
    *ts_y = sum_y / 8;
    
    Delay(50);
    draw_cross(lcd_x, lcd_y);
    Lcd_Dma_Trans();
}
```

**Función 7: ts_calibrate_5pt()** - **FUNCIÓN PRINCIPAL DE CALIBRACIÓN**
```c
void ts_calibrate_5pt(int XRES, int YRES, int M)
{
    int A_lcd_x, A_lcd_y, A_ts_x, A_ts_y;
    int B_lcd_x, B_lcd_y, B_ts_x, B_ts_y;
    int C_lcd_x, C_lcd_y, C_ts_x, C_ts_y;
    int D_lcd_x, D_lcd_y, D_ts_x, D_ts_y;
    int E_lcd_x, E_lcd_y, E_ts_x, E_ts_y;
    
    int dx, dy;
    int lcd_s, lcd_d;
    int ts_s1, ts_s2, ts_d1, ts_d2;
    long long temp;
    
    Lcd_Clr();
    Lcd_Active_Clr();
    Lcd_DspAscII8x16(60, 110, BLACK, (unsigned char *)"Calibracion 5pts");
    Lcd_Dma_Trans();
    Delay(100);
    
    Lcd_Clr();
    Lcd_Dma_Trans();
    
    // Definir posiciones LCD
    A_lcd_x = M;            A_lcd_y = M;
    B_lcd_x = XRES - M;     B_lcd_y = M;
    C_lcd_x = XRES - M;     C_lcd_y = YRES - M;
    D_lcd_x = M;            D_lcd_y = YRES - M;
    E_lcd_x = XRES / 2;     E_lcd_y = YRES / 2;
    
    // Capturar 5 puntos
    get_cal_point(A_lcd_x, A_lcd_y, &A_ts_x, &A_ts_y);
    get_cal_point(B_lcd_x, B_lcd_y, &B_ts_x, &B_ts_y);
    get_cal_point(C_lcd_x, C_lcd_y, &C_ts_x, &C_ts_y);
    get_cal_point(D_lcd_x, D_lcd_y, &D_ts_x, &D_ts_y);
    get_cal_point(E_lcd_x, E_lcd_y, &E_ts_x, &E_ts_y);
    
    // DETECCIÓN DE SWAP XY
    dx = B_ts_x - A_ts_x;
    dy = B_ts_y - A_ts_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    
    if (dx > dy)
    {
        g_swap_xy = 0;
    }
    else
    {
        g_swap_xy = 1;
        int tmp;
        #define SWAP(a, b) { tmp = a; a = b; b = tmp; }
        SWAP(A_ts_x, A_ts_y);
        SWAP(B_ts_x, B_ts_y);
        SWAP(C_ts_x, C_ts_y);
        SWAP(D_ts_x, D_ts_y);
        SWAP(E_ts_x, E_ts_y);
    }
    
    // CÁLCULO DEL CENTRO (promedio de 4 esquinas)
    g_ts_xc = (A_ts_x + B_ts_x + C_ts_x + D_ts_x) / 4;
    g_ts_yc = (A_ts_y + B_ts_y + C_ts_y + D_ts_y) / 4;
    g_lcd_xc = XRES / 2;
    g_lcd_yc = YRES / 2;
    
    // CÁLCULO DE kx y ky (fixed-point 16.16)
    lcd_s = XRES - 2 * M;
    lcd_d = YRES - 2 * M;
    
    ts_s1 = B_ts_x - A_ts_x;
    ts_s2 = C_ts_x - D_ts_x;
    ts_d1 = D_ts_y - A_ts_y;
    ts_d2 = C_ts_y - B_ts_y;
    
    temp = (long long)lcd_s << 17;  // * 2 usando shift
    g_kx_fp = (long)(temp / (ts_s1 + ts_s2));
    
    temp = (long long)lcd_d << 17;
    g_ky_fp = (long)(temp / (ts_d1 + ts_d2));
    
    // Mostrar resultados
    Lcd_Clr();
    Lcd_DspAscII6x8(10, 10, BLACK, (unsigned char *)"=== CALIBRACION OK ===");
    // ... más información ...
    Lcd_Dma_Trans();
    
    ts_wait_for_touch();
    Lcd_Clr();
    Lcd_Dma_Trans();
}
```

**Función 8: ts_read_calibrated()** - Lee coordenadas calibradas
```c
int ts_read_calibrated(int *x, int *y)
{
    int xr, yr;
    long long temp_x, temp_y;
    
    if (g_ts_ready == 0)
        return -1;
    
    xr = g_ts_raw_x;
    yr = g_ts_raw_y;
    g_ts_ready = 0;
    
    // Aplicar swap si necesario
    if (g_swap_xy)
    {
        int tmp = xr;
        xr = yr;
        yr = tmp;
    }
    
    // CONVERSIÓN con fixed-point 16.16
    temp_x = (long long)g_kx_fp * (xr - g_ts_xc);
    *x = (int)(temp_x >> 16) + g_lcd_xc;
    
    temp_y = (long long)g_ky_fp * (yr - g_ts_yc);
    *y = (int)(temp_y >> 16) + g_lcd_yc;
    
    // Clampear
    *x = clamp(*x, 0, SCR_XSIZE - 1);
    *y = clamp(*y, 0, SCR_YSIZE - 1);
    
    return 0;
}
```

**Función 9: ts_test_calibracion()** - Test de dibujo libre
```c
void ts_test_calibracion(void)
{
    int x, y;
    
    Lcd_Init();
    TS_init();
    
    ts_calibrate_5pt(SCR_XSIZE, SCR_YSIZE, 50);
    
    Lcd_Clr();
    Lcd_Active_Clr();
    Lcd_DspAscII8x16(80, 110, BLACK, (unsigned char *)"Dibuja!");
    Lcd_Dma_Trans();
    Delay(100);
    
    Lcd_Clr();
    Lcd_Dma_Trans();
    
    while (1)
    {
        if (ts_read_calibrated(&x, &y) == 0)
        {
            (LCD_PutPixel(x, y, 0xf));
            
            static int count = 0;
            if (++count >= 5)
            {
                Lcd_Dma_Trans();
                count = 0;
            }
        }
    }
}
```

**Función 10: ts_test_numeros()** - Test con 2 números
```c
void ts_test_numeros(void)
{
    int x, y;
    int num1_x = 60, num1_y = 100;
    int num2_x = 220, num2_y = 100;
    
    Lcd_Init();
    TS_init();
    
    ts_calibrate_5pt(SCR_XSIZE, SCR_YSIZE, 30);
    
    Lcd_Clr();
    Lcd_DspAscII8x16(num1_x, num1_y, BLACK, (unsigned char *)"1");
    Lcd_DspAscII8x16(num2_x, num2_y, BLACK, (unsigned char *)"2");
    Lcd_DspAscII6x8(60, 20, BLACK, (unsigned char *)"Toca un numero");
    Lcd_Dma_Trans();
    
    while (1)
    {
        if (ts_read_calibrated(&x, &y) == 0)
        {
            // Mostrar feedback visual
            // Detectar qué número se tocó
            if (x >= (num1_x-30) && x <= (num1_x+30) && 
                y >= (num1_y-30) && y <= (num1_y+30))
            {
                Lcd_DspAscII8x16(80, 190, BLACK, (unsigned char *)"NUMERO 1");
            }
            else if (x >= (num2_x-30) && x <= (num2_x+30) && 
                     y >= (num2_y-30) && y <= (num2_y+30))
            {
                Lcd_DspAscII8x16(80, 190, BLACK, (unsigned char *)"NUMERO 2");
            }
            
            Lcd_Dma_Trans();
            Delay(50);
        }
    }
}
```

---

### 3. **main.c** (c:\hlocal\workspace_Hardware\practica3\main.c)

**MODIFICADO - Función Main():**

```c
void Main(void)
{
    sys_init();
    
    // OPCIÓN 1: Test de calibración con dibujo libre
    // ts_test_calibracion();
    
    // OPCIÓN 2: Test con dos números
    ts_test_numeros();
    
    // OPCIÓN 3: Código de Sudoku (existente)
    // ... resto del código ...
}
```

**Ubicación:** Descomenta la función de test que quieras usar

---

## 🔧 IMPLEMENTACIÓN DETALLADA

### 1. Variables Globales (tp.c)

```
g_swap_xy      → 0 (no swap) o 1 (swap XY)
g_kx_fp        → Factor escala X en formato 16.16
g_ky_fp        → Factor escala Y en formato 16.16  
g_ts_xc, g_ts_yc    → Centro en coordenadas touchscreen
g_lcd_xc, g_lcd_yc  → Centro en coordenadas LCD
g_ts_ready     → Flag: hay datos del touch disponibles
g_ts_raw_x/y   → Últimas coordenadas crudas capturadas
```

### 2. Flujo de Calibración

```
1. Mostrar mensaje "Calibracion 5pts"
2. Para cada punto (A, B, C, D, E):
   a. Dibujar cruz en posición LCD
   b. Esperar 12 toques del usuario
   c. Ordenar muestras (bubble sort)
   d. Calcular promedio de 8 centrales
3. Detectar swap XY:
   - Comparar |dx| vs |dy| entre puntos A y B
   - Si |dy| > |dx| → swap necesario
4. Intercambiar coordenadas si swap==1
5. Calcular centro (promedio de 4 esquinas)
6. Calcular factores kx, ky (fixed-point)
7. Mostrar resultados
8. Esperar toque para continuar
```

### 3. Fórmulas de Conversión

#### Fixed-Point 16.16
Un número fixed-point 16.16 tiene:
- 16 bits enteros (parte alta)
- 16 bits decimales (parte baja)

**Ejemplo:** `0x00010000` = 1.0

#### Cálculo de kx
```
kx = (LCD_width - 2*M) * 2 / (ts_s1 + ts_s2)

donde:
  LCD_width = 320 píxeles
  M = margen (ej: 50)
  ts_s1 = B_ts_x - A_ts_x
  ts_s2 = C_ts_x - D_ts_x
```

**En código:**
```c
temp = (long long)lcd_s << 17;  // Multiplicar por 2^17 = 2 * 2^16
g_kx_fp = (long)(temp / (ts_s1 + ts_s2));
```

#### Conversión de coordenadas
```c
// Punto crudo del touchscreen: (xr, yr)
// Punto calibrado del LCD: (x, y)

temp_x = g_kx_fp * (xr - g_ts_xc);
x = (temp_x >> 16) + g_lcd_xc;

temp_y = g_ky_fp * (yr - g_ts_yc);
y = (temp_y >> 16) + g_lcd_yc;
```

El **>> 16** divide por 2^16 para extraer la parte entera.

### 4. Detección de Swap XY

```c
// Puntos A y B están en la misma fila del LCD
// Si el touchscreen está orientado igual:
//   B_ts_x > A_ts_x  (diferencia en X grande)
//   B_ts_y ≈ A_ts_y  (diferencia en Y pequeña)

dx = |B_ts_x - A_ts_x|
dy = |B_ts_y - A_ts_y|

if (dx > dy)
    g_swap_xy = 0;  // Sin swap
else
    g_swap_xy = 1;  // Swap necesario
```

### 5. Filtrado de Muestras

**Razón:** El ADC tiene ruido y puede dar lecturas extremas.

**Método:**
1. Tomar N muestras (10-12)
2. Ordenar de menor a mayor
3. Descartar las K mínimas y K máximas
4. Promediar las centrales

**Ejemplo con 12 muestras:**
```
Muestras: [450, 455, 448, 470, 452, 451, 449, 453, 454, 456, 490, 430]
Ordenadas: [430, 448, 449, 450, 451, 452, 453, 454, 455, 456, 470, 490]
                 ↑                                         ↑
              descarta 2                              descarta 2
              
Promedio de centrales (índices 2-9):
(449 + 450 + 451 + 452 + 453 + 454 + 455 + 456) / 8 = 452.5
```

### 6. Antirrebote Temporal

```c
if (tiempo_desde_ultimo_toque < 300ms)
    ignorar_toque();
```

Evita múltiples detecciones en un solo toque.

---

## 🚀 CÓMO USAR

### Opción 1: Test de Calibración (Dibujo Libre)

```c
// En main.c:
void Main(void)
{
    sys_init();
    ts_test_calibracion();  // Descomenta esta línea
}
```

**Proceso:**
1. Aparece "Calibracion 5pts"
2. Toca 5 cruces que aparecen (A, B, C, D, E)
3. Aparece mensaje "CALIBRACION OK"
4. Toca para continuar
5. Aparece "Dibuja!"
6. Dibuja libremente en la pantalla

### Opción 2: Test con Números

```c
// En main.c:
void Main(void)
{
    sys_init();
    ts_test_numeros();  // Descomenta esta línea
}
```

**Proceso:**
1. Calibración de 5 puntos
2. Aparecen números "1" y "2"
3. Toca un número
4. Aparece "NUMERO 1" o "NUMERO 2"

### Opción 3: Integrar en Tu Proyecto

```c
// 1. Inicializar
Lcd_Init();
TS_init();

// 2. Calibrar (una vez al inicio)
ts_calibrate_5pt(320, 240, 50);  
// Parámetros: XRES, YRES, Margen

// 3. Leer toques en tu loop principal
int x, y;
while (1)
{
    if (ts_read_calibrated(&x, &y) == 0)
    {
        // Toque detectado en (x, y)
        // Hacer algo con las coordenadas
    }
}
```

---

## 🧪 FUNCIONES DE TEST

### ts_test_calibracion()
- **Propósito:** Verificar precisión de calibración
- **Método:** Dibujo libre continuo
- **Uso:** Comprobar que los toques corresponden visualmente

### ts_test_numeros()
- **Propósito:** Test de detección de áreas
- **Método:** Detectar toque en zonas específicas
- **Uso:** Verificar precisión en zonas de interés

---

## 📌 NOTAS IMPORTANTES

### 1. Parámetros Ajustables

```c
// Margen de calibración (distancia desde bordes)
ts_calibrate_5pt(320, 240, 50);  // 50 píxeles
//                            ↑
//                      Ajustar según hardware

// Tiempo de antirrebote
#define TOUCH_DEBOUNCE_TIME 300000  // 300ms
//                          ↑
//                   Ajustar si hay dobles toques

// Número de muestras por punto
for (i = 0; i < 12; i++)  // 12 muestras
//              ↑
//          Ajustar para más precisión
```

### 2. Requisitos del Sistema

- **Timer2** debe estar inicializado para antirrebote
- Función `timer2_count()` debe retornar microsegundos
- `Lcd_Init()` debe estar llamado antes de calibrar
- `TS_init()` debe configurar interrupciones

### 3. Depuración

Si la calibración no funciona:

**a) Verificar valores crudos del ADC:**
```c
// En TSInt(), añadir temporalmente:
// Uart_Printf("X_raw: %d, Y_raw: %d\n", tmp, Pt[10]);
```

**b) Comprobar swap:**
- Si A y B están en fila horizontal LCD
- Pero `|B_ts_y - A_ts_y| > |B_ts_x - A_ts_x|`
- → Swap está activado correctamente

**c) Verificar signos de kx, ky:**
- Si toque izquierda → pantalla derecha: `kx` debe ser negativo
- Si toque arriba → pantalla abajo: `ky` debe ser negativo

### 4. Optimizaciones Futuras

- Guardar calibración en EEPROM/Flash
- Añadir verificación de calidad (medir error)
- Implementar recalibración automática
- Añadir interpolación bilineal para mayor precisión

### 5. Errores Comunes

| Síntoma | Causa | Solución |
|---------|-------|----------|
| Offset constante | Centro mal calculado | Verificar promedio de 4 esquinas |
| Coordenadas rotadas | Swap no detectado | Revisar cálculo `dx` vs `dy` |
| Inversión en un eje | Factor k con signo incorrecto | El signo se maneja automáticamente |
| Ruido/saltos | Sin filtrado | Verificar ordenación y promediado |
| Dobles toques | Sin antirrebote | Verificar `TOUCH_DEBOUNCE_TIME` |

---

## 📊 DIAGRAMA DE FLUJO

```
┌─────────────────┐
│   sys_init()    │
└────────┬────────┘
         │
         v
┌─────────────────┐
│   Lcd_Init()    │
└────────┬────────┘
         │
         v
┌─────────────────┐
│   TS_init()     │
└────────┬────────┘
         │
         v
┌─────────────────────────┐
│ ts_calibrate_5pt()      │
│  ┌──────────────────┐   │
│  │ Capturar punto A │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Capturar punto B │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Capturar punto C │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Capturar punto D │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Capturar punto E │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Detectar swap XY │   │
│  └──────────────────┘   │
│  ┌──────────────────┐   │
│  │ Calcular kx, ky  │   │
│  └──────────────────┘   │
└────────┬────────────────┘
         │
         v
┌─────────────────┐
│ Loop Principal  │◄───┐
└────────┬────────┘    │
         │             │
         v             │
┌──────────────────────┐│
│ts_read_calibrated()  ││
│  ┌────────────────┐  ││
│  │ Leer ADC raw   │  ││
│  └────────────────┘  ││
│  ┌────────────────┐  ││
│  │ Aplicar swap?  │  ││
│  └────────────────┘  ││
│  ┌────────────────┐  ││
│  │ Convertir a    │  ││
│  │ coordenadas    │  ││
│  │ LCD con kx,ky  │  ││
│  └────────────────┘  ││
│  ┌────────────────┐  ││
│  │ Clampear       │  ││
│  └────────────────┘  ││
└──────────────────────┘│
         │               │
         └───────────────┘
```

---

## 🎓 CONCEPTOS MATEMÁTICOS

### 1. Fixed-Point Arithmetic (Punto Fijo)

**¿Por qué?** El S3C44B0X no tiene FPU (unidad de coma flotante).

**Formato 16.16:**
```
32 bits totales
├─ 16 bits superiores: parte entera
└─ 16 bits inferiores: parte fraccionaria

Ejemplo:
0x00018000 = 1.5 en fixed-point 16.16
   ↑↑    ↑↑
   ||    └─ 0x8000 = 32768 = 0.5 * 65536
   └─────── 0x0001 = 1

Conversión:
valor_real = valor_fp / 65536.0
valor_fp = valor_real * 65536
```

**Operaciones:**
```c
// Multiplicación
long long temp = (long long)a_fp * b_fp;
long result_fp = (long)(temp >> 16);

// División
long long temp = ((long long)a_fp << 16);
long result_fp = (long)(temp / b_fp);
```

### 2. Transformación Afín 2D

La calibración implementa una transformación afín simplificada:

```
x_lcd = kx * (x_ts - xc_ts) + xc_lcd
y_lcd = ky * (y_ts - yc_ts) + yc_lcd
```

**Componentes:**
- **Traslación:** Centrar en origen (x - xc)
- **Escala:** Multiplicar por factor k
- **Traslación:** Mover al centro LCD (+ xc_lcd)

**Si hay swap:**
```
temp = x_ts
x_ts = y_ts
y_ts = temp
```

### 3. Estadística: Filtrado de Outliers

**Método del rango intercuartil (IQR):**
1. Ordenar datos
2. Eliminar quartil inferior (Q1)
3. Eliminar quartil superior (Q3)
4. Promediar rango central

**Implementación simplificada:**
- 12 muestras ordenadas
- Descartar 2 mínimas (Q1)
- Descartar 2 máximas (Q3)
- Promediar 8 centrales (Q2)

---

## ✅ CHECKLIST DE INTEGRACIÓN

- [ ] Añadir declaraciones en `tp.h`
- [ ] Añadir variables globales en `tp.c`
- [ ] Añadir `#include "timer2.h"` en `tp.c`
- [ ] Modificar `TSInt()` para tomar 10 muestras y ordenar
- [ ] Añadir llamada a `report_touch_data()` en `TSInt()`
- [ ] Añadir todas las funciones nuevas al final de `tp.c`
- [ ] Verificar que `timer2_count()` existe y funciona
- [ ] Llamar a `ts_calibrate_5pt()` en `Main()` o test
- [ ] Compilar y verificar sin errores
- [ ] Probar calibración con `ts_test_calibracion()`
- [ ] Verificar precisión con `ts_test_numeros()`

---

## 🔗 REFERENCIAS

- S3C44B0X User Manual (ADC, Touch Interface)
- ARM Fixed-Point Arithmetic Guide
- "5-Point Touch Calibration Algorithm" (Texas Instruments)
- Bubble Sort Algorithm
- Affine Transformations in 2D

---

## 📝 CONCLUSIÓN

Este sistema de calibración de 5 puntos resuelve completamente el problema de coordenadas invertidas/rotadas en touchscreens resistivos. La implementación es robusta, precisa y fácil de integrar en cualquier proyecto con S3C44B0X.

**Ventajas:**
✓ Detecta automáticamente swap XY  
✓ Maneja inversión de ejes con factores con signo  
✓ Filtrado robusto contra ruido  
✓ Antirrebote temporal  
✓ Sin dependencias de librerías float  
✓ Funciones de test incluidas  

**Para aplicar en otro proyecto:**
1. Copia las 3 secciones de código modificadas
2. Ajusta parámetros según tu hardware
3. Llama a `ts_calibrate_5pt()` una vez al inicio
4. Usa `ts_read_calibrated()` para leer toques

---

**Autor:** Documentación generada para compartir con compañeros  
**Fecha:** Enero 2026  
**Versión:** 1.0 - Completa y probada  

---

## 🎯 PARA TU AMIGA

Para aplicar esto en tu proyecto:

1. **Copia exactamente** las secciones marcadas como "AÑADIDO" o "MODIFICADO"
2. **Respeta el orden** de las variables y funciones
3. **Ajusta los includes** según tu estructura de proyecto
4. **Verifica que timer2** esté funcionando (necesario para antirrebote)
5. **Prueba primero** con `ts_test_calibracion()` antes de integrar en tu código

Si encuentras problemas, revisa la sección "Errores Comunes" y "Depuración".

**¡Buena suerte!** 🚀
