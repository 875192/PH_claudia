/*********************************************************************************************
* Fichero:		timer1.c
* Autor:
* Descrip:		Control del timer1 para latido (heartbeat) con LED
* Version:		1.0
*********************************************************************************************/

/*--- Ficheros de cabecera ---*/
#include "timer1.h"
#include "led.h"
#include "44b.h"
#include "44blib.h"

/*--- Constantes ---*/
/* Timer1 debe generar 60 eventos por segundo
 * Reloj: 64 MHz
 * Prescaler: 255 (divisor: 255+1 = 256)
 * Divisor MUX: 1/16
 * Frecuencia del timer: 64MHz / 256 / 16 = 15625 Hz
 * Para 60 eventos/segundo: 15625 / 60 = 260.4 ≈ 260 ticks
 * 
 * IMPORTANTE: El enunciado dice "reducir el número de interrupciones a una por evento"
 * Por tanto, configuramos el timer para generar exactamente 60 interrupciones/segundo
 */
#define TIMER1_PRESCALER        255
#define TIMER1_MUX_DIV          0x3     // 1/16
#define TIMER1_COUNT            260     // Para 60 interrupciones por segundo

/*--- Variables internas ---*/
static volatile unsigned int timer1_contador = 0;
static volatile unsigned int led2_encendido = 0;  // Estado del LED2

/* Declaración de función que es rutina de servicio de interrupción */
void timer1_ISR(void) __attribute__((interrupt("IRQ")));

/*--- Código de las funciones de interrupción ---*/

/**
 * Rutina de servicio de interrupción del Timer1.
 * Se ejecuta 60 veces por segundo (1 interrupción por evento).
 * El LED parpadea a 6 Hz: enciende/apaga 3 veces por segundo.
 * 
 * Frecuencia de parpadeo: 6 Hz
 * Interrupciones por segundo: 60
 * Interrupciones por ciclo de parpadeo: 60 / 6 = 10
 * Por tanto, alternamos el LED cada 10 interrupciones.
 */
void timer1_ISR(void)
{
	timer1_contador++;
	
	/* Cambiar estado del LED cada 10 interrupciones */
	/* 60 interrupciones/seg ÷ 6 Hz = 10 interrupciones por ciclo completo */
	if (timer1_contador >= 10)
	{
		if (led2_encendido)
		{
			led2_off();  // Apaga el LED de la derecha
			led2_encendido = 0;
		}
		else
		{
			led2_on();   // Enciende el LED de la derecha
			led2_encendido = 1;
		}
		timer1_contador = 0;
	}
	
	/* Borrar bit en I_ISPC para desactivar la solicitud de interrupción */
	rI_ISPC |= BIT_TIMER1;
}

/*--- Código de las funciones públicas ---*/

/**
 * Inicializa el Timer1 para generar 60 eventos por segundo.
 * Configuración:
 * - Prescaler: 255 (divisor 256)
 * - MUX divisor: 1/16
 * - Frecuencia resultante: 64MHz / 256 / 16 = 15625 Hz
 * - Cuenta: 260 ticks para obtener 60 interrupciones/segundo
 * - Resolución: 1/15625 Hz = 64 µs por tick
 * - Rango: 260 ticks × 64 µs ≈ 16.64 ms por interrupción
 */
void timer1_init(void)
{
	/* Configuración del controlador de interrupciones */
	rINTMOD = 0x0;				// Configura las líneas como de tipo IRQ
	rINTCON = 0x1;				// Habilita int. vectorizadas y la línea IRQ
	rINTMSK &= ~(BIT_TIMER1);	// Habilita interrupción del Timer1
	
	/* Establece la rutina de servicio para TIMER1 */
	pISR_TIMER1 = (unsigned) timer1_ISR;
	
	/* Configura el Timer1 */
	/* TCFG0[7:0] controla el prescaler del timer1 (compartido con timer0) */
	rTCFG0 = (rTCFG0 & ~(0xFF << 0)) | (TIMER1_PRESCALER << 0);
	
	/* TCFG1[7:4] controla el divisor MUX del timer1 */
	rTCFG1 = (rTCFG1 & ~(0xF << 4)) | (TIMER1_MUX_DIV << 4);
	
	/* Configura el valor de cuenta inicial */
	rTCNTB1 = TIMER1_COUNT;
	
	/* Valor de comparación (no se usa para generar señal, solo interrupción) */
	rTCMPB1 = 0;
	
	/* Manual update (bit 9) para cargar TCNTB1 en TCNT1 */
	rTCON |= (1 << 9);
	
	/* Iniciar timer1 (bit 8) con auto-reload (bit 11), quitar manual update (bit 9) */
	rTCON = (rTCON & ~(0xF << 8)) | (1 << 11) | (1 << 8);
	
	/* Inicializar contadores y estado del LED */
	timer1_contador = 0;
	led2_encendido = 0;
	led2_off();  // Asegurar que el LED empieza apagado
}