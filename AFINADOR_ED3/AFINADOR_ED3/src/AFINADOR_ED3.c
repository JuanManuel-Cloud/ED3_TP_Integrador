/*===============================================================================
 Name        : AFINADOR_ED3.c
 Author      : Magnelli Tomas, Irigoin Muñoz Jose, Gil Juan Manuel.
 Consigna :
===============================================================================*/

#include "AFINADOR_ED3.h"

int main(void) {
	// ************************* Configuraciones de los perifericos utilizados **************************************
 	confGPIO();		// Configuraciones de pines analogicos y digitales utilizados.
	confUart();     // Configuraciones periferico UART
	confTimers();	// Configuraciones de temporizadores utilizados.
	confADC();		// Configuraciones periferico ADC
	confIntGPIO(); 	// Configuraciones de interrupciones del teclado.
	confDMA();		// Configuraciones controlador DMA
	// ************************* Inicializaciones,prioridades y puesta en funcionamiento ****************************
	GPIO_ClearValue(0,(0x380<<7));	// Inicializo en bajo salidas P0.[9:7] con 0011_1000_0000 = 0xB80
	NVIC_SetPriority(ADC_IRQn,5);	// Prioridad menor que el teclado
	NVIC_SetPriority(TIMER3_IRQn,4);// Prioridad menor que el teclado
	NVIC_SetPriority(DMA_IRQn,0);
	NVIC_SetPriority(EINT3_IRQn,2);	// Mayor prioridad al teclado para que permita cambiar de funcion lo antes posible
	// ************************* Comienza la interaccion con el usuario *********************************************
	welcomeMessage();	// Prepara buffer con strings de bienvenida y los envia directamente por UART
	UART_strings(); 			// Prepara buffer con strings para envios DMA de Memoria a UART.
	NVIC_EnableIRQ(EINT3_IRQn); // Habilito interrupciones GPIO por teclado.
	while(1){}
    return 0 ;
}
/*=============================================================================================================================================
 * 																		CONFIG ZONE
 * =============================================================================================================================================*/
