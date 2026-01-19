# GUÍA DE USO: CALIBRACIÓN TOUCHSCREEN 5 PUNTOS

## 📋 RESUMEN

Se ha implementado exitosamente la calibración de 5 puntos del touchscreen basada en la implementación de Jaime, adaptada a tu proyecto. El sistema ahora incluye:

- **Calibración de 5 puntos**: Máxima precisión capturando esquinas y centro
- **Detección automática de swap XY**: Maneja ejes intercambiados
- **Factores de escala con signo**: Soporta inversión de ejes
- **Filtrado robusto**: 10 muestras por toque con eliminación de extremos
- **Antirrebote temporal**: 300ms de tiempo mínimo entre toques
- **Aritmética fixed-point 16.16**: Precisión sin punto flotante

---

## 📁 ARCHIVOS MODIFICADOS

### 1. `tp.h`
✅ **Añadidas** las siguientes declaraciones de funciones:
```c
void ts_calibrate_5pt(int XRES, int YRES, int M);
int ts_read_calibrated(int *x, int *y);
void report_touch_data(int x, int y);
void ts_test_calibracion(void);
void ts_test_numeros(void);
```

### 2. `tp.c`

#### Variables globales añadidas:
```c
static int g_swap_xy = 0;           // 1 si los ejes están intercambiados
static long g_kx_fp;                // Factor de escala X (16.16 fixed-point)
static long g_ky_fp;                // Factor de escala Y (16.16 fixed-point)
static int g_ts_xc;                 // Centro X crudo del touch
static int g_ts_yc;                 // Centro Y crudo del touch
static int g_lcd_xc;                // Centro X del LCD
static int g_lcd_yc;                // Centro Y del LCD
static volatile int g_ts_ready;     // Flag de datos listos
static int g_ts_raw_x;              // Última lectura X cruda
static int g_ts_raw_y;              // Última lectura Y cruda
static unsigned int g_last_touch_time = 0;
#define TOUCH_DEBOUNCE_TIME 300000  // 300ms en microsegundos
```

#### Función `TSInt()` mejorada:
- ✅ Array de 11 muestras (almacena 10 + promedio)
- ✅ Delays aumentados (2000 μs para estabilización)
- ✅ Toma 10 muestras por eje
- ✅ Ordenamiento bubble sort
- ✅ Promedio de 6 muestras centrales (descarta 2 máximas y 2 mínimas)
- ✅ Llamada a `report_touch_data()` para calibración

#### Funciones nuevas implementadas:

1. **`clamp()`** - Limita valores entre mínimo y máximo
2. **`draw_cross()`** - Dibuja cruz de calibración en LCD
3. **`report_touch_data()`** - Reporta coordenadas con antirrebote
4. **`ts_wait_for_touch()`** - Espera bloqueante de toque
5. **`ts_read_raw()`** - Lee coordenadas ADC sin calibrar
6. **`get_cal_point()`** - Captura punto con 12 muestras filtradas
7. **`ts_calibrate_5pt()`** - **FUNCIÓN PRINCIPAL DE CALIBRACIÓN**
8. **`ts_read_calibrated()`** - Lee coordenadas calibradas
9. **`ts_test_calibracion()`** - Test de dibujo libre
10. **`ts_test_numeros()`** - Test con 2 números

---

## 🚀 CÓMO USAR

### Opción 1: Test de Dibujo Libre

Añade en tu `main.c`:

```c
#include "tp.h"

void Main(void)
{
    sys_init();
    
    // Llamar directamente al test de calibración
    ts_test_calibracion();
    
    // Este test hace:
    // 1. Inicializa LCD y touchscreen
    // 2. Ejecuta calibración de 5 puntos (margen 50px)
    // 3. Permite dibujar libremente en la pantalla
    
    // NUNCA SALE DEL BUCLE - solo para pruebas
}
```

### Opción 2: Test de Números

```c
void Main(void)
{
    sys_init();
    
    // Test con dos números en pantalla
    ts_test_numeros();
    
    // Este test hace:
    // 1. Calibra con margen de 30 píxeles
    // 2. Muestra números "1" y "2" en pantalla
    // 3. Detecta qué número tocas
    // 4. Muestra las coordenadas del toque
}
```

