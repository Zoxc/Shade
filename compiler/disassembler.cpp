#include "disassembler.hpp"
#include "compiler.hpp"

#define WINDOWS
#define X86_32
#define DYNAMORIO_STANDALONE
#include <dr_api.h>
#include <iostream>

static void *dr;

std::ofstream Shade::code_log;

void Shade::detour(void *address, void *target, void *&trampoline)
{
	const size_t instr_max = 17;

	auto list = instrlist_create(dr);
	
	byte instr_data[instr_max];

	byte *current = (byte *)address;
	byte *min_pos = (byte *)address + 5;
	size_t size = 0;

	while(current < min_pos)
	{
		read(current, instr_data, instr_max);

		auto instr = instr_create(dr);

		byte *decoded = decode_from_copy(dr, instr_data, current, instr);

		if(!decoded)
			error("Unknown instruction");

		instrlist_append(list, instr);
		instr_make_persistent(dr, instr);

		current += (size_t)(decoded - instr_data);
		
		size += instr_length(dr, instr);
	}
	
	auto instr = INSTR_CREATE_jmp(dr, opnd_create_pc(current));
	size += instr_length(dr, instr);
	instrlist_append(list, instr);

	auto local_trampoline = alloca(size);
	
	if(!local_trampoline)
		error("Out of memory");
	
	void *remote = code_section.allocate(size, 4);
	
	if(!instrlist_encode_to_copy(dr, list, (byte *)local_trampoline, (byte *)remote, 0, true))
		error("Unable to encode instructions");

	instrlist_clear_and_destroy(dr, list);
	
	write(remote, local_trampoline, size);

	trampoline = remote;

	char code[5];
	
	DWORD offset = (size_t)target - (size_t)address - 5;
	
	code[0] = 0xE9; 
	
	*(DWORD *)(code + 1) = offset;
	
	access(address, 5, [&] {
		write(address, code, 5);
	});
}

void Shade::init_disassembler()
{
	disassemble_set_syntax(DR_DISASM_INTEL);

	code_log.open("code_log.txt");
}

static void print_instr(byte *address, instr_t *instr)
{
	char buffer[0x100];

	instr_disassemble_to_buffer(0, instr, buffer, 0x100);

	Shade::code_log << "0x" << (void *)address << " " << buffer << std::endl;
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
