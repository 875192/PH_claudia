/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 871961
 * Archivo: button.c
 * DescripciÃ³n: Funciones de manejo de los pulsadores (EINT6-7) y teclado matricial
 */

#include "button.h"
#include "8led.h"
#include "44blib.h"
#include "44b.h"
#include "def.h"
#include "cola.h"
#include "eventos.h"
#include "timer2.h"
#include "celda.h"
#include "sudoku_2025.h"
#include "tableros.h"
#include "sudoku_lcd.h"
#include "lcd.h"
#include "tp.h" // Para touchscreen

// AJUSTA ESTOS VALORES SEGÃšN DONDE SE DIBUJE TU TABLERO EN LA LCD
#define TABLERO_X_INI 34 // PÃ­xel X donde empieza el borde izquierdo del tablero
#define TABLERO_Y_INI 14 // PÃ­xel Y donde empieza el borde superior
#define ANCHO_CELDA 24   // Ancho de cada celda en pÃ­xeles (aprox)
#define ALTO_CELDA 24    // Alto de cada celda

// --- NUEVAS VARIABLES PARA EL ZOOM ---
static int modo_zoom = 0;             // 0 = Vista Normal, 1 = Vista Zoom Región, 2 = Vista Zoom Candidatos
static int region_zoom_fila = 0;      // 0, 3 o 6 (Fila inicial de la regiÃ³n)
static int region_zoom_col = 0;       // 0, 3 o 6 (Columna inicial de la regiÃ³n)
static int celda_candidatos_fila = 0; // Fila de la celda para seleccionar candidatos
static int celda_candidatos_col = 0;  // Columna de la celda para seleccionar candidatos

// Configuración visual del Zoom (Celdas grandes) - Más arriba para que no se corte
#define ZOOM_CELDA_SIZE 60
#define ZOOM_X_INI 80 // Centrado en pantalla 320px: (320-180)/2 = 70, ajustado a 80
#define ZOOM_Y_INI 40 // Más arriba para que no se corte

// Configuración de la flecha de salida
#define FLECHA_X 20
#define FLECHA_Y 50
#define FLECHA_WIDTH 30
#define FLECHA_HEIGHT 20

// Configuración visual del Zoom de candidatos (números 1-9 y 0) - Más hacia la derecha
#define CANDIDATOS_CELDA_SIZE 50
#define CANDIDATOS_X_INI 100 // Movido más hacia la derecha para evitar colisión con flecha
#define CANDIDATOS_Y_INI 30
#define CANDIDATOS_COLS 5 // 5 columnas: 1-5 arriba, 6-9+0 abajo

/**
 * CONSTANTES BOTONES
 */
#define TRP 200000
#define TEP 100000
#define TRD 50000
#define PULSACION_CORTA_US 500000
#define AUTOREP_T_US 300000

#define BOTON_EINT6 0
#define BOTON_EINT7 1
#define NUM_BOTONES 2

#define TICKS_MOSTRAR_ERROR 300 // Aumentado de 100 a 300 para mÃ¡s tiempo de error

/**
 * CONSTANTES TECLADO MATRICIAL
 */
#define TECLADO_ADDR_BASE 0x06000000
#define TECLADO_FILA0 (TECLADO_ADDR_BASE | 0xFD)
#define TECLADO_FILA1 (TECLADO_ADDR_BASE | 0xFB)
#define TECLADO_FILA2 (TECLADO_ADDR_BASE | 0xF7)
#define TECLADO_FILA3 (TECLADO_ADDR_BASE | 0xEF)

#define TECLADO_TRP_US 20000
#define TECLADO_TRD_US 100000

/**
 * TIPOS
 */
typedef enum
{
    e_reposo = 0,
    e_entrando,
    e_esperando,
    e_soltado
} estado_boton_t;

typedef enum
{
    ESTADO_INICIAL = 0,
    ESTADO_SELEC_FILA,
    ESTADO_SELEC_COLUMNA,
    ESTADO_SELEC_VALOR,
    ESTADO_MOSTRANDO_ERROR
} estado_juego_t;

typedef enum
{
    TECLADO_REPOSO = 0,
    TECLADO_ESPERANDO_TRP,
    TECLADO_ESCANEANDO,
    TECLADO_ESPERANDO_TRD
} estado_teclado_t;

typedef struct
{
    estado_boton_t estado;
    unsigned int timer;
    uint8_t codigo_int;
    uint8_t bit_pin;
    unsigned int inicio_pulsacion;
    unsigned int ultimo_auto;
    uint8_t auto_iniciado;
} contexto_boton_t;

/**
 * VARIABLES GLOBALES
 */
static contexto_boton_t botones[NUM_BOTONES] = {
    {e_reposo, 0, 4, 6, 0, 0, 0},
    {e_reposo, 0, 8, 7, 0, 0, 0}};

static estado_juego_t estado_juego = ESTADO_INICIAL;
static uint8_t numero_actual = 1;
static uint8_t fila_seleccionada = 0;
static uint8_t columna_seleccionada = 0;
static volatile uint8_t evento_boton_derecho = 0;
static volatile uint8_t evento_boton_izquierdo = 0;
static volatile uint8_t juego_iniciado = 0;
static int contador_error = 0;
static uint8_t pantalla_inicial_mostrada = 0;
static uint8_t tablero_dibujado = 0;

/* Teclado matricial */
static const uint8_t tecla_mapa[4][4] = {
    {1, 2, 3, 0xFF},
    {4, 5, 6, 0xFF},
    {7, 8, 9, 0xFF},
    {0xFF, 0, 0xFF, 0xFF}};

static estado_teclado_t teclado_estado = TECLADO_REPOSO;
static unsigned int teclado_timer = 0;
static volatile uint8_t evento_teclado_digito = 0xFF;
static volatile uint32_t tiempo_inicio;
static volatile uint32_t tiempo_fin;
static volatile uint32_t resta;

/* Variables para tiempo de partida */
static volatile uint32_t tiempo_inicio_partida = 0;
static uint32_t ultimo_tiempo_mostrado = 0;

/* Prototipos de funciones estÃ¡ticas */
static void procesar_rebote(int boton);
static inline uint8_t teclado_leer_columnas(volatile uint8_t *addr);
static uint8_t teclado_escanear(void);
static void estado_inicial(void);
static void estado_selec_fila(void);
static void estado_selec_columna(void);
static void estado_selec_valor(void);
static void estado_mostrando_error(void);
static void mostrar_tiempo_partida(void);
static void resetear_tiempo_partida(void);
static void mostrar_victoria_con_tiempo(void);
static void mostrar_partida_terminada(void);
static void limpiar_todos_eventos(void);
static void dibujar_zoom_region(void);
static void dibujar_flecha_salida(void);
static void dibujar_zoom_candidatos(uint8_t fila, uint8_t columna);
static int procesar_toque_candidatos(int x, int y);
static uint8_t es_celda_pista(uint8_t fila, uint8_t columna);
static uint8_t es_numero_candidato(CELDA celda, uint8_t numero);
static uint8_t es_numero_candidato(CELDA celda, uint8_t numero);

/* Prototipos adicionales para detecciÃ³n de errores */
static void limpiar_marcas_error(void);
static uint8_t verificar_tablero_completo(void);
static void finalizar_partida_completa(void);
static int detectar_y_marcar_errores_celda(uint8_t f, uint8_t c, uint8_t valor);
static void limpiar_errores_visuales(void);

/**
 * ISR EINT4567
 */
void Eint4567_ISR(void) __attribute__((interrupt("IRQ")));
void Eint4567_ISR(void)
{
    int which_int = rEXTINTPND;
    // Para el calculo del TRP y TRD
    tiempo_inicio = timer2_count();

    int i;

    for (i = 0; i < NUM_BOTONES; i++)
    {
        if (which_int == botones[i].codigo_int && botones[i].estado == e_reposo)
        {
            // Para el calculo del TRP y TRD
            cola_depuracion(2, tiempo_inicio, 2);

            botones[i].estado = e_entrando;

            rINTMSK |= BIT_EINT4567;
            botones[i].timer = timer2_count() + TRP;

            // CAMBIO: Cualquier botÃ³n puede iniciar el juego
            if (estado_juego == ESTADO_INICIAL)
            {
                juego_iniciado = 1;
                // Debug: mostrar quÃ© botÃ³n iniciÃ³
                if (i == BOTON_EINT6)
                    D8Led_symbol(6); // BotÃ³n izquierdo
                else
                    D8Led_symbol(7); // BotÃ³n derecho
            }

            break;
        }
    }

    rEXTINTPND = 0xf;
    rI_ISPC |= BIT_EINT4567;
}

/**
 * ISR EINT1
 */
void Eint1_ISR(void) __attribute__((interrupt("IRQ")));
void Eint1_ISR(void)
{
    // CAMBIO: Quitamos la condiciÃ³n "estado_juego != ESTADO_INICIAL"
    // para que el teclado funcione al principio y pueda despertar el juego.
    if (estado_juego != ESTADO_MOSTRANDO_ERROR &&
        teclado_estado == TECLADO_REPOSO)
    {
        teclado_estado = TECLADO_ESPERANDO_TRP;
        teclado_timer = timer2_count() + TECLADO_TRP_US;
    }

    rINTPND = BIT_EINT1;
    rEXTINTPND = (1 << 1);
    rI_ISPC = BIT_EINT1;
}

/**
 * InicializaciÃ³n general EINT y configuraciÃ³n de pines
 * Llama a eint1_init() al final.
 */
