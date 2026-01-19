/*********************************************************************************************
 * File��	tp.H
 * Author:	embest
 * Desc��	Touch Screen define file
 * History:
 *********************************************************************************************/
#include "def.h"
#include "44b.h"
#include "44blib.h"
#ifndef __TP_H__
#define __TP_H__

#endif /*__TP_H__*/

/*--- global  variables ---*/
volatile int CheckTSP, oneTouch;
unsigned int Vx, Vy;
unsigned int Xmax;
unsigned int Ymax;
unsigned int Xmin;
unsigned int Ymin;

void TS_Test(void);
void TS_init(void);
void TSInt(void);
void TS_close(void);
void Lcd_TC(void);
void DesignREC(ULONG tx, ULONG ty);
void Check_Sel(void);

// Funciones de calibración de 5 puntos (máxima precisión: 5 toques × 20 muestras)
void ts_calibrate_5pt(int XRES, int YRES, int M);
int ts_read_calibrated(int *x, int *y);
void report_touch_data(int x, int y);
void ts_test_calibracion(void);
void ts_test_numeros(void);
void ts_draw_touch_point(int x, int y); // Dibuja punto visual para debug

// void user_irq1(void);