void confGPIO(void) { // Configuraciones de pines analogicos y digitales utilizados.
	/* ******************************************PUERTO 0 *********************************************************/
	// ******************** GPIO para resultados de la afinación **************************************************
	PINSEL_CFG_Type pinsel0;			// Estructura para pines PUERTO 0 excepto pines UART
	pinsel0.Portnum = 0;				// Puerto 0
	pinsel0.OpenDrain = 0;				// OD desactivado
	// ******************** Pin entrada analogica para ADC ********************************************************
	pinsel0.Pinnum = 23;         		// Defino configurar Pin P0.23
	pinsel0.Pinmode = 2;				// Defino configurar pin P0.23 en PINMODE1: neither pull-up nor pull-down.
	pinsel0.Funcnum = 1;				// Defino configurar pin P0.23 como entrada analogica para AD0.0
	PINSEL_ConfigPin(&pinsel0);			// Configuro pin P0.23
	// ******************** Pines salidas digitales para AFINADOR y METRONOMO *************************************
	pinsel0.Funcnum = 0;				// Defino configurar funcion 0 para seleccionar GPIO
	for(uint8_t i=7;i<11;i++) {
		pinsel0.Pinnum = i; // CONEXIONES: [P0.7 LED ROJO] [P0.8 LED VERDE] [P0.9 LED AMARILLO] [P0.10 METRONOMO]
		PINSEL_ConfigPin(&pinsel0); 	// Configuro pines P0.[10:7]
		GPIO_SetDir(0,(1<<i),1); 		// Configuro pines P0.[10:7] como salidas
	}
	// ******************** Configuracion GPIO para Tx y Rx para UART *********************************************
	PINSEL_CFG_Type Pinsel_UART;		// Estructura exclusiva para configurar pines UART
	Pinsel_UART.Funcnum = 1;			// Defino configurar funcion 01: (Tx y Rx)
	Pinsel_UART.OpenDrain = 0;			// Defino configurar OD desactivado
	Pinsel_UART.Pinmode = 0;			// Defino configurar resistencias Pull-Up
	Pinsel_UART.Pinnum = 2;				// P0.2 para Tx (Conectar cable Blanco)
	Pinsel_UART.Portnum = 0;			// Puerto 0
	PINSEL_ConfigPin(&Pinsel_UART);		// Configuro pin P0.2 para Tx
	Pinsel_UART.Pinnum = 3;				// P0.3 para Rx (Conectar cable Verde)
	PINSEL_ConfigPin(&Pinsel_UART); 	// Configuro pin P0.3 para Rx
	/* ******************************************PUERTO 2 *********************************************************/
	// ********************  Configuración GPIO para teclado ******************************************************
	PINSEL_CFG_Type pinsel2;
		pinsel2.Portnum = 2;			// Puerto 2
		pinsel2.Funcnum	= 0;			// Defino configurar funcion 0 para seleccionar GPIO
		for(uint8_t j=0;j<4;j++){
			pinsel2.Pinnum = j;			// Pin P2.[3:0]
			PINSEL_ConfigPin(&pinsel2);	// Configuro pin P2.[3:0]
		}
		pinsel2.Pinmode = 0;			// Defino configurar resistencias Pull-Up en entradas
		for(uint8_t j=4;j<8;j++){
			pinsel2.Pinnum = j;			// Pin P2.[7:4]
			PINSEL_ConfigPin(&pinsel2);	// Configuro pin P2.[7:4]
		}
		GPIO_SetDir(2,(0xF),1); 		// Configuro pin P2.[3:0] como salidas
		GPIO_SetDir(2,(0xF<<4),0); 		// Configuro pin P2.[7:4] como entradas

	return;
}
void confTimers(void) { // Configuraciones de temporizadores utilizados.
	// ********************* SYSTICK para protocolo antirrebote por hardware **************************************
	SYSTICK_InternalInit(15);    		// Inicializo para interrumpir cada 15 ms
	SYSTICK_IntCmd(ENABLE);      		// Habilito interrupciones por systick
	// ********************* TIMER0 para ejecucion de conversiones ADC ********************************************
	TIM_TIMERCFG_Type timer0_ADC;
	LPC_SC->PCLKSEL0  |= 0x4; 			// Selecciono inicialmente PCLK = CCLK
	timer0_ADC.PrescaleOption = TIM_PRESCALE_TICKVAL; 	// Selecciono que atributo PrescaleValue sea registro PR
	timer0_ADC.PrescaleValue = 249; 	// TPR = (PR+1)/PCLK = (249+1)/100MHz = 2.5us
	TIM_Init(LPC_TIM0,TIM_TIMER_MODE,&timer0_ADC); 		// Configuro TIMER0
	// ********************* MATCH1 para ejecutar conversiones ADC ************************************************
	TIM_MATCHCFG_Type match1_ADC;
	match1_ADC.MatchChannel = 1;		// Selecciono configurar canal MATCH1 que permite ejecutar conversiones ADC.
	match1_ADC.MatchValue = 4;			// Ti = TPR*(MR+1) = 2.5us*(4+1) = 12.5us => Ts = 2*Ti = 25us => Fs = 40kHz
	match1_ADC.IntOnMatch = DISABLE;	// No interrumpe, el match gatilla conversion directamente.
	match1_ADC.StopOnMatch = DISABLE;	// No se detiene la cuenta ante un evento de match
	match1_ADC.ResetOnMatch = ENABLE;	// Se reinicia la cuenta cada vez que llega al match
	match1_ADC.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	TIM_ConfigMatch(LPC_TIM0,&match1_ADC);	// Configuro MATCH 1
	TIM_ResetCounter(LPC_TIM0);
	TIM_Cmd(LPC_TIM0,ENABLE);
	// ********************* TIMER2 para desplazamiento de bits en Teclado ****************************************
	TIM_TIMERCFG_Type timer2_Teclado;
	timer2_Teclado.PrescaleOption = TIM_PRESCALE_USVAL; // Selecciono que atributo PrescaleValue sea TPR en [us]
	timer2_Teclado.PrescaleValue = 60;	 	// TPR = 60us
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE, &timer2_Teclado);	// Configuro TIMER2
	// ********************* MATCH0 para desplazamiento de bits en Teclado ****************************************
	TIM_MATCHCFG_Type match0_Teclado;
	match0_Teclado.MatchChannel = 0;		// Selecciono configurar canal MATCH0
	match0_Teclado.MatchValue = 999; 		// Ti = TPR*(MR+1) = 60us*(999+1) = 60ms
	match0_Teclado.IntOnMatch = ENABLE;		// Habilito interrupcion para realizar desplazamiento periodicamente
	match0_Teclado.StopOnMatch = DISABLE;	// No se detiene la cuenta ante un evento de match
	match0_Teclado.ResetOnMatch = ENABLE; 	// Se reinicia la cuenta cada vez que llega al match
	match0_Teclado.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM2, &match0_Teclado); // Configuro MATCH 0
	TIM_Cmd(LPC_TIM2, ENABLE);				// Inicializo TIMER2
	NVIC_EnableIRQ(TIMER2_IRQn);			// Habilito interrupciones por TIMER2
	// ********************* TIMER3 para funcionamiento Metronomo *************************************************
	TIM_TIMERCFG_Type timer3_Metronomo;
	timer3_Metronomo.PrescaleOption = TIM_PRESCALE_USVAL; 	// Selecciono que atributo PrescaleValue sea TPR en [us]
	timer3_Metronomo.PrescaleValue = 100; 	// TPR = 100us
	TIM_Init(LPC_TIM3,TIM_TIMER_MODE, &timer3_Metronomo);	// Configuro TIMER2
	// ********************* MATCH0 para duracion encendido BUZZER ************************************************
	TIM_MATCHCFG_Type match0_Metronomo;
	match0_Metronomo.MatchChannel = 0;		// Selecciono configurar canal MATCH0
	match0_Metronomo.MatchValue = 2999; 	// Ti = TPR*(MR+1) = 100 * (2999 + 1) = 300ms
	match0_Metronomo.IntOnMatch = ENABLE;	// Habilito interrupcion para apagar el buzzer
	match0_Metronomo.StopOnMatch = DISABLE; // No se detiene la cuenta ante un evento de match
	match0_Metronomo.ResetOnMatch = DISABLE;// No se reinicia la cuenta ante un evento de match 0
	match0_Metronomo.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM3, &match0_Metronomo);  // Configuro MATCH 0
	// ********************* MATCH1 para inicializacion BPM BUZZER ************************************************
	// La estructura match1_Metronomo es definida global para modificarla al cambiar los bpm
	match1_Metronomo.MatchChannel = 1;			// Selecciono configurar canal MATCH1
	match1_Metronomo.MatchValue = 5999; 		// Default: 100bpm => Ti = TPR*(MR+1) = 100 * (5999 + 1) = 600ms
	match1_Metronomo.IntOnMatch = ENABLE;		// Habilito interrupcion para encender el buzzer
	match1_Metronomo.StopOnMatch = DISABLE;		// No se detiene la cuenta ante un evento de match
	match1_Metronomo.ResetOnMatch = ENABLE;		// Se reinicia la cuenta ante un evento de match 1
	match1_Metronomo.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_ConfigMatch(LPC_TIM3, &match1_Metronomo);	// Configuro MATCH 1
	return;
}
void confIntGPIO(void){			// Configuraciones de interrupciones del teclado.
	FIO_IntCmd(2,(0xF<<4),1); 	// Inicializo interrupciones por flanco descendente en P2.[7:4]
	FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
	return;
}
void confADC(void){	// Configuraciones periferico ADC
	ADC_Init(LPC_ADC,190000);// Energiza y habilita bit PDN de ADC. Selecciona divisor de clock como: PCLK=CCLK/8
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_1,DISABLE); 	// Deshabilito canal AD0.1 seleccionado desde el reset (0x01)
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE);  	// Habilito canal AD0.0
	ADC_BurstCmd(LPC_ADC,0);                		// Anulo bit 16 para seleccionar: Modo NO burst
	ADC_StartCmd(LPC_ADC,ADC_START_ON_MAT01);     	// Configuro conversion ante MAT0.1 (REGISTRO MR1 DE TIMER0)
	ADC_EdgeStartConfig(LPC_ADC,1);				// Un 1 en bit 27 significa comienza por FLANCO DESCENDENTE en EMR1
	ADC_IntConfig(LPC_ADC,ADC_ADINTEN0,1);   	// Habilito interrupciones por canal AD0.0
	return;
}