### Opción 3: Integración en tu Aplicación

```c
void Main(void)
{
    int x, y;
    
    sys_init();
    Lcd_Init();
    TS_init();
    timer2_init();
    timer2_start();
    
    // 1. CALIBRAR AL INICIO (solo una vez) - margen 50px para mejor precisión
    ts_calibrate_5pt(SCR_XSIZE, SCR_YSIZE, 50);
    
    // 2. Dibujar tu interfaz
    Lcd_Clr();
    // ... tu código de dibujo ...
    Lcd_Dma_Trans();
    
    // 3. USAR EN EL BUCLE PRINCIPAL
    while (1)
    {
        // Leer coordenadas calibradas (no bloqueante)
        if (ts_read_calibrated(&x, &y) == 0)
        {
            // ¡Toque detectado! x, y contienen coordenadas calibradas
            
            // Ejemplo: detectar si se tocó un botón
            if (x >= 100 && x <= 200 && y >= 50 && y <= 100)
            {
                // Se tocó el área del botón
                procesar_boton();
            }
            
            // O dibujar donde se toca
            LCD_PutPixel(x, y, 0xf);
            Lcd_Dma_Trans();
        }
        
        // ... resto de tu código ...
    }
}
```

---

## 🔧 PARÁMETROS DE CALIBRACIÓN

### `ts_calibrate_5pt(XRES, YRES, M)`

- **XRES**: Resolución horizontal del LCD (320 para MLCD_320_240)
- **YRES**: Resolución vertical del LCD (240 para MLCD_320_240)
- **M**: Margen desde los bordes (en píxeles)
  - **50**: Margen amplio, más fácil de tocar (recomendado para calibración inicial)
  - **30**: Margen medio, buen balance
  - **20**: Margen pequeño, máxima cobertura de pantalla

**Recomendación**: Usa `M=50` para calibrar, y luego `M=30` para uso normal si necesitas re-calibrar.

---

## 📊 SECUENCIA DE CALIBRACIÓN

Cuando llames a `ts_calibrate_5pt()`, el sistema:

1. **Muestra mensaje**: "Calibracion 5pts"
2. **Punto A** (esquina superior izquierda): "Toca punto A"
   - Espera 12 toques en la cruz
3. **Punto B** (esquina superior derecha): "Toca punto B"
4. **Punto C** (esquina inferior derecha): "Toca punto C"
5. **Punto D** (esquina inferior izquierda): "Toca punto D"
6. **Punto E** (centro): "Toca centro E"
7. **Cálculo automático**:
   - Detecta si X/Y están intercambiados (swap)
   - Calcula factores de escala kx, ky
   - Guarda configuración en variables globales
8. **Muestra resultado**:
   - "CALIBRACION OK"
   - "Swap XY: SI/NO"
   - "kx: POSITIVO/NEGATIVO"
   - "ky: POSITIVO/NEGATIVO"
9. **Espera toque** para continuar

---

## 💡 CONSEJOS IMPORTANTES

### ✅ Durante la Calibración:
- **Mantén el stylus presionado** en cada cruz hasta que desaparezca (~3 segundos)
- **Toca el CENTRO de cada cruz** lo más preciso posible
- **No toques rápidamente** - el sistema toma 12 muestras por punto
- Si ves que una cruz desaparece muy rápido, puede que necesites ajustar el delay

### ✅ Después de Calibrar:
- La calibración se mantiene **mientras no se reinicie el programa**
- Llama a `ts_calibrate_5pt()` solo **UNA VEZ** al inicio
- Usa `ts_read_calibrated()` tantas veces como quieras después

### ✅ Si la calibración no funciona bien:
1. Verifica que `timer2_init()` y `timer2_start()` se llamen antes de calibrar
2. Aumenta el margen M (ej: de 30 a 50)
3. Calibra de nuevo tocando más precisamente el centro de las cruces
4. Verifica que no haya toques fantasma (usa el antirrebote de 300ms)

---

## 🔍 DEBUGGING

### Ver valores de calibración:

Después de calibrar, las variables globales contienen:

