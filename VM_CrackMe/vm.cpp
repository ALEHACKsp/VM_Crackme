#include "vm.h"
#include "tools.h"
#include "instr.h"

//класс виртуальной машины.

OldVirtualMachine::OldVirtualMachine()
{
}

OldVirtualMachine::~OldVirtualMachine()
{
	//освободим память..
	if (mem.stack.real_addr) VirtualFree((LPVOID)mem.stack.real_addr, 0,MEM_RELEASE);
	if (mem.data.real_addr) VirtualFree((LPVOID)mem.data.real_addr, 0,MEM_RELEASE);
	if (mem.code.real_addr) VirtualFree((LPVOID)mem.code.real_addr, 0,MEM_RELEASE);
}

bool OldVirtualMachine::LoadProgramm(LPVOID CodeAddr, int CodeSize, LPVOID DataAddr, int DataSize)
{
	OBF_BEGIN
	mem_zero(&mem, sizeof(mem));

	//выделим память под код
	mem.code.real_addr = (DWORD)VirtualAlloc(0,code_block_size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	IF (V(mem.code.real_addr))
		mem.code.size = N(code_block_size);
	ENDIF

	//выделим память под данные
	mem.data.real_addr = (DWORD)VirtualAlloc(0,data_block_size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	IF (V(mem.data.real_addr))
		mem.data.size = N(data_block_size);
	ENDIF

	//выделим память под стек
	mem.stack.real_addr = (DWORD)VirtualAlloc(0,stack_block_size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	IF (V(mem.stack.real_addr))
		mem.stack.size = N(stack_block_size);
	ENDIF
	//проверим что всё ок
	IF (mem.code.size == N(0) || mem.data.size == N(0) || mem.stack.size == N(0))
		RETURN(false);
	ENDIF
	//загрузим код
	IF (V(CodeSize) > N(0) && V(CodeSize) < N(code_block_size))
		//тупо скопируем
		mem_copy(CodeAddr, (LPVOID)mem.code.real_addr, V(CodeSize));
	ELSE
		RETURN(false);
	ENDIF

	//загрузим данные
	IF (DataSize > 0 && DataSize < data_block_size)
		//тупо скопируем
		mem_copy(DataAddr, (LPVOID)mem.data.real_addr, V(DataSize));
	ELSE
		RETURN(false);
	ENDIF

	//настроим виртуальную память
	mem.code.virt_addr = N(code_base);
	mem.data.virt_addr = N(data_base);
	mem.stack.virt_addr = N(stack_base);

	//настроим регистры
	mem_zero(&reg, sizeof(reg));
	reg.eip = (DWORD)mem.code.virt_addr;
	reg.esp = (DWORD)mem.stack.virt_addr;

	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::Run()
{
	OBF_BEGIN
	//запускаем каждую команду по одной.
	WHILE(exec_command())
	ENDWHILE
	RETURN(false); //бесполезно
	OBF_END
}


///////////////////////////////////////////////////////////
//управление памятью
///////////////////////////////////////////////////////////

DWORD OldVirtualMachine::convert_addr(DWORD addr, bool virt_to_real)
{
	OBF_BEGIN
	//конвертер адресов
	//адреса идут один за другим
	section* section = 0;
	IF (V(virt_to_real))
		//узнаем из какой секции виртуальный адрес.
		IF (V(addr) >= V(mem.code.virt_addr) && V(addr) <= V(mem.code.virt_addr) + V(mem.code.size)) section = &mem.code; ENDIF
		IF (V(addr) >= V(mem.data.virt_addr) && V(addr) <= V(mem.data.virt_addr) + V(mem.data.size)) section = &mem.data; ENDIF
		IF (V(addr) >= V(mem.stack.virt_addr) && V(addr) <= V(mem.stack.virt_addr) + V(mem.stack.size)) section = &mem.stack; ENDIF
		IF (section == 0)RETURN(false);ENDIF //инвалид адрес ENDIF
		DWORD ss=(DWORD)section->real_addr;
		DWORD real = (V(addr) - V(section->virt_addr)) + V(ss);
		RETURN (real);
	ELSE
		//узнаем из какой секции реальный адрес.
		IF (V(addr) >= V(mem.code.real_addr) && V(addr) <= V(mem.code.real_addr) + V(mem.code.size)) section = &mem.code; ENDIF
		IF (V(addr) >= V(mem.data.real_addr) && V(addr) <= V(mem.data.real_addr) + V(mem.data.size)) section = &mem.data; ENDIF
		IF (V(addr) >= V(mem.stack.real_addr) && V(addr) <= V(mem.stack.real_addr) + V(mem.stack.size)) section = &mem.stack; ENDIF
		IF (section == 0)RETURN(false);ENDIF //инвалид адрес
		DWORD ss=(DWORD)section->virt_addr;
		DWORD real = (V(addr) - V(section->real_addr)) + V(ss);
		RETURN (real);
	ENDIF
	OBF_END
}

//проверка принадлежности виртуальных адресов к секциям
bool OldVirtualMachine::IsValid_VirtAddr(DWORD addr, DWORD size, section* sect)
{
	OBF_BEGIN
	IF (V(addr) >= V(sect->virt_addr) && (V(addr) + V(size)) <= (V(sect->virt_addr) + V(sect->size))) RETURN(true); ENDIF
	RETURN(false);
	OBF_END
}

//сразу во всех секциях
bool OldVirtualMachine::IsValid_VirtAddrAll(DWORD addr, DWORD size)
{
	OBF_BEGIN
	IF (IsValid_VirtAddr(V(addr), V(size), &mem.code)) RETURN(true); ENDIF
	IF (IsValid_VirtAddr(V(addr), V(size), &mem.data)) RETURN(true); ENDIF
	IF (IsValid_VirtAddr(V(addr), V(size), &mem.stack)) RETURN(true); ENDIF
	RETURN(false);
	OBF_END
}


///////////////////////////////////////////////////////////
//парсер опкодов
///////////////////////////////////////////////////////////

//дизассемблер байт-кода
bool OldVirtualMachine::parse_command()
{
	OBF_BEGIN
		//проверим выход за пределы секции кода
		IF(V(reg.eip) + max_opcode_size > (V(mem.code.virt_addr) + V(mem.code.size)))
		RETURN(false);
	ENDIF
		//распарсим по параметрам.
		DWORD real_addr = convert_addr(V(reg.eip), true);
	opcode opc;
	opc.opcodes = V(*(byte*)real_addr);
	opc.param1.byte = (*(byte*)(V(real_addr) + N(1)));
	opc.param2.byte = (*(byte*)(V(real_addr) + N(2)));
	//dword не может стоять на первом месте
	opc.param2.dword = (*(DWORD*)(V(real_addr) + N(2)));

	switch (V(opc.opcodes))
	{
	case halt:
		cmd.id = halt;
		cmd.len = N(1);
		break;
	case push_reg:
		cmd.id = push_reg;
		cmd.len = N(2);
		cmd.operand = V(opc.param1.byte);
		break;
	case pop_reg:
		cmd.id = pop_reg;
		cmd.len = N(2);
		cmd.operand = V(opc.param1.byte);
		break;
	case cmp_reg_reg:
		cmd.id = cmp_reg_reg;
		cmd.len = N(3);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.byte);
		break;
	case mov_reg_reg:
		cmd.id = mov_reg_reg;
		cmd.len = N(3);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.byte);
		break;
	case mov_reg_mem:
		cmd.id = mov_reg_mem;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.dword);
		break;
	case jmp:
		cmd.id = jmp;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte); //условие перехода
		cmd.operand2 = V(opc.param2.dword); // дельта
		break;
	case add_reg_reg:
		cmd.id = add_reg_reg;
		cmd.len = N(3);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.byte);
		break;
	case mod_reg_imm:
		cmd.id = mod_reg_imm;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.dword);
		break;
	case shl_reg_reg:
		cmd.id = shl_reg_reg;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.dword);
		break;

	case mov_reg_byte:
		cmd.id = mov_reg_byte;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte); //регистр
		cmd.operand2 = V(opc.param2.dword); //адрес
		break;

	case mov_reg_imm:
		cmd.id = mov_reg_imm;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte); //регистр
		cmd.operand2 = V(opc.param2.dword); //значение
		break;

	case mov_reg_ptr_reg:
		cmd.id = mov_reg_ptr_reg;
		cmd.len = N(3);
		cmd.operand = V(opc.param1.byte); //регистр
		cmd.operand2 = V(opc.param2.byte); //регистр
		break;

	case add_reg_imm:
		cmd.id = add_reg_imm;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte); //регистр
		cmd.operand2 = V(opc.param2.dword); //регистр
		break;

	case xor_byte_ptr_reg:
		cmd.id = xor_byte_ptr_reg;
		cmd.len = N(3);
		cmd.operand = V(opc.param1.byte); //регистр
		cmd.operand2 = V(opc.param2.byte); //параметр
		break;


	case antidebug_1:
		cmd.id = antidebug_1;
		cmd.len = N(6);
		cmd.operand = V(opc.param1.byte);
		cmd.operand2 = V(opc.param2.dword);
		break;

	default:
		cmd.id = INVALID_OPCODE;
		cmd.len = N(0);
		RETURN(false);
	}
	RETURN(true);
	OBF_END
}

