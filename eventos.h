/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: eventos.h
 * Descripción: Definición de IDs de eventos para la cola de depuración
 */

#ifndef EVENTOS_H
#define EVENTOS_H

#include <stdint.h>

typedef uint8_t evento_id_t;
typedef uint32_t timestamp_t;
typedef uint32_t auxdata_t;
#define EVT_BOTON_EINT6 0x13
#define EVT_BOTON_EINT7 0x14
#define EVT_LEDS_CAMBIO 0x15
#define EVT_TECLADO 0x20

#define EVT_CALCULO_TRP_TRD 0x02
#endif /* EVENTOS_H */
