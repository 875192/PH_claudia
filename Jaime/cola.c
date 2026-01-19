/*********************************************************************************************
* Fichero:		cola.c
* Autor:		
* Descrip:		Implementación de la cola circular de depuración
* Version:		1.0
*********************************************************************************************/

/*--- Ficheros de cabecera ---*/
#include "cola.h"
#include "eventos.h"
#include "timer2.h"
#include <stdint.h>

/*--- Variables globales del módulo ---*/

/* Puntero a la cola global en memoria
 * Se ubica en una dirección fija de memoria (COLA_ADDRESS)
 * definida en cola.h */
ColaDebug* cola_global = (ColaDebug*)COLA_ADDRESS;

/*--- Código de las funciones públicas ---*/

/**
 * Inicializa la cola de depuración.
 * Pone a cero todos los contadores y limpia la estructura.
 */
void cola_init(void)
{
	int i;

	/* Inicializar los índices de control */
	cola_global->indice_escritura = 0;
	cola_global->num_eventos = 0;

	/* Limpiar todos los eventos de la cola */
	for (i = 0; i < COLA_SIZE; i++)
	{
		cola_global->eventos[i].instante = 0;
		cola_global->eventos[i].ID_evento = 0;
		cola_global->eventos[i].auxData = 0;
	}
}

/**
 * Introduce un nuevo evento en la cola de depuración.
 * 
 * Implementa una cola circular (FIFO):
 * - Si la cola no está llena, añade el evento al final
 * - Si la cola está llena, sobreescribe el evento más antiguo
 * 
 * @param instant Instante de tiempo en microsegundos (normalmente de timer2_count())
 * @param ID_evento Identificador del tipo de evento (ver eventos.h)
 * @param auxData Datos auxiliares específicos del evento
 */
void cola_depuracion(uint32_t instant, uint8_t ID_evento, uint32_t auxData)
{
	/* Guardar el evento en la posición actual de escritura */
	cola_global->eventos[cola_global->indice_escritura].instante = instant;
	cola_global->eventos[cola_global->indice_escritura].ID_evento = ID_evento;
	cola_global->eventos[cola_global->indice_escritura].auxData = auxData;

	/* Incrementar el índice de escritura de forma circular */
	cola_global->indice_escritura++;
	if (cola_global->indice_escritura >= COLA_SIZE)
	{
		cola_global->indice_escritura = 0;  // Volver al inicio (circular)
	}

	/* Incrementar el contador total de eventos (para estadísticas) */
	cola_global->num_eventos++;
}

/**
 * Obtiene el número total de eventos registrados.
 * 
 * @return Número total de eventos desde la inicialización
 *         Puede ser mayor que COLA_SIZE si la cola ha dado varias vueltas
 */
uint32_t cola_get_num_eventos(void)
{
	return cola_global->num_eventos;
}

/**
 * Obtiene el índice actual de escritura.
 * 
 * @return Índice donde se escribirá el próximo evento (0 a COLA_SIZE-1)
 */
uint32_t cola_get_indice(void)
{
	return cola_global->indice_escritura;
}