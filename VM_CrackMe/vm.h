#pragma once

#define ITHARE_OBF_SEED  0x4b295ebab3333abc
#define ITHARE_OBF_SEED2 0x36e007a38ae8e0ea

#include "obf/obf.h"

#include "Windows.h"

struct virt_proc
{
	DWORD eax;
	DWORD ecx;
	DWORD edx;
	DWORD ebx;
	DWORD ebp;
	DWORD esp;
	DWORD eip;
	bool zero_flag;
	bool carry_flag;
};

struct section
{
	DWORD virt_addr;
	DWORD real_addr;
	DWORD size;
};

struct virt_mem
{
	section code;
	section data;
	section stack;
};

#define code_block_size 0x1000
#define data_block_size  0x1000
#define stack_block_size  0x10000

#define code_base 0
#define data_base code_base+code_block_size
#define stack_base data_base+data_block_size

struct command
{
	DWORD id;
	DWORD operand;
	DWORD operand2;
	int len;
};

enum Command
{
	halt=0x00,
	push_reg,
	pop_reg,
	cmp_reg_reg,
	mov_reg_reg,
	mov_reg_mem,
	jmp,
	add_reg_reg,
	mod_reg_imm,
	shl_reg_reg,
	mov_reg_byte,
	mov_reg_imm,
	mov_reg_ptr_reg,
	add_reg_imm,
	xor_byte_ptr_reg,
	antidebug_1
};

enum Regs { eax=0x00, ecx, edx, ebx, ebp, esp, eip };

#define max_opcode_size 10 //2-4-4
#define INVALID_OPCODE -1

enum Jmps { jmp_above, jmp_below, jmp_equal };


struct param
{
	byte byte;
	DWORD dword;
};

struct opcode
{
	byte opcodes;
	param param1;
	param param2;
};

class OldVirtualMachine
{
public:
	OldVirtualMachine();
	~OldVirtualMachine();
	bool LoadProgramm(LPVOID CodeAddr, int CodeSize, LPVOID DataAddr, int DataSize);
	bool Run();
	virt_mem mem;
	static bool TestVm(const char* sec);

private:
	virt_proc reg;
	command cmd;
	bool cmd_push(DWORD reg);
	bool cmd_pop(DWORD reg);
	static bool cmd_halt();
	DWORD GetRegValue(int regs);
	void SetRegValue(int regs, DWORD value);
	bool parse_command();
	DWORD convert_addr(DWORD addr, bool virt_to_real);
	bool exec_command();
	bool cmd_cmp_reg_reg(DWORD regs, DWORD regs2);
	bool cmd_mov_reg_reg(DWORD regs, DWORD regs2);
	bool cmd_jmp(DWORD delta, DWORD condition);
	bool cmd_mov_reg_mem(DWORD regs, DWORD addr);
	static bool IsValid_VirtAddr(DWORD addr, DWORD size, section* sect);
	bool IsValid_VirtAddrAll(DWORD addr, DWORD size);
	bool cmd_add_reg_reg(DWORD reg1, DWORD reg2);
	bool cmd_mod_reg_imm(DWORD reg1, DWORD reg2);
	bool cmd_shl_reg_reg(DWORD reg1, DWORD reg2);
	bool cmd_mov_reg_byte(DWORD reg1, DWORD addr);
	bool cmd_mov_reg_imm(DWORD reg1, DWORD imm);
	bool cmd_mov_reg_ptr_reg(DWORD reg1, DWORD reg2);
	bool cmd_add_reg_imm(DWORD reg1, DWORD imm);
	bool cmd_xor_byte_ptr_reg(DWORD reg1, DWORD imm);
	static bool cmd_antidebug_1(DWORD reg1, DWORD imm);
};

bool TestVm(char* sec);