//переход на обработчик команды
bool OldVirtualMachine::exec_command()
{
	OBF_BEGIN
	IF (!parse_command()) RETURN (false);ENDIF

	switch (V(cmd.id))
	{
	case halt:
		cmd_halt();
		RETURN(false);
	case push_reg:
		cmd_push(V(cmd.operand));
		break;
	case pop_reg:
		cmd_pop(V(cmd.operand));
		break;
	case cmp_reg_reg:
		cmd_cmp_reg_reg(V(cmd.operand), V(cmd.operand2));
		break;
	case mov_reg_reg:
		cmd_mov_reg_reg(V(cmd.operand), V(cmd.operand2));
		break;
	case mov_reg_mem:
		cmd_mov_reg_mem(V(cmd.operand), V(cmd.operand2));
		break;

	case jmp:
		cmd_jmp(V(cmd.operand2), V(cmd.operand));
		break;

	case add_reg_reg:
		cmd_add_reg_reg(V(cmd.operand), V(cmd.operand2));
		break;

	case mod_reg_imm:
		cmd_mod_reg_imm(V(cmd.operand), V(cmd.operand2));
		break;

	case shl_reg_reg:
		cmd_shl_reg_reg(V(cmd.operand), V(cmd.operand2));
		break;

	case mov_reg_byte:
		cmd_mov_reg_byte(V(cmd.operand), V(cmd.operand2));
		break;

	case mov_reg_imm:
		cmd_mov_reg_imm(V(cmd.operand), V(cmd.operand2));
		break;

	case mov_reg_ptr_reg:
		cmd_mov_reg_ptr_reg(V(cmd.operand), V(cmd.operand2));
		break;

	case add_reg_imm:
		cmd_add_reg_imm(V(cmd.operand), V(cmd.operand2));
		break;

	case xor_byte_ptr_reg:
		cmd_xor_byte_ptr_reg(V(cmd.operand), V(cmd.operand2));
		break;

	case antidebug_1:
		cmd_antidebug_1(V(cmd.operand), V(cmd.operand2));
		break;

	default:
		RETURN(false);
	}
	//след команда..
	reg.eip += V(cmd.len);
	RETURN(true);
	OBF_END
}

