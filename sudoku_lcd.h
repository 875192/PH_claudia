/*
 * Asignatura: Proyecto hardware
 * Fecha: 09/12/2025
 * Autores: Claudia Mateo Cuellar
 * Archivo: sudoku_lcd.h
 * Descripción: Interfaz gráfica del juego Sudoku en LCD
 */

#ifndef SUDOKU_LCD_H
#define SUDOKU_LCD_H

#include <stdint.h>
#include "celda.h"
#include "sudoku_2025.h"

/**
 * Muestra la pantalla inicial con instrucciones del juego
 */
void sudoku_mostrar_pantalla_inicial(void);

/**
 * Dibuja el marco completo del tablero (líneas y números de fila/columna)
 */
void sudoku_dibujar_marco_tablero(void);

/**
 * Dibuja un número en una celda específica del tablero
 * @param fila: fila del tablero (0-8)
 * @param columna: columna del tablero (0-8)
 * @param valor: valor a mostrar (1-9, 0 para vacío)
 * @param es_pista: 1 si es una pista inicial, 0 si la introdujo el usuario
 * @param es_error: 1 si el valor es erróneo
 */
void sudoku_dibujar_numero_celda(uint8_t fila, uint8_t columna, uint8_t valor, 
                                  uint8_t es_pista, uint8_t es_error);

/**
 * Resalta una celda específica (para indicar selección)
 * @param fila: fila del tablero (0-8)
 * @param columna: columna del tablero (0-8)
 * @param resaltar: 1 para resaltar, 0 para quitar resaltado
 */
void sudoku_resaltar_celda(uint8_t fila, uint8_t columna, uint8_t resaltar);

/**
 * Dibuja el tablero completo con todos sus valores
 * @param cuadricula: matriz del sudoku a dibujar
 */
void sudoku_dibujar_tablero_completo(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

/**
 * Actualiza una celda individual en pantalla
 * @param cuadricula: matriz del sudoku
 * @param fila: fila a actualizar (0-8)
 * @param columna: columna a actualizar (0-8)
 */
void sudoku_actualizar_celda(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS], 
                              uint8_t fila, uint8_t columna);

/**
 * Muestra un mensaje de victoria cuando se completa el juego
 */
void sudoku_mostrar_victoria(void);

/**
 * Inicializa el juego mostrando la pantalla inicial
 * @param cuadricula: matriz del sudoku a utilizar
 */
void sudoku_iniciar_juego(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

#endif /* SUDOKU_LCD_H */