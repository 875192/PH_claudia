#include <stddef.h>
#include "sudoku_2025.h"
#include "tableros.h"
#include "../common/def.h"

/* *****************************************************************************
 * propaga el valor de una determinada celda
 * para actualizar las listas de candidatos
 * de las celdas en su su fila, columna y regi�n */
/* Recibe como par�metro la cuadr�cula, y la fila y columna de
 * la celda a propagar; no devuelve nada
 */
void candidatos_propagar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
                             uint8_t fila, uint8_t columna)
{
	uint8_t j, i , init_i, init_j;
    /* puede ayudar esta "look up table" a mejorar el rendimiento */
    const uint8_t init_region[NUM_FILAS] = {0, 0, 0, 3, 3, 3, 6, 6, 6};

    /* valor que se propaga */
    uint8_t valor = celda_leer_valor(cuadricula[fila][columna]);

    /* recorrer fila descartando valor de listas candidatos (incluyendo la celda origen) */
    for (j = 0; j < NUM_FILAS; j++) {
        celda_eliminar_candidato(&cuadricula[fila][j], valor);
    }

    /* recorrer columna descartando valor de listas candidatos (incluyendo la celda origen) */
    for (i = 0; i < NUM_FILAS; i++) {
        celda_eliminar_candidato(&cuadricula[i][columna], valor);
    }

    /* recorrer región descartando valor de listas candidatos */
    init_i = init_region[fila];
    init_j = init_region[columna];
    
    for (i = init_i; i < init_i + 3; i++) {
        for (j = init_j; j < init_j + 3; j++) {
            celda_eliminar_candidato(&cuadricula[i][j], valor);
        }
    }
}

/* *****************************************************************************
 * calcula todas las listas de candidatos (9x9)
 * necesario tras borrar o cambiar un valor (listas corrompidas)
 * retorna el numero de celdas vac�as
 */

int candidatos_actualizar_c(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
    int celdas_vacias = 0;
    uint8_t i, j;

    //borrar todos los candidatos de TODAS las celdas y contar las vacías
    for (i = 0; i < NUM_FILAS; i++) {
        for (j = 0; j < NUM_FILAS; j++) {
            // Resetear candidatos para TODAS las celdas (poner todos a 0, es decir, todos son candidatos)
            cuadricula[i][j] = cuadricula[i][j] & 0x007F; // Limpiar bits de candidatos [15,7]
            
            if (celda_leer_valor(cuadricula[i][j]) == 0) {
                celdas_vacias++;
            }
        }
    }
    

    //recalcular candidatos usando candidatos_propagar_c
    for (i = 0; i < NUM_FILAS; i++) {
        for (j = 0; j < NUM_FILAS; j++) {
            if (celda_leer_valor(cuadricula[i][j]) != 0) {
                // Celda con valor - propagar su valor para eliminar candidatos
                candidatos_propagar_c(cuadricula, i, j);
            }
        }
    }
    
    //retornar el numero de celdas vacías
    return celdas_vacias;
}


/* Init del sudoku en c�digo C invocando a propagar en arm
 * Recibe la cuadr�cula como primer par�metro
 * y devuelve en celdas_vacias el n�mero de celdas vac�as
 */

int candidatos_actualizar_c_arm(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
	int celdas_vacias = 0;
	uint8_t i, j;

	//borrar todos los candidatos de TODAS las celdas y contar las vacías
	for (i = 0; i < NUM_FILAS; i++) {
		for (j = 0; j < NUM_FILAS; j++) {
			// Resetear candidatos para TODAS las celdas (poner todos a 0, es decir, todos son candidatos)
			cuadricula[i][j] = cuadricula[i][j] & 0x007F; // Limpiar bits de candidatos [15,7]
			
			if (celda_leer_valor(cuadricula[i][j]) == 0) {
				celdas_vacias++;
			}
		}
	}

	//recalcular candidatos usando la función ARM
	for (i = 0; i < NUM_FILAS; i++) {
		for (j = 0; j < NUM_FILAS; j++) {
			if (celda_leer_valor(cuadricula[i][j]) != 0) {
				// Celda con valor - propagar usando función ARM optimizada
				candidatos_propagar_arm(cuadricula, i, j);
			}
		}
	}

	//retornar el numero de celdas vacías
	return celdas_vacias;
}

