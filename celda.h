/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: celda.h
 * Descripción: Utilidades para manipular una CELDA de Sudoku
 */

#ifndef CELDA_H
#define CELDA_H

#include <inttypes.h>

/* Cada celda se codifica en 16 bits:
 * bits [15:7]: vector de candidatos (bit = 0 => candidato, 1 => no candidato)
 * bit 6:     no usado
 * bit 5:     error flag
 * bit 4:     pista flag
 * bits [3:0]: valor (0 = vacío, 1..9 valores)
 */

enum
{
    BIT_CANDIDATOS = 7
};

typedef uint16_t CELDA;

/* Funciones en línea para poder definirlas en el header sin provocar
   multiple definition al enlazar. Usar static inline para portabilidad. */

static inline void celda_eliminar_candidato(CELDA *celdaptr, uint8_t valor)
{
    /* Poner a 1 el bit correspondiente para marcar NO candidato */
    *celdaptr |= (uint16_t)(1u << (BIT_CANDIDATOS + (valor - 1)));
}

static inline void celda_poner_valor(CELDA *celdaptr, uint8_t val)
{
    /* Mantener los bits altos (candidatos/flags) y escribir valor en bits [3:0] */
    *celdaptr = (uint16_t)((*celdaptr & 0xFFF0u) | (val & 0x0Fu));
}

/* elimina la definición static inline y deja sólo el prototipo */
uint8_t celda_leer_valor(CELDA celda);

#endif // CELDA_H