void Eint4567_init(void)
{
    rI_ISPC = 0x3ffffff;
    rEXTINTPND = 0xf;
    rINTMOD = 0x0;
    rINTCON = 0x1;
    rINTMSK &= ~(BIT_EINT4567);
    pISR_EINT4567 = (int)Eint4567_ISR;

    rPCONG = 0xffff;
    rPUPG = 0x0;
    rEXTINT = rEXTINT | 0x22222222;

    rEXTINTPND = 0xf;
    rI_ISPC |= BIT_EINT4567;

    eint1_init();
}

/**
 * Inicializar submÃ³dulo teclado
 * Configura EINT1 y el manejo de filas para el escaneo.
 */
void eint1_init(void)
{
    rEXTINT = (rEXTINT & ~(7 << 4)) | (2 << 4);
    pISR_EINT1 = (int)Eint1_ISR;
    rINTMSK &= ~BIT_EINT1;
    rEXTINTPND = (1 << 1);
    rI_ISPC = BIT_EINT1;
}

/**
 * Procesar rebote (mÃ¡quina de estados)
 * Parametros:
 *  - boton: Ã­ndice en el array botones (BOTON_EINT6/BOTON_EINT7)
 * Comportamiento:
 *  - implementa la mÃ¡quina de estados del antirrebote (entrando, esperando, soltado)
 *  - genera evento_boton_derecho/evento_boton_izquierdo segÃºn corresponda
 */
static void procesar_rebote(int boton)
{
    unsigned int ahora = timer2_count();
    contexto_boton_t *ctx = &botones[boton];

    switch (ctx->estado)
    {
    case e_reposo:
        break;

    case e_entrando:
        tiempo_fin = timer2_count();

        resta = tiempo_fin - tiempo_inicio;
        if ((int)(ahora - ctx->timer) >= 0)
        {
            cola_depuracion(4, resta, 4);

            ctx->estado = e_esperando;
            ctx->timer = ahora + TEP;
            ctx->inicio_pulsacion = ahora;
            ctx->ultimo_auto = ahora;
            ctx->auto_iniciado = 0;
        }
        break;

    case e_esperando:
        if ((int)(ahora - ctx->timer) >= 0)
        {
            if (leer_estado_boton(boton))
            {
                ctx->estado = e_soltado;
                ctx->timer = ahora + TRD;
            }
            else
            {
                ctx->timer = ahora + TEP;

                if (boton == BOTON_EINT7)
                {
                    if ((ahora - ctx->inicio_pulsacion) >= PULSACION_CORTA_US &&
                        (ahora - ctx->ultimo_auto) >= AUTOREP_T_US)
                    {
                        ctx->ultimo_auto = ahora;
                        if (!ctx->auto_iniciado)
                            ctx->auto_iniciado = 1;
                        evento_boton_derecho = 1;
                    }
                }
            }
        }
        break;

    case e_soltado:
        if ((int)(ahora - ctx->timer) >= 0)
        {
            ctx->estado = e_reposo;
            rINTMSK &= ~BIT_EINT4567;
            rEXTINTPND = 0xf;

            if (boton == BOTON_EINT7)
            {
                if (!ctx->auto_iniciado)
                    evento_boton_derecho = 1;
            }
            else if (boton == BOTON_EINT6)
            {
                evento_boton_izquierdo = 1;
            }
        }
        break;
    }
}

/**
 * Procesar mÃ¡quina de estados del juego
 * Llama al manejador correspondiente segÃºn estado_juego.
 */
void procesar_maquina_estados_botones(void)
{
    switch (estado_juego)
    {
    case ESTADO_INICIAL:
        estado_inicial();
        break;
    case ESTADO_SELEC_FILA:
        estado_selec_fila();
        // Actualizar tiempo durante el juego
        if (tiempo_inicio_partida > 0)
        {
            mostrar_tiempo_partida();
        }
        break;
    case ESTADO_SELEC_COLUMNA:
        estado_selec_columna();
        if (tiempo_inicio_partida > 0)
        {
            mostrar_tiempo_partida();
        }
        break;
    case ESTADO_SELEC_VALOR:
        estado_selec_valor();
        if (tiempo_inicio_partida > 0)
        {
            mostrar_tiempo_partida();
        }
        break;
    case ESTADO_MOSTRANDO_ERROR:
        estado_mostrando_error();
        if (tiempo_inicio_partida > 0)
        {
            mostrar_tiempo_partida();
        }
        break;
    }
}

/**
 * Procesar teclado (mÃ¡quina de estados)
 * Debe llamarse periÃ³dicamente para:
 *  - pasar por los estados TRP/TRD y escanear teclas
 *  - publicar evento_teclado_digito cuando detecta un dÃ­gito
 */
void procesar_teclado(void)
{
    unsigned int ahora = timer2_count();
    uint8_t digito;

    // CAMBIO: Permitimos procesar teclado en ESTADO_INICIAL
    // Solo bloqueamos si estamos mostrando un error (parpadeo)
    if (estado_juego == ESTADO_MOSTRANDO_ERROR)
    {
        teclado_estado = TECLADO_REPOSO;
        return;
    }

    switch (teclado_estado)
    {
    case TECLADO_REPOSO:
        break;

    case TECLADO_ESPERANDO_TRP:
        if ((int)(ahora - teclado_timer) >= 0)
            teclado_estado = TECLADO_ESCANEANDO;
        break;

    case TECLADO_ESCANEANDO:
        digito = teclado_escanear();
        if (digito != 0xFF)
        {
            evento_teclado_digito = digito;
            teclado_timer = ahora;
            teclado_estado = TECLADO_ESPERANDO_TRD;
        }
        break;

    case TECLADO_ESPERANDO_TRD:
        if (teclado_soltado())
        {
            if ((int)(ahora - teclado_timer) >= TECLADO_TRD_US)
                teclado_estado = TECLADO_REPOSO;
        }
        else
        {
            teclado_timer = ahora;
        }
        break;
    }
}

/**
 * Leer estado fÃ­sico del botÃ³n
 * Parametros:
 *  - boton: Ã­ndice in el array botones
 * Retorno:
 *  - 1 si el pin asociado estÃ¡ a 1 (libre), 0 si estÃ¡ a 0 (pulsado)
 */
inline int leer_estado_boton(int boton)
{
    return (rPDATG & (1 << botones[boton].bit_pin)) ? 1 : 0;
}

/**
 * Procesar antirrebote de todos los botones
 * Debe llamarse periÃ³dicamente (ej. desde el timer).
 */
void procesar_antirebote_botones(void)
{
    int i;
    for (i = 0; i < NUM_BOTONES; i++)
        procesar_rebote(i);
}

/**
 * Marcar error en celda y mostrar en display
 * Parametros:
 *  - f: fila
 *  - c: columna
 * Efectos:
 *  - marca bit de error en la celda, setea contador_error y muestra sÃ­mbolo
 */
static inline void mostrar_error_en_celda(uint8_t f, uint8_t c)
{
    cuadricula_C_C[f][c] |= 0x0020;
    contador_error = TICKS_MOSTRAR_ERROR;

    // Parpadear display para indicar error claramente - MÃ�S TIEMPO
    D8Led_symbol(0xE);
    Delay(20000); // Aumentado de 10000
    D8Led_symbol(0x0);
    Delay(20000); // Aumentado de 10000
    D8Led_symbol(0xE);
    Delay(20000); // Aumentado de 10000
    D8Led_symbol(0x0);
    Delay(20000); // Aumentado de 10000
    D8Led_symbol(0xE);
    Delay(20000); // Aumentado de 10000
}

/**
 * Estado: inicial del juego
 * Arranca el juego cuando se detectÃ³ inicio (cualquier botÃ³n o tecla del teclado matricial).
 */
static void estado_inicial(void)
{
    int i, j;

    // 1. Mostrar pantalla inicial si aÃºn no se ha hecho
    if (!pantalla_inicial_mostrada)
    {
        sudoku_mostrar_pantalla_inicial();
        pantalla_inicial_mostrada = 1;

        // Reseteamos variables para asegurar un estado limpio
        resetear_tiempo_partida();
        juego_iniciado = 0;
        evento_boton_derecho = 0;
        evento_boton_izquierdo = 0;
        evento_teclado_digito = 0xFF; // Limpiar eventos del teclado
        D8Led_symbol(0x0);
        return;
    }

    // 2. Comprobar si se ha pulsado algÃºn botÃ³n o tecla para empezar
    if (juego_iniciado || evento_teclado_digito != 0xFF)
    {
        // --- CORRECCIÃ“N CLAVE ---
        // Si el juego ha sido iniciado (flag de interrupciÃ³n activado),
        // NO avanzamos hasta que la mÃ¡quina de estados de los botones confirme
        // que el botÃ³n se ha soltado y ha vuelto al estado de reposo.
        // Esto evita que el evento "soltar botÃ³n" se cuele en el siguiente estado.
        if (botones[BOTON_EINT6].estado != e_reposo ||
            botones[BOTON_EINT7].estado != e_reposo ||
            teclado_estado != TECLADO_REPOSO)
        {
            return; // Esperamos en este estado hasta que se suelten los botones/teclas
        }

        // Feedback visual de arranque (opcional)
        D8Led_symbol(0xF);
    }
    else
    {
        // Si no se ha pulsado nada, seguimos esperando
        D8Led_symbol(0x0);
        return;
    }

    // 3. InicializaciÃ³n de la Partida
    // Si llegamos aquÃ­, es porque juego_iniciado = 1 O hay evento de teclado Y los inputs ya estÃ¡n sueltos.

    // Limpiamos el flag de inicio para la prÃ³xima vez
    juego_iniciado = 0;

    // IMPORTANTE: Borramos los eventos acumulados.
    // Como ya hemos validado el inicio, no necesitamos procesar estos eventos
    // en el siguiente estado.
    evento_boton_derecho = 0;
    evento_boton_izquierdo = 0;
    evento_teclado_digito = 0xFF; // Limpiar eventos del teclado

    // Configurar estado siguiente
    estado_juego = ESTADO_SELEC_FILA;

    // Variables de control de interfaz
    tablero_dibujado = 0; // Forzar redibujado del tablero

    // Variables de lÃ³gica de juego
    numero_actual = 0; // Empezamos pidiendo fila 0 (o la que seleccione)
    fila_seleccionada = 0;
    columna_seleccionada = 0;
    contador_error = 0;

    // 4. Copiar el tablero inicial (ARM -> C)
    // Restauramos el tablero original para empezar la partida limpia
    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < 9; j++)
        {
            cuadricula_C_C[i][j] = cuadricula_C_ARM[i][j];
        }
    }

    // Limpiar cualquier error previo
    limpiar_marcas_error();

    // Inicializar tiempo de partida
    tiempo_inicio_partida = timer2_count();
    ultimo_tiempo_mostrado = 0;

    // Marcar las pistas antes de actualizar candidatos
    marcar_pistas_iniciales(cuadricula_C_C);

    candidatos_actualizar_c(cuadricula_C_C);

    // Dibujar el tablero completo
    sudoku_dibujar_tablero_completo(cuadricula_C_C);
    tablero_dibujado = 1;

    numero_actual = 0;
    D8Led_symbol(0x0);

    // Mostrar tiempo inicial
    mostrar_tiempo_partida();
}

