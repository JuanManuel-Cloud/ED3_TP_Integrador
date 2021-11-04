
#define SAMP_FREQ	40000

#define FREQ_BUFF   15

void confGPIO(void); 		// Prototipo de la funcion de conf. de puertos
void confADC(void); 		//Prototipo de la funcion de conf. de interrupciones externas
void confTimers(void); 		// Prototipo de la funcion de conf. de timer
void confIntGPIO(void);
void confUart(void);
void push_response(void);
void testFlagLED(void);
void COL0_ISR(void);
void COL1_ISR(void);
void COL2_ISR(void);
void COL3_ISR(void);
void modifyBPM(int8_t value);

int8_t index = 5;
uint32_t bpmTable[11] = {
		19999,9999,8570,7500,6666,
		5999,5454,4999,4614,4285,3999
};

uint32_t abs_calc(int32_t);

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
uint32_t comp_freq = 184;
uint32_t error_margin = 40;
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
