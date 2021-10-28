/*
===============================================================================
 Name        : UART_Test.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

void confPin(void);
void confUart(void);

int main (void){

	confPin();
	confUart();
	//uint8_t info[] = "Hola mundo\t-\tElectr�nica Digital 3\t-\tFCEFyN-UNC \n\r";
	uint8_t info[]={0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
	while(1){
		UART_Send(LPC_UART0, info, sizeof(info), BLOCKING);
	}
	return 0;
}
void confPin(void){
	PINSEL_CFG_Type PinCfg;
	//configuraci�n pin de Tx y Rx
	PinCfg.Funcnum = 1;			// Funcion 01: (Tx y Rx)
	PinCfg.OpenDrain = 0;		// OD desactivado
	PinCfg.Pinmode = 0;			// Pull-Up
	PinCfg.Pinnum = 2;			// P0.2 para Tx (Conectar cable Blanco)
	PinCfg.Portnum = 0;			// Puerto 0
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 3;			// P0.3 para Rx (Conectar cable Verde)
	PINSEL_ConfigPin(&PinCfg);
	return;
}
void confUart(void){
	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;			// configuraci�n por defecto:
	UART_ConfigStructInit(&UARTConfigStruct);			// inicializa perif�rico
	UART_Init(LPC_UART0, &UARTConfigStruct);
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);	// Inicializa FIFO
	UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);	// Habilita transmisi�n
	UART_TxCmd(LPC_UART0, ENABLE);
	return;
}