/**
 * Estado: seleccionar fila
 * Gestiona eventos de teclado y botones para elegir la fila.
 * Permite fila 0 para acabar la partida.
 */
static void estado_selec_fila(void)
{
    static uint8_t fila_anterior = 0xFF;

    if (evento_teclado_digito != 0xFF)
    {
        // Permitir dÃ­gitos del 0 al 9 (0 para salir)
        if (evento_teclado_digito >= 0 && evento_teclado_digito <= 9)
        {
            numero_actual = evento_teclado_digito;
            evento_teclado_digito = 0xFF;
            D8Led_symbol(numero_actual);
        }
        return;
    }

    if (evento_boton_derecho)
    {
        evento_boton_derecho = 0;

        // Quitar resaltado de fila anterior si existÃ­a y no era fila 0
        if (fila_anterior != 0xFF && numero_actual >= 1 && numero_actual <= 9)
        {
            uint8_t col;
            for (col = 0; col < 9; col++)
            {
                sudoku_resaltar_celda(numero_actual - 1, col, 0);
            }
        }

        numero_actual++;
        if (numero_actual > 9)
            numero_actual = 0; // Cambio: ahora permite ir a 0
        D8Led_symbol(numero_actual);

        // Resaltar nueva fila (solo si no es 0)
        if (numero_actual >= 1 && numero_actual <= 9)
        {
            uint8_t col;
            for (col = 0; col < 9; col++)
            {
                sudoku_resaltar_celda(numero_actual - 1, col, 1);
            }
            fila_anterior = numero_actual - 1;
        }
        else
        {
            fila_anterior = 0xFF; // No hay fila resaltada para fila 0
        }

        return;
    }

    if (evento_boton_izquierdo)
    {
        evento_boton_izquierdo = 0;

        // Si se seleccionÃ³ fila 0, acabar la partida
        if (numero_actual == 0)
        {
            // Limpiar resaltados si los hay
            if (fila_anterior != 0xFF)
            {
                uint8_t col;
                for (col = 0; col < 9; col++)
                {
                    sudoku_resaltar_celda(fila_anterior, col, 0);
                }
            }

            // Mostrar resultado de partida terminada con tiempo
            mostrar_partida_terminada();

            // REINICIO COMPLETO igual que en victoria
            estado_juego = ESTADO_INICIAL;
            pantalla_inicial_mostrada = 0; // Forzar mostrar pantalla inicial
            tablero_dibujado = 0;
            resetear_tiempo_partida();
            numero_actual = 0;
            fila_seleccionada = 0;
            columna_seleccionada = 0;
            contador_error = 0;
            fila_anterior = 0xFF;

            // LIMPIAR TODOS los eventos y estados
            limpiar_todos_eventos();

            // RESTAURAR tablero original
            uint8_t i, j;
            for (i = 0; i < NUM_FILAS; i++)
            {
                for (j = 0; j < 9; j++)
                {
                    cuadricula_C_C[i][j] = cuadricula_C_ARM[i][j];
                }
            }

            D8Led_symbol(0x0);
            return;
        }

        // Funcionalidad normal para filas 1-9
        fila_seleccionada = numero_actual - 1;

        // Quitar resaltado de toda la fila
        uint8_t col;
        for (col = 0; col < 9; col++)
        {
            sudoku_resaltar_celda(fila_seleccionada, col, 0);
        }
        fila_anterior = 0xFF;

        estado_juego = ESTADO_SELEC_COLUMNA;
        numero_actual = 1;
        D8Led_symbol(0xC);

        // Resaltar primera columna de la fila seleccionada
        sudoku_resaltar_celda(fila_seleccionada, 0, 1);
    }
}

/**
 * Estado: seleccionar columna
 * Gestiona eventos para elegir la columna y valida precondiciones.
 */
static void estado_selec_columna(void)
{
    uint8_t valor_previo;
    uint16_t candidatos;
    uint8_t es_pista;
    static uint8_t columna_anterior = 0;

    if (evento_teclado_digito != 0xFF)
    {
        if (evento_teclado_digito >= 1 && evento_teclado_digito <= 9)
        {
            numero_actual = evento_teclado_digito;
            evento_teclado_digito = 0xFF;
            D8Led_symbol(numero_actual);
        }
        return;
    }

    if (evento_boton_derecho)
    {
        evento_boton_derecho = 0;

        // Quitar resaltado de columna anterior
        sudoku_resaltar_celda(fila_seleccionada, columna_anterior, 0);

        numero_actual++;
        if (numero_actual > 9)
            numero_actual = 1;
        D8Led_symbol(numero_actual);

        // Resaltar nueva columna
        columna_anterior = numero_actual - 1;
        sudoku_resaltar_celda(fila_seleccionada, columna_anterior, 1);

        return;
    }

    if (!evento_boton_izquierdo)
        return;

    evento_boton_izquierdo = 0;
    columna_seleccionada = numero_actual - 1;

    // Quitar resaltado
    sudoku_resaltar_celda(fila_seleccionada, columna_seleccionada, 0);

    // Verificar si la celda es una pista (bit 4 = 1)
    es_pista = (cuadricula_C_C[fila_seleccionada][columna_seleccionada] & (1 << 4)) ? 1 : 0;

    // Si es pista, no se puede modificar - volver a selecciÃ³n de fila
    if (es_pista)
    {
        estado_juego = ESTADO_SELEC_FILA;
        numero_actual = 1;
        D8Led_symbol(0xF);
        return;
    }

    valor_previo = celda_leer_valor(cuadricula_C_C[fila_seleccionada][columna_seleccionada]);
    candidatos = (cuadricula_C_C[fila_seleccionada][columna_seleccionada] >> BIT_CANDIDATOS) & 0x1FF;

    // Esta verificaciÃ³n adicional es para celdas que tienen un valor pero no son pistas
    // (valores introducidos por el usuario anteriormente)
    if (valor_previo > 0 && candidatos == 0x1FF)
    {
        estado_juego = ESTADO_SELEC_FILA;
        numero_actual = 1;
        D8Led_symbol(0xF);
        return;
    }

    // Resaltar celda seleccionada para ediciÃ³n
    sudoku_resaltar_celda(fila_seleccionada, columna_seleccionada, 1);

    estado_juego = ESTADO_SELEC_VALOR;
    numero_actual = 0;
    D8Led_symbol(numero_actual);
}

/**
 * Estado: seleccionar valor
 * Gestiona la inserciÃ³n de un valor en la celda seleccionada,
 * detecta errores y verifica si la partida estÃ¡ completa.
 */
