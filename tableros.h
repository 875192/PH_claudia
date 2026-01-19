/*
 * Asignatura: Proyecto hardware
 * Fecha: 09/10/2025
 * Autores: Francisco Jose Martinez, Claudia Mateo, Nayara Gomez
 * Archivo: tableros.h
 *
 * Define las cuadriculas de test (CELDA[NUM_FILAS][NUM_COLUMNAS]) y la solucion.
 * Todas las matrices estan alineadas para uso con las rutinas ARM del proyecto.
 */
#ifndef TABLEROS_H
#define TABLEROS_H

#include "celda.h"
#include "sudoku_2025.h"

extern CELDA cuadricula_C_C[NUM_FILAS][NUM_COLUMNAS];
extern CELDA cuadricula_C_ARM[NUM_FILAS][NUM_COLUMNAS];
extern CELDA cuadricula_ARM_C[NUM_FILAS][NUM_COLUMNAS];
extern CELDA cuadricula_ARM_ARM[NUM_FILAS][NUM_COLUMNAS];
extern CELDA solucion[NUM_FILAS][NUM_COLUMNAS];

#endif
