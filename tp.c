/*********************************************************************************************
* File£º	tp.c




* Author:	embest	
* Desc£º	LCD touch screen control function
* History:	
*********************************************************************************************/

/*--- include files ---*/
#include <string.h>
#include "tp.h"
#include "lcd.h"

void TSInt(void) __attribute__((interrupt("IRQ")));

void TS_Test(void)
{
		Lcd_TC();
		TS_init();
		Check_Sel();

//		Uart_Printf("\n Pixel: 320 X 240. Coordinate Rang in: (0,0) - (320,240)\n");
//		Uart_Printf("\nLCD TouchScreen Test Example(please touch LCD screen)\n");
}
/*--- function code ---*/
/*********************************************************************************************
* name:		TSInt
* func:		TouchScreen interrupt handler function
* para:		none
* ret:		none
* modify:
* comment:		
********************************************************************************************/
void TSInt(void)
{
    int   i;
    char fail = 0;
    ULONG tmp;
    ULONG Pt[6];

	// <X-Position Read>
	// TSPX(GPE4_Q4(+)) TSPY(GPE5_Q3(-)) TSMY(GPE6_Q2(+)) TSMX(GPE7_Q1(-))
    //       0               1                 1                0
	rPDATE=0x68;
	rADCCON=0x1<<2;			// AIN1

	DelayTime(1000);                // delay to set up the next channel
	for( i=0; i<5; i++ )
	{
		rADCCON |= 0x1;				// Start X-position A/D conversion
	    while( rADCCON & 0x1 );		// Check if Enable_start is low
    	while( !(rADCCON & 0x40) );	// Check ECFLG
	    Pt[i] = (0x3ff&rADCDAT);
	}
	// read X-position average value
	Pt[5] = (Pt[0]+Pt[1]+Pt[2]+Pt[3]+Pt[4])/5;

	tmp = Pt[5];	

    // <Y-Position Read>
	// TSPX(GPE4_Q4(-)) TSPY(GPE5_Q3(+)) TSMY(GPE6_Q2(-)) TSMX(GPE7_Q1(+))
    //       1               0                 0                1
	rPDATE=0x98;
	rADCCON=0x0<<2;		        	// AIN0

	DelayTime(1000);                // delay to set up the next channel
	for( i=0; i<5; i++ )
	{
    	rADCCON |= 0x1;             // Start Y-position conversion
	    while( rADCCON & 0x1 );     // Check if Enable_start is low
    	while( !(rADCCON & 0x40) ); // Check ECFLG
	    Pt[i] = (0x3ff&rADCDAT);
	}
	// read Y-position average value
	Pt[5] = (Pt[0]+Pt[1]+Pt[2]+Pt[3]+Pt[4])/5;

	if(!(CheckTSP|(tmp < Xmin)|(tmp > Xmax)|(Pt[5] < Ymin)|(Pt[5] > Ymax)))   // Is valid value?
	  {
		tmp = 320*(tmp - Xmin)/(Xmax - Xmin);   // X - position
//		Uart_Printf("X-Posion[AIN1] is %04d   ", tmp);



		Pt[5] = 240*(Pt[5] - Xmin)/(Ymax - Ymin);
//		Uart_Printf("  Y-Posion[AIN0] is %04d\n", Pt[5]);
      }
















    if(CheckTSP)
 	/*----------- check to ensure Xmax Ymax Xmin Ymin ------------*/
 	    DesignREC(tmp,Pt[5]);

	rPDATE = 0xb8;                  // should be enabled	
	DelayTime(3000);                // delay to set up the next channel	

    rI_ISPC = BIT_EINT2;            // clear pending_bit
}

/*********************************************************************************************
* name:		TS_init
* func:		initialize TouchScreen
* para:		none
* ret:		none
* modify:
* comment:		
*********************************************************************************************/
void TS_init(void)
{
    /* enable interrupt */
	rINTMOD=0x0;
	rINTCON=0x1;
    rI_ISPC |= BIT_EINT2;            // clear pending_bit

	// TSPX(GPE4_Q4(-)) TSPY(GPE5_Q3(-)) TSMY(GPE6_Q2(-)) TSMX(GPE7_Q1(+)) 
	//          1               1                0                 1
    rPUPE  = 0x0;	                 // Pull up
    rPDATE = 0xb8;                   // should be enabled	
    DelayTime(100); 

    rEXTINT |= 0x200;                // falling edge trigger
    pISR_EINT2=(unsigned *)TSInt;       // set interrupt handler

    rCLKCON = 0x7ff8;                // enable clock
    rADCPSR = 0x1;//0x4;             // A/D prescaler
    rINTMSK &= ~(BIT_EINT2);

    oneTouch = 0;
}

/*********************************************************************************************
* name:		TS_close
* func:		close TouchScreen
* para:		none
* ret:		none
* modify:
* comment:		
*********************************************************************************************/
void TS_close(void)
{
	/* Mask interrupt */
	rINTMSK |=BIT_GLOBAL|BIT_EINT2;
	pISR_EINT2 = (int)NULL;
}


















