static void estado_selec_valor(void)
{
    uint8_t valor_previo;
    int error_detectado = 0;

    // --- Manejo de Entrada (Teclado y BotÃ³n Derecho) ---
    if (evento_teclado_digito != 0xFF)
    {
        numero_actual = evento_teclado_digito;
        evento_teclado_digito = 0xFF;
        D8Led_symbol(numero_actual);
        return;
    }

    if (evento_boton_derecho)
    {
        evento_boton_derecho = 0;
        numero_actual++;
        if (numero_actual > 9)
            numero_actual = 0;
        D8Led_symbol(numero_actual);
        return;
    }

    // --- ConfirmaciÃ³n (BotÃ³n Izquierdo) ---
    if (!evento_boton_izquierdo)
        return;

    evento_boton_izquierdo = 0;
    valor_previo = celda_leer_valor(cuadricula_C_C[fila_seleccionada][columna_seleccionada]);

    // Caso A: Borrar celda (Valor 0)
    if (numero_actual == 0)
    {
        cuadricula_C_C[fila_seleccionada][columna_seleccionada] &= 0xFF80; // Borrar valor y candidatos

        // CORRECCIÃ“N: Actualizar todos los candidatos del tablero cuando se borra un valor
        candidatos_actualizar_c(cuadricula_C_C);

        // CORRECCIÃ“N: Redibujar TODO el tablero para actualizar candidatos visualmente
        sudoku_dibujar_tablero_completo(cuadricula_C_C);

        // Quitar resaltado de selecciÃ³n
        sudoku_resaltar_celda(fila_seleccionada, columna_seleccionada, 0);

        // Volver a pedir fila
        estado_juego = ESTADO_SELEC_FILA;
        D8Led_symbol(0x0);
        return;
    }

    // Caso B: Introducir valor (1-9) - VERIFICACIÃ“N DE ERROR
    // Primero comprobamos si este valor choca con otros
    error_detectado = detectar_y_marcar_errores_celda(fila_seleccionada, columna_seleccionada, numero_actual);

    if (error_detectado)
    {
        // 1. Marcar tambiÃ©n la celda actual como error
        cuadricula_C_C[fila_seleccionada][columna_seleccionada] |= 0x0020;

        // 2. Escribir el numero temporalmente para que se vea (en estilo error)
        celda_poner_valor(&cuadricula_C_C[fila_seleccionada][columna_seleccionada], numero_actual);

        // 3. Quitar el recuadro de selecciÃ³n azul/verde para que se vea bien el rojo/negro de error
        sudoku_resaltar_celda(fila_seleccionada, columna_seleccionada, 0);

        // 4. Actualizar visualmente la celda actual
        sudoku_actualizar_celda(cuadricula_C_C, fila_seleccionada, columna_seleccionada);

        // 5. Cambiar a estado de error para esperar un tiempo y luego limpiar
        contador_error = TICKS_MOSTRAR_ERROR;
        estado_juego = ESTADO_MOSTRANDO_ERROR;

        // Feedback MEJORADO en 8Led - mÃ¡s tiempo de parpadeo
        D8Led_symbol(0xE);
        Delay(30000); // Mayor duraciÃ³n inicial
        D8Led_symbol(0x0);
        Delay(15000);
        D8Led_symbol(0xE);
        return;
    }

    // Caso C: Valor VÃ¡lido
    celda_poner_valor(&cuadricula_C_C[fila_seleccionada][columna_seleccionada], numero_actual);

    // CORRECCIÃ“N: Actualizar candidatos correctamente
    if (valor_previo != 0)
    {
        // Si habÃ­a un valor previo, recalcular todos los candidatos
        candidatos_actualizar_c(cuadricula_C_C);
    }
    else
    {
        // Si era una celda vacÃ­a, propagar la eliminaciÃ³n de candidatos
        candidatos_propagar_c(cuadricula_C_C, fila_seleccionada, columna_seleccionada);
    }

    // CORRECCIÃ“N: Redibujar TODO el tablero para mostrar cambios en candidatos
    sudoku_dibujar_tablero_completo(cuadricula_C_C);

    // Quitar resaltado
    sudoku_resaltar_celda(fila_seleccionada, columna_seleccionada, 0);

    // Verificar Victoria
    if (verificar_tablero_completo())
    {
        finalizar_partida_completa();
        return;
    }

    // Siguiente jugada
    estado_juego = ESTADO_SELEC_FILA;
    numero_actual = 0;
    D8Led_symbol(0x0);
}

/**
 * Estado: mostrando error
 * Temporiza la visualizaciÃ³n del error y vuelve al estado de selecciÃ³n.
 */
static void estado_mostrando_error(void)
{
    if (contador_error > 0)
    {
        contador_error--;

        // Efecto visual MEJORADO en LEDs con mÃ¡s tiempo visible
        if ((contador_error % 30) == 0) // Cambiado de 20 a 30
            D8Led_symbol(0xE);
        else if ((contador_error % 15) == 0) // Cambiado de 10 a 15
            D8Led_symbol(0x0);

        return;
    }

    // Cuando acaba el tiempo de mostrar error:

    // 1. Limpiar TODAS las celdas marcadas con error (la actual y las que chocaban)
    limpiar_errores_visuales();

    // 2. Revertir el valor invÃ¡lido de la celda del usuario a 0 (o al valor previo si prefieres)
    // AquÃ­ asumimos que si fallÃ³, se vacÃ­a.
    cuadricula_C_C[fila_seleccionada][columna_seleccionada] &= 0xFF80; // Limpiar valor

    // CORRECCIÃ“N: Actualizar candidatos despuÃ©s de limpiar el valor errÃ³neo
    candidatos_actualizar_c(cuadricula_C_C);

    // CORRECCIÃ“N: Redibujar todo el tablero para actualizar candidatos
    sudoku_dibujar_tablero_completo(cuadricula_C_C);

    // 3. Volver al juego
    estado_juego = ESTADO_SELEC_FILA;
    numero_actual = 0;
    D8Led_symbol(0x0);
}

/**
 * TECLADO MATRICIAL - FUNCIONES
 */
static inline uint8_t teclado_leer_columnas(volatile uint8_t *addr)
{
    return (*addr) & 0x0F;
}

/**
 * Indica si el teclado estÃ¡ suelto (pin de fila 1)
 * Retorno:
 *  - 1 si suelto, 0 si pulsado
 */
inline int teclado_soltado(void)
{
    return (rPDATG & (1 << 1)) ? 1 : 0;
}

/**
 * Escanear teclado matricial.
 *
 * Recorre las filas y compara el valor leÃ­do de las columnas con
 * mÃ¡scaras para identificar la columna activa. Devuelve el dÃ­gito
 * correspondiente segÃºn tecla_mapa, o 0xFF si no hay tecla detectada.
 *
 * Retorno:
 *  - dÃ­gito (0..9) si hay tecla detectable, 0xFF si no hay tecla.
 */
static uint8_t teclado_escanear(void)
{
    uint8_t fila, col, columnas_leidas;
    volatile uint8_t *direcciones[4] = {
        (volatile uint8_t *)TECLADO_FILA0,
        (volatile uint8_t *)TECLADO_FILA1,
        (volatile uint8_t *)TECLADO_FILA2,
        (volatile uint8_t *)TECLADO_FILA3};

    for (fila = 0; fila < 4; fila++)
    {
        columnas_leidas = teclado_leer_columnas(direcciones[fila]);

        if (columnas_leidas != 0x0F)
        {
            uint8_t mask_col[4] = {0x7, 0xB, 0xD, 0xE};
            for (col = 0; col < 4; col++)
            {
                if (columnas_leidas == mask_col[col])
                {
                    uint8_t digito = tecla_mapa[fila][col];
                    if (digito != 0xFF)
                        return digito;
                }
            }
        }
    }
    return 0xFF;
}

/**
 * Marca como pistas todas las celdas que tienen valor inicial
 */
void marcar_pistas_iniciales(CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS])
{
    uint8_t i, j;

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < 9; j++) // Solo las 9 primeras columnas
        {
            // Si la celda tiene un valor (no es 0), marcarla como pista
            if (celda_leer_valor(cuadricula[i][j]) > 0)
            {
                cuadricula[i][j] |= (1 << 4); // Activar bit de pista
            }
        }
    }
}

/**
 * Muestra el tiempo transcurrido de la partida en la esquina superior derecha
 */
static void mostrar_tiempo_partida(void)
{
    static uint32_t ultimo_segundo_mostrado = 0;

    // CORRECCIÃ“N: Verificar que el tiempo de inicio es vÃ¡lido
    if (tiempo_inicio_partida == 0)
        return;

    uint32_t tiempo_actual = timer2_count();

    // CORRECCIÃ“N: Verificar que el tiempo actual es mayor que el de inicio
    if (tiempo_actual < tiempo_inicio_partida)
        return;

    uint32_t tiempo_transcurrido_us = tiempo_actual - tiempo_inicio_partida;

    // Limitar el tiempo máximo para evitar overflow (10 minutos máximo para juego normal)
    if (tiempo_transcurrido_us > 600000000UL) // 10 minutos en microsegundos
        tiempo_transcurrido_us = 600000000UL;

    uint32_t segundos_actuales = tiempo_transcurrido_us / 1000000UL;

    // Solo actualizar si ha cambiado el segundo
    if (segundos_actuales != ultimo_segundo_mostrado)
    {
        uint32_t minutos = segundos_actuales / 60;
        uint32_t segundos = segundos_actuales % 60;

        char tiempo_str[8];

        // Formatear tiempo como MM:SS
        if (minutos > 9)
        {
            // Si pasa de 9 minutos, mostrar solo "09:59"
            minutos = 9;
            segundos = 59;
        }

        // CORRECCIÃ“N: Validar valores antes de formatear
        if (minutos <= 9 && segundos < 60 && segundos_actuales < 600) // Máximo 09:59
        {
            // Crear string manualmente (sin sprintf por compatibilidad)
            tiempo_str[0] = (minutos / 10) + '0';
            tiempo_str[1] = (minutos % 10) + '0';
            tiempo_str[2] = ':';
            tiempo_str[3] = (segundos / 10) + '0';
            tiempo_str[4] = (segundos % 10) + '0';
            tiempo_str[5] = '\0';

            // Limpiar Ã¡rea del tiempo (solo el Ã¡rea del tiempo, no la etiqueta)
            LcdClrRect(275, 12, 315, 20, WHITE);

            // Mostrar el tiempo actualizado
            Lcd_DspAscII6x8(275, 12, BLACK, (INT8U *)tiempo_str);

            // Transferir al LCD
            Lcd_Dma_Trans();

            ultimo_segundo_mostrado = segundos_actuales;
        }
    }
}

/**
 * Resetea las variables de tiempo de partida
 */
static void resetear_tiempo_partida(void)
{
    tiempo_inicio_partida = 0;
    ultimo_tiempo_mostrado = 0;
    // Resetear también la variable static de mostrar_tiempo_partida
    // (se hace automáticamente al ser static)
}

/**
 * Muestra mensaje de victoria incluyendo el tiempo final
 */
