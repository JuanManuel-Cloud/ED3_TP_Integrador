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
	confIntGPIO();
	confUart();
	confTimers();
	confADC();

	GPIO_ClearValue(0,(0xB80<<7));	// Inicializo en bajo salidas P0.11 y P0.[9:7] con 1011_1000_0000 = 0xB80

	NVIC_EnableIRQ(ADC_IRQn);           // Habilito interrupciones por ADC

    while(1) {
    	if(det_freq != old_det_freq){
    		printf("%d\r\n",det_freq);
    	}
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
	for(uint8_t i=7;i<11;i++) {
		pinsel.Pinnum = i;		// CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO] [P0.10 BUZZER]
		PINSEL_ConfigPin(&pinsel); 	// Inicializo pines P0.[10:7]
		GPIO_SetDir(0,(1<<i),1); 	// Configuro pines P0.[10:7] como salidas
	}

	/**
	 * Configuracion GPIO para Tx y Rx para UART
	 * */

	PINSEL_CFG_Type Pinsel_UART;
	Pinsel_UART.Funcnum = 1;			// Funcion 01: (Tx y Rx)
	Pinsel_UART.OpenDrain = 0;		// OD desactivado
	Pinsel_UART.Pinmode = 0;			// Pull-Up
	Pinsel_UART.Pinnum = 2;			// P0.2 para Tx (Conectar cable Blanco)
	Pinsel_UART.Portnum = 0;			// Puerto 0
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
	 * Configuraciones Timer 0 match 1
	 * */

	TIM_TIMERCFG_Type timer;
	TIM_MATCHCFG_Type match;
	timer.PrescaleOption = TIM_PRESCALE_USVAL;
	timer.PrescaleValue = 2;
	// MR1 configurado en 5 (Ti = TPR*(MR+1) = 2us*5 = 10us) => Muestreo cada 20us => Fs = 50kHz
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
	 * Configuraciones Timer 2 match 0
	 * */

	TIM_TIMERCFG_Type timer2;
	TIM_MATCHCFG_Type match0;
	timer2.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2.PrescaleValue = 85;
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


	/**
	 * Configuración timer 3 match 0 y 1 (Metrónomo)
	 * */

	TIM_TIMERCFG_Type timer3;
	TIM_MATCHCFG_Type match1;
	timer3.PrescaleOption = TIM_PRESCALE_USVAL;
	timer3.PrescaleValue = 100;
	/**
	 * Ti = TPR*(MR+1) = TPR * (MR+1) = 400.000us = 100 * (3999 + 1) = 400.000us
	 * TPR = 100
	 * MR = 3999
	 * */
	TIM_Init(LPC_TIM3,TIM_TIMER_MODE, &timer3);
	match0.MatchChannel = 0;
	match0.MatchValue = 3999;
	match0.IntOnMatch = ENABLE;
	match0.StopOnMatch = DISABLE;
	match0.ResetOnMatch = DISABLE;
	match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING; //no voy a usar el toggle del timer, necesito más control
	TIM_ConfigMatch(LPC_TIM3, &match0);
	/**
	 * Degault de 100bpm
	 * Ti = TPR*(MR+1) = TPR * (MR+1) = 600.000us = 100 * (5999 + 1) = 600.000us
	 * TPR = 100
	 * MR = 5999
	 * */
	match1.MatchChannel = 1;
	match1.MatchValue = 5999;
	match1.IntOnMatch = ENABLE;
	match1.StopOnMatch = DISABLE;
	match1.ResetOnMatch = ENABLE;
	match1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM3, &match1);
	TIM_ClearIntPending(LPC_TIM3, TIM_MR0_INT);
	TIM_ClearIntPending(LPC_TIM3, TIM_MR1_INT);
	TIM_Cmd(LPC_TIM3, ENABLE);
	NVIC_EnableIRQ(TIMER3_IRQn);

	return;
}

void confIntGPIO(void){
	//Interrupciones para el teclado
	FIO_IntCmd(2,(0xF<<4),1); 	// Inicializo interrupciones por flanco descendente en P2.[7:4]
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	NVIC_EnableIRQ(EINT3_IRQn); // Habilito interrupciones por GPIO
	return;
}

void confADC(void){
	ADC_Init(LPC_ADC,192307); // Energizo el ADC, Habilito el ADC bit PDN y Selecciono divisor de clock que llega al periferico como: CCLK/8
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE); // Canal AD0.1 seleccionado desde el reset (0x01)
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_1,DISABLE); // Canal AD0.1 seleccionado desde el reset (0x01)
	//	ADC_BurstCmd(LPC_ADC,0);                // Anulo bit 16 para seleccionar: Modo NO burst
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

