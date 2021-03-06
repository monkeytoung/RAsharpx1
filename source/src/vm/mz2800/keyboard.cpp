/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ keyboard ]
*/

#include "keyboard.h"
#include "../i8255.h"
#include "../z80pio.h"

static const int key_map[14][8] = {
	{0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77},
	{0x78, 0x79, 0x68, 0x69, 0x6c, 0x6e, 0x6b, 0x6d},
	{0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67},
	{0x09, 0x20, 0x0d, 0x26, 0x28, 0x25, 0x27, 0x13},
	{0xbf, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47},
	{0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f},
	{0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57},
	{0x58, 0x59, 0x5a, 0xde, 0xdc, 0xe2, 0xbe, 0xbc},
	{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37},
	{0x38, 0x39, 0xba, 0xbb, 0xbd, 0xc0, 0xdb, 0x00},
	{0xdd, 0x7b, 0x24, 0x2e, 0x08, 0x1b, 0x6a, 0x6f},
	{0xa4, 0x14, 0x10, 0x15, 0xa2, 0x00, 0x00, 0x00},
	{0x1d, 0x1c, 0x00, 0xa3, 0xa5, 0x00, 0x00, 0x00},
	{0x19, 0x7a, 0x00, 0x7c, 0x7d, 0x7e, 0x7d, 0x00}
};

void KEYBOARD::initialize()
{
	key_stat = emu->get_key_buffer();
	column = 0;
	register_frame_event(this);
}

void KEYBOARD::write_signal(int id, uint32_t data, uint32_t mask)
{
	if(id == SIG_KEYBOARD_COLUMN) {
		column = data & mask;	// from z80pio port a
		create_keystat();
	}
}

void KEYBOARD::event_frame()
{
	// update key status
	uint8_t buf[256];
	memcpy(buf, key_stat, sizeof(buf));
	
	if(!buf[0x10] && buf[0x21]) {
		buf[0x7c] = 0x80;	// PAGE UP -> F13
	}
	if(!buf[0x10] && buf[0x22]) {
		buf[0x7d] = 0x80;	// PAGE UP -> F14
	}
	if(buf[0x10] && buf[0x21]) {
		buf[0x7e] = 0x80;	// SHIFT + PAGE UP -> F15
	}
	if(buf[0x10] && buf[0x22]) {
		buf[0x7f] = 0x80;	// SHIFT + PAGE DOWN -> F16
	}
	if(buf[0x2d]) {
		buf[0x10] = buf[0x2e] = 0x80;	// INS -> SHIFT + DEL
	}
	buf[0] = 0;
	
	keys[0xf] = 0xff;
	for(int i = 0; i <= 0xd; i++) {
		uint8_t tmp = 0;
		for(int j = 0; j < 8; j++) {
			tmp |= (buf[key_map[i][j]]) ? 0 : (1 << j);
		}
		keys[i] = tmp;
		keys[0xf] &= tmp;
	}
	create_keystat();
}

void KEYBOARD::create_keystat()
{
	uint8_t val = (!(column & 0x10)) ? keys[0xf] : ((column & 0xf) > 0xd) ? 0xff : keys[column & 0xf];
	d_pio0->write_signal(SIG_I8255_PORT_B, val, 0x80);	// to i8255 port b
	d_pio1->write_signal(SIG_Z80PIO_PORT_B, val, 0xff);	// to z80pio port b
}

#define STATE_VERSION	1

bool KEYBOARD::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(column);
	return true;
}

