/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: button.h
 * Descripción: Funciones de manejo de los pulsadores (EINT6-7) y teclado matricial
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdint.h>
#include "sudoku_2025.h"
#include "celda.h"

/**
 * Inicializa las interrupciones EINT4/5/6/7 y el teclado (EINT1).
 */
void Eint4567_init(void);

/**
 * Inicializa específicamente EINT1 para el teclado matricial.
 */
void eint1_init(void);

/**
 * Procesa el antirrebote de los botones.
 * Debe llamarse periódicamente desde el bucle principal o timer.
 */
void procesar_antirebote_botones(void);

/**
 * Procesa toques del touchscreen en el tablero de Sudoku.
 * Detecta clics en regiones 3x3 y activa el zoom.
 */
void procesar_touchscreen_sudoku(void);

/**
 * Ejecuta la máquina de estados del juego (selección fila/columna/valor).
 * Debe llamarse periódicamente desde el bucle principal.
 */
void procesar_maquina_estados_botones(void);

/**
 * Procesa el teclado matricial (escaneo y debounce).
 * Debe llamarse periódicamente.
 */
void procesar_teclado(void);

/**
 * Lee el estado físico de un botón.
 * @param boton: índice del botón (0 o 1)
 * @return: 1 si libre, 0 si pulsado
 */
int leer_estado_boton(int boton);

/**
 * Verifica si el teclado está soltado.
 * @return: 1 si soltado, 0 si hay tecla pulsada
 */
int teclado_soltado(void);

/**
 * Dibuja la vista zoom de una región 3x3 del tablero.
 * @param fila_ini: fila inicial de la región (0, 3 o 6)
 * @param col_ini: columna inicial de la región (0, 3 o 6)
 */
void dibujar_vista_zoom(int fila_ini, int col_ini);

/**
 * Marca las pistas iniciales en el tablero.
 * @param cuadricula: matriz del tablero de sudoku
 */
void marcar_pistas_iniciales(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

#endif /* _BUTTON_H_ */