void EINT3_IRQHandler(void){
	NVIC_DisableIRQ(EINT3_IRQn); // Deshabilito interrupcion por EINT3
	SYSTICK_InternalInit(20);    // Inicializo para interrumpir cada 20 ms, con el clock interno: SystemCoreClock
	SysTick->VAL = 0;            // Valor inicial de cuenta en 0
	SYSTICK_ClearCounterFlag();  // Anulo flag de fin de cuenta
	SYSTICK_IntCmd(ENABLE);      // Habilito interrupciones por systick
	SYSTICK_Cmd(ENABLE);		// Habilito la cuenta del systick
	FIO_ClearInt(2,(0xF<<4)); // Limpio bandera de interrupcion por P0.6
	return;
}

void ADC_IRQHandler(void) {
	adcValue = ADC_ChannelGetData(LPC_ADC,0);
	buffer[aux] = adcValue;

	//============== COMIENZO FILTRO 1 ========================

	for(uint32_t k = 0; k < BUFF_SIZE; k++){
		parc_sum += buffer[k];
	}
	aux++;

	if(aux >= BUFF_SIZE) aux = 0; //cuando superemos el nivel del buffer empezamos a pisar la pos 0 y continuamos

	old_sample_filt = current_sample_filt;
	current_sample_filt = parc_sum / BUFF_SIZE;
	parc_sum = 0;

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

	if(is_crossing && ((falling_sample - rising_sample) != 0)) {
		old_det_freq = det_freq;

		det_freq = SAMP_FREQ / (2 * abs(falling_sample - rising_sample)); // Tsamp*num_samp_rise_fall = Tdet

		//============== COMIENZO FILTRO 2 ========================

//			for(uint32_t j=0;j<FREQ_BUFF;j++){
//				parc_sum_freq += freq_buff[j];
//			}
//			aux_freq++;
//
//			if(aux_freq >= FREQ_BUFF) aux_freq=0;

//			old_det_freq = det_freq;
//			det_freq = parc_sum_freq / FREQ_BUFF;
//			parc_sum_freq = 0;

		//============== FIN FILTRO 2 ========================

		//============== COMIENZO UART =======================

		//TODO: REVISAR ESTA FUNCIÓN, XQ' ESPERA CHAR* Y LE PASAMOS STRING, POR LO QUE DEBE HACER UN CASTEO INTERNO, ¿USA ASCII? ¿EXPLICA PROBLEMAS?

		sprintf(UART_array,"%4d\r\n", det_freq); // seteo el formato para el envío de datos de la freq det y lo guardo en el array
		UART_Send(LPC_UART0, UART_array, sizeof(UART_array), BLOCKING); // envío el bloque de datos en forma de bloques

		//============== FIN UART =======================
	}

	is_crossing = 0;
	
	encenderLeds();

	return;
}

/**
 * Systema antirebotes
 * */

void SysTick_Handler(void){
	if( ~((GPIO_ReadValue(2)) & (0xF<<4)) & (0xF<<4) ){  // Operador logico para determinar si P2.[7:4] sigue presionado
		test_count ++ ;             // Si sigue presionado se incrementa variable test_count
		if(test_count == 3){        // Si se incremento 3 veces, se ingresa al switch(counter)
			push_response(); //
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

void TIMER3_IRQHandler(void) {
	//Si la interrupción se dió por match0, apago el buzzer
	if (TIM_GetIntStatus(LPC_TIM3, TIM_MR0_INT)) {
		LPC_GPIO0->FIOCLR |= (0x1 << 10);
		TIM_ClearIntPending(LPC_TIM3, TIM_MR0_INT);
	} //la interrupción la generó el match1, vuelvo a prender el buzzer
	else {
		LPC_GPIO0->FIOSET |= (0x1 << 10);
		TIM_ClearIntPending(LPC_TIM3, TIM_MR1_INT);
	}
}

/**
 * =============================================================================================================================================
 * 																		HELPER FUNCTIONS ZONE
 * =============================================================================================================================================
 * */

/**
 * Está función es un pseudo handler para reponder según la tecla presionada
 * */

void push_response(void) {
	switch( ~((GPIO_ReadValue(2)) & (0xF<<4)) & (0xF<<4) ){
	case(1<<4): // P2.4
		COL0_ISR();
	break;
	case(1<<5): // P2.5
		COL1_ISR();
	break;
	case(1<<6): // P2.6
		COL2_ISR();
	break;
	case(1<<7): // P2.7
		COL3_ISR();
	break;
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
		cambiarValorMetronomo(-1);
	break;
	case 1 :		// [COL1;FIL1]
		cambiarValorMetronomo(1);
		// +1bpm
	break;
	case 2 :		// [COL1;FIL2]
		cambiarValorMetronomo(-10);
		// -10bpm
	break;
	case 3 :		// [COL1;FIL3]
		cambiarValorMetronomo(10);
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

void encenderLeds(void) {
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

void cambiarValorMetronomo(int8_t value) {
	uint32_t matchValue = LPC_TIM3->MR1 + BPM * value;
	TIM_UpdateMatchValue(LPC_TIM3, 1, matchValue);
}

/**
 * Función para calcular valor absoluto
 * */

uint32_t abs_calc(int32_t value) {
	uint32_t aux;
	aux = value > 0 ? value : - value;
	return aux;
}