void confUart(void){ // Configuraciones periferico UART
	// ********************* Configuraciones protocolo UART *******************************************************
	UART_CFG_Type UARTConfigStruct;
	UARTConfigStruct.Baud_rate  = 115200;			// Tasa de transferencia de baudios de 115200
	UARTConfigStruct.Databits 	= UART_DATABIT_8;	// 8 bits de datos     		(por defecto)
	UARTConfigStruct.Parity 	= UART_PARITY_NONE;	// Sin bit de paridad  		(por defecto)
	UARTConfigStruct.Stopbits 	= UART_STOPBIT_1;	// Bit de stop unico   		(por defecto)
	UART_Init(LPC_UART0, &UARTConfigStruct);		// Configura modulo UART    (por defecto)
	// ********************* Configuraciones hardware UART ********************************************************
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	UARTFIFOConfigStruct.FIFO_DMAMode 	 = ENABLE;	// Habilitacion de modo DMA
	UARTFIFOConfigStruct.FIFO_Level 	 = UART_FIFO_TRGLEV0;	// Configuracion Rx en TRGLEV0  (por defecto)
	UARTFIFOConfigStruct.FIFO_ResetRxBuf = ENABLE;	// Habilitacion de reseteo de buffer Rx 	(por defecto)
	UARTFIFOConfigStruct.FIFO_ResetTxBuf = ENABLE;	// Habilitacion de reseteo de buffer Tx 	(por defecto)
	UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);	// Configura FIFO de UART
	UART_TxCmd(LPC_UART0, ENABLE);						// Habilita comunicacion UART
	return;
}
void confDMA(void){ // Configuraciones controlador DMA
	// ********************* Estructura LLI utilizada *************************************************************
	GPDMA_LLI_Type DMA_LLI_Struct;
	DMA_LLI_Struct.SrcAddr= (uint32_t)str_send_package; // Puntero fuente a primera ubicacion de memoria de string.
	DMA_LLI_Struct.DstAddr= (uint32_t)&(LPC_UART0->THR);// Puntero destino a registro de Tx de UART0
	DMA_LLI_Struct.NextLLI= 0; 							// Apunta a null ya que no se requiere linkear una lista.
	DMA_LLI_Struct.Control= DMA_SIZE |(1<<26) 			// Tamaño de transferencia de 468 datos. Incremento unitario.
							|(0<<18)|(0<<21); 			// Ancho parabra fuente y destino de 8 bits.
	GPDMA_Init(); // Inicializacion de controlador GPDMA
	// ********************* Estructura configuracion GPDMA ********************************************************
	GPDMA_Struct.ChannelNum 	= 0;					// Selecciono configurar canal 0 de DMA (Maxima prioridad).
	GPDMA_Struct.SrcMemAddr 	= (uint32_t)(str_send_package); // Puntero fuente: Buffer con string a mostrar.
	GPDMA_Struct.DstMemAddr 	= 0;					// Puntero destino nulo ya que es hacia un periferico
	GPDMA_Struct.TransferSize 	= DMA_SIZE;				// Tamaño de transferencia de 468 datos.
	GPDMA_Struct.TransferWidth 	= 0;					// Ancho de transferencia no utilizado, transf. hacia perif.
	GPDMA_Struct.TransferType 	= GPDMA_TRANSFERTYPE_M2P;	// Transferencia de memoria a periferico.
	GPDMA_Struct.SrcConn 		= 0;					// Conexion fuente no utilizada ya que es desde memoria.
	GPDMA_Struct.DstConn 		= GPDMA_CONN_UART0_Tx;	// Conecion destino a Tx de UART0.
	GPDMA_Struct.DMALLI 		= (uint32_t)&DMA_LLI_Struct;// Seleccion de LLI configurado anteriormente.
//	GPDMA_Setup(&GPDMA_Struct);		// Configuracion de DMA se realiza al presionar pulsador
	return;
}
/*=============================================================================================================================================
 * 																		HANDLER ZONE
 * ============================================================================================================================================*/
