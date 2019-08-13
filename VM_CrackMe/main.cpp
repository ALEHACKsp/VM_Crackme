#define ITHARE_OBF_SEED  0x4b295ebab3333abc
#define ITHARE_OBF_SEED2 0x36e007a38ae8e0ea

#include "obf/obf.h"

#include "vm.h"
#include "instr.h"
#include <iostream>
#include <Windows.h>
#include <string>

inline void log()
{
}

template <typename First, typename ...Rest>
void log(First&& message, Rest&& ...rest)
{
	std::cout << std::forward<First>(message);
	log(std::forward<Rest>(rest)...);
}

int main()
{
	OBF_BEGIN
	OldVirtualMachine vm;

	// correct key "viva la revolution"
	std::string pass;

	std::cout << "key: ";
	std::getline(std::cin, pass);

	const char* pass_check = pass.c_str();
	bool password = vm.TestVm(pass_check);

	IF (password==true)
		log("Correct key\r\n");
	ELSE
		log("Fail\r\n");
	ENDIF

	system("pause");

	RETURN (0);
	OBF_END
}
