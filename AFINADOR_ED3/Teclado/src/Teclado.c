/*
===============================================================================
 Name        : Teclado.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include <stdlib.h>
#include <stdio.h>
#include "LPC17xx.h"
//#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_pinsel.h"

void confGPIO(void); 		// Prototipo de la funcion de conf. de puertos
//void confADC(void); 		//Prototipo de la funcion de conf. de interrupciones externas
void confTimers(void); 		// Prototipo de la funcion de conf. de timer
void confIntGPIO(void);
void Push_Response(void);
void COL0_ISR(void);
void COL1_ISR(void);
void COL2_ISR(void);
void COL3_ISR(void);

uint8_t test_count =0;
uint8_t row_value = 0;

int main(void) {

	confGPIO();
	confTimers();
	confIntGPIO();
//	confADC();

	NVIC_EnableIRQ(EINT3_IRQn); // Habilito interrupciones por GPIO
    while(1) {
    }
    return 0 ;
}

void confGPIO(void){
	PINSEL_CFG_Type pinsel2;
	pinsel2.Portnum = 2;			// Puerto 2
	pinsel2.Funcnum	= 0;
	for(uint8_t j=0;j<4;j++){
		pinsel2.Pinnum = j;			// Pin P2.[3:0]
		PINSEL_ConfigPin(&pinsel2);	// Inicializo pin P2.[3:0]
	}
	pinsel2.Pinmode = 0;
	for(uint8_t j=4;j<8;j++){
		pinsel2.Pinnum = j;			// Pin P2.[7:4]
		PINSEL_ConfigPin(&pinsel2);	// Inicializo pin P2.[7:4]
	}
	GPIO_SetDir(2,(0xF),1); 	// Configuro pin P2.[3:0] como salidas
	GPIO_SetDir(2,(0xF<<4),0); 	// Configuro pin P2.[7:4] como entradas
	return;
}

void confTimers(void){
	//Configuraciones Timer 2
	TIM_TIMERCFG_Type timer2;
	TIM_MATCHCFG_Type match2;
	timer2.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2.PrescaleValue = 85;
	// MR1 (Ti = TPR*(MR+1) = 40us*1000 = 40ms)
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&timer2);
	match2.MatchChannel = 0;
	match2.MatchValue = 999;
	match2.IntOnMatch = ENABLE;
	match2.StopOnMatch = DISABLE;
	match2.ResetOnMatch = ENABLE;
	match2.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM2,&match2);
//	TIM_ResetCounter(LPC_TIM2);
	TIM_Cmd(LPC_TIM2,ENABLE);
	NVIC_EnableIRQ(TIMER2_IRQn);
	return;
}

void SysTick_Handler(void){
	if( ~((GPIO_ReadValue(2)) & (0xF<<4)) & (0xF<<4) ){  // Operador logico para determinar si P2.[7:4] sigue presionado
		// 1111_1111_0001_1111 & 0000_0000_1111_0000 = 0010_0000
		test_count ++ ;             // Si sigue presionado se incrementa variable test_count
		if(test_count == 3){        // Si se incremento 3 veces, se ingresa al switch(counter)
			Push_Response();
			SysTick->CTRL &= SysTick->CTRL; // Anulo flag de fin de cuenta
			SYSTICK_Cmd(DISABLE);
			NVIC_EnableIRQ(EINT3_IRQn);     // Vuelvo a habilitar interrupciones por EINT3
			test_count=0;                   // Anulo variable test_count para proximo antirrebote
		}
		else{} // Si no se incremento test_count 3 veces, pero sigue presionado el pulsador no se hace nada.
	}
	else{
		SysTick->CTRL &= SysTick->CTRL;  // Anulo flag de fin de cuenta
		SYSTICK_Cmd(DISABLE);
		NVIC_EnableIRQ(EINT3_IRQn);   // Vuelvo a habilitar interrupciones por EINT3
		test_count = 0; // Si no se incremento el test_count 3 veces, pero se solto el pulsador, se anula variable test_count para proximo antirrebote.
	}
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	SYSTICK_ClearCounterFlag();
	return;
}
void Push_Response(void){
	switch( ~((GPIO_ReadValue(2)) & (0xF<<4)) & (0xF<<4) ){
	case(1<<4): // P2.4
		COL0_ISR();
	break;
	case(1<<5): // P2.5
		COL1_ISR();
	break;
	case(1<<6):
		COL2_ISR();
	break;
	case(1<<7):
		COL3_ISR();
	break;
	}
}

void TIMER2_IRQHandler(void){
	//Atencion a rutina timer, SHIFTREGISTER en filas de teclado matricial

	switch(row_value){
	case 0 :
		LPC_GPIO2->FIOPIN = 0x7; // 0111
		row_value = 3;
		break; // row_value sale valiendo 0
	case 3:
		LPC_GPIO2->FIOPIN = 0xB; // 1011
		row_value--; // row_value sale valiendo 1
		break;
	case 2:
		LPC_GPIO2->FIOPIN = 0xD; // 1101
		row_value--; // row_value sale valiendo 2
		break;
	case 1:
		LPC_GPIO2->FIOPIN = 0xE; // 1110
		row_value--; // row_value sale valiendo 3
		break;
	}
	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);       // Anulo bandera de itnerrupcion pendiente
	return;
}

void confIntGPIO(void){
	FIO_IntCmd(2,(0xF<<4),1); 	// Inicializo interrupciones por flanco descendente en P2.[7:4]
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	return;
}

void EINT3_IRQHandler(void){
	NVIC_DisableIRQ(EINT3_IRQn); // Deshabilito interrupcion por EINT3
	SYSTICK_InternalInit(20);    // Inicializo para interrumpir cada 20 ms, con el clock interno: SystemCoreClock
	SysTick->VAL = 0;            // Valor inicial de cuenta en 0
	SYSTICK_ClearCounterFlag();  // Anulo flag de fin de cuenta
	SYSTICK_IntCmd(ENABLE);      // Habilito interrupciones por systick
	SYSTICK_Cmd(ENABLE);
	FIO_ClearInt(2,(0xF<<4)); // Limpio bandera de interrupcion por P0.6
	return;
}

void COL0_ISR(void){
	switch(row_value){
	case 0 :		// [COL0;FIL0]
		// Ingreso a funcion AFINADOR
		printf("Columna: 0, Fila: %4d\r\n",row_value);
	break;
	case 1 :		// [COL0;FIL1]
		printf("Columna: 0, Fila: %4d\r\n",row_value);
	break;
	case 2 :		// [COL0;FIL2]
		printf("Columna: 0, Fila: %4d\r\n",row_value);
	break;
	case 3 :		// [COL0;FIL3]
		printf("Columna: 0, Fila: %4d\r\n",row_value);

	break; // <<---???
	}
}
void COL1_ISR(void){
	switch(row_value){
	case 0 :		// [COL1;FIL0]
		// -1bpm
		// Cambiar tabla buffer con salida ciclica para DMA
		// Redefinir TRANSFER_SIZE = # de datos de 32 bit a transferir (1 bit en 1?)
		// 111111111000000000111111111
		// 11111111110000000000
		printf("Columna: 1, Fila: %4d\r\n",row_value);
	break;
	case 1 :		// [COL1;FIL1]
		printf("Columna: 1, Fila: %4d\r\n",row_value);
		// +1bpm
	break;
	case 2 :		// [COL1;FIL2]
		printf("Columna: 1, Fila: %4d\r\n",row_value);
		// -10bpm
	break;
	case 3 :		// [COL1;FIL3]
		printf("Columna: 1, Fila: %4d\r\n",row_value);
		// +10bpm
	break;
	}
}
void COL2_ISR(void){
	switch(row_value){
	case 0 :		// [COL2;FIL0]
		printf("Columna: 2, Fila: %4d\r\n",row_value);
	break;
	case 1 :		// [COL2;FIL1]
		printf("Columna: 2, Fila: %4d\r\n",row_value);
	break;
	case 2 :		// [COL2;FIL2]
		printf("Columna: 2, Fila: %4d\r\n",row_value);
	break;
	case 3 :		// [COL2;FIL3]
		printf("Columna: 2, Fila: %4d\r\n",row_value);
	break;
	}
}
void COL3_ISR(void){
	switch(row_value){
	case 0 :		// [COL3;FIL0]
		printf("Columna: 3, Fila: %4d\r\n",row_value);
		comp_freq = 200;
	break;
	case 1 :		// [COL3;FIL1]
		printf("Columna: 3, Fila: %4d\r\n",row_value);
		comp_freq = 400;
	break;
	case 2 :		// [COL3;FIL2]
		printf("Columna: 3, Fila: %4d\r\n",row_value);
		comp_freq = 600;
	break;
	case 3 :		// [COL3;FIL3]
		printf("Columna: 3, Fila: %4d\r\n",row_value);
		comp_freq = 800;
	break;
	}
}


