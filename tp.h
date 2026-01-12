/*********************************************************************************************
* File:	tp.H
* Author:	embest	
* Desc:	Touch Screen define file
* History:	
*********************************************************************************************/
#ifndef __TP_H__
#define __TP_H__

#include "def.h"
#include "44b.h"
#include "44blib.h"

void TS_Test(void);
void TS_init(void);
void TSInt(void);
void TS_close(void);
void TS_Calibrar(void);
int TS_HayEvento(void);
void TS_LeerEvento(INT16U *x, INT16U *y);
void Lcd_TC(void);
void DesignREC(ULONG tx, ULONG ty);
void Check_Sel(void);
//void user_irq1(void);

#endif /*__TP_H__*/
