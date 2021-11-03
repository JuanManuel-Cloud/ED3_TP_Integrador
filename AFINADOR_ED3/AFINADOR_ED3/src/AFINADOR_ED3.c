/*
===============================================================================
 Name        : TPFINAL_ED3.c
 Author      : Magnelli Tomas, Muñoz Jose Irigoin, Gil Juan Manuel.
 Consigna :
===============================================================================
*/
#include <stdlib.h>
#include <stdio.h>
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_systick.h"
#include "AFINADOR_ED3.h"

int main(void) {
	confGPIO();
	confUart();
	confTimers();
	confADC();
	SYSTICK_InternalInit(20);    // Inicializo para interrumpir cada 20 ms, con el clock interno: SystemCoreClock
	SYSTICK_IntCmd(ENABLE);      // Habilito interrupciones por systick
	confIntGPIO();


	GPIO_ClearValue(0,(0x380<<7));	// Inicializo en bajo salidas P0.[9:7] con 0011_1000_0000 = 0xB80

	NVIC_SetPriority(ADC_IRQn,5);
	NVIC_SetPriority(EINT3_IRQn,2);

	NVIC_EnableIRQ(EINT3_IRQn); 	// Habilito interrupciones por GPIO
	NVIC_EnableIRQ(ADC_IRQn);       // Habilito interrupciones por ADC



    while(1) {

    }
    return 0 ;
}

/**
 * =============================================================================================================================================
 * 																		CONFIG ZONE
 * =============================================================================================================================================
 * */

void confGPIO(void) {
	/**
	 * GPIO para resultados de la afinación
	 * */
	PINSEL_CFG_Type pinsel;
	pinsel.Portnum = 0;			// Puerto 0
	pinsel.Pinnum = 23;         // Pin P0.23
	pinsel.Pinmode = 2;			// Configuro pin P0.23 en PINMODE1: neither pull-up nor pull-down.
	pinsel.Funcnum = 1;			// Configuro pin P0.23 como entrada analogica para AD0.0
	PINSEL_ConfigPin(&pinsel);	// Inicializo pin P0.23
	pinsel.Funcnum = 0;			// Funcion MAT2.1 para salida al overrun
	for(uint8_t i=7;i<10;i++) {
		pinsel.Pinnum = i;		// CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO]
		PINSEL_ConfigPin(&pinsel); 	// Inicializo pines P0.[9:7]
		GPIO_SetDir(0,(1<<i),1); 	// Configuro pines P0.[9:7] como salidas
	}

	/**
	 * Configuracion GPIO para Tx y Rx para UART
	 * */

	PINSEL_CFG_Type Pinsel_UART;
	Pinsel_UART.Funcnum = 1;		// Funcion 01: (Tx y Rx)
	Pinsel_UART.OpenDrain = 0;		// OD desactivado
	Pinsel_UART.Pinmode = 0;		// Pull-Up
	Pinsel_UART.Pinnum = 2;			// P0.2 para Tx (Conectar cable Blanco)
	Pinsel_UART.Portnum = 0;		// Puerto 0
	PINSEL_ConfigPin(&Pinsel_UART);
	Pinsel_UART.Pinnum = 3;			// P0.3 para Rx (Conectar cable Verde)
	PINSEL_ConfigPin(&Pinsel_UART);

	/**
	 *  Configuración GPIO para keyboard
	 * */

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

void confTimers(void) {
	/**
	 * Configuraciones Timer 0 Match 1 para ejecutar conversiones ADC
	 * */
	TIM_TIMERCFG_Type timer;
	TIM_MATCHCFG_Type match;
	LPC_SC->PCLKSEL0  |= 0x4; // Selecciono inicialmente PCLK = CCLK
	timer.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer.PrescaleValue = 249; // TPR = (PR+1)/PCLK
	// MR1 configurado en 5 (Ti = TPR*(MR+1) = 2.5us*5 = 12.5us) => Muestreo cada 25us => Fs = 40kHz
	TIM_Init(LPC_TIM0,TIM_TIMER_MODE,&timer);
	match.MatchChannel = 1;
	match.MatchValue = 4;
	match.IntOnMatch = DISABLE;
	match.StopOnMatch = DISABLE;
	match.ResetOnMatch = ENABLE;
	match.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	TIM_ConfigMatch(LPC_TIM0,&match);
	TIM_ResetCounter(LPC_TIM0);
	TIM_Cmd(LPC_TIM0,ENABLE);

	/**
	 * Configuraciones Timer 2 match 0 para desplazamiento de bits en Teclado
	 * */

	TIM_TIMERCFG_Type timer2;
	TIM_MATCHCFG_Type match0;
	timer2.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2.PrescaleValue = 90;
	// MR1 (Ti = TPR*(MR+1) = 40us*1000 = 40ms)
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE, &timer2);
	match0.MatchChannel = 0;
	match0.MatchValue = 999;
	match0.IntOnMatch = ENABLE;
	match0.StopOnMatch = DISABLE;
	match0.ResetOnMatch = ENABLE;
	match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM2, &match0);
	TIM_Cmd(LPC_TIM2, ENABLE);
	NVIC_EnableIRQ(TIMER2_IRQn);

	return;
}

