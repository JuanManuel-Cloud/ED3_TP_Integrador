/*
===============================================================================
 Name        : TPFINAL_ED3.c
 Author      : Magnelli Tomas, Muñoz Jose Irigoin, Gil Juan Manuel.
 Consigna :
===============================================================================
*/
#include "stdlib.h"
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define BUFFER_SIZE 600
#define SAMP_FREQ	40000

void confGPIO(void); 		// Prototipo de la funcion de conf. de puertos
void confADC(void); 		//Prototipo de la funcion de conf. de interrupciones externas
void confTimers(void); 		// Prototipo de la funcion de conf. de timer
void confIntGPIO(void);
uint32_t abs_calc(int value);

// Variables funcionalidad deteccion de frecuencia
uint32_t buffer[2];
uint32_t is_crossing = 0;
uint32_t rising_sample = 0;
uint32_t falling_sample = 0;
uint32_t sample_count = 0;
uint32_t det_freq = 0;
uint32_t comp_freq = 4200;
uint32_t old_det_freq = 0;
uint8_t is_first_cross = 1;



TIM_TIMERCFG_Type timer;
TIM_MATCHCFG_Type match;

int main(void) {
	confGPIO();
	confIntGPIO();
	confTimers();
	confADC();

	GPIO_ClearValue(0,(0xB80<<7));	// Inicializo en bajo salidas P0.11 y P0.[9:7] con 1011_1000_0000 = 0xB80

    while(1) {
    }
    return 0 ;
}
void confGPIO(void){
	PINSEL_CFG_Type pinsel;
	pinsel.Portnum = 0;			// Puerto 0
	pinsel.Pinnum = 1;			// Pin P0.11
	PINSEL_ConfigPin(&pinsel);	// Inicializo pin P0.11
	GPIO_SetDir(0,(1<<11),1); 	// Configuro pin P0.11 como salida
	pinsel.Pinmode = 0;			// Configuro resistencia de pull-up en P0.6
	pinsel.Pinnum = 6;			// Pin P0.6
	PINSEL_ConfigPin(&pinsel);	// Inicializo pin P0.6
	GPIO_SetDir(0,(1<<6),0);	// Configuro pin P0.6 como entrada
	pinsel.Pinnum = 23;         // Pin P0.23
	pinsel.Pinmode = 2;			// Configuro pin P0.23 en PINMODE1: neither pull-up nor pull-down.
	pinsel.Funcnum = 1;			// Configuro pin P0.23 como entrada analogica para AD0.0
	PINSEL_ConfigPin(&pinsel);	// Inicializo pin P0.23
	pinsel.Funcnum = 0;			// Funcion MAT2.1 para salida al overrun
	for(uint8_t i=7;i<10;i++){
		pinsel.Pinnum = i;		// CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO]
		PINSEL_ConfigPin(&pinsel); 	// Inicializo pin P0.23
		GPIO_SetDir(0,(1<<i),1); 	// Configuro pin P0.11 como salida
	}
	return;
}
void confTimers(void){
	//Configuraciones Timer 0
	timer.PrescaleOption = TIM_PRESCALE_USVAL;
	timer.PrescaleValue = 12;
	// MR1 configurado en 9 (Ti = TPR*(MR+1) = 12us*10 = 12us) => Muestreo cada 24us => Fs = 41.6666kHz
	TIM_Init(LPC_TIM0,TIM_TIMER_MODE,&timer);
	match.MatchChannel = 1;
	match.MatchValue = 9;
	match.IntOnMatch = DISABLE;
	match.StopOnMatch = DISABLE;
	match.ResetOnMatch = ENABLE;
	match.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	TIM_ConfigMatch(LPC_TIM0,&match);
	TIM_ResetCounter(LPC_TIM0);
	TIM_Cmd(LPC_TIM0,ENABLE);
	return;
}
void confIntGPIO(void){
	FIO_IntCmd(0,(1<<6),0); 	// Inicializo interrupciones por flanco ascendente en P0.6
	FIO_ClearInt(0,(1<<6));		// Limpio bandera de interrupcion en P0.6 antes de habilitarlas
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
	ADC_GlobalGetStatus(LPC_ADC,1);
	NVIC_EnableIRQ(ADC_IRQn);           // Habilito interrupciones por ADC
	return;
}
void EINT3_IRQHandler(void){
//	comp_freq = // Nueva frecuencia de comparacion segun instrumento y nota seleccionada
	FIO_ClearInt(0,(1<<6)); // Limpio bandera de interrupcion por P0.6
	return;
}
void ADC_IRQHandler(void){
	buffer[1] = buffer[0];
	buffer[0] = ADC_ChannelGetData(LPC_ADC,0);

	if(buffer[0]>2048){
		// Condicion nueva muestra mayor que "cero" (flanco ascendente)
		if(buffer[1]<2048){
			is_crossing = 1;
			rising_sample = sample_count;
		}
		else{
			is_crossing = 0;
		}
	}
	else{
		// Condicion nueva muestra menor que "cero" (flanco descendente)
		if(buffer[1]>2048){
			is_crossing = 1;
			falling_sample = sample_count;
		}
		else{
			is_crossing = 0;
		}
	}
	sample_count ++;
	if(is_crossing && (falling_sample - rising_sample)){
		if(is_first_cross){
			is_first_cross = 0;
		}
		else{
			is_first_cross = 1;
			sample_count = 0;
			det_freq = SAMP_FREQ/(abs_calc(falling_sample - rising_sample));
		}
	}
	if(det_freq != old_det_freq){    // CONEXIONES:    [P0.7 LED ROJO]    [P0.8 LED VERDE]    [P0.9 LED AMARILLO]
		if((det_freq-30)>comp_freq){ // Si la diferencia es mayor a 30 Hz
			// La frecuencia detectada se encuentra a mas de 30 Hz por encima de la deseada
			// Encender un [P0.7 LED ROJO] (INDICA AL USUARIO BAJAR LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0,(1<<7));               	// Pongo en alto P0.7
			GPIO_ClearValue(0,(3<<8));			// Pongo en bajo P0.[9:8]
		}
		else if((det_freq+30)<comp_freq){
			// La frecuencia detectada se encuentra a mas de 30 Hz por debajo de la deseada
			// Encender un [P0.9 LED AMARILLO] (INDICA AL USUARIO SUBIR LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0,(1<<9));              	// Pongo en alto P0.9
			GPIO_ClearValue(0,(3<<7));			// Pongo en bajo P0.[8:7]
		}
		else{
			// La frecuencia detectada se encuentra a menos de 30 Hz de la deseada
			// Encender un [P0.8 LED VERDE] (INDICA AL USUARIO MANTENER LA FRECUENCIA DEL INSTRUMENTO)
			GPIO_SetValue(0,(1<<8));               	// Pongo en alto P0.8
			GPIO_ClearValue(0,(5<<7));			// Pongo en bajo P0.7 y P0.9
		}
	}
	old_det_freq = det_freq;
	if((buffer[0])>2048)
		GPIO_SetValue(0,(1<<11));               // Pongo en alto P0.11 si el valor de AD0.0 > 1.65V
	else
		GPIO_ClearValue(0,(1<<11));			// Pongo en bajo P0.11 si el valor de AD0.0 < 1.65V
	return;
}
uint32_t abs_calc(int value){
	uint32_t aux;
	aux = value > 0 ? value : - value;
	return aux;
}

