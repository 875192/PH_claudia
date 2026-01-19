/*********************************************************************************************
 * Fichero:		led.c
 * Autor:
 * Descrip:		funciones de control de los LED de la placa
 * Version:
 *********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "led.h"
#include "44b.h"
#include "44blib.h"
#include "eventos.h"
#include "timer2.h"
#include "cola.h"

/*--- variables globales del módulo ---*/
static int led_state = 0; /* estado del LED */

/*--- código de las funciones públicas ---*/
void leds_on()
{
	Led_Display(0x3);
}

void leds_off()
{
	Led_Display(0x0);
}

void led1_on()
{
	led_state = led_state | 0x1;
	Led_Display(led_state);
}

void led1_off()
{
	led_state = led_state & 0xfe;
	Led_Display(led_state);
}

void led2_on()
{
	led_state = led_state | 0x2;
}

void led2_off()
{
	led_state = led_state & 0xfd;
	Led_Display(led_state);
}

void leds_switch()
{
	led_state ^= 0x03;
	Led_Display(led_state);
	// cola_depuracion(EVT_LEDS_CAMBIO, timer2_count(), 5);
}

/* */
void led2_switch(void)
{
	/* El rPDATB es el registro de datos del puerto B.
	 * El LED2 está conectado al bit 10 del puerto B.
	 * Si el bit 10 es 1, el LED está apagado; si es 0, el LED está encendido.
	 * El 400 en hexadecimal es 2^10, que corresponde al bit 10.
	 * Si rPDATB & 0x400 es verdadero, significa que el bit 10 es 1 (LED apagado).
	 * Si es falso, el bit 10 es 0 (LED encendido).
	 */
	if (rPDATB & 0x400)
		led2_on();
	else
		led2_off();
}

void Led_Display(int LedStatus)
{
	led_state = LedStatus;

	if ((LedStatus & 0x01) == 0x01)
		rPDATB = rPDATB & 0x5ff; /* poner a 0 el bit 9 del puerto B */
	else
		rPDATB = rPDATB | 0x200; /* poner a 1 el bit 9 del puerto B */

	if ((LedStatus & 0x02) == 0x02)
		rPDATB = rPDATB & 0x3ff; /* poner a 0 el bit 10 del puerto B */
	else
		rPDATB = rPDATB | 0x400; /* poner a 1 el bit 10 del puerto B */
}