DWORD OldVirtualMachine::GetRegValue(int regs)
{
	OBF_BEGIN
	switch (V(regs))
	{
	case eax:
		return V(reg.eax);
	case ecx:
		return V(reg.ecx);
	case edx:
		return V(reg.edx);
	case ebx:
		return V(reg.ebx);
	case ebp:
		return V(reg.ebp);
	case esp:
		return V(reg.esp);
	case eip:
		return V(reg.eip);
	default:
		//бесполезно - надо генерить исключение
		RETURN(false);
	}
	OBF_END
}


void OldVirtualMachine::SetRegValue(int regs, DWORD value)
{
	switch (regs)
	{
	case eax:
		reg.eax = value;
		break;
	case ecx:
		reg.ecx = value;
		break;
	case edx:
		reg.edx = value;
		break;
	case ebx:
		reg.ebx = value;
		break;
	case ebp:
		reg.ebp = value;
		break;
	case esp:
		reg.esp = value;
		break;
	case eip:
		reg.eip = value;
		break;
	default:
		//бесполезно - надо генерить исключение
		break;
	}
}

///////////////////////////////////////////////////////////
//обработчики опкодов
///////////////////////////////////////////////////////////
bool OldVirtualMachine::cmd_push(DWORD regs)
{
	OBF_BEGIN
	//printf("push",regs);
	//засунем в стек данные
	//проверим выход за пределы стека
	//возврат false - крушение стека!
	IF (IsValid_VirtAddr(V(reg.esp), N(4), &mem.stack)) RETURN(false);ENDIF

	DWORD real_addr = convert_addr(V(reg.esp), true);
	IF (V(real_addr))
		*(DWORD*)real_addr = GetRegValue(V(regs));
		DWORD s=sizeof(DWORD);
		reg.esp += V(s);
		RETURN(true);
	ENDIF

	RETURN(false);
	OBF_END
}