```c
// En tp.c (accesibles para debug con breakpoint):
g_swap_xy    // 0 o 1
g_kx_fp      // Factor X (16.16 fixed-point, puede ser negativo)
g_ky_fp      // Factor Y (16.16 fixed-point, puede ser negativo)
g_ts_xc      // Centro X en coordenadas ADC
g_ts_yc      // Centro Y en coordenadas ADC
g_lcd_xc     // Centro X en coordenadas LCD (= XRES/2)
g_lcd_yc     // Centro Y en coordenadas LCD (= YRES/2)
```

### Valores típicos esperados:
- `g_swap_xy`: 0 o 1 (depende de tu hardware)
- `g_kx_fp`: ~4000 a ~8000 (positivo o negativo)
- `g_ky_fp`: ~4000 a ~8000 (positivo o negativo)
- `g_ts_xc`: ~500 (valor ADC del centro)
- `g_ts_yc`: ~370 (valor ADC del centro)

---

## 📝 EJEMPLO COMPLETO DE INTEGRACIÓN

```c
#include "44blib.h"
#include "44b.h"
#include "lcd.h"
#include "tp.h"
#include "timer2.h"

void Main(void)
{
    int x, y;
    int boton1_toca = 0;
    
    // Inicializaciones
    sys_init();
    Lcd_Init();
    TS_init();
    timer2_init();
    timer2_start();
    
    // CALIBRAR (solo una vez)
    ts_calibrate_5pt(320, 240, 40);
    
    // Dibujar interfaz
    Lcd_Clr();
    Lcd_DspAscII8x16(100, 100, 0xf, "BOTON 1");
    Lcd_Draw_Box(90, 90, 210, 120, 0xf);  // Marco del botón
    Lcd_Dma_Trans();
    
    // Bucle principal
    while (1)
    {
        // Leer touchscreen (no bloqueante)
        if (ts_read_calibrated(&x, &y) == 0)
        {
            // Verificar si se tocó el botón 1
            if (x >= 90 && x <= 210 && y >= 90 && y <= 120)
            {
                if (!boton1_toca)
                {
                    // Primera vez que se toca
                    boton1_toca = 1;
                    
                    // Dibujar feedback visual
                    Lcd_Draw_Box(90, 90, 210, 120, 0xa);  // Cambiar color
                    Lcd_Dma_Trans();
                    
                    // Acción del botón
                    // ... tu código ...
                }
            }
            else
            {
                // Fuera del botón
                if (boton1_toca)
                {
                    // Se soltó el botón
                    boton1_toca = 0;
                    
                    // Restaurar color original
                    Lcd_Draw_Box(90, 90, 210, 120, 0xf);
                    Lcd_Dma_Trans();
                }
            }
        }
        
        // ... resto de tu aplicación ...
    }
}
```

---

## ✅ VERIFICACIÓN DE IMPLEMENTACIÓN

- ✅ **tp.h** actualizado con 5 nuevas funciones
- ✅ **tp.c** con variables globales de calibración
- ✅ **tp.c** con include de timer2.h
- ✅ **TSInt()** mejorado: 10 muestras + filtrado + antirrebote
- ✅ **Funciones auxiliares** implementadas (8 funciones)
- ✅ **ts_calibrate_5pt()** completa con detección swap XY
- ✅ **ts_read_calibrated()** con conversión fixed-point
- ✅ **2 funciones de test** listas para usar

---

## 🎯 PRÓXIMOS PASOS

1. **Compila el proyecto** para verificar que no hay errores
2. **Prueba con `ts_test_calibracion()`** para verificar que funciona
3. **Ajusta el margen M** si es necesario (30-50 píxeles)
4. **Integra en tu aplicación** usando `ts_calibrate_5pt()` + `ts_read_calibrated()`
5. **¡Disfruta del touchscreen calibrado!** 🎉

---

## 📞 SOPORTE

Si encuentras problemas:
1. Verifica que timer2 esté inicializado y corriendo
2. Revisa que las funciones de LCD (Lcd_Init, LCD_PutPixel, etc.) funcionen
3. Usa los tests incluidos para verificar el funcionamiento
4. Ajusta los delays si el hardware responde diferente

**¡La implementación está lista para usar!** 🚀
