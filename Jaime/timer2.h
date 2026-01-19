/*********************************************************************************************
* Fichero:		timer2.h
* Autor:		
* Descrip:		Librería de medición de tiempo usando el timer2 del s3c44b0x
* Version:		1.0
*********************************************************************************************/

#ifndef _TIMER2_H_
#define _TIMER2_H_

/*--- declaración de funciones visibles del módulo timer2.c/timer2.h ---*/

/**
 * Inicializa el timer2 para trabajar a la máxima precisión posible.
 * El timer genera una interrupción cada vez que complete un ciclo de cuenta.
 */
void timer2_init(void);

/**
 * Reinicia la cuenta de tiempo (contador y variable de interrupciones)
 * y comienza a medir desde cero.
 */
void timer2_start(void);

/**
 * Lee la cuenta actual del temporizador y el número de interrupciones generadas.
 * Retorna el tiempo transcurrido en microsegundos desde la última llamada a timer2_start().
 */
unsigned int timer2_count(void);

#endif /* _TIMER2_H_ */