static void mostrar_victoria_con_tiempo(void)
{
    uint32_t tiempo_actual = timer2_count();
    uint32_t tiempo_transcurrido_us;

    // Protección contra overflow del timer
    if (tiempo_actual < tiempo_inicio_partida)
    {
        tiempo_transcurrido_us = 0;
    }
    else
    {
        tiempo_transcurrido_us = tiempo_actual - tiempo_inicio_partida;
    }

    // Limitar a 10 minutos máximo (600000000 microsegundos)
    if (tiempo_transcurrido_us > 600000000UL)
    {
        tiempo_transcurrido_us = 600000000UL;
    }

    uint32_t segundos_totales = tiempo_transcurrido_us / 1000000;
    uint32_t minutos = segundos_totales / 60;
    uint32_t segundos = segundos_totales % 60;

    // Limitar la visualización a 09:59
    if (minutos > 9)
    {
        minutos = 9;
        segundos = 59;
    }

    char tiempo_final_str[25];

    // Crear mensaje de tiempo final
    tiempo_final_str[0] = 'T';
    tiempo_final_str[1] = 'i';
    tiempo_final_str[2] = 'e';
    tiempo_final_str[3] = 'm';
    tiempo_final_str[4] = 'p';
    tiempo_final_str[5] = 'o';
    tiempo_final_str[6] = ':';
    tiempo_final_str[7] = ' ';
    tiempo_final_str[8] = (minutos / 10) + '0';
    tiempo_final_str[9] = (minutos % 10) + '0';
    tiempo_final_str[10] = ':';
    tiempo_final_str[11] = (segundos / 10) + '0';
    tiempo_final_str[12] = (segundos % 10) + '0';
    tiempo_final_str[13] = '\0';

    // Limpiar pantalla
    Lcd_Clr();

    /* Mensaje de victoria MÃ�S GRANDE */
    Lcd_DspAscII8x16(60, 60, BLACK, (INT8U *)"FELICIDADES!");
    Lcd_DspAscII8x16(40, 90, BLACK, (INT8U *)"Sudoku completado");

    /* Mostrar tiempo final MÃ�S GRANDE */
    Lcd_DspAscII8x16(80, 120, BLACK, (INT8U *)tiempo_final_str);

    /* InstrucciÃ³n para continuar MÃ�S VISIBLE */
    Lcd_DspAscII8x16(50, 160, BLACK, (INT8U *)"Pulse un boton");
    Lcd_DspAscII8x16(70, 180, BLACK, (INT8U *)"para jugar");

    Lcd_Dma_Trans();

    // ESPERAR MÃ�S TIEMPO antes de permitir continuar
    Delay(50000); // Pausa para poder leer el mensaje
}

/**
 * Muestra mensaje cuando la partida termina prematuramente (fila 0)
 */
static void mostrar_partida_terminada(void)
{
    uint32_t tiempo_actual = timer2_count();
    uint32_t tiempo_transcurrido_us;

    // Protección contra overflow del timer
    if (tiempo_actual < tiempo_inicio_partida)
    {
        tiempo_transcurrido_us = 0;
    }
    else
    {
        tiempo_transcurrido_us = tiempo_actual - tiempo_inicio_partida;
    }

    // Limitar a 10 minutos máximo (600000000 microsegundos)
    if (tiempo_transcurrido_us > 600000000UL)
    {
        tiempo_transcurrido_us = 600000000UL;
    }

    uint32_t segundos_totales = tiempo_transcurrido_us / 1000000;
    uint32_t minutos = segundos_totales / 60;
    uint32_t segundos = segundos_totales % 60;

    // Limitar la visualización a 09:59
    if (minutos > 9)
    {
        minutos = 9;
        segundos = 59;
    }

    char tiempo_final_str[25];

    // Crear mensaje de tiempo final
    tiempo_final_str[0] = 'T';
    tiempo_final_str[1] = 'i';
    tiempo_final_str[2] = 'e';
    tiempo_final_str[3] = 'm';
    tiempo_final_str[4] = 'p';
    tiempo_final_str[5] = 'o';
    tiempo_final_str[6] = ':';
    tiempo_final_str[7] = ' ';
    tiempo_final_str[8] = (minutos / 10) + '0';
    tiempo_final_str[9] = (minutos % 10) + '0';
    tiempo_final_str[10] = ':';
    tiempo_final_str[11] = (segundos / 10) + '0';
    tiempo_final_str[12] = (segundos % 10) + '0';
    tiempo_final_str[13] = '\0';

    // Limpiar pantalla
    Lcd_Clr();

    /* Mensaje de partida terminada */
    Lcd_DspAscII8x16(40, 60, BLACK, (INT8U *)"Partida terminada");
    Lcd_DspAscII6x8(60, 90, BLACK, (INT8U *)"Salida por el usuario");

    /* Mostrar tiempo transcurrido MÃ�S GRANDE */
    Lcd_DspAscII8x16(80, 120, BLACK, (INT8U *)tiempo_final_str);

    /* InstrucciÃ³n para continuar */
    Lcd_DspAscII8x16(50, 160, BLACK, (INT8U *)"Pulse un boton");
    Lcd_DspAscII8x16(70, 180, BLACK, (INT8U *)"para jugar");

    Lcd_Dma_Trans();

    // ESPERAR MÃ�S TIEMPO antes de permitir continuar
    Delay(50000); // Pausa para poder leer el mensaje
}

/**
 * Detectar y marcar errores en el tablero
 * Recorre el tablero y marca las celdas con errores evidentes
 * (por ejemplo, celdas con valores fuera de rango o duplicados en fila/columna)
 */
static void detectar_y_marcar_errores(void)
{
    uint8_t i, j, k, l;
    uint8_t fila[9], columna[9];
    uint16_t bloque[3][3];

    // Limpiar marcas de error anteriores
    limpiar_marcas_error();

    // Recorre todas las celdas
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 9; j++)
        {
            // Leer valor y verificar si es pista (bit 4 = 1)
            uint8_t valor = celda_leer_valor(cuadricula_C_C[i][j]);
            uint8_t es_pista = (cuadricula_C_C[i][j] & (1 << 4)) ? 1 : 0;

            // Si la celda es una pista, continuar (no marcar error)
            if (es_pista)
                continue;

            // Reiniciar datos de fila, columna y bloque
            for (k = 0; k < 9; k++)
            {
                fila[k] = 0;
                columna[k] = 0;
            }
            for (k = 0; k < 3; k++)
                for (l = 0; l < 3; l++)
                    bloque[k][l] = 0;

            // Volver a recorrer para detectar errores
            for (k = 0; k < 9; k++)
            {
                // Verificar fila
                uint8_t valor_fila = celda_leer_valor(cuadricula_C_C[i][k]);
                if (valor_fila > 0 && valor_fila < 10)
                {
                    if (fila[valor_fila - 1] == 0)
                        fila[valor_fila - 1] = 1;
                    else
                    {
                        cuadricula_C_C[i][k] |= 0x0020;
                        sudoku_actualizar_celda(cuadricula_C_C, i, k);
                    }
                }

                // Verificar columna
                uint8_t valor_columna = celda_leer_valor(cuadricula_C_C[k][j]);
                if (valor_columna > 0 && valor_columna < 10)
                {
                    if (columna[valor_columna - 1] == 0)
                        columna[valor_columna - 1] = 1;
                    else
                    {
                        cuadricula_C_C[k][j] |= 0x0020;
                        sudoku_actualizar_celda(cuadricula_C_C, k, j);
                    }
                }

                // Verificar bloque 3x3
                uint8_t fila_bloque = i / 3 * 3 + k / 3;
                uint8_t columna_bloque = j / 3 * 3 + k % 3;
                uint8_t valor_bloque = celda_leer_valor(cuadricula_C_C[fila_bloque][columna_bloque]);
                if (valor_bloque > 0 && valor_bloque < 10)
                {
                    if (bloque[k / 3][k % 3] == 0)
                        bloque[k / 3][k % 3] = 1;
                    else
                    {
                        cuadricula_C_C[fila_bloque][columna_bloque] |= 0x0020;
                        sudoku_actualizar_celda(cuadricula_C_C, fila_bloque, columna_bloque);
                    }
                }
            }
        }
    }
}

/**
 * Limpiar marcas de error en el tablero
 * Elimina las marcas de error (bit 5) en todas las celdas del tablero.
 */
static void limpiar_marcas_error(void)
{
    uint8_t i, j;

    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            cuadricula_C_C[i][j] &= ~(1 << 5); // Limpiar bit 5 (marca de error)
        }
    }
}

/**
 * Verificar si el tablero estÃ¡ completo
 * Retorno:
 *  - 1 si el tablero estÃ¡ completo (sin celdas vacÃ­as), 0 si hay celdas vacÃ­as
 */
static uint8_t verificar_tablero_completo(void)
{
    uint8_t i, j;
    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            uint8_t valor = celda_leer_valor(cuadricula_C_C[i][j]);
            // Si se encuentra una celda vacÃ­a (valor 0), retornar 0
            if (valor == 0)
                return 0;
        }
    }
    // Si no se encontraron celdas vacÃ­as, retornar 1
    return 1;
}

/**
 * Finalizar partida completa
 * Se llama cuando el tablero estÃ¡ completo y se ha verificado que no hay errores.
 * Muestra un mensaje de victoria y el tiempo de la partida.
 */
