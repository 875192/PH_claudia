	/* guarda para evitar inclusiones mÃºltiples (include guard) */
	#ifndef CELDA_H
	#define CELDA_H
	
	#include <inttypes.h>
	
	/* Cada celda se codifica en 16 bits
	 * bits [15,7]: los 9 bits mÃ¡s significativos representan el vector de candidatos,
	 * si el bit 7 + valor - 1 estÃ¡ a 0, valor es candidato, 1 en caso contrario
	 * bit 6: no empleado
	 * bit 5: error
	 * bit 4: pista
	 * bits [3,0]: valor de la celda
	 */
	
enum { BIT_CANDIDATOS = 7, BIT_NO_USADO = 6, BIT_ERROR = 5, BIT_PISTA = 4 };
	
	typedef uint16_t CELDA;
	
	/* *****************************************************************************
	 * elimina el candidato del valor almacenado en la celda indicada */
	__inline static void
	celda_eliminar_candidato(CELDA *celdaptr, uint8_t valor)
	{
		*celdaptr = *celdaptr | (0x0001 << (BIT_CANDIDATOS + valor - 1));
	}
	
	/* *****************************************************************************
	 * modifica el valor almacenado en la celda indicada */
	__inline static void
	celda_poner_valor(CELDA *celdaptr, uint8_t val)
	{
		*celdaptr = (*celdaptr & 0xFFF0) | (val & 0x000F);
	}
	
	/* *****************************************************************************
	 * extrae el valor almacenado en los 16 bits de una celda */
        __inline static uint8_t
        celda_leer_valor(CELDA celda)
        {
                return (celda & 0x000F);
        }

        /* *****************************************************************************
         * comprueba si el valor es candidato en la celda indicada */
        __inline static int
        celda_es_candidato(CELDA celda, uint8_t valor)
        {
                return (celda & (0x0001 << (BIT_CANDIDATOS + valor - 1))) == 0;
        }

        /* *****************************************************************************
         * comprueba si la celda es una pista */
        __inline static int
        celda_es_pista(CELDA celda)
        {
                return (celda & (1 << BIT_PISTA)) != 0;
        }

        /* *****************************************************************************
         * marca o limpia el bit de error de la celda */
        __inline static void
        celda_marcar_error(CELDA *celdaptr)
        {
                *celdaptr |= (1 << BIT_ERROR);
        }

        __inline static void
        celda_limpiar_error(CELDA *celdaptr)
        {
                *celdaptr &= ~(1 << BIT_ERROR);
        }
        #endif // CELDA_H
