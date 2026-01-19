/*********************************************************************************************
* Fichero:		timer1.h
* Autor:
* Descrip:		Control del timer1 para latido (heartbeat) con LED
* Version:		1.0
*********************************************************************************************/

#ifndef _TIMER1_H_
#define _TIMER1_H_

/*--- Declaración de funciones visibles del módulo timer1.c/timer1.h ---*/

/**
 * Inicializa el Timer1 para generar 60 eventos por segundo.
 * El LED de la derecha parpadeará a 6 Hz (3 veces por segundo on/off).
 */
void timer1_init(void);

#endif /* _TIMER1_H_ */