static void finalizar_partida_completa(void)
{
    uint8_t i, j;
    // Limpiar resaltados en el tablero
    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < NUM_COLUMNAS; j++)
        {
            sudoku_resaltar_celda(i, j, 0);
        }
    }

    // Mostrar tiempo de la partida
    mostrar_victoria_con_tiempo();

    // REINICIO COMPLETO - resetear TODAS las variables de estado
    estado_juego = ESTADO_INICIAL;
    pantalla_inicial_mostrada = 0; // Forzar mostrar pantalla inicial
    tablero_dibujado = 0;
    resetear_tiempo_partida();
    numero_actual = 0;
    fila_seleccionada = 0;
    columna_seleccionada = 0;
    contador_error = 0;

    // LIMPIAR TODOS los eventos y estados
    limpiar_todos_eventos();

    // RESTAURAR tablero original
    for (i = 0; i < NUM_FILAS; i++)
    {
        for (j = 0; j < 9; j++)
        {
            cuadricula_C_C[i][j] = cuadricula_C_ARM[i][j];
        }
    }
}

/**
 * Agregar funciÃ³n para limpiar todos los eventos
 */
static void limpiar_todos_eventos(void)
{
    evento_boton_derecho = 0;
    evento_boton_izquierdo = 0;
    juego_iniciado = 0;
    evento_teclado_digito = 0xFF;

    // TambiÃ©n resetear estados de botones si estÃ¡n en proceso
    int i;
    for (i = 0; i < NUM_BOTONES; i++)
    {
        if (botones[i].estado != e_reposo)
        {
            botones[i].estado = e_reposo;
        }
    }

    // Limpiar interrupciones pendientes
    rEXTINTPND = 0xf;
    rINTMSK &= ~BIT_EINT4567; // Habilitar interrupciones
}

/**
 * Busca conflictos para un valor en una posiciÃ³n especÃ­fica.
 * Si encuentra conflicto:
 * 1. Marca la celda conflictiva (la vieja) con el bit de error.
 * 2. Actualiza visualmente la celda conflictiva.
 * 3. Devuelve 1 para indicar que hubo error.
 */
static int detectar_y_marcar_errores_celda(uint8_t f, uint8_t c, uint8_t valor)
{
    int hay_error = 0;
    int k, l;

    // 1. Comprobar Fila
    for (k = 0; k < 9; k++)
    {
        if (k == c)
            continue; // No compararse consigo mismo

        // Leemos el valor de la celda vecina
        if (celda_leer_valor(cuadricula_C_C[f][k]) == valor)
        {
            // Â¡Conflicto encontrado!
            cuadricula_C_C[f][k] |= 0x0020;                // Activar Bit Error
            sudoku_actualizar_celda(cuadricula_C_C, f, k); // Repintar invertido
            hay_error = 1;
        }
    }

    // 2. Comprobar Columna
    for (k = 0; k < 9; k++)
    {
        if (k == f)
            continue;

        if (celda_leer_valor(cuadricula_C_C[k][c]) == valor)
        {
            cuadricula_C_C[k][c] |= 0x0020;
            sudoku_actualizar_celda(cuadricula_C_C, k, c);
            hay_error = 1;
        }
    }

    // 3. Comprobar Recuadro 3x3
    int inicio_f = (f / 3) * 3;
    int inicio_c = (c / 3) * 3;

    for (k = inicio_f; k < inicio_f + 3; k++)
    {
        for (l = inicio_c; l < inicio_c + 3; l++)
        {
            if (k == f && l == c)
                continue;

            if (celda_leer_valor(cuadricula_C_C[k][l]) == valor)
            {
                cuadricula_C_C[k][l] |= 0x0020;
                sudoku_actualizar_celda(cuadricula_C_C, k, l);
                hay_error = 1;
            }
        }
    }

    return hay_error;
}

/**
 * Limpia el bit de error de TODAS las celdas del tablero
 * y las redibuja para restaurar su estado normal.
 */
static void limpiar_errores_visuales(void)
{
    int i, j;
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 9; j++)
        {
            // Si la celda tiene el bit de error activo (0x20)
            if (cuadricula_C_C[i][j] & 0x0020)
            {
                cuadricula_C_C[i][j] &= ~0x0020;               // Apagar bit error
                sudoku_actualizar_celda(cuadricula_C_C, i, j); // Repintar normal
            }
        }
    }
}

/**
 * Procesa toques del touchscreen en el tablero de Sudoku.
 * Maneja tres modos:
 * - Modo 0: Vista normal - hace zoom a región 3x3
 * - Modo 1: Zoom región - permite seleccionar celda individual o salir
 * - Modo 2: Zoom candidatos - permite seleccionar número 1-9 o 0 para borrar
 */
void procesar_touchscreen_sudoku(void)
{
    int x, y;

    // Solo procesar si hay toque válido
    if (ts_read_calibrated(&x, &y) == 0)
    {
        // *** DEBUG: Dibujar punto donde se detectó el toque ***
        ts_draw_touch_point(x, y);

        // CORRECCIÓN: Si es el primer toque y no se ha dibujado el tablero, inicializar
        if (!tablero_dibujado && estado_juego == ESTADO_INICIAL)
        {
            // Inicializar juego como se hace en los botones
            estado_juego = ESTADO_SELEC_FILA;
            fila_seleccionada = 0;
            columna_seleccionada = 0;
            tiempo_inicio_partida = timer2_count();
            ultimo_tiempo_mostrado = 0;

            // Marcar las pistas antes de actualizar candidatos
            marcar_pistas_iniciales(cuadricula_C_C);
            candidatos_actualizar_c(cuadricula_C_C);

            // Dibujar el tablero completo
            sudoku_dibujar_tablero_completo(cuadricula_C_C);
            tablero_dibujado = 1;

            // Mostrar tiempo inicial
            mostrar_tiempo_partida();
        }
        // MODO 2: Zoom de candidatos (selección de número 1-9 o 0)
        if (modo_zoom == 2)
        {
            // Verificar primero si toca en la flecha de salida
            if (x >= FLECHA_X && x <= (FLECHA_X + FLECHA_WIDTH) &&
                y >= FLECHA_Y && y <= (FLECHA_Y + FLECHA_HEIGHT))
            {
                // Volver al zoom de región sin cambios
                modo_zoom = 1;
                dibujar_zoom_region();
                Delay(200);
                return;
            }

            int numero_seleccionado = procesar_toque_candidatos(x, y);

            if (numero_seleccionado >= 0) // -1 = toque fuera, 0-9 = número válido
            {
                uint8_t valor_actual = celda_leer_valor(cuadricula_C_C[celda_candidatos_fila][celda_candidatos_col]);

                // Verificar si la selección es válida según la nueva lógica
                int seleccion_valida = 0;
                if (numero_seleccionado == 0)
                {
                    // 0 solo es válido si hay un valor actual (para borrarlo)
                    seleccion_valida = (valor_actual != 0);
                }
                else
                {
                    // Números 1-9 solo son válidos si no hay valor actual y son candidatos
                    seleccion_valida = (valor_actual == 0) && es_numero_candidato(cuadricula_C_C[celda_candidatos_fila][celda_candidatos_col], numero_seleccionado);
                }

                if (seleccion_valida)
                {
                    // Aplicar el cambio al tablero
                    if (numero_seleccionado == 0)
                    {
                        // Borrar celda
                        cuadricula_C_C[celda_candidatos_fila][celda_candidatos_col] &= 0xFF80;
                        candidatos_actualizar_c(cuadricula_C_C);
                    }
                    else
                    {
                        // Poner nuevo valor
                        celda_poner_valor(&cuadricula_C_C[celda_candidatos_fila][celda_candidatos_col], numero_seleccionado);
                        candidatos_propagar_c(cuadricula_C_C, celda_candidatos_fila, celda_candidatos_col);
                    }

                    // Volver al zoom de región
                    modo_zoom = 1;
                    dibujar_zoom_region();
                }
                else
                {
                    // Número no válido - buscar y resaltar el número conflictivo como con los botones

                    // Buscar en la misma fila el número conflictivo
                    uint8_t numero_conflicto = numero_seleccionado;
                    int col_conflicto = -1, fila_conflicto = -1;

                    // Buscar en la fila
                    int c, f; // Variables de iteración
                    for (c = 0; c < 9; c++)
                    {
                        uint8_t valor = celda_leer_valor(cuadricula_C_C[celda_candidatos_fila][c]);
                        if (valor == numero_conflicto)
                        {
                            fila_conflicto = celda_candidatos_fila;
                            col_conflicto = c;
                            break;
                        }
                    }

                    // Si no se encontró en la fila, buscar en la columna
                    if (col_conflicto == -1)
                    {
                        for (f = 0; f < 9; f++)
                        {
                            uint8_t valor = celda_leer_valor(cuadricula_C_C[f][celda_candidatos_col]);
                            if (valor == numero_conflicto)
                            {
                                fila_conflicto = f;
                                col_conflicto = celda_candidatos_col;
                                break;
                            }
                        }
                    }

                    // Si no se encontró en fila/columna, buscar en la región 3x3
                    if (col_conflicto == -1)
                    {
                        int region_fila = (celda_candidatos_fila / 3) * 3;
                        int region_col = (celda_candidatos_col / 3) * 3;

                        for (f = region_fila; f < region_fila + 3 && col_conflicto == -1; f++)
                        {
                            for (c = region_col; c < region_col + 3; c++)
                            {
                                uint8_t valor = celda_leer_valor(cuadricula_C_C[f][c]);
                                if (valor == numero_conflicto)
                                {
                                    fila_conflicto = f;
                                    col_conflicto = c;
                                    break;
                                }
                            }
                        }
                    }

                    // Volver a vista normal y resaltar el número conflictivo
                    modo_zoom = 0;
                    Lcd_Clr();
                    sudoku_dibujar_marco_tablero();
                    sudoku_dibujar_tablero_completo(cuadricula_C_C);

                    if (col_conflicto != -1)
                    {
                        // Resaltar la celda que causa el conflicto
                        sudoku_resaltar_celda(fila_conflicto, col_conflicto, 1);
                    }

                    Lcd_Dma_Trans();

                    // Mostrar error en LED por 1 segundo
                    D8Led_symbol(E);
                    Delay(1000);

                    if (col_conflicto != -1)
                    {
                        // Quitar resaltado
                        sudoku_resaltar_celda(fila_conflicto, col_conflicto, 0);
                        Lcd_Dma_Trans();
                    }

                    // No volver al zoom - quedarse en vista normal como con los botones
                }
            }

            Delay(200); // Delay para evitar múltiples detecciones
            return;
        }

        // MODO 1: Zoom de región 3x3
        if (modo_zoom == 1)
        {
            // Verificar primero si toca en la flecha de salida
            if (x >= FLECHA_X && x <= (FLECHA_X + FLECHA_WIDTH) &&
                y >= FLECHA_Y && y <= (FLECHA_Y + FLECHA_HEIGHT))
            {
                // Salir del zoom - volver a vista normal
                modo_zoom = 0;

                // Redibujar tablero completo
                Lcd_Clr();
                sudoku_dibujar_marco_tablero();
                sudoku_dibujar_tablero_completo(cuadricula_C_C);
                Lcd_Dma_Trans();

                Delay(200);
                return;
            }

            // Calcular límites de la región zoom en pantalla
            int zoom_x_min = ZOOM_X_INI;
            int zoom_x_max = ZOOM_X_INI + (3 * ZOOM_CELDA_SIZE);
            int zoom_y_min = ZOOM_Y_INI;
            int zoom_y_max = ZOOM_Y_INI + (3 * ZOOM_CELDA_SIZE);

            // Si toca dentro de la región zoom, calcular qué celda específica
            if (x >= zoom_x_min && x <= zoom_x_max && y >= zoom_y_min && y <= zoom_y_max)
            {
                int col_rel = (x - zoom_x_min) / ZOOM_CELDA_SIZE;  // 0, 1, 2
                int fila_rel = (y - zoom_y_min) / ZOOM_CELDA_SIZE; // 0, 1, 2
                int fila_abs = region_zoom_fila + fila_rel;
                int col_abs = region_zoom_col + col_rel;

                // Verificar si es una pista (no se puede modificar)
                if (!es_celda_pista(fila_abs, col_abs))
                {
                    uint8_t valor_actual = celda_leer_valor(cuadricula_C_C[fila_abs][col_abs]);

                    // Si la celda ya tiene un valor del usuario (no es pista)
                    if (valor_actual != 0)
                    {
                        // COMPORTAMIENTO COMO BOTONES: Deshacer zoom, resaltar y explicar
                        // Volver a vista normal
                        modo_zoom = 0;
                        Lcd_Clr();
                        sudoku_dibujar_marco_tablero();
                        sudoku_dibujar_tablero_completo(cuadricula_C_C);

                        // Resaltar la celda momentáneamente para mostrar el problema
                        sudoku_resaltar_celda(fila_abs, col_abs, 1);
                        Lcd_Dma_Trans();
                        Delay(1000); // Mantener resaltado 1 segundo

                        // Quitar resaltado
                        sudoku_resaltar_celda(fila_abs, col_abs, 0);
                        Lcd_Dma_Trans();

                        Delay(200);
                        return;
                    }
                    else
                    {
                        // Celda vacía: proceder con zoom de candidatos
                        celda_candidatos_fila = fila_abs;
                        celda_candidatos_col = col_abs;

                        // Cambiar a modo zoom de candidatos
                        modo_zoom = 2;
                        dibujar_zoom_candidatos(fila_abs, col_abs);
                    }
                }
                else
                {
                    // Es una pista: también resaltar momentáneamente para indicar que no se puede modificar
                    // Volver a vista normal
                    modo_zoom = 0;
                    Lcd_Clr();
                    sudoku_dibujar_marco_tablero();
                    sudoku_dibujar_tablero_completo(cuadricula_C_C);

                    // Resaltar la celda momentáneamente
                    sudoku_resaltar_celda(fila_abs, col_abs, 1);
                    Lcd_Dma_Trans();
                    Delay(1000); // Mantener resaltado 1 segundo

                    // Quitar resaltado
                    sudoku_resaltar_celda(fila_abs, col_abs, 0);
                    Lcd_Dma_Trans();
                }
            }

            Delay(200);
            return;
        }

        // MODO 0: Vista normal - activar zoom de región
        int pixel_x = x - TABLERO_X_INI;
        int pixel_y = y - TABLERO_Y_INI;

        // Verificar si el toque está dentro del tablero
        if (pixel_x >= 0 && pixel_x < (ANCHO_CELDA * 9) &&
            pixel_y >= 0 && pixel_y < (ALTO_CELDA * 9))
        {
            // Calcular celda tocada
            int col = pixel_x / ANCHO_CELDA;
            int fila = pixel_y / ALTO_CELDA;

            // Calcular región 3x3 a la que pertenece
            int region_fila = (fila / 3) * 3; // 0, 3 o 6
            int region_col = (col / 3) * 3;   // 0, 3 o 6

            // Activar zoom en la región tocada
            modo_zoom = 1;
            region_zoom_fila = region_fila;
            region_zoom_col = region_col;

            dibujar_zoom_region();
        }

        Delay(100);
    }
}

