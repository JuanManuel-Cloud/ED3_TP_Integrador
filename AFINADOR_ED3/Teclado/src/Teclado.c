/*
===============================================================================
 Name        : Teclado.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "stdlib.h"
#include "LPC17xx.h"
//#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

void confGPIO(void); 		// Prototipo de la funcion de conf. de puertos
//void confADC(void); 		//Prototipo de la funcion de conf. de interrupciones externas
void confTimers(void); 		// Prototipo de la funcion de conf. de timer
void confIntGPIO(void);

int main(void) {

	confGPIO();
	confIntGPIO();
	confTimers();
//	confADC();

    while(1) {

    }
    return 0 ;
}

void confGPIO(void){
	PINSEL_CFG_Type pinsel2;
	pinsel2.Portnum = 2;			// Puerto 2
	for(uint8_t j=0;j<4;j++){
		pinsel2.Pinnum = j;			// Pin P2.[3:0]
		PINSEL_ConfigPin(&pinsel2);	// Inicializo pin P2.[3:0]
		GPIO_SetDir(2,(1<<j),1); 	// Configuro pin P2.[3:0] como salidas
	}
	pinsel2.Pinmode = PINSEL_PINMODE_PULLUP;
	for(uint8_t j=4;j<8;j++){
		pinsel2.Pinnum = j;			// Pin P2.[7:4]
		PINSEL_ConfigPin(&pinsel2);	// Inicializo pin P2.[7:4]
		GPIO_SetDir(2,(1<<j),0); 	// Configuro pin P2.[7:4] como entradas
	}
	return;
}

void confTimers(void){
	//Configuraciones Timer 2
	TIM_TIMERCFG_Type timer2;
	TIM_MATCHCFG_Type match2;
	timer2.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2.PrescaleValue = 5;
	// MR1 (Ti = TPR*(MR+1) = 5us*1000 = 5ms)
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&timer2);
	match2.MatchChannel = 0;
	match2.MatchValue = 1000;
	match2.IntOnMatch = ENABLE;
	match2.StopOnMatch = DISABLE;
	match2.ResetOnMatch = ENABLE;
	match2.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM2,&match2);
	TIM_ResetCounter(LPC_TIM2);
	TIM_Cmd(LPC_TIM2,ENABLE);
	return;
}

void TIMER2_IRQHandler(void){
	//Atencion a rutina timer, SHIFTREGISTER en filas de teclado matricial

	switch(row_value){
	case 0 :
		LPC_GPIO2->FIOPIN |= 0x7;
		row_value++;
		break;
	case 1:
		LPC_GPIO2->FIOPIN |= 0xB;
		row_value++;
		break;
	case 2:
		LPC_GPIO2->FIOPIN |= 0xD;
		row_value++;
		break;
	case 3:
		LPC_GPIO2->FIOPIN |= 0xE;
		row_value++;
		break;
	case 4:
		row_value = 0;
		break;
	}
	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);       // Anulo bandera de itnerrupcion pendiente
	return;
}

void confIntGPIO(void){

	FIO_IntCmd(2,(0xF<<4),1); 	// Inicializo interrupciones por flanco descendente en P2.[7:4]
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	NVIC_EnableIRQ(EINT3_IRQn); // Habilito interrupciones por GPIO
	return;
}

void EINT3_IRQHandler(void){

	if(FIO_GetIntStatus(2,4,1)){ // COL 0
		switch(row_value){
		case 0 :		// [COL0;FIL0]
			// Ingreso a funcion AFINADOR
		break;
		case 1 :		// [COL0;FIL1]
		break;
		case 2 :		// [COL0;FIL2]
		break;
		case 3 :		// [COL0;FIL3]
		break;
		}
	}
	else if(FIO_GetIntStatus(2,5,1)){
		switch(row_value){
		case 0 :		// [COL1;FIL0]
			// -1bpm
			// Cambiar tabla buffer con salida ciclica para DMA
			// Redefinir TRANSFER_SIZE = # de datos de 32 bit a transferir (1 bit en 1?)
			// 111111111000000000111111111
			// 11111111110000000000
		break;
		case 1 :		// [COL1;FIL1]
			// +1bpm
		break;
		case 2 :		// [COL1;FIL2]
			// -10bpm
		break;
		case 3 :		// [COL1;FIL3]
			// +10bpm
		break;
		}
	}
	else if(FIO_GetIntStatus(2,6,1)){
		switch(row_value){
		case 0 :		// [COL2;FIL0]
		break;
		case 1 :		// [COL2;FIL1]
		break;
		case 2 :		// [COL2;FIL2]
		break;
		case 3 :		// [COL2;FIL3]
		break;
		}
	}
	else if(FIO_GetIntStatus(2,7,1)){
		switch(row_value){
		case 0 :		// [COL3;FIL0]
		break;
		case 1 :		// [COL3;FIL1]
		break;
		case 2 :		// [COL3;FIL2]
		break;
		case 3 :		// [COL3;FIL3]
		break;
		}
	}
	FIO_ClearInt(0,(0xF<<4)); // Limpio bandera de interrupcion por P0.6
	return;
}
