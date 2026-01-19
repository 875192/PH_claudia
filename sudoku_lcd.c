/*
 * Asignatura: Proyecto hardware
 * Fecha: 09/12/2025
 * Autores: Claudia Mateo Cuellar
 * Archivo: sudoku_lcd.c
 * Descripción: Interfaz gráfica del juego Sudoku en LCD
 */

#include "sudoku_lcd.h"
#include "lcd.h"
#include "celda.h"
#include "sudoku_2025.h"
#include "tableros.h"
#include "def.h"

/* Definiciones de diseño */
#define TABLERO_SIZE 216      // Tamaño del tablero (216x216 píxeles, múltiplo de 9)
#define CELDA_SIZE 24         // Tamaño de cada celda (216/9 = 24 píxeles)
#define TABLERO_X0 52         // Posición X inicial del tablero (centrado en 320px)
#define TABLERO_Y0 12         // Posición Y inicial del tablero (centrado en 240px)
#define MARGEN_NUMERO 8       // Margen para los números de fila/columna
#define GROSOR_LINEA_FINA 1   // Grosor de líneas normales
#define GROSOR_LINEA_GRUESA 2 // Grosor de líneas de región 3x3

/* Definiciones para candidatos */
#define CANDIDATO_GRID_SIZE 7  // Tamaño de cada subcuadrícula para candidatos (24/3 ≈ 7)
#define CANDIDATO_OFFSET 2     // Offset desde el borde de la celda
#define CANDIDATO_MARCA_SIZE 1 // Tamaño del punto que marca el candidato

/* Colores */
#define COLOR_FONDO WHITE
#define COLOR_LINEA BLACK
#define COLOR_PISTA BLACK
#define COLOR_USUARIO DARKGRAY
#define COLOR_ERROR 0x0C     // Rojo oscuro
#define COLOR_SELECCION 0x06 // Gris claro para selección
#define COLOR_CANDIDATO 0x08 // Gris medio para candidatos

/**
 * Muestra la pantalla inicial con instrucciones
 */
void sudoku_mostrar_pantalla_inicial(void)
{
    Lcd_Clr();

    /* Título */
    Lcd_DspAscII8x16(100, 20, BLACK, (INT8U *)"SUDOKU");

    /* Instrucciones */
    Lcd_DspAscII6x8(40, 60, BLACK, (INT8U *)"INSTRUCCIONES:");
    Lcd_DspAscII6x8(20, 80, BLACK, (INT8U *)"1. Boton IZQ: Navegar");
    Lcd_DspAscII6x8(20, 95, BLACK, (INT8U *)"2. Boton DER: Confirmar");
    Lcd_DspAscII6x8(20, 110, BLACK, (INT8U *)"3. Teclado: Introducir");
    Lcd_DspAscII6x8(20, 125, BLACK, (INT8U *)"   numeros (0-9)"); // Cambio: 0-9 en lugar de 1-9
    Lcd_DspAscII6x8(20, 140, BLACK, (INT8U *)"4. Completar tablero");
    Lcd_DspAscII6x8(20, 155, BLACK, (INT8U *)"5. Fila 0: Salir"); // Nueva instrucción

    /* Mensaje para iniciar */
    Lcd_DspAscII8x16(30, 180, BLACK, (INT8U *)"Pulse un boton");
    Lcd_DspAscII8x16(50, 200, BLACK, (INT8U *)"para jugar");

    Lcd_Dma_Trans();
}

/**
 * Dibuja el marco del tablero con líneas y números
 */
