
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_gpdma.h"


#define SAMP_FREQ	40000
#define FREQ_BUFF   15

#define DMA_SIZE 500//468
#define SERIAL_CHAR 5

#define POW_AFI_POS 104
#define COMP_FREQ_POS 140
#define MARGIN_ERRROR_POS 180
#define LAST_FREQ_DET_POS 231
#define DIFF_FREQ_POS 308
#define POW_MET_POS 430
#define BPM_POS 455

void confGPIO(void); 		// Prototipo de la funcion de conf. de puertos
void confADC(void); 		//Prototipo de la funcion de conf. de interrupciones externas
void confTimers(void); 		// Prototipo de la funcion de conf. de timer
void confIntGPIO(void);
void confUart(void);
void confDMA(void);
void push_response(void);
void testFlagLED(void);
void COL0_ISR(void);
void COL1_ISR(void);
void COL2_ISR(void);
void COL3_ISR(void);
void modifyBPM(int8_t value);
void catFrecValue(void);
void UART_strings(void);
void sustFunction(char str_aux[5],uint32_t sust_pos);
void welcomeMessage(void);

TIM_MATCHCFG_Type match1_Metronomo;
int8_t index = 0;
uint32_t matchValue;
uint32_t bpmTable[11] = {
		19999,9999,8570,7500,6666,
		5999,5454,4999,4614,4285,3999
};
uint8_t bpmValueTable[11] = {
		50,60,70,80,90,100,110,120,130,140,150
};

 // ****** Variables mensaje bienvenida *******
uint8_t is_loaded = 0; 		// Variable para carga unica de mensaje
char str_welcome_package[1700];
char str_equals[100];
char str_header[100];
char str_developers[80];
char str_institution[110];
char str_teacher[50];
char str_subject[50];
char str_func1[60];
char str_func1_explain[110];
char str_func1a[60];
char str_func1b[60];
char str_func1c[60];
char str_func1d[60];
char str_func2[60];
char str_func2_explain[110];
char str_func2_explain2[110];
char str_func2_lower[65];
char str_func2_100[60];
char str_func2_higher[65];
char str_func3[60];
char str_func3_explain[110];
char str_func3_show[100];
char str_welcome[110];
char str_finish[65];

GPDMA_Channel_CFG_Type GPDMA_Struct;

uint8_t TIMER3_IntStatus = 0;
uint8_t ADC_IntStatus = 0;

char str_send_package[760];
char str_header[100];
char str_header_afinador[100];
char str_afinador_state[60];
char str_comp_freq[60];
char str_error_margin[60];
char str_det_freq[60];
char str_diff_freq[100];
char str_header_met[100];
char str_bpm_state[60];
char str_bpm_value[60];

// Variables funcionalidad deteccion de frecuencia

uint32_t freq_buff[FREQ_BUFF];
uint32_t old_sample = 0;
uint32_t current_sample = 0;
uint32_t adcValue = 0;
uint32_t is_crossing = 0;
uint32_t rising_sample = 0;
uint32_t falling_sample = 0;
uint32_t sample_count = 0;
uint32_t det_freq = 0;
uint32_t det_freq_aux;
uint32_t comp_freq = 0;
uint32_t error_margin = 0;
uint32_t old_det_freq;

uint32_t aux = 0;
uint32_t parc_sum = 0;
uint32_t aux_freq = 0;
uint32_t parc_sum_freq = 0;

// Variables funcionalidad UART
char UART_array[64];

// Variables funcionalidad Keyboard
uint8_t test_count = 0;
uint8_t row_value = 0;
uint8_t port2GlobalInt = 0;
