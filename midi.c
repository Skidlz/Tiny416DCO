#ifndef byte
#define byte uint8_t
#endif
#include <stddef.h>
#include "midi.h"

//removed functions to save space

byte midi_chan = 0xff; //MIDI channel

void set_MIDI_key_press(void (*ptr)(byte, byte)){
	MIDI_key_press = ptr;
}
void set_MIDI_key_release(void (*ptr)(byte)){
	MIDI_key_release = ptr;
}
void set_cntrl_chng(void (*ptr)(byte,byte)){
	cntrl_chng = ptr;
}
void set_MIDI_pitch_bend(void (*ptr)(int)){
	MIDI_pitch_bend = ptr;
}

byte handle_com(byte com, byte arg_cnt, byte *args) {
	switch (com & 0xf0) {
	case 0x90: {//key press
		if(arg_cnt < 2)return arg_cnt;
		byte note = args[0];
		byte vel = args[1];
		if(vel > 0){ //on
			if(MIDI_key_press != NULL) MIDI_key_press(note, vel);
		} else if(MIDI_key_release != NULL){ //off
			MIDI_key_release(note);
		}
		return 0; //zero args remain
	}
	case 0x80:  //key release
		if(arg_cnt < 2)return arg_cnt;
		byte note = args[0];
		//byte vel = args[1];
		if(MIDI_key_release != NULL)MIDI_key_release(note);
		return 0;
	case 0xb0: //control
		if(arg_cnt < 2)return arg_cnt;
		byte con = args[0];
		byte val = args[1];
		if(cntrl_chng != NULL)cntrl_chng(con, val);
		return 0;
	case 0xe0: //pitch bend
		if(arg_cnt < 2)return arg_cnt;
		int value = args[0] | (args[1] << 7); //msb
		if(MIDI_pitch_bend != NULL)MIDI_pitch_bend(value);
		return 0;
	//case 0xc0: //Program Change 1 arg
	//case 0xa0: //Polyphonic Key Pressure (Aftertouch) 2 arg
	//case 0xd0: //Channel Pressure 1 arg
	default: //error
		return arg_cnt;
	}
}

void handle_midi() {
	static byte last_com = 0x90;
	static byte arg_cnt = 0;
	static byte args[2];
	const int arg_max = 2;
	//function returns remaining args
	byte loop_temp = USART0_readChar();
	if (loop_temp < 0xf8){ //not realtime
		if (loop_temp & 0x80) { //command
			last_com = loop_temp;
			arg_cnt = 0;
		} else if (arg_cnt < arg_max){
			args[arg_cnt++] = loop_temp;
			if (arg_cnt >= arg_max) arg_cnt = arg_max; //prevent overflow
		}

		if (last_com < 0xf0) {
			if (midi_chan == 0xff) midi_chan = last_com & 0x0f; //define our chan?
			
			if ((last_com & 0x0f) == midi_chan) arg_cnt = handle_com(last_com, arg_cnt, args); //right chan
			//else arg_cnt = skip_com(last_com, arg_cnt, args); //wrong channel
		}
	}
}