/**
 * Verifica si una celda es una pista (no se puede modificar)
 * @param fila: fila de la celda (0-8)
 * @param columna: columna de la celda (0-8)
 * @return: 1 si es pista, 0 si no es pista
 */
static uint8_t es_celda_pista(uint8_t fila, uint8_t columna)
{
    return (cuadricula_C_C[fila][columna] & (1 << 4)) ? 1 : 0;
}

/**
 * Dibuja la región 3x3 en modo zoom con candidatos visibles
 */
static void dibujar_zoom_region(void)
{
    int i, j, k;

    Lcd_Clr();

    // Dibujar título indicando la región
    Lcd_DspAscII8x16(120, 10, BLACK, (INT8U *)"Region 3x3");

    // Dibujar flecha de salida
    dibujar_flecha_salida();

    // Dibujar las 9 celdas de la región ampliadas
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            int fila_abs = region_zoom_fila + i;
            int col_abs = region_zoom_col + j;

            // Posición en pantalla para celdas grandes
            int x_celda = ZOOM_X_INI + j * ZOOM_CELDA_SIZE;
            int y_celda = ZOOM_Y_INI + i * ZOOM_CELDA_SIZE;

            // Dibujar cuadrado de la celda
            Lcd_Draw_Box(x_celda, y_celda, x_celda + ZOOM_CELDA_SIZE, y_celda + ZOOM_CELDA_SIZE, BLACK);

            // Obtener valor y estado de la celda
            uint8_t valor = celda_leer_valor(cuadricula_C_C[fila_abs][col_abs]);
            uint8_t es_pista = es_celda_pista(fila_abs, col_abs);

            if (valor != 0)
            {
                // Celda tiene valor - dibujar número grande centrado
                char str[2];
                str[0] = '0' + valor;
                str[1] = '\0';

                // Centrar el número en la celda grande
                int txt_x = x_celda + (ZOOM_CELDA_SIZE / 2) - 4;
                int txt_y = y_celda + (ZOOM_CELDA_SIZE / 2) - 8;

                // Usar color diferente para pistas
                int color = es_pista ? BLACK : DARKGRAY;
                Lcd_DspAscII8x16(txt_x, txt_y, color, (INT8U *)str);
            }
            else if (!es_pista)
            {
                // Celda vacía no-pista - mostrar candidatos pequeños
                // Dividir celda en cuadrícula 3x3 para candidatos 1-9
                for (k = 1; k <= 9; k++)
                {
                    if (es_numero_candidato(cuadricula_C_C[fila_abs][col_abs], k))
                    {
                        int cand_fila = (k - 1) / 3; // 0, 1, 2
                        int cand_col = (k - 1) % 3;  // 0, 1, 2

                        // Posición del candidato dentro de la celda
                        int cand_x = x_celda + 8 + cand_col * 15;
                        int cand_y = y_celda + 8 + cand_fila * 15;

                        // Dibujar número candidato pequeño
                        char cand_str[2];
                        cand_str[0] = '0' + k;
                        cand_str[1] = '\0';

                        Lcd_DspAscII6x8(cand_x, cand_y, BLACK, (INT8U *)cand_str);
                    }
                }
            }
        }
    }
    Lcd_Dma_Trans();
}

/**
 * Dibuja la pantalla de selección de candidatos (números 1-9 y 0)
 * @param fila: fila de la celda a editar
 * @param columna: columna de la celda a editar
 */
