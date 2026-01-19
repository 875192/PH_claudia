/*
 * Asignatura: Proyecto hardware
 * Fecha: 09/10/2025
 * Autores: Francisco Jose MartÃ­nez, Claudia Mateo, Nayara Gomez
 * Archivo: sudoku_2025.c
 */

#include "sudoku_2025.h"
#include "tableros.h"
#include "celda.h"

#define CORRECTO 0
#define INCORRECTO -1
typedef unsigned int size_t;

/**
 * FUNCIÃ“N: candidatos_propagar_c
 * Elimina el valor de una celda de todos los candidatos de su fila, columna y regiÃ³n 3x3
 * PARÃ�METROS: cuadricula (matriz 9x16), fila (0-8), columna (0-8)
 * Las variables utilizadas son: valor, Ã­ndices i/j, lÃ­mites regiÃ³n init_i/j, end_i/j
 * Extrae valor â†’ elimina de fila â†’ elimina de columna â†’ elimina de regiÃ³n
 */
void candidatos_propagar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
                           uint8_t fila, uint8_t columna)
{
    uint8_t j, i, init_i, init_j, end_i, end_j;
    const uint8_t init_region[NUM_FILAS] = {0, 0, 0, 3, 3, 3, 6, 6, 6};

    uint8_t valor = celda_leer_valor(cuadricula[fila][columna]);
    celda_eliminar_candidato(&cuadricula[fila][columna], valor);

    init_i = init_region[fila];
    end_i = init_i + 3;
    init_j = init_region[columna];
    end_j = init_j + 3;

    for (j = 0; j < 9; j++)
    {
        if (j != columna)
        {
            celda_eliminar_candidato(&cuadricula[fila][j], valor);
        }
    }

    for (i = 0; i < NUM_FILAS; i++)
    {
        if (i != fila)
        {
            celda_eliminar_candidato(&cuadricula[i][columna], valor);
        }
    }

    for (i = init_i; i < end_i; i++)
    {
        for (j = init_j; j < end_j; j++)
        {
            if (!(i == fila && j == columna))
            {
                celda_eliminar_candidato(&cuadricula[i][j], valor);
            }
        }
    }
}

/**
 * FUNCIÃ“N: candidatos_actualizar_c
 * Actualiza candidatos usando solo funciones C (versiÃ³n referencia)
 * PARÃ�METROS: cuadricula (matriz 9x16, modificada in-place)
 * Las variables utilizadas son: celdas_vacias (retorno), Ã­ndices i/j, valor
 * Limpia candidatos (AND 0x7F) â†’ propaga cada celda con valor
 * Retorna: NÃºmero de celdas vacÃ­as
 */
int candidatos_actualizar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])

{
    int celdas_vacias = 0;
    uint8_t i, j, valor, num;
    uint8_t fila, columna;

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            cuadricula[i][j] &= 0x007F;
        }
    }

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            valor = celda_leer_valor(cuadricula[i][j]);
            if (valor > 0)
            {
                candidatos_propagar_c(cuadricula, i, j);
            }
            else
            {
                celdas_vacias++;
            }
        }
    }
    return celdas_vacias;
}

/**
 * FUNCIÃ“N: candidatos_actualizar_c_arm
 * VersiÃ³n hÃ­brida: limpieza en C + propagaciÃ³n en ARM
 * PARÃ�METROS: cuadricula (matriz 9x16, modificada in-place)
 * Las variables utilizadas son: celdas_vacias (retorno), Ã­ndices i/j, valor
 * La diferencia es que llama candidatos_propagar_arm en lugar de candidatos_propagar_c
 * Retorna: NÃºmero de celdas vacÃ­as
 **/
static int candidatos_actualizar_c_arm(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
    int celdas_vacias = 0;
    uint8_t i, j, valor, num;
    uint8_t fila, columna;

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            cuadricula[i][j] &= 0x007F;
        }
    }

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            valor = celda_leer_valor(cuadricula[i][j]);
            if (valor > 0)
            {
                candidatos_propagar_arm(cuadricula, i, j);
            }
            else
            {
                celdas_vacias++;
            }
        }
    }
    return celdas_vacias;
}
