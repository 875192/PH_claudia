/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: timer2.c
 * Descripción: Implementación de Timer2 para generar interrupciones periódicas y gestionar el antirrebote de botones y teclado matricial
 */

#include "timer2.h"
#include "44b.h"
#include "44blib.h"
#include "button.h"
#include "44b.h"

static volatile unsigned int timer2_numero_int = 0;
volatile uint32_t tiempo_inicio;
volatile uint32_t tiempo_fin;
volatile uint32_t resta;

/**
 * ISR de Timer2
 *
 * Descripción:
 *  - Ejecutada en cada interrupción de Timer2.
 *  - Incrementa el contador interno de interrupciones y llama a las
 *    rutinas que procesan el antirrebote de botones y el teclado.
 */
void timer2_ISR(void) __attribute__((interrupt("IRQ")));
void timer2_ISR(void)
{
    timer2_numero_int++;

    procesar_antirebote_botones();
    procesar_teclado();

    rI_ISPC |= BIT_TIMER2;
}

/**
 * Inicializa Timer2
 * Configura:
 *  - modo de interrupción vectorizada (IRQ),
 *  - prescaler/MUX para el timer2,
 *  - carga de registros TCNTB2/TCMPB2 y manual update.
 */
void timer2_init(void)
{
    rINTMOD = 0x0; /* IRQ vect. */
    rINTCON = 0x1; /* enable vectored IRQs */
    rINTMSK &= ~(BIT_TIMER2);

    pISR_TIMER2 = (unsigned)timer2_ISR;

    rTCFG0 = (rTCFG0 & ~(0xF << 8)) | (0x0 << 8);
    rTCFG1 = (rTCFG1 & ~(0xF << 8)) | (0x0 << 8);

    rTCNTB2 = 0xFFFF;
    rTCMPB2 = 0;

    rTCON |= (1 << 13);
    rTCON &= ~(1 << 13);
}

/**
 * Inicia Timer2
 * Acciones:
 *  - (Re)inicializa contador interno,
 *  - recarga TCNTB2 y realiza manual update,
 *  - pone el timer en modo start con auto-reload.
 */
void timer2_start(void)
{
    timer2_numero_int = 0;

    rTCNTB2 = 0xFFFF;
    rTCON |= (1 << 13);
    rTCON &= ~(1 << 13);

    rTCON &= ~(1 << 12);
    rTCON |= (1 << 12);

    rTCON |= (1 << 15);
}

/**
 * timer2_count
 * Devuelve un valor de tiempo relativo (µs aproximados) basado en el
 * número de underflows y el valor actual del contador del Timer2.
 * Retorno:
 *  - tiempo en microsegundos (aproximado). Ajustar divisor si la
 *    relación ticks/µs difiere en tu plataforma.
 */
unsigned int timer2_count(void)
{
    unsigned int val;
    unsigned int entero;
    unsigned long ticks;
    val = rTCNTO2;
    entero = timer2_numero_int;
    ticks = (unsigned long)entero * 65536 + (unsigned long)(65535 - val + 1);

    return (unsigned int)(ticks / 32);
}