void ADC_IRQHandler(void){ // Rutina de servicio a la interrupcion por finalizacion de conversion de ADC.
	old_sample = current_sample;					// Conservo valor de muestra anterior
	current_sample = ADC_ChannelGetData(LPC_ADC,0);	// Adquiero nueva muestra
	if(current_sample >= 2048) { // Condicion nueva muestra mayor que "cero" (flanco ascendente)
		if(old_sample < 2048) {
			is_crossing = 1;				// Activo bandera de deteccion de cruze de mitad de rango dinamico
			rising_sample = sample_count;	// Registro valor de contador de muestras adquiridas en evento de cruce
		}
	}
	else { // Condicion nueva muestra menor que "cero" (flanco descendente)
		if(old_sample > 2048){
			is_crossing = 1;				// Activo bandera de deteccion de cruze de mitad de rango dinamico
			falling_sample = sample_count; 	// Registro valor de contador de muestras adquiridas en evento de cruce
		}
	}
	sample_count ++;	// Incremento contador de muestras adquiridas
	if(is_crossing && (abs(falling_sample - rising_sample) != 0)) {
// ***** Implementacion de filtro de deteccion de minima frecuencia de las ultimas 15 detectadas ********************
		freq_buff[aux_freq] = 16*(SAMP_FREQ / (2 * abs(falling_sample - rising_sample)));
		// El tiempo entre eventos de cruce es: abs(falling_sample - rising_sample)/SAMP_FREQ
		// Duplicando ese tiempo y tomando la inversa, se espera obtener la frecuencia de la señal.
		// Debido a observaciones experimentales de los resultados, se mejora la deteccion incorporando el factor *16.
		det_freq_aux = ~(0);	// Inicializacion en un valor grande para permitir detectar el minimo desde comienzo.
		for(uint32_t j=0;j<FREQ_BUFF;j++){	// Recorrido de todas las muestras del buffer
			if((freq_buff[j] <= det_freq_aux) && (freq_buff[j] > 0)){	// Deteccion del elemento de minimo valor
				det_freq_aux = freq_buff[j]; // Elemento j del buffer es menor que el detectado y se retiene valor.
			}
		}
		old_det_freq = det_freq; // Almaceno ultima frecuencia detectada para identificar si hubo un cambio.
		det_freq = det_freq_aux; // Se toma como detectada a la menor frecuencia detectada en las ultimas 15 detecciones.
		aux_freq++;				 // Incremento indice de almacenamiento de ultimas frecuencias detectadas.
		if(aux_freq >= FREQ_BUFF) aux_freq=0; // Anulo indice de almacenamiento si se ha llegado al final del buffer
	}
	is_crossing = 0;				// Anulo bandera de evento de cruce de mitad de rango dinamico.
	if(det_freq != old_det_freq){	// Verifico si se ha modificado la frecuencia detectada.
		testFlagLED();				// Si se ha modificado, re-evaluo condicion de realimentacion visual al usuario.
	}
	return;
}
// **************************** Protocolo antirrebotes utilizando SYSTICK *******************************************
void EINT3_IRQHandler(void){ // Rutina de servicio a la interrupcion por GPIO del Teclado.
	NVIC_DisableIRQ(EINT3_IRQn); 	// Deshabilito interrupcion por EINT3
	SysTick->VAL = 0;            	// Valor inicial de cuenta en 0
	SysTick->CTRL &= SysTick->CTRL; // Anulo flag de fin de cuenta
	SYSTICK_Cmd(ENABLE);		 	// Habilito la cuenta del systick
	return;
}
void SysTick_Handler(void){ // Rutina de servicio a la interrupcion por completar cuenta de SYSTICK
	if(~(GPIO_ReadValue(2) | ~(0xF0) )){// Verifico si se encuentra presionado algun pulsador
		test_count ++ ;             	// Si sigue presionado se incrementa variable test_count
		if(test_count == 3){        	// Si se incremento 3 veces, se ejecuta respuesta al estimulo del teclado
			push_response();			// Funcion de respuesta a estimulo del teclado
			SYSTICK_Cmd(DISABLE);		// Deshabilito interrupciones por SYSTICK
			FIO_ClearInt(2,(0xF<<4));	// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
			NVIC_EnableIRQ(EINT3_IRQn); // Vuelvo a habilitar interrupciones por EINT3
			test_count = 0;             // Anulo variable test_count para proximo antirrebote
		}
		// Si no se incremento test_count 3 veces, pero sigue presionado el pulsador no se hace nada.
	}
	else{// Si se solto el pulsador sin llegar a test_count = 3
		SYSTICK_Cmd(DISABLE);			// Deshabilito interrupciones por SYSTICK
		FIO_ClearInt(2,(0xF<<4));		// Limpio bandera de interrupcion en P2.[7:4] antes de habilitarlas
		NVIC_EnableIRQ(EINT3_IRQn); 	// Vuelvo a habilitar interrupciones por EINT3
		test_count = 0; 				// Se anula test_count para proximo antirrebote.
	}
	SysTick->CTRL &= SysTick->CTRL; // Anulo flag de fin de cuenta de SYSTICK siempre que salgo del handler.
	return;
}
void TIMER2_IRQHandler(void){// Rutina de servicio a interrupcion Timer 2, SHIFTREGISTER en filas de teclado matricial
	if(!(~(GPIO_ReadValue(2) | ~(0xF0) ))){// Solo ejecuta desplazamiento cuando no se esta presionando ninguna tecla
		switch(row_value){
		case 0 :// Si el 0 se encuentra en columna 0 lo ubico en columna 3
			LPC_GPIO2->FIOPIN = 0x7; // 0b0111
			row_value = 3;			 // row_value sale valiendo 3
			break;
		case 3:// Si el 0 se encuentra en columna 3 lo ubico en columna 2
			LPC_GPIO2->FIOPIN = 0xB; // 1011
			row_value--; // row_value sale valiendo 2
			break;
		case 2:// Si el 0 se encuentra en columna 2 lo ubico en columna 1
			LPC_GPIO2->FIOPIN = 0xD; // 1101
			row_value--; // row_value sale valiendo 1
			break;
		case 1:// Si el 0 se encuentra en columna 1 lo ubico en columna 0
			LPC_GPIO2->FIOPIN = 0xE; // 1110
			row_value--; // row_value sale valiendo 0
			break;
		}
	}
	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);	// Anulo bandera de interrupcion por MATCH0 de TIMER2 pendiente
	return;
}
void TIMER3_IRQHandler(void){// Rutina de servicio a la interrupción por TIMER3. Intermitencia BUZZER segun MR0 y MR1.
	if (TIM_GetIntStatus(LPC_TIM3, TIM_MR0_INT)) {
		LPC_GPIO0->FIOCLR |= (0x1 << 10);			// Apago BUZZER
		TIM_ClearIntPending(LPC_TIM3, TIM_MR0_INT); // Anulo bandera de interrupcion por MATCH0 de TIMER3 pendiente
	} //la interrupción la generó el match1, vuelvo a prender el buzzer
	else {
		LPC_GPIO0->FIOSET |= (0x1 << 10);			// Enciendo BUZZER
		TIM_ClearIntPending(LPC_TIM3, TIM_MR1_INT); // Anulo bandera de interrupcion por MATCH1 de TIMER3 pendiente
	}
}
void DMA_IRQHandler(void){ // Rutina de servicio a la interrupcion por completar transferencia DMA.
	GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR,0);	// Anulo bandera de interrupcion por errores en transferencia
	GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC,0);	// Anulo bandera de interrupcion por completar transferencia de DMA
	NVIC_DisableIRQ(DMA_IRQn);						// Deshabilito interrupciones por DMA
	if(ADC_IntStatus){	// Si se encontraba en funcionamiento el AFINADOR
		NVIC_EnableIRQ(ADC_IRQn);	// Habilito interrupciones por ADC
	}
	else if(TIMER3_IntStatus){		// Si se encontraba en funcionamiento el METRONOMO
		TIM_Cmd(LPC_TIM3, ENABLE);		// Inicializo TIMER3
		NVIC_EnableIRQ(TIMER3_IRQn); 	// Habilito interrupciones por TIMER3
	}

}
/*=============================================================================================================================================
 * 														KEYBOARD ZONE
 * ============================================================================================================================================*/
 // ************** Funcion para ordenar que codigo se ejecuta según la tecla presionada*******************************
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
 // ************** Funcion para teclas de la columna 0 ***************************************************************