bool OldVirtualMachine::cmd_pop(DWORD regs)
{
	OBF_BEGIN
	//printf("pop",regs);
	//получим данные из стека
	IF (IsValid_VirtAddr(V(reg.esp) - N(4), N(4), &mem.stack)) RETURN(false); ENDIF

	DWORD real_addr = convert_addr(V(reg.esp), true);
	IF (V(real_addr))
		DWORD value = V(*(DWORD*)real_addr);
		SetRegValue(V(regs), V(value));
		DWORD s=sizeof(DWORD);
		reg.esp -= V(s);
		RETURN(true);
	ENDIF

	RETURN(false);
	OBF_END
}


bool OldVirtualMachine::cmd_halt()
{
	OBF_BEGIN
	//printf("halt");
	RETURN(true);
	OBF_END
}


bool OldVirtualMachine::cmd_cmp_reg_reg(DWORD regs, DWORD regs2)
{
	OBF_BEGIN
	//printf("cmp reg,reg");
	//сравнение двух регистров
	DWORD reg1 = GetRegValue(V(regs));
	DWORD reg2 = GetRegValue(V(regs2));
	IF (V(reg1) == V(reg2))
		reg.zero_flag = true;
		reg.carry_flag = false;
	ENDIF
	IF (V(reg1) > V(reg2))
		reg.zero_flag = false;
		reg.carry_flag = false;
	ENDIF
	IF (V(reg1) < V(reg2))
		reg.zero_flag = false;
		reg.carry_flag = true;
	ENDIF
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_mov_reg_reg(DWORD regs, DWORD regs2)
{
	OBF_BEGIN
	//printf("mov reg,reg");
	//записываем одно значение в другое
	SetRegValue(V(regs), GetRegValue(V(regs2)));
	RETURN(true);
	OBF_END
}

//нужна функция проверки адреса!!
bool OldVirtualMachine::cmd_mov_reg_mem(DWORD regs, DWORD addr)
{
	OBF_BEGIN
	//printf("mov reg,mem");
	//записываем одно значение в другое
	IF (!IsValid_VirtAddrAll(V(addr), N(4))) RETURN(false); ENDIF

	DWORD real_addr = convert_addr(V(addr), true);
	DWORD real_src = V(*(DWORD*)real_addr);
	SetRegValue(V(regs), V(real_src));
	RETURN(true);
	OBF_END
}


bool OldVirtualMachine::cmd_jmp(DWORD delta, DWORD condition)
{
	OBF_BEGIN
	//printf("JMP");
	//проверим не выйдем ли за предел..
	DWORD NewEip = V(reg.eip) + V(delta);
	bool cond_valid = false;
	IF (!IsValid_VirtAddr(V(NewEip), N(1), &mem.code)) RETURN(false); ENDIF

	switch (condition)
	{
	case jmp_above:
		IF (reg.carry_flag == 0 && reg.zero_flag == 0) cond_valid = true; ENDIF
		break;
	case jmp_below:
		IF (reg.carry_flag == 1 && reg.zero_flag == 0) cond_valid = true;ENDIF
		break;
	case jmp_equal:
		IF (reg.carry_flag == 0 && reg.zero_flag == 1) cond_valid = true;ENDIF
		break;
	default:
		return (false);
	}
	//сменим EIP
	IF (cond_valid)
		reg.eip = V(NewEip);
		RETURN(true);
	ENDIF
	RETURN(false);
	OBF_END
}


bool OldVirtualMachine::cmd_add_reg_reg(DWORD reg1, DWORD reg2)
{
	OBF_BEGIN
	//printf("add reg,reg");
	//прибавляем один регистр к другому
	SetRegValue(V(reg1), GetRegValue(V(reg1)) + GetRegValue(V(reg2)));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_mod_reg_imm(DWORD reg1, DWORD imm)
{
	//printf("mod reg,reg");
	//остаток от деления
	OBF_BEGIN
	DWORD regs1 = GetRegValue(V(reg1));
	regs1 = V(regs1) % V(imm);
	SetRegValue(V(reg1), V(regs1));
	RETURN(true);
	OBF_END
}

bool trick1()
{
	OBF_BEGIN
	int i = N(0);
	FOR(V(i), V(i) < N(1000), V(i)++)OutputDebugStringW(GetCommandLineW());
	ENDFOR
	OBF_END
}

bool OldVirtualMachine::cmd_antidebug_1(DWORD reg1, DWORD imm)
{
	OBF_BEGIN
	//printf("antidebug");
	trick1();
	RETURN(true);
	OBF_END
}

void trick2()
{
	//!!
}

bool OldVirtualMachine::cmd_shl_reg_reg(DWORD reg1, DWORD reg2)
{
	//printf("shl reg,reg");
	//побитовое смещение
	OBF_BEGIN
	DWORD regs1 = GetRegValue(V(reg1));
	DWORD regs2 = V(reg2);
	regs1 = V(regs1) << V(regs2);
	SetRegValue(V(reg1), V(regs1));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_mov_reg_byte(DWORD reg1, DWORD addr)
{
	//printf("mov reg,byte ptr [addr]");
	OBF_BEGIN
	DWORD real_addr = convert_addr(V(addr), true);
	BYTE real_src = V(*(byte*)real_addr);
	SetRegValue(V(reg1), V(real_src));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_mov_reg_imm(DWORD reg1, DWORD imm)
{
	OBF_BEGIN
	//printf("mov reg,imm");
	SetRegValue(V(reg1), V(imm));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_mov_reg_ptr_reg(DWORD reg1, DWORD reg2)
{
	OBF_BEGIN
	//printf("mov reg,byte ptr [reg]");
	DWORD real_addr = GetRegValue(V(reg2));
	IF (!IsValid_VirtAddrAll(V(real_addr), N(4))) RETURN(false);ENDIF
	real_addr = convert_addr(V(real_addr), true);
	byte real_src = V(*(byte*)real_addr);
	SetRegValue(V(reg1), V(real_src));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_add_reg_imm(DWORD reg1, DWORD imm)
{
	OBF_BEGIN
	//printf("add reg,imm");
	SetRegValue(V(reg1), GetRegValue(V(reg1)) + V(imm));
	RETURN(true);
	OBF_END
}

bool OldVirtualMachine::cmd_xor_byte_ptr_reg(DWORD reg1, DWORD reg2)
{
	OBF_BEGIN
	//printf("xor byte ptr [reg],reg2
	DWORD real_addr = GetRegValue(reg1);
	IF (!IsValid_VirtAddrAll(V(real_addr), N(4))) RETURN(false);ENDIF
	real_addr = convert_addr(V(real_addr), true);
	byte real_src = V(*(byte*)real_addr);
	real_src ^= (byte)GetRegValue(V(reg2));
	*(byte*)real_addr = V(real_src);
	RETURN(true);
	OBF_END
}

char prog[] = "\x0B\x00\x01\x10\x00\x00\x0A\x01\x00\x10\x00\x00\x0B\x02\x9A\x02\x00\x00\x0B\x03\x00\x00\x00\x00\x0F\x03\x03\x00\x30\x00\x0E\x00\x02\x07\x02\x01\x0D\x00\x01\x00\x00\x00\x0D\x01\xFF\xFF\xFF\xFF\x03\x01\x03\x06\x00\xDF\xFF\xFF\xFF\x00";
char valid[] = "\x12\xec\xc5\xcb\xac\xfc\x86\x96\x23\x7c\x7d\x57\x46\x5c\x43\x4f\x56\x2d\x2a";

bool OldVirtualMachine::TestVm(const char* sec)
{
	int len = lstrlenA(sec);
	if (len > 100) return false;
	char buf[512];
	wsprintfA(buf, "x%s", sec);
	*(byte*)buf = len;

	OldVirtualMachine MyVm;
	DWORD tmp;
	tmp = MyVm.LoadProgramm(prog, sizeof(prog), buf, len + 1);
	tmp = MyVm.Run();

	return CompareString2((byte*)MyVm.mem.data.real_addr, (byte*)valid, sizeof(valid));
}
