/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: cola.c
 * Descripción: Implementación de una cola circular de depuración para almacenar eventos con timestamp y datos auxiliares
 */

#include "cola.h"
#include "timer2.h"
#include "eventos.h"
#include "44b.h"

/**
 * Buffer circular de eventos de depuración.
 * Se coloca en la sección ".cola_debug" y alineado a 4 bytes.
 */
static evento_t cola_buffer[COLA_MAX_EVENTOS] __attribute__((section(".cola_debug"), aligned(4)));

/**
 * Índice de escritura actual en la cola (siguiente posición libre).
 */
static volatile uint32_t cola_indice_esc = 0;

/**
 * Número de eventos actualmente almacenados en la cola.
 */
static volatile uint32_t cola_num_eventos = 0;

/**
 * Inicializa la cola de depuración.
 * Comportamiento:
 *  - Marca todos los eventos del buffer a cero.
 *  - Resetea los índices internos.
 */
void cola_init(void)
{
    uint32_t i;

    for (i = 0; i < COLA_MAX_EVENTOS; i++)
    {
        cola_buffer[i].ID_evento = 0;
        cola_buffer[i].instant = 0;
        cola_buffer[i].auxData = 0;
    }
    cola_indice_esc = 0;
    cola_num_eventos = 0;
}

/**
 * Inserta un evento en la cola de depuración.
 * Parámetros:
 *  - ID_evento: identificador del evento (1 byte).
 *  - instant: timestamp (por ejemplo microsegundos) asociado al evento.
 *  - auxData: dato auxiliar (p.ej. valor leído, ticks, etc.).
 */
void cola_depuracion(uint8_t ID_evento, uint32_t instant, uint32_t auxData)
{
    cola_buffer[cola_indice_esc].ID_evento = ID_evento;
    cola_buffer[cola_indice_esc].instant = instant;
    cola_buffer[cola_indice_esc].auxData = auxData;

    cola_indice_esc++;

    if (cola_indice_esc >= COLA_MAX_EVENTOS)
    {
        cola_indice_esc = 0;
    }
}