void COL0_ISR(void){
	switch(row_value){
	case 0 :		// [COL0;FIL0]
		printf("Columna: 0, Fila: %d\r\n",row_value);
		welcomeMessage();
	break;
	case 1 :		// [COL0;FIL1]
		printf("Columna: 0, Fila: %d\r\n",row_value);
	break;
	case 2 :		// [COL0;FIL2]
		printf("Columna: 0, Fila: %d\r\n",row_value);
	break;
	case 3 :		// [COL0;FIL3] = BOTON *
		printf("Columna: 0, Fila: %d\r\n",row_value);
		ADC_IntStatus = 0;
		TIMER3_IntStatus = 1;
		NVIC_DisableIRQ(ADC_IRQn);		// Inhabilito conversor ADC
		TIM_Cmd(LPC_TIM3, ENABLE);		// Inicializo TIMER3
		NVIC_EnableIRQ(TIMER3_IRQn); 	// Habilito interrupciones por TIMER3
		modifyBPM(-1); // -10 bpm
	break;
	}
}
 // ************** Funcion para teclas de la columna 1 ***************************************************************
void COL1_ISR(void){
	switch(row_value){
	case 0 :		// [COL1;FIL0]
		printf("Columna: 1, Fila: %d\r\n",row_value);
	break;
	case 1 :		// [COL1;FIL1]
		printf("Columna: 1, Fila: %d\r\n",row_value);
	break;
	case 2 :		// [COL1;FIL2]
		printf("Columna: 1, Fila: %d\r\n",row_value);
	break;
	case 3 :		// [COL1;FIL3] = BOTON 0
		printf("Columna: 1, Fila: %d\r\n",row_value);
		ADC_IntStatus = 0;
		TIMER3_IntStatus = 1;
		NVIC_DisableIRQ(ADC_IRQn);		// Inhabilito conversor ADC
		TIM_Cmd(LPC_TIM3, ENABLE);		// Inicializo TIMER3
		NVIC_EnableIRQ(TIMER3_IRQn); 	// Habilito interrupciones por TIMER3
		modifyBPM(5); // Regreso a intermedio 100 bpm
	break;
	}
}
 // ************** Funcion para teclas de la columna 2 ***************************************************************