void Lcd_TC(void)
{
	/* initial LCD controller */
	Lcd_Init();
	/* clear screen */
	Lcd_Clr();
	Lcd_Active_Clr();
	/* draw rectangle pattern */ 
	Lcd_Draw_Box(0,0,80,60,15);
	Lcd_Draw_Box(80,0,160,60,15);
	Lcd_Draw_Box(160,0,240,60,15);
	Lcd_Draw_Box(240,0,320,60,15);
	Lcd_Draw_Box(0,60,80,120,15);
	Lcd_Draw_Box(80,60,160,120,15);
	Lcd_Draw_Box(160,60,240,120,15);
	Lcd_Draw_Box(240,60,320,120,15);
	Lcd_Draw_Box(0,120,80,180,15);
	Lcd_Draw_Box(80,120,160,180,15);
	Lcd_Draw_Box(160,120,240,180,15);
	Lcd_Draw_Box(240,120,320,180,15);
	Lcd_Draw_Box(0,180,80,240,15);
	Lcd_Draw_Box(80,180,160,240,15);
	Lcd_Draw_Box(160,180,240,240,15);
	Lcd_Draw_Box(240,180,320,240,15);
	/* output ASCII symbol */
	Lcd_DspAscII6x8(37,26,BLACK,"0");
	Lcd_DspAscII6x8(117,26,BLACK,"1");
	Lcd_DspAscII6x8(197,26,BLACK,"2");
	Lcd_DspAscII6x8(277,26,BLACK,"3");
	Lcd_DspAscII6x8(37,86,BLACK,"4");
	Lcd_DspAscII6x8(117,86,BLACK,"5");
	Lcd_DspAscII6x8(197,86,BLACK,"6");
	Lcd_DspAscII6x8(277,86,BLACK,"7");
	Lcd_DspAscII6x8(37,146,BLACK,"8");
	Lcd_DspAscII6x8(117,146,BLACK,"9");
	Lcd_DspAscII6x8(197,146,BLACK,"A");
	Lcd_DspAscII6x8(277,146,BLACK,"B");
	Lcd_DspAscII6x8(37,206,BLACK,"C");
	Lcd_DspAscII6x8(117,206,BLACK,"D");
	Lcd_DspAscII6x8(197,206,BLACK,"E");
	Lcd_DspAscII6x8(277,206,BLACK,"F");
	Lcd_Dma_Trans();
	Delay(100);
}

/*********************************************************************************************
* name:		DesignREC
* func:		confirm the coordinate rang
* para:		none
* ret:		none
* modify:
* comment:		
*********************************************************************************************/
void DesignREC(ULONG tx, ULONG ty)
{
    int tm;

//  Uart_Printf("\n\r User touch coordinate(X,Y) is :");
//	Uart_Printf("(%04d",tx);	
//	Uart_Printf(",%04d)\n",ty);    
    if(oneTouch == 0)
     {
       Vx = tx;                   // Vx as Xmax
       Vy = ty;                   // Vy as Ymax
       oneTouch = 1;       		           
    }else{

    if(Vx < tx )
     {
       tm = tx; tx = Vx; Vx = tm; // tx as Xmin
     }
    if(Vy < ty )
     { 
       tm = ty; ty = Vy; Vy = tm; // ty as Ymin
     }

    Xmax = Vx;    Xmin = tx;
    Ymax = Vy;    Ymin = ty;

    oneTouch = 0;
	CheckTSP = 0;// has checked

	}// end if(oneTouch == 0)

}

