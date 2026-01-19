/*
 * Asignatura: Proyecto hardware
 * Fecha: 09/10/2025
 * Autores: Francisco Jose Martinez, Claudia Mateo, Nayara Gomez
 * Archivo: sudoku_2025.h
 *
 * Declara tamaños y funciones para actualizar/propagar candidatos en
 * tableros Sudoku 9x9 (CELDA[NUM_FILAS][NUM_COLUMNAS]). Usado por las
 * implementaciones C y ARM del proyecto.
 */

#ifndef SUDOKU_H_2025
#define SUDOKU_H_2025

#include <inttypes.h>
#include "celda.h"

/* Tamaños de la cuadrícula */
enum
{
      NUM_FILAS = 9,
      PADDING = 7,
      NUM_COLUMNAS = NUM_FILAS + PADDING
};

/* Definiciones para valores muy utilizados */
enum
{
      FALSO = 0,
      VERDADERO = 1
};

/* Prototipos de funciones de lógica de candidatos */
void candidatos_propagar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
                           uint8_t fila, uint8_t columna);

int candidatos_actualizar_arm(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

__attribute__((noinline)) int candidatos_actualizar_arm_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

int candidatos_propagar_arm(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
                            uint8_t fila, uint8_t columna);

int candidatos_actualizar_all(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

int candidatos_actualizar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS]);

/* FUNCIONES DE VISUALIZACIÓN - USAR LAS DE sudoku_lcd.h */
/* NO re definir aquí */

/* Funciones de control del juego */
int Sudoku_Esta_Region_Expandida_Activa(void);
void Sudoku_Procesar_Touch_Region_Expandida(int x, int y);
void Sudoku_Procesar_Touch(int x, int y);
int Sudoku_Juego_En_Progreso(void);
int Sudoku_Partida_Terminada(void);
unsigned int Sudoku_Obtener_Tiempo_Inicio(void);

#endif /* SUDOKU_H_2025 */
