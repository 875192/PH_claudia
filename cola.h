/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: cola.h
 * Descripción: Implementación de una cola circular de depuración para almacenar eventos con timestamp y datos auxiliares
 */

#ifndef COLA_H
#define COLA_H

#include <stdint.h>

/**
 * Número máximo de eventos en la cola.
 * Tamaño estimado: COLA_MAX_EVENTOS × sizeof(evento_t) = 256 × 12 = 3 KB
 */
#define COLA_MAX_EVENTOS 256

/**
 * Estructura que representa un evento de depuración.
 * - ID_evento: identificador pequeño del evento.
 * - instant: timestamp (µs) cuando ocurrió el evento.
 * - auxData: campo auxiliar para datos adicionales.
 */
typedef struct
{
    uint8_t ID_evento;
    uint32_t instant;
    uint32_t auxData;
} evento_t;

/**
 * Inicializa la cola de depuración.
 * Debe llamarse una vez al arrancar el sistema antes de usar cola_depuracion().
 */
void cola_init(void);

/**
 * Inserta un evento en la cola de depuración.
 * Parámetros:
 *  - ID_evento: identificador del evento (1 byte).
 *  - instant: timestamp asociado al evento (µs).
 *  - auxData: dato auxiliar (p.ej. valor leído).
 */
void cola_depuracion(uint8_t ID_evento, uint32_t instant, uint32_t auxData);

#endif /* COLA_H */
