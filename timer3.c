/*********************************************************************************************
* Fichero:              timer3.c
* Autor:
* Descrip:              Gestión del timer3 para la eliminación de rebotes por máquina de estados
* Version:              1.0
*********************************************************************************************/

/*--- Ficheros de cabecera ---*/
#include "timer3.h"
#include "eventos.h"
#include "44b.h"
#include "44blib.h"

/*--- Constantes ---*/
#define TIMER3_1MS_COUNT           32000U        /* Cuenta necesaria para 1 ms con preescalado 0 y divisor 1/2 */
#define TIMER3_TICK_MS             1U
#define TIMER3_TRP_MS              30U           /* Retardo tras la pulsación */
#define TIMER3_TRD_MS              30U           /* Retardo tras soltar el botón */
#define TIMER3_MONITOR_MS          100U          /* Periodo de monitorización */
#define TIMER3_TIEMPO_AUTOREPETICION_MS  500U    /* Tiempo para activar auto-repetición */
#define TIMER3_PERIODO_AUTOREPETICION_MS 300U    /* Periodo de auto-incremento */

#define BOTON_MASK_EINT6           (1U << 6)    /* EINT6 usa el pin PG6 */
#define BOTON_MASK_EINT7           (1U << 7)    /* EINT7 usa el pin PG7 */

/*--- Variables internas ---*/
static volatile EstadoPulsador estado_actual = ESPERANDO_PULSACION;
static volatile uint32_t contador_ms = 0;
static volatile uint32_t monitor_ms = 0;
static volatile uint32_t tiempo_pulsacion_ms = 0;  /* Tiempo total que lleva pulsado */
static volatile uint8_t boton_en_proceso = 0;
static volatile unsigned int mascara_boton = 0;
static timer3_callback_t callback_validacion = 0;

/* Declaración de la ISR del timer3 */
void timer3_ISR(void) __attribute__((interrupt("IRQ")));

/*--- Funciones auxiliares internas ---*/
static inline int boton_sigue_pulsado(void)
{
        unsigned int estado = (~rPDATG) & mascara_boton;
        return (estado != 0);
}

static void timer3_detener(void)
{
        /* Parar el timer y enmascarar la interrupción */
        rTCON &= ~(1 << 16);             /* Bit 16 controla start/stop del timer3 */
        rINTMSK |= BIT_TIMER3;
}

static void timer3_preparar_tick(void)
{
        /* Configurar el valor de cuenta para 1 ms */
        rTCNTB3 = TIMER3_1MS_COUNT;
        rTCMPB3 = 0;

        /* Cargar los nuevos valores (manual update) */
        rTCON = (rTCON & ~(0xF << 16)) | (1 << 17);
        /* Activar auto-reload y arrancar el timer */
        rTCON = (rTCON & ~(0xF << 16)) | (1 << 19) | (1 << 16);
}

static void finalizar_antirrebote(void)
{
        /* Rehabilitar las interrupciones de los botones */
        rEXTINTPND = 0xf;
        rINTMSK &= ~(BIT_EINT4567);
        rI_ISPC |= BIT_EINT4567;

        /* Resetear el estado interno */
        estado_actual = ESPERANDO_PULSACION;
        contador_ms = 0;
        monitor_ms = 0;
        tiempo_pulsacion_ms = 0;
        boton_en_proceso = 0;
        mascara_boton = 0;
}

/*--- Código de las funciones públicas ---*/
void timer3_init(timer3_callback_t callback)
{
        callback_validacion = callback;
        estado_actual = ESPERANDO_PULSACION;
        contador_ms = 0;
        monitor_ms = 0;
        boton_en_proceso = 0;
        mascara_boton = 0;

        /* Enmascarar la interrupción del timer3 inicialmente */
        rINTMSK |= BIT_TIMER3;

        /* Configurar el vector de interrupción */
        pISR_TIMER3 = (unsigned) timer3_ISR;

        /* Configurar divisor (1/2) compartido con el timer2 */
        rTCFG1 = (rTCFG1 & ~(0xF << 12)) | (0x0 << 12);

        /* Cargar el valor inicial y dejar el timer parado */
        rTCNTB3 = TIMER3_1MS_COUNT;
        rTCMPB3 = 0;
        rTCON = (rTCON & ~(0xF << 16)) | (1 << 17);
        rTCON &= ~(0xF << 16);

        /* Limpiar posibles peticiones pendientes */
        rI_ISPC |= BIT_TIMER3;
}