void COL2_ISR(void){
	switch(row_value){
	case 0 :		// [COL2;FIL0]
		printf("Columna: 2, Fila: %d\r\n",row_value);
	break;
	case 1 :		// [COL2;FIL1]
		printf("Columna: 2, Fila: %d\r\n",row_value);
	break;
	case 2 :		// [COL2;FIL2]
		printf("Columna: 2, Fila: %d\r\n",row_value);

		NVIC_DisableIRQ(ADC_IRQn);		// Inhabilito conversor ADC
		TIM_Cmd(LPC_TIM3,DISABLE);			// Reinicio y freno contador de TIMER3
		NVIC_DisableIRQ(TIMER3_IRQn);		// Inhabilito interrupciones por TIMER3
		LPC_GPIO0->FIOCLR |= (0x1 << 10);	// Aseguro que BUZZER quede apagado

		catFrecValue();
		GPDMA_Setup(&GPDMA_Struct);
		GPDMA_ChannelCmd(0, ENABLE);
		NVIC_EnableIRQ(DMA_IRQn);
	break;
	case 3 :		// [COL2;FIL3] = BOTON #
		printf("Columna: 2, Fila: %d\r\n",row_value);
		ADC_IntStatus = 0;
		TIMER3_IntStatus = 1;

		NVIC_DisableIRQ(ADC_IRQn);		// Inhabilito conversor ADC
		TIM_Cmd(LPC_TIM3, ENABLE);		// Inicializo TIMER3
		NVIC_EnableIRQ(TIMER3_IRQn); 	// Habilito interrupciones por TIMER3
		modifyBPM(1); // +10 bpm
	break;
	}
}
 // ************** Funcion para teclas de la columna 3 ***************************************************************
void COL3_ISR(void){
	TIM_Cmd(LPC_TIM3,DISABLE);			// Reinicio y freno contador de TIMER3
	NVIC_DisableIRQ(TIMER3_IRQn);		// Inhabilito interrupciones por TIMER3
	LPC_GPIO0->FIOCLR |= (0x1 << 10);	// Aseguro que BUZZER quede apagado
	NVIC_EnableIRQ(ADC_IRQn);       	// Habilito interrupciones por ADC

	switch(row_value){
	case 0 :		// [COL3;FIL0]
		printf("Columna: 3, Fila: %d\r\n",row_value);
		comp_freq = 200; 	// Cuando generador en 200
	break;
	case 1 :		// [COL3;FIL1]
		printf("Columna: 3, Fila: %d\r\n",row_value);
		comp_freq = 800; 	// Cuando generador en 400
	break;
	case 2 :		// [COL3;FIL2]
		printf("Columna: 3, Fila: %d\r\n",row_value);
		comp_freq = 2000;	// Cuando generador en 800
	break;
	case 3 :		// [COL3;FIL3]
		printf("Columna: 3, Fila: %d\r\n",row_value);
		comp_freq = 3000; 	// Cuando generador en 1000
	break;
	}
	ADC_IntStatus = 1;
	TIMER3_IntStatus = 0;
	error_margin = comp_freq/5;
}
/*=============================================================================================================================================
 * 					Funciones complementarias de FUNCIONALIDAD 1: AFINADOR
 * ============================================================================================================================================*/
 // ******************* Enciende el led correspondiente acorde a la afinación del instrumento*************************
void testFlagLED(void){    // CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO]
	if(det_freq > (comp_freq + error_margin)){ // Si la diferencia es mayor a 50 Hz
		// La frecuencia detectada se encuentra a mas de 50 Hz por encima de la deseada
		// Encender un [P0.7 LED ROJO] (INDICA AL USUARIO BAJAR LA FRECUENCIA DEL INSTRUMENTO)
		GPIO_SetValue(0, (1 << 7));               	// Pongo en alto P0.7
		GPIO_ClearValue(0, (3 << 8));			// Pongo en bajo P0.[9:8]
	}
	else if(det_freq < (comp_freq - error_margin)){
		// La frecuencia detectada se encuentra a mas de 50 Hz por debajo de la deseada
		// Encender un [P0.9 LED AMARILLO] (INDICA AL USUARIO SUBIR LA FRECUENCIA DEL INSTRUMENTO)
		GPIO_SetValue(0, (1 << 9));              	// Pongo en alto P0.9
		GPIO_ClearValue(0, (3 << 7));			// Pongo en bajo P0.[8:7]
	}
	else{
		// La frecuencia detectada se encuentra a menos de 50 Hz de la deseada
		// Encender un [P0.8 LED VERDE] (INDICA AL USUARIO MANTENER LA FRECUENCIA DEL INSTRUMENTO)
		GPIO_SetValue(0, (1 << 8));               	// Pongo en alto P0.8
		GPIO_ClearValue(0, (5 << 7));			// Pongo en bajo P0.7 y P0.9
	}
}
/*=============================================================================================================================================
 * 					Funciones complementarias de FUNCIONALIDAD 2: METRONOMO
 * ============================================================================================================================================*/
 // ******************* Modifica los BPM segun la tecla que se haya pulsado *******************************************