void confIntGPIO(void){
	//Interrupciones para el teclado
	FIO_IntCmd(2,(0xF<<4),1); 	// Inicializo interrupciones por flanco descendente en P2.[7:4]
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	return;
}

void confADC(void){
	ADC_Init(LPC_ADC,190000); // Energizo el ADC, Habilito el ADC bit PDN y Selecciono divisor de clock que llega al periferico como: CCLK/8
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE); // Canal AD0.1 seleccionado desde el reset (0x01)
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_1,DISABLE); // Canal AD0.1 seleccionado desde el reset (0x01)
	ADC_BurstCmd(LPC_ADC,0);                // Anulo bit 16 para seleccionar: Modo NO burst
	ADC_StartCmd(LPC_ADC,ADC_START_ON_MAT01);     // Configuro conversion ante MAT0.1 (REGISTRO MR1 DE TIMER0)
	ADC_EdgeStartConfig(LPC_ADC,1);			// Un 1 en bit 27 significa comienza por FLANCO DESCENDENTE en EMR1
	ADC_IntConfig(LPC_ADC,ADC_ADINTEN0,1);   // Habilito interrupciones por canal AD0.0
	return;
}

void confUart(void) {
	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;			// configuraci�n por defecto:
	UART_ConfigStructInit(&UARTConfigStruct);			// inicializa perif�rico
	UART_Init(LPC_UART0, &UARTConfigStruct);
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);	// Inicializa FIFO
	UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);	// Habilita transmisi�n
	UART_TxCmd(LPC_UART0, ENABLE);

	return;
}

/**
 * =============================================================================================================================================
 * 																		HANDLER ZONE
 * =============================================================================================================================================
 * */