void sudoku_dibujar_marco_tablero(void)
{
    uint8_t i;
    INT8U numero_str[2];
    numero_str[1] = '\0';

    /* Dibujar líneas horizontales */
    for (i = 0; i <= 9; i++)
    {
        INT16U y = TABLERO_Y0 + i * CELDA_SIZE;
        INT8U grosor = (i % 3 == 0) ? GROSOR_LINEA_GRUESA : GROSOR_LINEA_FINA;
        Lcd_Draw_HLine(TABLERO_X0, TABLERO_X0 + TABLERO_SIZE, y, COLOR_LINEA, grosor);
    }

    /* Dibujar líneas verticales */
    for (i = 0; i <= 9; i++)
    {
        INT16U x = TABLERO_X0 + i * CELDA_SIZE;
        INT8U grosor = (i % 3 == 0) ? GROSOR_LINEA_GRUESA : GROSOR_LINEA_FINA;
        Lcd_Draw_VLine(TABLERO_Y0, TABLERO_Y0 + TABLERO_SIZE, x, COLOR_LINEA, grosor);
    }

    /* Dibujar números de fila (1-9) a la izquierda */
    for (i = 0; i < 9; i++)
    {
        numero_str[0] = '1' + i;
        INT16U y = TABLERO_Y0 + i * CELDA_SIZE + CELDA_SIZE / 2 - 4;
        Lcd_DspAscII6x8(TABLERO_X0 - 15, y, COLOR_LINEA, numero_str);
    }

    /* Dibujar números de columna (1-9) arriba */
    for (i = 0; i < 9; i++)
    {
        numero_str[0] = '1' + i;
        INT16U x = TABLERO_X0 + i * CELDA_SIZE + CELDA_SIZE / 2 - 3;
        Lcd_DspAscII6x8(x, TABLERO_Y0 - 10, COLOR_LINEA, numero_str);
    }

    /* Dibujar etiqueta y tiempo en la esquina superior derecha */
    Lcd_DspAscII6x8(270, 2, BLACK, (INT8U *)"Tiempo:");
    Lcd_DspAscII6x8(275, 12, BLACK, (INT8U *)"00:00");

    /* Leyenda para salir del juego */
    Lcd_DspAscII6x8(20, 230, BLACK, (INT8U *)"Fila 0: Acabar partida");
}

/**
 * Dibuja los candidatos en una celda vacía
 * @param fila: fila del tablero (0-8)
 * @param columna: columna del tablero (0-8)
 * @param candidatos: vector de bits con candidatos (bit 0 = candidato 1, etc.)
 *                   Un 0 en el bit indica que ES candidato
 *                   Un 1 en el bit indica que NO es candidato
 */
static void sudoku_dibujar_candidatos(uint8_t fila, uint8_t columna, uint16_t candidatos)
{
    uint8_t num;
    INT16U base_x = TABLERO_X0 + columna * CELDA_SIZE + CANDIDATO_OFFSET;
    INT16U base_y = TABLERO_Y0 + fila * CELDA_SIZE + CANDIDATO_OFFSET;

    /* Posiciones en la cuadrícula 3x3 para cada número 1-9:
     * 1 2 3
     * 4 5 6
     * 7 8 9
     */
    for (num = 1; num <= 9; num++)
    {
        uint8_t es_candidato = ((candidatos & (1 << (num - 1))) == 0) ? 1 : 0;

        if (es_candidato)
        {
            /* Calcular posición en la cuadrícula 3x3 */
            uint8_t grid_row = (num - 1) / 3; /* 0, 0, 0, 1, 1, 1, 2, 2, 2 */
            uint8_t grid_col = (num - 1) % 3; /* 0, 1, 2, 0, 1, 2, 0, 1, 2 */

            INT16U x = base_x + grid_col * CANDIDATO_GRID_SIZE + CANDIDATO_GRID_SIZE / 2;
            INT16U y = base_y + grid_row * CANDIDATO_GRID_SIZE + CANDIDATO_GRID_SIZE / 2;

            /* Dibujar un pequeño punto o círculo para marcar el candidato */
            /* Como no tenemos función de círculo, dibujamos un cuadrado pequeño */
            LcdClrRect(x - CANDIDATO_MARCA_SIZE, y - CANDIDATO_MARCA_SIZE,
                       x + CANDIDATO_MARCA_SIZE, y + CANDIDATO_MARCA_SIZE,
                       COLOR_CANDIDATO);
        }
    }
}

