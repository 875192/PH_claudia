/*********************************************************************************************
* Fichero:		cola.h
* Autor:		
* Descrip:		Cola circular de depuración para tracking de eventos
* Version:		1.0
*********************************************************************************************/

#ifndef _COLA_H_
#define _COLA_H_

#include <stdint.h>

/*--- Configuración de la cola ---*/
#define COLA_SIZE 150			// Tamaño de la cola (número de eventos que almacena)

/* Dirección de memoria de la cola
 * Se ubica al final del espacio de memoria, antes de las pilas de los distintos modos
 * El espacio de memoria disponible es de 0xc000000 a 0xc7fffff (8 MB)
 * Las pilas están al final, así que ubicamos la cola justo antes
 */
#define COLA_ADDRESS 0xc700000	// Dirección base de la cola

/*--- Estructura de un evento en la cola ---*/
typedef struct {
	uint32_t instante;		// Momento exacto en que se produjo el evento (en microsegundos)
	uint8_t ID_evento;		// Identificador del tipo de evento
	uint32_t auxData;		// Datos auxiliares del evento (botón pulsado, ticks, etc.)
} EventoCola;

/*--- Estructura de la cola circular ---*/
typedef struct {
	uint32_t indice_escritura;		// Índice donde se escribirá el próximo evento
	uint32_t num_eventos;			// Número total de eventos escritos (para estadísticas)
	EventoCola eventos[COLA_SIZE];	// Array circular de eventos
} ColaDebug;

/*--- Variables visibles del módulo cola.c/cola.h ---*/
extern ColaDebug* cola_global;

/*--- Declaración de funciones visibles del módulo cola.c/cola.h ---*/

/**
 * Inicializa la cola de depuración.
 * Debe ser llamada al inicio del programa antes de usar la cola.
 */
void cola_init(void);

/**
 * Introduce un nuevo evento en la cola de depuración.
 * La cola es circular, por lo que si está llena, sobreescribirá los eventos más antiguos.
 * 
 * @param instant Instante de tiempo en que ocurrió el evento (en microsegundos)
 *                Normalmente se obtiene con timer2_count()
 * @param ID_evento Identificador del tipo de evento (definido en eventos.h)
 * @param auxData Datos auxiliares del evento (ej: número de botón, valor, etc.)
 */
void cola_depuracion(uint32_t instant, uint8_t ID_evento, uint32_t auxData);

/**
 * Obtiene el número total de eventos que se han registrado en la cola.
 * Este número puede ser mayor que COLA_SIZE si la cola ha dado varias vueltas.
 * 
 * @return Número total de eventos registrados desde la inicialización
 */
uint32_t cola_get_num_eventos(void);

/**
 * Obtiene el índice actual de escritura en la cola.
 * Útil para depuración.
 * 
 * @return Índice donde se escribirá el próximo evento (0 a COLA_SIZE-1)
 */
uint32_t cola_get_indice(void);

#endif /* _COLA_H_ */