void ADC_IRQHandler(void) {
	adcValue = ADC_ChannelGetData(LPC_ADC,0);

	old_sample_filt = current_sample_filt;
	current_sample_filt = adcValue;


	//============== COMIENZO FILTRO 1 ========================
//
//	buffer[aux] = adcValue;
//
//	for(uint32_t k = 0; k < BUFF_SIZE; k++){
//		parc_sum += buffer[k];
//	}
//	aux++;
//
//	if(aux >= BUFF_SIZE) aux = 0; //cuando superemos el nivel del buffer empezamos a pisar la pos 0 y continuamos
//
//	old_sample_filt = current_sample_filt;
//	current_sample_filt = parc_sum / BUFF_SIZE;
//	parc_sum = 0;

	//============== FIN FILTRO 1 ========================

	if(current_sample_filt >= 2048) {
		// Condicion nueva muestra mayor que "cero" (flanco ascendente)
		if(old_sample_filt < 2048) {
			is_crossing = 1;
			rising_sample = sample_count;
		}
	}
	else {
		// Condicion nueva muestra menor que "cero" (flanco descendente)
		if(old_sample_filt > 2048){
			is_crossing = 1;
			falling_sample = sample_count;
		}
	}

	sample_count ++;

	if(is_crossing && (abs(falling_sample - rising_sample) != 0)) {

//		freq_buff[aux_freq] = 16*(SAMP_FREQ / (2 * abs(falling_sample - rising_sample))); // Tsamp*num_samp_rise_fall = Tdet;
//
//		for(uint32_t j=0;j<FREQ_BUFF;j++){
//			if((freq_buff[j] < freq_buff[aux_freq]) && (freq_buff[j] < det_freq_aux)){
//				det_freq_aux = freq_buff[j]; // Se asigna cada vez que el elemento j del buffer es menor que el detectado y menor que el anterior
//			}
//		}

		old_det_freq = det_freq;
		det_freq = 16*(SAMP_FREQ / (2 * abs(falling_sample - rising_sample))); // Tsamp*num_samp_rise_fall = Tdet;

		//		aux_freq++;
//		if(aux_freq >= FREQ_BUFF) aux_freq=0;

		//============== COMIENZO FILTRO 2 ========================

//		freq_buff[aux_freq] = SAMP_FREQ / (2 * abs(falling_sample - rising_sample)); // Tsamp*num_samp_rise_fall = Tdet
//
//		for(uint32_t j=0;j<FREQ_BUFF;j++){
//			parc_sum_freq += freq_buff[j];
//		}
//		aux_freq++;
//
//		if(aux_freq >= FREQ_BUFF) aux_freq=0;
//
//		old_det_freq = det_freq;
//		det_freq = parc_sum_freq / FREQ_BUFF;
//		parc_sum_freq = 0;

		//============== FIN FILTRO 2 ========================

		//============== COMIENZO UART =======================

		//TODO: REVISAR ESTA FUNCIÓN, XQ' ESPERA CHAR* Y LE PASAMOS STRING, POR LO QUE DEBE HACER UN CASTEO INTERNO, ¿USA ASCII? ¿EXPLICA PROBLEMAS?

//		sprintf(UART_array,"%d\r\n", det_freq); // seteo el formato para el envío de datos de la freq det y lo guardo en el array
//		UART_Send(LPC_UART0, UART_array, sizeof(UART_array), BLOCKING); // envío el bloque de datos en forma de bloques

		//============== FIN UART =======================
	}

//	sprintf(UART_array,"%d\r\n",current_sample_filt); // seteo el formato para el envío de datos de la freq det y lo guardo en el array
//	UART_Send(LPC_UART0, UART_array, sizeof(UART_array), BLOCKING); // envío el bloque de datos en forma de bloques

	is_crossing = 0;
	
//	testFlagLED();

//	ADC_GlobalGetStatus(LPC_ADC,0);
//	ADC_GlobalGetStatus(LPC_ADC,1);
	return;
}

/**
 * Sistema antirrebotes utilizando SysTick
 * */

void EINT3_IRQHandler(void){
	NVIC_DisableIRQ(EINT3_IRQn); 	// Deshabilito interrupcion por EINT3
	NVIC_DisableIRQ(ADC_IRQn);  	// Deshabilito interrupcion por ADC

	SysTick->VAL = 0;            	// Valor inicial de cuenta en 0
	SysTick->CTRL &= SysTick->CTRL; // Anulo flag de fin de cuenta

	SYSTICK_Cmd(ENABLE);		 	// Habilito la cuenta del systick

	return;
}

