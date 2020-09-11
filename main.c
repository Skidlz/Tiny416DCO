//2164 DCO
#define F_CPU 20000000 //3333333
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <util/delay.h>
#include "midi.h"

int bend_rng = 2;

uint8_t cur_note = 0;
int cur_bend = 0;
uint16_t old_CMP = 0;

void USART0_init(void);
void USART0_sendChar(char c);
void USART0_sendString(char *str);

void TCA0_init(void);
void PORT_init(void);

void VREF_init(void);
void DAC0_init(void);
void DAC0_setVal(uint8_t val);

void VREF_init(void) {
    VREF.CTRLA |= VREF_DAC0REFSEL_4V34_gc; /* Voltage reference at 4.34V */
    VREF.CTRLB |= VREF_DAC0REFEN_bm; /* DAC0/AC0 reference enable: enabled */
    _delay_us(25); /* Wait VREF start-up time in micros*/
}

void DAC0_init(void) {
    PORTA.PIN6CTRL &= ~PORT_ISC_gm; /* Disable digital input buffer */
    PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm; /* Disable pull-up resistor */
    
    /* Enable DAC, Output Buffer, Run in Standby */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm | DAC_RUNSTDBY_bm;
}
//--------------------------------------------------
void TCA0_init(void) {
	PORTB.DIR |= PIN0_bm | PIN1_bm; //1 CMP1 output, 0 CMP0

	TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTEI_bm); /* disable event counting */
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV8_gc /* set clock source (sys_clk/8) */
	| TCA_SINGLE_ENABLE_bm;                /* start timer */
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_FRQ_gc;
}
//----------------------------------------------------
void USART0_init(void) {
	// Set pin direction to output
	PORTB.DIR |= PIN2_bm; //tx
	PORTB.OUT &= ~PIN2_bm;
	
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(31250);
	USART0.CTRLB = 0 << USART_MPCM_bp       /* Multi-processor Communication Mode: disabled */
	| 0 << USART_ODME_bp     /* Open Drain Mode Enable: disabled */
	| 1 << USART_RXEN_bp     /* Receiver enable: enabled */
	| USART_RXMODE_NORMAL_gc /* Normal mode */
	| 0 << USART_SFDEN_bp    /* Start Frame Detection Enable: disabled */
	| 1 << USART_TXEN_bp;    /* Transmitter Enable: enabled */
}

void USART0_sendChar(char c) {
	while (!(USART0.STATUS & USART_DREIF_bm)){;}
	USART0.TXDATAL = c;
}

char USART0_readChar(void) {
	while (!(USART0.STATUS & USART_RXCIF_bm)) { ;}
	return USART0.RXDATAL;
}
//----------------------------------------------------
void set_freq(float note){
	uint16_t freq = (440/16) * pow(2,(note/12)); // note to frequency, 440 tuning
	uint16_t CMP = (F_CPU/8) / freq; // freq to timer comparison value

	uint32_t CNT = TCA0.SINGLE.CNT;
	CNT *= CMP;
	CNT /= old_CMP;
	CNT += 85; // compensate for time spent doing math
	
	PORTB.OUTTGL = PIN5_bm; //toggle LED
	TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm; // stop timer
	
	if( (TCA0.SINGLE.CNT + 85 ) >= old_CMP) CNT = CMP -1; // if we missed a reset, cue one up ASAP
	else if(CNT > CMP) CNT -= CMP;
	
	PORTB.OUTTGL = PIN5_bm; //toggle LED
	TCA0.SINGLE.CNT = (uint16_t)CNT; 

	TCA0.SINGLE.CMP0 = CMP;
	TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; // start timer
	old_CMP = CMP;
}

void note_on(uint8_t note, uint8_t vel){
	cur_note = note;
	note;// += cur_bend/4096;
	
	set_freq(note + (cur_bend * bend_rng / 8192.0));
	note = note * 2 + (cur_bend * bend_rng / 4096);
	DAC0.DATA = note;
}

void pitch_bend(int bend){
	cur_bend = bend;
	set_freq(cur_note + (bend * bend_rng / 8192.0));
	uint8_t note = (cur_note * 2) + (bend * bend_rng / 4096);
	DAC0.DATA = note;
}

//----------------------------------------------------
int main(void){
	CPU_CCP = 0xD8;
	CLKCTRL.MCLKCTRLB = 0;
	
	PORTB.DIR |= PIN5_bm; //output led
	//A6 DAC, B0 timer cmp, B2 TX, B5 LED
	
	TCA0_init();
	VREF_init();
    DAC0_init();
	USART0_init();

	set_MIDI_key_press(note_on); //set function to handle note on
	set_MIDI_pitch_bend(pitch_bend);
	
	//sei(); //enable interrupts

	while (1) {
		if (USART0.STATUS & USART_RXCIF_bm) handle_midi();
	}
}