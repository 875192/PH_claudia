/*
 * Asignatura: Proyecto hardware
 * Fecha: 10/11/2025
 * Autores: Claudia Mateo Cuellar 971961
 * Archivo: main.c
 * Descripción: Programa principal
 */

#include "8led.h"
#include "button.h"
#include "led.h"
#include "timer.h"
#include "timer1.h"
#include "timer2.h"
#include "44blib.h"
#include "44b.h"
#include "cola.h"
#include "tableros.h"
#include <stdint.h>
#include "lcd.h"		/* Añadir este include */
#include "tp.h"			/* Para calibración touchscreen */
#include "sudoku_lcd.h" /* Para funciones de pantalla de Sudoku */

/*--- function declare ---*/
void Main(void);

/*--- extern function ---*/
extern void Lcd_Test();

/*--- function code ---*/
/*********************************************************************************************
 * name:		main
 * func:		c code entry
 * para:		none
 * ret:		none
 * modify:
 * comment:
 *********************************************************************************************/
void Main(void)
{
	sys_init(); /* Initial 44B0X's Interrupt, Port and UART */

	/* Inicializar LCD pero NO llamar a Lcd_Test() */
	Lcd_Init();		  /* Solo inicializar el hardware del LCD */
	Lcd_Clr();		  /* Limpiar pantalla */
	Lcd_Active_Clr(); /* Limpiar buffer activo */

	Eint4567_init();
	D8Led_init();
	timer_init();
	timer1_init();
	timer2_init();
	timer2_start();
	cola_init();
	leds_off();

	/* ============================================
	 * OPCIÓN 1: TEST DE CALIBRACIÓN TOUCHSCREEN
	 * Comenta esta línea y descomenta el while(1) de abajo
	 * para volver a tu aplicación normal
	 * ============================================ */

	TS_init(); // Inicializar touchscreen

	// Calibrar touchscreen (retorna después de calibrar)
	ts_test_calibracion();

	// Mostrar pantalla de instrucciones
	sudoku_mostrar_pantalla_inicial();

	// Esperar a que presione un botón para continuar
	// (esto lo maneja la máquina de estados)

	/* ============================================
	 * APLICACIÓN SUDOKU CON TOUCHSCREEN
	 * ============================================ */
	while (1)
	{
		procesar_maquina_estados_botones();
		procesar_touchscreen_sudoku();
	}
}