void SysTick_Handler(void){
	port2GlobalInt = GPIO_GetIntStatus(2,4,1) | GPIO_GetIntStatus(2,5,1) | GPIO_GetIntStatus(2,6,1) | GPIO_GetIntStatus(2,7,1);

	if(port2GlobalInt){
		test_count ++ ;             	// Si sigue presionado se incrementa variable test_count
		if(test_count == 3){        	// Si se incremento 3 veces, se ingresa al switch(counter)
			push_response();
			SYSTICK_Cmd(DISABLE);
			FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
			NVIC_EnableIRQ(ADC_IRQn);   // Deshabilito interrupcion por ADC
			NVIC_EnableIRQ(EINT3_IRQn); // Vuelvo a habilitar interrupciones por EINT3
			test_count = 0;             // Anulo variable test_count para proximo antirrebote
		}
		else{} // Si no se incremento test_count 3 veces, pero sigue presionado el pulsador no se hace nada.
	}
	else{

		SYSTICK_Cmd(DISABLE);
		FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
		NVIC_EnableIRQ(ADC_IRQn);   // Deshabilito interrupcion por ADC
		NVIC_EnableIRQ(EINT3_IRQn);   // Vuelvo a habilitar interrupciones por EINT3
		test_count = 0; // Si no se incremento el test_count 3 veces, pero se solto el pulsador, se anula variable test_count para proximo antirrebote.
	}
	SysTick->CTRL &= SysTick->CTRL; // Anulo flag de fin de cuenta
	return;
}

void TIMER2_IRQHandler(void){
	//Atencion a rutina Timer 2, SHIFTREGISTER en filas de teclado matricial
	if(!(~(GPIO_ReadValue(2) | ~(0xF0) ))){ // Solo ejecuto desplazamiento cuando no se esta presionando ninguna tecla
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
	}
	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);       // Anulo bandera de itnerrupcion pendiente
	return;
}

/**
 * =============================================================================================================================================
 * 																		HELPER FUNCTIONS ZONE
 * =============================================================================================================================================
 * */

/**
 * Funcion para ordenar que codigo se ejecuta según la tecla presionada
 * */

void push_response(void) {
	if(GPIO_GetIntStatus(2,4,1)){
		COL0_ISR();
	}
	else if(GPIO_GetIntStatus(2,5,1)){
		COL1_ISR();
	}
	else if(GPIO_GetIntStatus(2,6,1)){
		COL2_ISR();
	}
	else if(GPIO_GetIntStatus(2,7,1)){
		COL3_ISR();
	}
}
/**
 * Handler para teclas de la columna 0
 * */

void COL0_ISR(void){
	switch(row_value){
	case 0 :		// [COL0;FIL0]
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
	break;
	}
}

/**
 * Handler para teclas de la columna 1
 * */

void COL1_ISR(void){
	switch(row_value){
	case 0 :		// [COL1;FIL0]
		// -1bpm
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

/**
 * Handler para teclas de la columna 2
 * */

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

/**
 * Handler para teclas de la columna 3 (Selección de frecuencias de referencia)
 * */

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

/**
 * Enciende el led correspondiente acorde a la afinación del instrumento
 * */

void testFlagLED(void) {
	if(det_freq != old_det_freq) {    // CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO]
		if(det_freq > (comp_freq + 50)) { // Si la diferencia es mayor a 50 Hz
			// La frecuencia detectada se encuentra a mas de 50 Hz por encima de la deseada
			// Encender un [P0.7 LED ROJO] (INDICA AL USUARIO BAJAR LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0, (1 << 7));               	// Pongo en alto P0.7
			GPIO_ClearValue(0, (3 << 8));			// Pongo en bajo P0.[9:8]
		}
		else if(det_freq < (comp_freq - 50)) {
			// La frecuencia detectada se encuentra a mas de 50 Hz por debajo de la deseada
			// Encender un [P0.9 LED AMARILLO] (INDICA AL USUARIO SUBIR LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0, (1 << 9));              	// Pongo en alto P0.9
			GPIO_ClearValue(0, (3 << 7));			// Pongo en bajo P0.[8:7]
		}
		else {
			// La frecuencia detectada se encuentra a menos de 50 Hz de la deseada
			// Encender un [P0.8 LED VERDE] (INDICA AL USUARIO MANTENER LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0, (1 << 8));               	// Pongo en alto P0.8
			GPIO_ClearValue(0, (5 << 7));			// Pongo en bajo P0.7 y P0.9
		}
	 }
}

/**
 * Función para calcular valor absoluto
 * */

uint32_t abs_calc(int32_t value) {
	uint32_t aux;
	aux = value > 0 ? value : - value;
	return aux;
}