void modifyBPM(int8_t value) {
	if(value < 5){							// Si el valor ingresado es 1 o -1
		index += value;						// Incremento o disminuyo el indice en 1 unidad, segun el caso
		if		(index >= 10) 	index = 10;	// "Saturo" el indice al maximo valor posible para max BPM = 150 BPM
		else if	(index <= 0) 	index = 0 ; // "Saturo" el indice al minimo valor posible para min BPM =  50 BPM
	}
	else index = value;						// Si el valor ingresado es 5 al presionar el boton 0, vuelvo a 100 BPM
	matchValue = bpmTable[index];					// Defino el valor de matchValue segun el valor de la tabla
	match1_Metronomo.MatchValue = matchValue;		// Sobreescribo atributo de valor de MATCH1 de TIMER 3
	TIM_ConfigMatch(LPC_TIM3, &match1_Metronomo);	// Re-configuro MATCH 1 de TIMER3
	return;
}
/*=============================================================================================================================================
 * 					Funciones complementarias de FUNCIONALIDAD 3: MONITOREO (Envio datos por DMA de Memoria a UART)
 * ============================================================================================================================================*/
// ************** Funcion para mensaje bienvenida y definicion inicial de strings para envios por DMA *****************
void UART_strings(void){

	sprintf(str_header_afinador,"\n\t==================================== AFINADOR =============================\r\n");
	sprintf(str_afinador_state,"\n\tSe encuentra en estado        \r\n\n");
	sprintf(str_comp_freq,"\tSe encuentra afinando en        Hz\r\n\n");
	sprintf(str_error_margin,"\tCon un margen de error de         Hz\r\n\n");
	sprintf(str_det_freq,"\tLa ultima frecuencia detectada fue de         Hz\r\n\n");
	sprintf(str_diff_freq,"\tPara alcanzar la frecuencia deseada, ajuste el instrumento en        Hz\t\t\r\n\n");
	sprintf(str_header_met,"\n\t==================================== METRONOMO ============================\t\t\t\t\t\r\n");
	sprintf(str_bpm_state,"\n\tSe encuentra en estado        \r\n");
	sprintf(str_bpm_value,"\tConfigurado en        bpm\t\r\n");

	strcat(str_send_package,str_header_afinador);
	strcat(str_send_package,str_afinador_state);
	strcat(str_send_package,str_comp_freq);
	strcat(str_send_package,str_error_margin);
	strcat(str_send_package,str_det_freq);
	strcat(str_send_package,str_diff_freq);
	strcat(str_send_package,str_header_met);
	strcat(str_send_package,str_bpm_state);
	strcat(str_send_package,str_bpm_value);

/*	Codigo utilizado para pruebas de funcionamiento de funcion catFrecValue();
	for(uint8_t k=0;k<4;k++){
		comp_freq = comp_freq/(k + 1);
		error_margin = error_margin/(k + 1);
		det_freq = det_freq/(k + 1);
		index += 1;
		if(k%2) {
			NVIC_EnableIRQ(TIMER3_IRQn);
			NVIC_DisableIRQ(ADC_IRQn);}
		else {
			NVIC_DisableIRQ(TIMER3_IRQn);
			NVIC_EnableIRQ(ADC_IRQn);}
		catFrecValue();
		printf(str_send_package);}
*/
}
 // ******************* Prepara variables de sustitucion y ubicacion a sustituir en el mensaje ************************
void catFrecValue(void) {
	char str_aux[6];		// Variable char de 6 elementos para permitir maximo tamaño de valores
	int var_sust;			// Variable entera para almacenar variable a sustituir
	int sust_pos;			// Variable entera para almacenar posicion a sustituir
	for(uint8_t j=0;j<7;j++) {	// Ejecuto 7 llamados a funcion sustFunction(str_aux,sust_pos);
		switch(j) {
			case 0:				// Verifico si se encuentra encendido el AFINADOR para notificar estado
				if(ADC_IntStatus){
					sprintf(str_aux,"  ON ");	// Directamente sobreescribo con string de encendido
				}
				else{
					sprintf(str_aux," OFF ");	// Directamente sobreescribo con string de apagado
				}
				sust_pos = POW_AFI_POS;			// Posicion de variable de estado AFINADOR
			break;
			case 1:
				var_sust = comp_freq;			// Variable de estado frecuencia de comparacion
				sust_pos = COMP_FREQ_POS;		// Posicion de variable de estado frecuencia de comparacion
				sprintf(str_aux,"%d",var_sust);
			break;
			case 2:
				var_sust = error_margin;		// Variable de estado margen de error
				sust_pos = MARGIN_ERRROR_POS; 	// Posicion de variable de estado margen de error
				sprintf(str_aux,"%d",var_sust);
			break;
			case 3:
				var_sust = det_freq;			// Variable de estado frecuencia detectada
				sust_pos = LAST_FREQ_DET_POS;	// Posicion de variable de estado frecuencia detectada
				sprintf(str_aux,"%d",var_sust);
			break;
			case 4:
				var_sust = (comp_freq-det_freq);// Variable de estado frecuencia a corregir por usuario
				sust_pos = DIFF_FREQ_POS;		// Posicion de variable de estado frecuencia a corregir por usuario
				sprintf(str_aux,"%d",var_sust);
			break;
			case 5:
				if(TIMER3_IntStatus){		// Verifico si se encuentra encendido el METRONOMO para notificar estado
					sprintf(str_aux,"  ON ");	// Directamente sobreescribo con string de encendido
				}
				else{
					sprintf(str_aux," OFF ");	// Directamente sobreescribo con string de apagado
				}
				sust_pos = POW_MET_POS;			// Posicion de variable de estado METRONOMO
			break;
			case 6:
				var_sust = bpmValueTable[index];// Variable de estado BPM seleccionados actualmente
				sust_pos = BPM_POS;				// Posicion de variable de estado BPM seleccionados actualmente
				sprintf(str_aux,"%d",var_sust);	//
			break;
		}
		sustFunction(str_aux,sust_pos);
	}
	return;
}
 // ******************* Sustituye los campos variables del mensaje a imprimir ****************************************
