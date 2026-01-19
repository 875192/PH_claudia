/*********************************************************************************************
* Fichero:		timer2.c
* Autor:		
* Descrip:		Librería de medición de tiempo usando el timer2 del s3c44b0x
* Version:		1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "timer2.h"
#include "44b.h"
#include "44blib.h"

/*--- variables internas ---*/
/* Variable compartida con la rutina de interrupción que cuenta los periodos completos */
static volatile unsigned int timer2_numero_int = 0;

/* Declaración de función que es rutina de servicio de interrupción
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void timer2_ISR(void) __attribute__((interrupt("IRQ")));

/*--- código de las funciones ---*/

/**
 * Rutina de servicio de interrupción del Timer2.
 * Se ejecuta cada vez que el timer2 completa un ciclo de cuenta.
 * Incrementa el contador de interrupciones.
 * 
 * NOTA: No registramos cada interrupción en la cola para no sobrecargarla,
 * ya que el timer2 genera muchas interrupciones por segundo (~488 Hz).
 * Solo registramos si es necesario para depuración específica.
 */
void timer2_ISR(void)
{
	/* Incrementar el contador de periodos completos */
	timer2_numero_int++;
	
	/* Borrar bit en I_ISPC para desactivar la solicitud de interrupción */
	rI_ISPC |= BIT_TIMER2; // BIT_TIMER2 pone un uno en el bit 11 que corresponde al Timer2
}

/**
 * Inicializa el timer2 para trabajar a la máxima precisión posible.
 * Configuración:
 * - Reloj de la placa: 64 MHz
 * - Preescalado: 1 (valor 0 en TCFG0 para timer2/3)
 * - Divisor: 1/2 (valor 0 en TCFG1 para timer2)
 * - Frecuencia final del timer: 64 MHz / (0+1) / 2 = 32 MHz
 * - Periodo del timer: 1/32 MHz = 0.03125 microsegundos
 * - Valor de cuenta máximo: 65535 (16 bits)
 * - Tiempo por ciclo completo: 65535 * 0.03125 = 2047.96875 microsegundos (~2.048 ms)
 */
void timer2_init(void)
{
	/* Configuración del controlador de interrupciones */
	rINTMOD = 0x0;              // Configura las líneas como de tipo IRQ
	rINTCON = 0x1;              // Habilita int. vectorizadas y la línea IRQ (FIQ no)
	rINTMSK &= ~(BIT_TIMER2);   // Habilita en vector de máscaras de interrupción el Timer2
	
	/* Establece la rutina de servicio para TIMER2 */
	pISR_TIMER2 = (unsigned) timer2_ISR;
	
	/* Configura el Timer2 para máxima precisión */
	/* TCFG0: registro de preescalado
	 * Timer2 y Timer3 comparten los bits [15:8]
	 * Preescalado = 0 para máxima precisión (divide por 1) */
	rTCFG0 &= ~(0xFF << 8);     // Limpia los bits [15:8]
	rTCFG0 |= (0 << 8);         // Establece preescalado = 0 (divide por 0+1 = 1)
	
	/* TCFG1: registro de selección de divisor
	 * Timer2 usa los bits [11:8]
	 * Valor 0000 = divisor 1/2 */
	rTCFG1 &= ~(0xF << 8);      // Limpia los bits [11:8]
	rTCFG1 |= (0x0 << 8);       // Establece divisor = 1/2
	
	/* TCNTB2: valor inicial de cuenta (cuenta descendente) 
	 * Usamos el valor máximo de 16 bits para maximizar el rango */
	rTCNTB2 = 65535;
	
	/* TCMPB2: valor de comparación (no se usa para medición de tiempo) */
	rTCMPB2 = 0;
	
	/* TCON: registro de control del timer
	 * Timer2 usa los bits [15:12]
	 * Bit 15: Timer2 start/stop (0=stop, 1=start)
	 * Bit 14: Timer2 manual update (1=update TCNTB2 y TCMPB2)
	 * Bit 13: Timer2 output inverter on/off
	 * Bit 12: Timer2 auto reload on/off (1=auto-reload) */
	
	/* Primero establecer manual update para cargar los valores */
	rTCON &= ~(0xF << 12);      // Limpia los bits del Timer2
	rTCON |= (1 << 13);         // Establece manual update
	
	/* Luego iniciar el timer con auto-reload (sin manual update) */
	rTCON &= ~(0xF << 12);      // Limpia los bits del Timer2
	rTCON |= (1 << 15) | (1 << 12);  // Start + auto-reload
}

/**
 * Reinicia la cuenta de tiempo y comienza a medir.
 * Resetea tanto el contador de interrupciones como el valor del timer.
 */
void timer2_start(void)
{
	/* Reiniciar el contador de interrupciones */
	timer2_numero_int = 0;
	
	/* Detener el timer */
	rTCON &= ~(1 << 15);        // Clear bit 15 (stop timer2)
	
	/* Recargar el valor inicial del contador */
	rTCNTB2 = 65535;
	
	/* Establecer manual update para recargar el valor */
	rTCON |= (1 << 13);         // Set manual update
	
	/* Iniciar el timer con auto-reload */
	rTCON &= ~(0xF << 12);      // Limpia los bits del Timer2
	rTCON |= (1 << 15) | (1 << 12);  // Start + auto-reload
}

/**
 * Lee la cuenta actual del temporizador y calcula el tiempo transcurrido.
 * 
 * Retorna el tiempo en microsegundos desde la última llamada a timer2_start().
 * 
 * Cálculo:
 * - Frecuencia del timer: 32 MHz (64 MHz / 1 / 2)
 * - Cada tick del timer: 0.03125 microsegundos (1/32 MHz)
 * - Ticks transcurridos = (65535 - TCNTO2) + (65536 * timer2_numero_int)
 * - Tiempo (µs) = ticks * 0.03125 = ticks / 32
 */
unsigned int timer2_count(void)
{
	unsigned int interrupciones;
	unsigned int contador_actual;
	unsigned int ticks_totales;
	unsigned int tiempo_us;
	
	/* Leer el número de interrupciones de forma atómica */
	interrupciones = timer2_numero_int;
	
	/* Leer el valor actual del contador (cuenta descendente desde 65535) */
	contador_actual = rTCNTO2;
	
	/* Calcular los ticks transcurridos en el periodo actual */
	/* Como cuenta descendente: ticks = valor_inicial - valor_actual */
	unsigned int ticks_periodo_actual = 65535 - contador_actual;
	
	/* Calcular el total de ticks:
	 * ticks totales = ticks de periodos completos + ticks del periodo actual */
	ticks_totales = (interrupciones * 65536) + ticks_periodo_actual;
	
	/* Convertir ticks a microsegundos:
	 * Cada tick = 1/(32 MHz) = 0.03125 µs
	 * Tiempo (µs) = ticks / 32
	 * Para evitar usar división en punto flotante: ticks * 1000000 / 32000000 = ticks / 32 */
	tiempo_us = ticks_totales / 32;
	
	return tiempo_us;
}
