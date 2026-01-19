/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: timer2.h
 * Descripción: Declaración de funciones para la gestión del Timer2
 */

#ifndef _TIMER2_H_
#define _TIMER2_H_

/*--- declaracion de funciones visibles del módulo timer2.c/timer2.h ---*/
void timer2_init(void);
void timer2_start(void);
unsigned int timer2_count(void);

/* Nueva función para procesar anti-rebote desde ISR */
void timer2_procesar_antirebote(void);

#endif /* _TIMER2_H_ */