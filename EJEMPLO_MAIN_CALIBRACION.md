/*
 * EJEMPLO: Integración de calibración touchscreen en tu proyecto
 * 
 * Este archivo muestra cómo integrar la calibración de 5 puntos
 * en tu aplicación existente.
 */

#include "44blib.h"
#include "44b.h"
#include "lcd.h"
#include "tp.h"
#include "timer2.h"
#include "8led.h"
#include "button.h"
#include "led.h"

void Main(void)
{
    int x, y;
    
    // ========================================
    // 1. INICIALIZACIONES BÁSICAS
    // ========================================
    sys_init();         // Sistema
    Lcd_Init();         // LCD
    TS_init();          // Touchscreen
    timer2_init();      // Timer para timestamps
    timer2_start();     // Iniciar conteo
    
    // Otras inicializaciones de tu proyecto
    D8Led_init();
    Eint4567_init();
    // ... más inicializaciones ...
    
    
    // ========================================
    // 2. CALIBRACIÓN (solo una vez al inicio)
    // ========================================
    // Margen de 40 píxeles desde los bordes
    ts_calibrate_5pt(320, 240, 40);
    
    // Después de calibrar, puedes usar ts_read_calibrated()
    // tantas veces como quieras
    
    
    // ========================================
    // 3. DIBUJAR TU INTERFAZ
    // ========================================
    Lcd_Clr();
    Lcd_Active_Clr();
    
    // Ejemplo: dibujar un botón
    Lcd_Draw_Box(50, 50, 150, 100, 0xf);
    Lcd_DspAscII8x16(70, 65, 0xf, "BOTON 1");
    
    // Otro botón
    Lcd_Draw_Box(170, 50, 270, 100, 0xf);
    Lcd_DspAscII8x16(190, 65, 0xf, "BOTON 2");
    
    Lcd_Dma_Trans();
    
    
    // ========================================
    // 4. BUCLE PRINCIPAL
    // ========================================
    while (1)
    {
        // Leer touchscreen (NO bloqueante)
        if (ts_read_calibrated(&x, &y) == 0)
        {
            // ¡Toque detectado!
            // x, y contienen las coordenadas calibradas
            
            // Verificar qué se tocó
            if (x >= 50 && x <= 150 && y >= 50 && y <= 100)
            {
                // Se tocó BOTÓN 1
                led1_on();  // Ejemplo de acción
                
                // Feedback visual
                Lcd_Draw_Box(50, 50, 150, 100, 0xa);
                Lcd_Dma_Trans();
            }
            else if (x >= 170 && x <= 270 && y >= 50 && y <= 100)
            {
                // Se tocó BOTÓN 2
                led2_on();  // Ejemplo de acción
                
                // Feedback visual
                Lcd_Draw_Box(170, 50, 270, 100, 0xa);
                Lcd_Dma_Trans();
            }
            else
            {
                // Toque fuera de botones
                // Opcional: dibujar donde se toca
                LCD_PutPixel(x, y, 0xf);
                Lcd_Dma_Trans();
            }
        }
        
        // Resto de tu lógica de aplicación
        // ... tu código existente ...
    }
}


// ============================================================================
// EJEMPLO 2: Solo test rápido de calibración
// ============================================================================
/*
void Main(void)
{
    sys_init();
    
    // Test de dibujo libre
    // (calibra automáticamente y permite dibujar)
    ts_test_calibracion();
    
    // NUNCA SALE - solo para pruebas
}
*/


// ============================================================================
// EJEMPLO 3: Test con números
// ============================================================================
/*
void Main(void)
{
    sys_init();
    
    // Test con dos números en pantalla
    ts_test_numeros();
    
    // NUNCA SALE - solo para pruebas
}
*/


// ============================================================================
// EJEMPLO 4: Integración con tu aplicación Sudoku existente
// ============================================================================
/*
#include "sudoku_2025.h"
#include "tableros.h"

void Main(void)
{
    int x, y;
    int celda_fila, celda_col;
    
    // Inicializaciones
    sys_init();
    Lcd_Init();
    TS_init();
    timer2_init();
    timer2_start();
    D8Led_init();
    Eint4567_init();
    
    // Calibrar touchscreen con margen de 50 píxeles (mejor precisión)
    ts_calibrate_5pt(320, 240, 50);
    
    // Dibujar tablero de Sudoku
    Lcd_Clr();
    sudoku_dibujar_tablero_completo(cuadricula_ARM_ARM);
    Lcd_Dma_Trans();
    
    // Bucle de juego
    while (1)
    {
        // Leer touchscreen
        if (ts_read_calibrated(&x, &y) == 0)
        {
            // Convertir coordenadas LCD a celda de Sudoku
            // Asumiendo que el tablero está en (TABLERO_X0, TABLERO_Y0)
            // y cada celda mide CELDA_SIZE píxeles
            
            if (x >= TABLERO_X0 && x < TABLERO_X0 + 9*CELDA_SIZE &&
                y >= TABLERO_Y0 && y < TABLERO_Y0 + 9*CELDA_SIZE)
            {
                celda_col = (x - TABLERO_X0) / CELDA_SIZE;
                celda_fila = (y - TABLERO_Y0) / CELDA_SIZE;
                
                // Procesar selección de celda
                sudoku_resaltar_celda(celda_fila, celda_col, 1);
                Lcd_Dma_Trans();
                
                // ... tu lógica de juego ...
            }
        }
        
        // Resto de tu código de Sudoku
        procesar_maquina_estados_botones();
    }
}
*/
