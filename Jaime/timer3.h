/*********************************************************************************************
* Fichero:              timer3.h
* Autor:
* Descrip:              Temporizador para eliminación de rebotes con máquina de estados
* Version:              1.0
*********************************************************************************************/

#ifndef _TIMER3_H_
#define _TIMER3_H_

#include <stdint.h>
#include "eventos.h"

/* Tipo de callback para notificar una pulsación validada */
typedef void (*timer3_callback_t)(uint8_t boton_id);

/* Inicializa el timer3 para gestionar la eliminación de rebotes */
void timer3_init(timer3_callback_t callback);

/* Arranca la máquina de estados de antirrebote para el botón indicado
 * Devuelve 1 si se inicia correctamente o 0 si ya estaba en curso.
 */
int timer3_start_antirrebote(uint8_t boton_id);

/* Indica si la máquina de antirrebote está procesando una pulsación */
int timer3_esta_ocupado(void);

/* Variables globales del Sudoku accesibles desde main.c */
extern volatile EstadoSudoku estado_sudoku;

#endif /* _TIMER3_H_ */
