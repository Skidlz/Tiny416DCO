/*
 * midi.h
 *
 *  Created on: May 19, 2014
 *      Author: Owner
 */
#include <stdint.h>
#ifndef byte
#define byte uint8_t
#endif
#ifndef MIDI_H_
#define MIDI_H_
byte handle_com(byte com, byte arg_cnt, byte *args);
byte skip_com(byte com, byte arg_cnt, byte *args);
void handle_midi(void);
void set_MIDI_key_press(void (*ptr)(byte, byte));
void set_MIDI_key_release(void (*ptr)(byte));
void set_cntrl_chng(void (*ptr)(byte,byte));
void set_MIDI_pitch_bend(void (*ptr)(int));

void (*MIDI_key_press)(byte, byte);
void (*MIDI_key_release)(byte);
void (*cntrl_chng)(byte, byte);
void (*MIDI_pitch_bend)(int);

#endif /* MIDI_H_ */
