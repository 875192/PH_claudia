/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: celda.c
 * Descripción: Implementación de utilidades para manipular una CELDA de Sudoku
 */

#include "celda.h"

uint8_t celda_leer_valor(CELDA celda)
{
    return (uint8_t)(celda & 0x0Fu);
}