int timer3_start_antirrebote(uint8_t boton_id)
{
        if (estado_actual != ESPERANDO_PULSACION)
        {
                return 0;
        }

        /* Seleccionar la máscara del puerto correspondiente */
        switch (boton_id)
        {
                case EVENTO_BOTON_IZQUIERDO:
                        mascara_boton = BOTON_MASK_EINT6;
                        break;
                case EVENTO_BOTON_DERECHO:
                        mascara_boton = BOTON_MASK_EINT7;
                        break;
                default:
                        return 0;
        }

        boton_en_proceso = boton_id;
        estado_actual = REBOTE_PRESION;
        contador_ms = TIMER3_TRP_MS;
        monitor_ms = TIMER3_MONITOR_MS;
        tiempo_pulsacion_ms = 0;

        /* Deshabilitar las interrupciones de los botones */
        rINTMSK |= BIT_EINT4567;

        /* Preparar y arrancar el temporizador */
        timer3_preparar_tick();

        /* Habilitar la interrupción del timer3 */
        rINTMSK &= ~(BIT_TIMER3);

        return 1;
}

int timer3_esta_ocupado(void)
{
        return (estado_actual != ESPERANDO_PULSACION);
}

/*--- Código de la ISR ---*/
void timer3_ISR(void)
{
        switch (estado_actual)
        {
                case REBOTE_PRESION:
                        if (contador_ms > 0)
                        {
                                contador_ms -= TIMER3_TICK_MS;
                        }
                        if (contador_ms == 0)
                        {
                                /* Notificar la pulsación validada (cuando se confirma que está pulsado) */
                                if (callback_validacion)
                                {
                                        callback_validacion(boton_en_proceso);
                                }
                                
                                estado_actual = MONITORIZANDO;
                                monitor_ms = TIMER3_MONITOR_MS;
                        }
                        break;

                case MONITORIZANDO:
                        /* Incrementar el tiempo total de pulsación */
                        tiempo_pulsacion_ms += TIMER3_TICK_MS;
                        
                        if (monitor_ms > 0)
                        {
                                monitor_ms -= TIMER3_TICK_MS;
                        }
                        if (monitor_ms == 0)
                        {
                                if (boton_sigue_pulsado())
                                {
                                        monitor_ms = TIMER3_MONITOR_MS;
                                        
                                        /* Comprobar si se debe activar auto-repetición */
                                        if (tiempo_pulsacion_ms >= TIMER3_TIEMPO_AUTOREPETICION_MS)
                                        {
                                                /* Activar auto-repetición solo para botón derecho */
                                                if (boton_en_proceso == EVENTO_BOTON_DERECHO)
                                                {
                                                        estado_actual = AUTOREPETICION;
                                                        contador_ms = TIMER3_PERIODO_AUTOREPETICION_MS;
                                                }
                                        }
                                }
                                else
                                {
                                        estado_actual = REBOTE_DEPRESION;
                                        contador_ms = TIMER3_TRD_MS;
                                }
                        }
                        break;

                case AUTOREPETICION:
                        /* Incrementar el tiempo total de pulsación */
                        tiempo_pulsacion_ms += TIMER3_TICK_MS;
                        
                        if (contador_ms > 0)
                        {
                                contador_ms -= TIMER3_TICK_MS;
                        }
                        if (contador_ms == 0)
                        {
                                if (boton_sigue_pulsado())
                                {
                                        /* Generar evento de auto-incremento */
                                        if (callback_validacion)
                                        {
                                                callback_validacion(boton_en_proceso);
                                        }
                                        
                                        /* Reiniciar contador para siguiente auto-incremento */
                                        contador_ms = TIMER3_PERIODO_AUTOREPETICION_MS;
                                }
                                else
                                {
                                        estado_actual = REBOTE_DEPRESION;
                                        contador_ms = TIMER3_TRD_MS;
                                }
                        }
                        break;

                case REBOTE_DEPRESION:
                        if (contador_ms > 0)
                        {
                                contador_ms -= TIMER3_TICK_MS;
                        }
                        if (contador_ms == 0)
                        {
                                /* Detener el temporizador */
                                timer3_detener();
                                
                                /* Finalizar el proceso de antirrebote */
                                finalizar_antirrebote();
                        }
                        break;

                default:
                        break;
        }

        /* Limpiar la petición de interrupción */
        rI_ISPC |= BIT_TIMER3;
}
