#include "disassembler.hpp"

#define WINDOWS
#define X86_32
#define DYNAMORIO_STANDALONE
#include <dr_api.h>
#include <iostream>

void Shade::init_disassembler()
{
	disassemble_set_syntax(DR_DISASM_INTEL);
}

static void print_instr(byte *address, instr_t *instr)
{
	char buffer[0x100];

	instr_disassemble_to_buffer(0, instr, buffer, 0x100);

	std::cout << "0x" << (void *)address << " " << buffer << std::endl;
}

void Shade::disassemble_code(void *code, void *target, size_t size)
{
	instr_t instr;
	
	byte *pos = (byte *)code;
	byte *stop = (byte *)code + size;

	while(pos < stop)
	{
		instr_init(0, &instr);

		byte *next = decode_from_copy(0, pos, (byte *)target + (pos - (byte *)code), &instr);

		print_instr(pos - (byte *)code + (byte *)target, &instr);

		pos = next;

		instr_free(0, &instr);
	}

}