void Check_Sel(void)
{
	char yn;

	do{

	  rINTMSK |=BIT_GLOBAL|BIT_EINT2;
//    Uart_Printf("\n\r Touch Screen coordinate Rang in:\n");
//    Uart_Printf("   (Xmin,Ymin) is :(%04d,%04d)\n",Xmin,Ymin);	
//	  Uart_Printf("   (Xmax,Ymax) is :(%04d,%04d)\n",Xmax,Ymax);	
//	  Uart_Printf("\n  To use current settings. Press N/n key. ");
//	  Uart_Printf("\n\n\r  Want to Set Again(Y/N)? ");	        	      
//	  yn = Uart_Getch();
      rI_ISPC = BIT_EINT2;              // clear pending_bit
      rINTMSK =~(BIT_GLOBAL|BIT_EINT2);
//	  if((yn == 0x59)|(yn == 0x79)|(yn == 0x4E)|(yn == 0x6E)) Uart_SendByte(yn);	  
	  if((yn == 0x59)|(yn == 0x79))
	    {
//	      Uart_Printf("\n\n Touch TSP's Cornor to ensure Xmax,Ymax,Xmax,Xmin");	      

         //Init X Y rectangle
          Xmax = 750;    Xmin = 200;
          Ymax = 620;    Ymin = 120;

	      oneTouch = 0;
	      CheckTSP = 1; // mask to check
	      while(CheckTSP);

	    }else break;	    

	}while(1);
}
/*********************************************************************************************
* name:		TS_Calibrar
* func:		Calibración del touchscreen tocando 4 esquinas
* para:		none
* ret:		none
* modify:
* comment:	Usa polling directo del hardware para detectar toques
*********************************************************************************************/
void TS_Calibrar(void)
{
	unsigned int i, j;
	ULONG x_vals[4], y_vals[4];
	ULONG Pt[5];
	ULONG tmp_x, tmp_y;
	
	/* Mensajes para cada esquina */
	INT8U* mensajes[4] = {
		(INT8U*)"Toque esquina superior izquierda",
		(INT8U*)"Toque esquina superior derecha",
		(INT8U*)"Toque esquina inferior derecha",
		(INT8U*)"Toque esquina inferior izquierda"
	};
	
	/* Posiciones donde dibujar las cruces */
	const int pos_x[4] = {20, 300, 300, 20};
	const int pos_y[4] = {20, 20, 220, 220};
	
	/* Pantalla inicial */
	Lcd_Clr();
	Lcd_DspAscII8x16(80, 100, BLACK, (INT8U*)"CALIBRACION");
	Lcd_DspAscII6x8(60, 120, BLACK, (INT8U*)"Toque las 4 esquinas");
	Lcd_Dma_Trans();
	Delay(200);
	
	/* Deshabilitar interrupciones durante calibración */
	rINTMSK |= BIT_EINT2;
	
	/* Asegurar que el ADC esté configurado */
	rCLKCON = 0x7ff8;
	rADCPSR = 0x1;
	
	/* Calibrar cada esquina */
	for (i = 0; i < 4; i++)
	{
		/* Limpiar y mostrar instrucciones */
		Lcd_Clr();
		Lcd_DspAscII6x8(30, 100, BLACK, mensajes[i]);
		
		/* Dibujar cruz en la posición */
		Lcd_Draw_HLine(pos_x[i] - 10, pos_x[i] + 10, pos_y[i], BLACK, 1);
		Lcd_Draw_VLine(pos_y[i] - 10, pos_y[i] + 10, pos_x[i], BLACK, 1);
		Lcd_Dma_Trans();
		
		/* Configurar para detectar toque */
		rPDATE = 0xb8;
		DelayTime(10000);
		
		/* Esperar a que se presione la pantalla (polling) */
		while ((rPDATG & 0x4) != 0)
		{
			/* Esperar toque */
		}
		
		/* Estabilización */
		DelayTime(5000);
		
		/* Leer posición X */
		rPDATE = 0x68;
		rADCCON = 0x1 << 2;
		DelayTime(1000);
		
		for (j = 0; j < 5; j++)
		{
			rADCCON |= 0x1;
			while (rADCCON & 0x1);
			while (!(rADCCON & 0x40));
			Pt[j] = (0x3ff & rADCDAT);
		}
		tmp_x = (Pt[0] + Pt[1] + Pt[2] + Pt[3] + Pt[4]) / 5;
		
		/* Leer posición Y */
		rPDATE = 0x98;
		rADCCON = 0x0 << 2;
		DelayTime(1000);
		
		for (j = 0; j < 5; j++)
		{
			rADCCON |= 0x1;
			while (rADCCON & 0x1);
			while (!(rADCCON & 0x40));
			Pt[j] = (0x3ff & rADCDAT);
		}
		tmp_y = (Pt[0] + Pt[1] + Pt[2] + Pt[3] + Pt[4]) / 5;
		
		/* Guardar valores */
		x_vals[i] = tmp_x;
		y_vals[i] = tmp_y;
		
		/* Indicar que se detectó */
		Lcd_Draw_Box(pos_x[i] - 15, pos_y[i] - 15, pos_x[i] + 15, pos_y[i] + 15, BLACK);
		Lcd_Dma_Trans();
		
		/* Esperar a que suelte */
		rPDATE = 0xb8;
		DelayTime(5000);
		while ((rPDATG & 0x4) == 0)
		{
			/* Esperar */
		}
		
		Delay(100);
	}
	
	/* Calcular límites */
	Xmin = (x_vals[0] + x_vals[3]) / 2;
	Xmax = (x_vals[1] + x_vals[2]) / 2;
	Ymin = (y_vals[0] + y_vals[1]) / 2;
	Ymax = (y_vals[2] + y_vals[3]) / 2;
	
	/* Mensaje final */
	Lcd_Clr();
	Lcd_DspAscII8x16(80, 100, BLACK, (INT8U*)"CALIBRADO!");
	Lcd_DspAscII6x8(80, 130, BLACK, (INT8U*)"Puede continuar");
	Lcd_Dma_Trans();
	Delay(200);
	
	/* Restaurar modo normal */
	CheckTSP = 0;
	oneTouch = 0;
	rPDATE = 0xb8;
	DelayTime(3000);
	rI_ISPC = BIT_EINT2;
	rINTMSK &= ~BIT_EINT2;
}