static int
cuadricula_candidatos_verificar(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS],
	 CELDA solucion[NUM_FILAS][NUM_COLUMNAS])
{
    int correcto = TRUE;
    uint8_t i;
    uint8_t j;

    // Verificar que los candidatos coincidan con la solución
    for (i = 0; i < NUM_FILAS && correcto; i++) {

    	for (j = 0; j < NUM_COLUMNAS && correcto; j++) {
            // Compara candidatos con solución
			// Extraer bits de candidatos [15,7] de ambas celdas
			uint16_t valor_cuadricula = cuadricula[i][j];
			uint16_t valor_solucion = solucion[i][j];
                
			// Los candidatos deben coincidir exactamente
			if (valor_cuadricula != valor_solucion) {
				correcto = FALSE;
			}
        }
    }

    return correcto;
}


/* *****************************************************************************
 * Funciones p�blicas
 * (pueden ser invocadas desde otro fichero) */

/* *****************************************************************************
 * programa principal del juego que recibe el tablero */

int
sudoku9x9(CELDA cuadricula_C_C[NUM_FILAS][NUM_COLUMNAS],
	CELDA cuadricula_C_ARM[NUM_FILAS][NUM_COLUMNAS],
	CELDA cuadricula_ARM_ARM[NUM_FILAS][NUM_COLUMNAS],
	CELDA cuadricula_ARM_ARM_ALL[NUM_FILAS][NUM_COLUMNAS],
	CELDA cuadricula_ARM_C[NUM_FILAS][NUM_COLUMNAS],
	CELDA solucion[NUM_FILAS][NUM_COLUMNAS])
{
    int celdas_vacias[5];    //numero de celdas aun vacías
    int correcto = 0;
    size_t i;

    /* calcula lista de candidatos, versión C */
    celdas_vacias[0] = candidatos_actualizar_c(cuadricula_C_C);

    /* Calcular candidatos para las otras versiones */
    celdas_vacias[1] = candidatos_actualizar_c_arm(cuadricula_C_ARM);
    celdas_vacias[2] = candidatos_actualizar_arm_arm(cuadricula_ARM_ARM);
    celdas_vacias[3] = candidatos_actualizar_all(cuadricula_ARM_ARM_ALL);
    celdas_vacias[4] = candidatos_actualizar_arm_c(cuadricula_ARM_C);

 	for (i=1; i < 5; ++i) {
		if (celdas_vacias[i] != celdas_vacias[0]) {
		return -1;
		}
	}

    /* verificar que la lista de candidatos calculada es correcta */
 	// cuadricula_C_C
    correcto = cuadricula_candidatos_verificar(cuadricula_C_C, solucion);
    if (correcto == FALSE) {
        return -2;
    }

    // cuadricula_C_ARM
    correcto = cuadricula_candidatos_verificar(cuadricula_C_ARM, solucion);
    if (correcto == FALSE) {
    	return -3;
    }

    // cuadricula_ARM_ARM
    correcto = cuadricula_candidatos_verificar(cuadricula_ARM_ARM, solucion);
	if (correcto == FALSE) {
		return -4;
	}

	// cuadricula_ARM_ARM_ALL
    correcto = cuadricula_candidatos_verificar(cuadricula_ARM_ARM_ALL, solucion);
	if (correcto == FALSE) {
		return -5;
	}

	// cuadricula_ARM_C
    correcto = cuadricula_candidatos_verificar(cuadricula_ARM_C, solucion);
	if (correcto == FALSE) {
		return -6;
	}

	return 0; // Todo correcto
}

int main(int argc, char const *argv[])
{
    sudoku9x9(cuadricula_C_C, cuadricula_C_ARM, cuadricula_ARM_ARM, cuadricula_ARM_ARM_ALL, cuadricula_ARM_C, solucion);
    return 0;
}
