/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: timer1.c
 * Descripción: Implementación de Timer1 para generar interrupciones periódicas y controlar el parpadeo de un LED
 */

#include "timer1.h"
#include "led.h"
#include "44b.h"
#include "44blib.h"


static volatile unsigned int contador_eventos = 0;

/**
 * ISR del Timer1
 * Ejecutada en cada underflow del Timer1. Incrementa el contador de
 * eventos y, cada N ocurrencias (establecido para lograr 6 Hz del LED),
 * alterna el estado del LED.
 */
void timer1_ISR(void) __attribute__((interrupt("IRQ")));

void timer1_ISR(void)
{
   contador_eventos++;

   /*Queremos que genere 60 eventos por segundo, LED parpadea a 6 Hz
   (3 veces/segundo on/off) 60/6 = 10  */
   if (contador_eventos >= 10)
   {
      led2_switch();
      contador_eventos = 0;
   }

   rI_ISPC |= BIT_TIMER1;
}

/**
 * Inicializa Timer1 para generar interrupciones a ~60 Hz.
 *
 * Configuración:
 *  - Prescaler 255 -> divisor 256 sobre PCLK (66 MHz) => ~257812.5 Hz
 *  - División adicional por 4 mediante rTCFG1 para obtener ~64.45 kHz
 *  - rTCNTB1 se carga con 1074 para producir ~60 interrupciones por segundo
 *  - Se habilitan auto-reload y arranque del timer
 */
void timer1_init(void)
{
   rINTMSK &= ~(BIT_TIMER1);
   pISR_TIMER1 = (unsigned)timer1_ISR;

   rTCFG0 = 255;
   
   rTCFG1 = 0x10;

   rTCNTB1 = 1074;
   rTCMPB1 = 0;

   rTCON |= (1 << 9);
   rTCON &= ~(1 << 9);

   rTCON |= (0x9 << 8);
}