static void dibujar_zoom_candidatos(uint8_t fila, uint8_t columna)
{
    int i, x, y;
    char titulo[50];
    char str[2];

    Lcd_Clr();

    // Título con información de la celda - construir string manualmente
    titulo[0] = 'C';
    titulo[1] = 'e';
    titulo[2] = 'l';
    titulo[3] = 'd';
    titulo[4] = 'a';
    titulo[5] = ' ';
    titulo[6] = '(';
    titulo[7] = '0' + (fila + 1);
    titulo[8] = ',';
    titulo[9] = '0' + (columna + 1);
    titulo[10] = ')';
    titulo[11] = '\0';
    Lcd_DspAscII8x16(110, 10, BLACK, (INT8U *)titulo); // Más hacia la derecha

    // Dibujar flecha de salida
    dibujar_flecha_salida();

    uint8_t valor_actual = celda_leer_valor(cuadricula_C_C[fila][columna]);
    if (valor_actual != 0)
    {
        // Construir string "Valor actual: X" manualmente
        titulo[0] = 'V';
        titulo[1] = 'a';
        titulo[2] = 'l';
        titulo[3] = 'o';
        titulo[4] = 'r';
        titulo[5] = ' ';
        titulo[6] = 'a';
        titulo[7] = 'c';
        titulo[8] = 't';
        titulo[9] = 'u';
        titulo[10] = 'a';
        titulo[11] = 'l';
        titulo[12] = ':';
        titulo[13] = ' ';
        titulo[14] = '0' + valor_actual;
        titulo[15] = '\0';
        Lcd_DspAscII6x8(120, 30, DARKGRAY, (INT8U *)titulo); // Más hacia la derecha
    }

    // Dibujar cuadrícula de números 1-9 (3x3) y 0 por separado
    // Fila 1: 1, 2, 3
    // Fila 2: 4, 5, 6
    // Fila 3: 7, 8, 9
    // Fila 4: 0 (centrado)

    for (i = 1; i <= 9; i++)
    {
        int fila_num = (i - 1) / 3;
        int col_num = (i - 1) % 3;

        x = CANDIDATOS_X_INI + col_num * CANDIDATOS_CELDA_SIZE;
        y = CANDIDATOS_Y_INI + fila_num * CANDIDATOS_CELDA_SIZE;

        // Dibujar cuadrado del número
        Lcd_Draw_Box(x, y, x + CANDIDATOS_CELDA_SIZE, y + CANDIDATOS_CELDA_SIZE, BLACK);

        // Dibujar el número centrado
        str[0] = '0' + i;
        str[1] = '\0';

        int txt_x = x + (CANDIDATOS_CELDA_SIZE / 2) - 4;
        int txt_y = y + (CANDIDATOS_CELDA_SIZE / 2) - 8;

        // Verificar si es candidato válido o el valor actual
        uint8_t es_candidato = es_numero_candidato(cuadricula_C_C[fila][columna], i);
        uint8_t es_valor_actual = (i == valor_actual);

        int color, fondo_color;
        // Nueva lógica:
        // - Si hay valor actual: NINGÚN número 1-9 seleccionable (todos con fondo blanco)
        // - Si no hay valor: solo candidatos válidos con fondo negro
        if (valor_actual != 0)
        {
            // Celda con valor: ningún número 1-9 seleccionable
            fondo_color = WHITE;
            color = BLACK;
        }
        else
        {
            // Celda vacía: solo candidatos válidos en negro
            if (es_candidato)
            {
                fondo_color = BLACK;
                color = WHITE;
            }
            else
            {
                fondo_color = WHITE;
                color = BLACK;
            }
        }

        // Llenar el fondo
        LcdClrRect(x + 1, y + 1, x + CANDIDATOS_CELDA_SIZE - 1, y + CANDIDATOS_CELDA_SIZE - 1, fondo_color);

        Lcd_DspAscII8x16(txt_x, txt_y, color, (INT8U *)str);
    }

    // Dibujar el 0 con lógica actualizada
    x = CANDIDATOS_X_INI + CANDIDATOS_CELDA_SIZE; // Centrado (segunda columna)
    y = CANDIDATOS_Y_INI + 3 * CANDIDATOS_CELDA_SIZE;

    Lcd_Draw_Box(x, y, x + CANDIDATOS_CELDA_SIZE, y + CANDIDATOS_CELDA_SIZE, BLACK);

    str[0] = '0';
    str[1] = '\0';

    int txt_x = x + (CANDIDATOS_CELDA_SIZE / 2) - 4;
    int txt_y = y + (CANDIDATOS_CELDA_SIZE / 2) - 8;

    if (valor_actual != 0)
    {
        // Hay valor: 0 con fondo negro (seleccionable para borrar)
        LcdClrRect(x + 1, y + 1, x + CANDIDATOS_CELDA_SIZE - 1, y + CANDIDATOS_CELDA_SIZE - 1, BLACK);
        Lcd_DspAscII8x16(txt_x, txt_y, WHITE, (INT8U *)str);
    }
    else
    {
        // No hay valor: 0 sombreado (no seleccionable)
        LcdClrRect(x + 1, y + 1, x + CANDIDATOS_CELDA_SIZE - 1, y + CANDIDATOS_CELDA_SIZE - 1, DARKGRAY);
        Lcd_DspAscII8x16(txt_x, txt_y, BLACK, (INT8U *)str);
    }

    // Añadir texto explicativo abajo a la izquierda
    char texto_ayuda[20];
    texto_ayuda[0] = 'f';
    texto_ayuda[1] = 'o';
    texto_ayuda[2] = 'n';
    texto_ayuda[3] = 'd';
    texto_ayuda[4] = 'o';
    texto_ayuda[5] = ' ';
    texto_ayuda[6] = 'n';
    texto_ayuda[7] = 'e';
    texto_ayuda[8] = 'g';
    texto_ayuda[9] = 'r';
    texto_ayuda[10] = 'o';
    texto_ayuda[11] = ' ';
    texto_ayuda[12] = '=';
    texto_ayuda[13] = ' ';
    texto_ayuda[14] = 'c';
    texto_ayuda[15] = 'a';
    texto_ayuda[16] = 'n';
    texto_ayuda[17] = 'd';
    texto_ayuda[18] = '.';
    texto_ayuda[19] = '\0';
    Lcd_DspAscII6x8(10, 220, DARKGRAY, (INT8U *)texto_ayuda);

    Lcd_Dma_Trans();
}

/**
 * Procesa un toque en la pantalla de candidatos
 * @param x: coordenada X del toque
 * @param y: coordenada Y del toque
 * @return: número seleccionado (0-9), -1 si toque inválido
 */
static int procesar_toque_candidatos(int x, int y)
{
    // Verificar si toca en la zona de números 1-9 (cuadrícula 3x3)
    int x_rel = x - CANDIDATOS_X_INI;
    int y_rel = y - CANDIDATOS_Y_INI;

    if (x_rel >= 0 && x_rel < (3 * CANDIDATOS_CELDA_SIZE) &&
        y_rel >= 0 && y_rel < (3 * CANDIDATOS_CELDA_SIZE))
    {
        int col_num = x_rel / CANDIDATOS_CELDA_SIZE;
        int fila_num = y_rel / CANDIDATOS_CELDA_SIZE;

        if (col_num >= 0 && col_num < 3 && fila_num >= 0 && fila_num < 3)
        {
            int numero = fila_num * 3 + col_num + 1; // 1-9
            return numero;
        }
    }

    // Verificar si toca el 0 (fila inferior, centrado)
    int x0_min = CANDIDATOS_X_INI + CANDIDATOS_CELDA_SIZE;
    int x0_max = x0_min + CANDIDATOS_CELDA_SIZE;
    int y0_min = CANDIDATOS_Y_INI + 3 * CANDIDATOS_CELDA_SIZE;
    int y0_max = y0_min + CANDIDATOS_CELDA_SIZE;

    if (x >= x0_min && x < x0_max && y >= y0_min && y < y0_max)
    {
        return 0;
    }

    // Si toca muy fuera de la zona de candidatos, cancelar
    if (x < CANDIDATOS_X_INI - 20 || x > CANDIDATOS_X_INI + 3 * CANDIDATOS_CELDA_SIZE + 20 ||
        y < CANDIDATOS_Y_INI - 20 || y > CANDIDATOS_Y_INI + 4 * CANDIDATOS_CELDA_SIZE + 20)
    {
        return -2; // Cancelar
    }

    return -1; // Toque inválido dentro de la zona
}

/**
 * Verifica si un número es candidato válido para una celda
 * @param celda: la celda a verificar
 * @param numero: número a verificar (1-9)
 * @return: 1 si es candidato válido, 0 si no lo es
 */
static uint8_t es_numero_candidato(CELDA celda, uint8_t numero)
{
    if (numero < 1 || numero > 9)
        return 0;

    // Los candidatos están en bits [15:7]
    // bit = 0 significa candidato válido, bit = 1 significa NO candidato
    uint16_t bit_candidato = 1u << (BIT_CANDIDATOS + (numero - 1));
    return (celda & bit_candidato) ? 0 : 1; // Invertir la lógica
}

/**
 * Dibuja una flecha "<" para salir del zoom
 */
static void dibujar_flecha_salida(void)
{
    // Dibujar un rectángulo de fondo para la flecha
    Lcd_Draw_Box(FLECHA_X, FLECHA_Y, FLECHA_X + FLECHA_WIDTH, FLECHA_Y + FLECHA_HEIGHT, BLACK);

    // Rellenar el fondo con color blanco
    LcdClrRect(FLECHA_X + 1, FLECHA_Y + 1, FLECHA_X + FLECHA_WIDTH - 1, FLECHA_Y + FLECHA_HEIGHT - 1, WHITE);

    // Dibujar la flecha "<" usando líneas
    int centro_x = FLECHA_X + FLECHA_WIDTH / 2;
    int centro_y = FLECHA_Y + FLECHA_HEIGHT / 2;

    // Línea diagonal superior de la flecha (de centro hacia arriba-izquierda)
    Lcd_Draw_Line(centro_x + 3, centro_y - 6, centro_x - 3, centro_y, BLACK, 1);
    Lcd_Draw_Line(centro_x + 2, centro_y - 6, centro_x - 4, centro_y, BLACK, 1);

    // Línea diagonal inferior de la flecha (de centro hacia abajo-izquierda)
    Lcd_Draw_Line(centro_x + 3, centro_y + 6, centro_x - 3, centro_y, BLACK, 1);
    Lcd_Draw_Line(centro_x + 2, centro_y + 6, centro_x - 4, centro_y, BLACK, 1);
}