void sustFunction(char str_aux[6],uint32_t sust_pos){
	uint8_t str_aux_len = strlen(str_aux);		// Almaceno largo del string en variable auxiliar
	for(uint8_t i = 0; i < SERIAL_CHAR; i++) {
		if (i >= (SERIAL_CHAR - str_aux_len))	// Reemplazo con caracter siempre que se trate de  un caracter no-nulo
			str_send_package[i + sust_pos] = (char) str_aux[i-(SERIAL_CHAR - str_aux_len)]; // Asignacion del caracter
		else
			str_send_package[i + sust_pos] = ' ';	// Completo con espacios las ubicaciones no utilizadas
	}
}
/*=============================================================================================================================================
 * 						Funcion de mensaje de bienvenida.
 * ============================================================================================================================================*/
void welcomeMessage(void){ // Funcion para imprimir mensaje de bienvenida via UART.
	if(!is_loaded){ // Carga mansaje de bienvenida una vez, luego si vuelvo a ejecutar solo lo envia.
		// Mensaje inicial.
		sprintf(str_equals,"\r\n\t===================================================================================\r\n");
		sprintf(str_header,"\n\t====================== BIENVENIDO A SU AFINADOR DE CONFIANZA ======================\r\n");
		sprintf(str_developers,"\n\tDesarrolladores: Gil Juan Manuel, Magnelli Tomas e Irigoin Muñoz Jose.\r\n\n");
		sprintf(str_institution,"\tFacultad de Ciencias Exactas, Fisicas y Naturales - Universidad Nacional de Cordoba.\r\n\n");
		sprintf(str_teacher,"\tDocente: Ing. Martin Ayarde.\r\n\n");
		sprintf(str_subject,"\tMateria: Electronica Digital III\r\n\n");
		// Explicacion de funcionalidades al usuario.
		sprintf(str_func1,"\t\nFUNCIONALIDAD 1: AFINADOR. \r\n\n");
		sprintf(str_func1_explain,"\t\tElija la frecuencia de afinacion, luego ajuste hasta encender el led verde.\r\n\n");
		sprintf(str_func1a,"\t\t-Presione tecla 'A' para afinar en  200 Hz \r\n");
		sprintf(str_func1b,"\t\t-Presione tecla 'B' para afinar en  800 Hz \r\n");
		sprintf(str_func1c,"\t\t-Presione tecla 'C' para afinar en 2000 Hz \r\n");
		sprintf(str_func1d,"\t\t-Presione tecla 'D' para afinar en 3000 Hz \r\n");
		sprintf(str_func2,"\t\nFUNCIONALIDAD 2: METRONOMO. \r\n\n");
		sprintf(str_func2_explain,"\t\tElija los beats por minuto a los que desea que suene el BUZZER.\r\n");
		sprintf(str_func2_explain2,"\t\tPor defecto 100 bpm. Minimo 50 bpm. Maximo 150 bpm.\r\n\n");
		sprintf(str_func2_lower, "\t\t-Presione tecla '*' para disminuir el tempo en 10 bpm\r\n");
		sprintf(str_func2_100,	 "\t\t-Presione tecla '0' para un tempo de 100 bpm \r\n");
		sprintf(str_func2_higher,"\t\t-Presione tecla '#' para incrementar el tempo en 10 bpm\r\n");
		sprintf(str_func3,"\t\nFUNCIONALIDAD 3: MONITOREO. \r\n\n");
		sprintf(str_func3_explain,"\t\tPermite observar el estado actual del dispositivo sin frenar su funcionamiento.\r\n\n");
		sprintf(str_func3_show,"\t\t-Presione tecla '9' para solicitar informacion del estado actual.\r\n");
		sprintf(str_welcome,"\n\n\tPresionando la tecla '1', podra volver a imprimir el mensaje de bienvenida.\r\n");
		sprintf(str_finish,"\n\n\t\t¡ Ahora disfrute de su afinador de confianza, A ROCKEAR ! \r\n\n\n");

		strcat(str_welcome_package,str_equals);
		strcat(str_welcome_package,str_header);
		strcat(str_welcome_package,str_equals);
		strcat(str_welcome_package,str_developers);
		strcat(str_welcome_package,str_institution);
		strcat(str_welcome_package,str_teacher);
		strcat(str_welcome_package,str_subject);
		strcat(str_welcome_package,str_func1);
		strcat(str_welcome_package,str_func1_explain);
		strcat(str_welcome_package,str_func1a);
		strcat(str_welcome_package,str_func1b);
		strcat(str_welcome_package,str_func1c);
		strcat(str_welcome_package,str_func1d);
		strcat(str_welcome_package,str_func2);
		strcat(str_welcome_package,str_func2_explain);
		strcat(str_welcome_package,str_func2_explain2);
		strcat(str_welcome_package,str_func2_lower);
		strcat(str_welcome_package,str_func2_100);
		strcat(str_welcome_package,str_func2_higher);
		strcat(str_welcome_package,str_func3);
		strcat(str_welcome_package,str_func3_explain);
		strcat(str_welcome_package,str_func3_show);
		strcat(str_welcome_package,str_welcome);
		strcat(str_welcome_package,str_finish);
		is_loaded = 1; // Guardo mensaje de bienvenida.
	}
	UART_Send(LPC_UART0,str_welcome_package,sizeof(str_welcome_package),BLOCKING);	//Envio mensaje de bienvenida via UART
}
