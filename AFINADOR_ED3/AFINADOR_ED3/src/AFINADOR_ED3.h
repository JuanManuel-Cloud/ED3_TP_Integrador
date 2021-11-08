// ************************** Librerias incluidas ************************************************
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

// ************************** Definicion de constantes globales ***********************************
#define SAMP_FREQ	40000	// Frecuencia de muestreo para calculo de frecuencia detectada.
#define FREQ_BUFF   15		// Tamaño de buffer de ultimas det_freq para encontrar minimo.
#define DMA_SIZE 500		// Cantidad de datos a enviar por transferencia DMA de memoria a UART.
#define SERIAL_CHAR 5		// Cantidad de caracteres totales para reemplazo en buffer a enviar.
// *** Posiciones en buffer a enviar por UART al presionar tecla "9" ***
#define POW_AFI_POS 104			// Posicion de estado de afinador.
#define COMP_FREQ_POS 140		// Posicion de frecuencia de comparacion.
#define MARGIN_ERROR_POS 180	// Posicion de margen de error.
#define LAST_FREQ_DET_POS 231	// Posicion de frecuencia detectada.
#define DIFF_FREQ_POS 308		// Posicion de diferencia de frecuencia a compensar para afinar
#define POW_MET_POS 430			// Posicion de estado de metronomo.
#define BPM_POS 455				// Posicion de BPM configurado en metronomo.

// *********************** Prototipos de funciones ************************************************
// *** Configuraciones iniciales ***
void confGPIO(void); 		// Funcion de configuracion de pines utilizados.
void confADC(void); 		// Funcion de configuracion ADC sin habilitacion de interrupcion.
void confTimers(void); 		// Funcion de configuracion systick y perifericos TIMER.
void confIntGPIO(void);		// Funcion de configuracion interrupciones TECLADO.
void confUart(void);		// Funcion de configuracion de periferico UART en 115200 baudios.
void confDMA(void);			// Funcion de preparacion de variables para configurar controlador DMA.
// *** Funciones de respuesta al teclado ****
void push_response(void);	// Funcion para ordenar llamado por columnas.
void COL0_ISR(void);		// Funcion para ordenar llamados por filas de columna 0.
void COL1_ISR(void);		// Funcion para ordenar llamados por filas de columna 1.
void COL2_ISR(void);		// Funcion para ordenar llamados por filas de columna 2.
void COL3_ISR(void);		// Funcion para ordenar llamados por filas de columna 3.
// **************** Funciones complementarias a funcionalidades provistas al usuario****************
// *** Mensaje de bienvenida ***
void welcomeMessage(void); 	// Se ejecuta al comienzo del programa y al presionar el boton "1"
							// Variable is_loaded evita volver a cargar el buffer.
// *** Funcionalidad 1 ***
void testFlagLED(void);		// Enciende/apaga leds segun distancia a frecuencia elegida por usuario.
// *** Funcionalidad 2 ***
void modifyBPM(int8_t value);// Modifica los BPM sobreescribiendo valor de Match 1 de Timer 3.
// *** Funcionalidad 3 ***
void UART_strings(void);	// Prepara el buffer a enviar por DMA, con espacios en campos a sustituir.
							// UART_strings() se ejecuta una sola vez y al comienzo.
void catFrecValue(void);	// Define los valores a sustituir y llama 7 veces a la funcion sustFunction()
void sustFunction(char str_aux[5],uint32_t sust_pos);
							// Sustituye en buffer a enviar, los valores ingresados por catFrecValue(void)
// **************** Variables utilizadas en el programa *******************************************

// *** Variables funcionalidad 1: AFINADOR ***
uint32_t freq_buff[FREQ_BUFF];	// Buffer con ultimas 15 frecuencias detectadas.
uint32_t current_sample = 0;	// Muestra actual tomada por el ADC.
uint32_t old_sample = 0;		// Muestra anterior tomada por el ADC.
uint32_t is_crossing = 0;		// Flag de aviso de deteccion de cruce por mitad de rango dinamico.
uint32_t rising_sample = 0;		// Almacena valor de cuenta ante un cruce ascendente.
uint32_t falling_sample = 0;	// Almacena valor de cuenta ante un cruce descendente.
uint32_t sample_count = 0;		// Lleva cuenta de interrupciones ADC para asignar a rising y falling sample.
uint32_t det_freq = 0;			// Variable para almacenar resultado de formula de deteccion de frecuencia.
uint32_t old_det_freq;			// Variable para almacenar anterior frecuencia detectada.
uint32_t det_freq_aux;			// Variable auxiliar para encontrar minimo valor de frecuencia detectada.
uint32_t aux_freq = 0;			// Variable auxiliar para recorrer buffer de ultimas frecuencias detectadas.
uint32_t comp_freq = 0;			// Almacena eleccion del usuario de frecuencia deseada de afinacion.
uint32_t error_margin = 0;		// Margen de error calculado como la 5ta parte de la frecuencia elegida.

// *** Variables funcionalidad 2: METRONOMO ***
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
// *** Variables funcionalidad 3: MONITOREO ***
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

// *** Variables funcionamiento Teclado ***
uint8_t test_count = 0;
uint8_t row_value = 0;

// *** Variables mensaje bienvenida ***
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