/**
 * Dibuja un número en una celda específica
 * @param fila: fila del tablero (0-8)
 * @param columna: columna del tablero (0-8)
 * @param valor: valor a mostrar (1-9, 0 para vacío)
 * @param es_pista: 1 si es una pista inicial, 0 si la introdujo el usuario
 * @param es_error: 1 si el valor es erróneo
 * @param candidatos: vector de candidatos (solo usado si valor == 0)
 */
void sudoku_dibujar_celda_completa(uint8_t fila, uint8_t columna, uint8_t valor,
                                   uint8_t es_pista, uint8_t es_error, uint16_t candidatos)
{
    INT16U x = TABLERO_X0 + columna * CELDA_SIZE + CELDA_SIZE / 2 - 4;
    INT16U y = TABLERO_Y0 + fila * CELDA_SIZE + CELDA_SIZE / 2 - 4;
    INT8U numero_str[2];
    INT8U color;
    INT8U color_fondo = WHITE; // Fondo blanco normal

    /* Si hay error, usar fondo negro (inversión) */
    if (es_error)
    {
        color_fondo = BLACK; // Fondo negro para errores
    }

    /* Limpiar la celda primero */
    LcdClrRect(TABLERO_X0 + columna * CELDA_SIZE + 2,
               TABLERO_Y0 + fila * CELDA_SIZE + 2,
               TABLERO_X0 + (columna + 1) * CELDA_SIZE - 2,
               TABLERO_Y0 + (fila + 1) * CELDA_SIZE - 2,
               color_fondo);

    /* Si es pista, dibujar BORDE GRUESO para destacar */
    if (es_pista)
    {
        INT16U x0 = TABLERO_X0 + columna * CELDA_SIZE;
        INT16U y0 = TABLERO_Y0 + fila * CELDA_SIZE;
        INT16U x1 = x0 + CELDA_SIZE - 1;
        INT16U y1 = y0 + CELDA_SIZE - 1;

        // Dibujar borde grueso para pistas (2 píxeles de grosor)
        Lcd_Draw_Box(x0, y0, x1, y1, BLACK);
        Lcd_Draw_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, BLACK);
    }

    /* Si hay error, dibujar un BORDE MUY GRUESO ENCIMA */
    if (es_error)
    {
        INT16U x0 = TABLERO_X0 + columna * CELDA_SIZE;
        INT16U y0 = TABLERO_Y0 + fila * CELDA_SIZE;
        INT16U x1 = x0 + CELDA_SIZE - 1;
        INT16U y1 = y0 + CELDA_SIZE - 1;

        // Dibujar múltiples bordes para hacerlo MUY grueso
        Lcd_Draw_Box(x0, y0, x1, y1, BLACK);
        Lcd_Draw_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, BLACK);
        Lcd_Draw_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, BLACK);
        Lcd_Draw_Box(x0 + 3, y0 + 3, x1 - 3, y1 - 3, BLACK);
    }

    /* Si hay valor, dibujarlo */
    if (valor > 0 && valor <= 9)
    {
        numero_str[0] = '0' + valor;
        numero_str[1] = '\0';

        /* Determinar color del texto */
        if (es_error)
        {
            color = WHITE; // Texto blanco sobre fondo negro
        }
        else if (es_pista)
        {
            color = BLACK; // Pistas en negro
        }
        else
        {
            color = DARKGRAY; // Valores usuario en gris
        }

        Lcd_DspAscII6x8(x, y, color, numero_str);
    }
    else
    {
        /* Celda vacía: mostrar candidatos */
        sudoku_dibujar_candidatos(fila, columna, candidatos);
    }
}

/**
 * Dibuja un número en una celda específica (versión antigua - mantener compatibilidad)
 */
void sudoku_dibujar_numero_celda(uint8_t fila, uint8_t columna, uint8_t valor,
                                 uint8_t es_pista, uint8_t es_error)
{
    /* Por compatibilidad, llamar a la versión completa con candidatos = 0x1FF (todos no candidatos) */
    sudoku_dibujar_celda_completa(fila, columna, valor, es_pista, es_error, 0x1FF);
}

/**
 * Resalta una celda (para mostrar la selección actual)
 * @param fila: fila del tablero (0-8)
 * @param columna: columna del tablero (0-8)
 * @param resaltar: 1 para resaltar, 0 para quitar resaltado
 */
void sudoku_resaltar_celda(uint8_t fila, uint8_t columna, uint8_t resaltar)
{
    INT16U x0 = TABLERO_X0 + columna * CELDA_SIZE + 1;
    INT16U y0 = TABLERO_Y0 + fila * CELDA_SIZE + 1;
    INT16U x1 = x0 + CELDA_SIZE - 2;
    INT16U y1 = y0 + CELDA_SIZE - 2;
    INT8U color = resaltar ? COLOR_SELECCION : COLOR_FONDO;

    /* Dibujar un borde interno en la celda */
    Lcd_Draw_HLine(x0, x1, y0, color, 1);
    Lcd_Draw_HLine(x0, x1, y1, color, 1);
    Lcd_Draw_VLine(y0, y1, x0, color, 1);
    Lcd_Draw_VLine(y0, y1, x1, color, 1);
}

/**
 * Dibuja todo el tablero con sus valores actuales
 * @param cuadricula: matriz del sudoku
 */
void sudoku_dibujar_tablero_completo(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
    uint8_t fila, columna;
    uint8_t valor;
    uint8_t es_pista;
    uint8_t es_error;
    uint16_t candidatos;

    /* Limpiar pantalla */
    Lcd_Clr();

    /* Dibujar marco con líneas y números */
    sudoku_dibujar_marco_tablero();

    /* Dibujar todos los valores */
    for (fila = 0; fila < 9; fila++)
    {
        for (columna = 0; columna < 9; columna++)
        {
            valor = celda_leer_valor(cuadricula[fila][columna]);
            es_pista = (cuadricula[fila][columna] & (1 << 4)) ? 1 : 0; // bit 4
            es_error = (cuadricula[fila][columna] & (1 << 5)) ? 1 : 0; // bit 5
            candidatos = (cuadricula[fila][columna] >> 7) & 0x1FF;     // bits [15:7]

            sudoku_dibujar_celda_completa(fila, columna, valor, es_pista, es_error, candidatos);
        }
    }

    /* Transferir buffer virtual a pantalla */
    Lcd_Dma_Trans();
}

/**
 * Actualiza una celda individual en pantalla
 * @param cuadricula: matriz del sudoku
 * @param fila: fila a actualizar (0-8)
 * @param columna: columna a actualizar (0-8)
 */
void sudoku_actualizar_celda(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
                             uint8_t fila, uint8_t columna)
{
    uint8_t valor;
    uint8_t es_pista;
    uint8_t es_error;
    uint16_t candidatos;

    valor = celda_leer_valor(cuadricula[fila][columna]);
    es_pista = (cuadricula[fila][columna] & (1 << 4)) ? 1 : 0; // bit 4
    es_error = (cuadricula[fila][columna] & (1 << 5)) ? 1 : 0; // bit 5
    candidatos = (cuadricula[fila][columna] >> 7) & 0x1FF;     // bits [15:7]

    sudoku_dibujar_celda_completa(fila, columna, valor, es_pista, es_error, candidatos);
    Lcd_Dma_Trans();
}

/**
 * Muestra un mensaje de victoria
 */
void sudoku_mostrar_victoria(void)
{
    /* Mensaje de victoria en la parte inferior de la pantalla */
    LcdClrRect(0, 220, 319, 239, COLOR_SELECCION);
    Lcd_DspAscII8x16(80, 224, BLACK, (INT8U *)"FELICIDADES!");
    Lcd_Dma_Trans();
}

/**
 * Inicializa el tablero para jugar
 * @param cuadricula: matriz del sudoku a utilizar
 */
void sudoku_iniciar_juego(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
    /* Mostrar pantalla inicial */
    sudoku_mostrar_pantalla_inicial();

    /* Aquí esperaríamos a que el usuario pulse un botón */
    /* Esa lógica estará en button.c */

    /* Después se dibujará el tablero completo */
    /* sudoku_dibujar_tablero_completo(cuadricula); */
}