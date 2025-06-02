#include <stdio.h>
#include <iostream>

#include "iced++/iced.hpp"

void iced_test() {
	unsigned char code[] = { 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x10, 0x48, 0x89, 0xC8, 0x48, 0x01, 0xD0, 0x4C, 0x01, 0xC0, 0x4C, 0x01, 0xC8, 0x48, 0xC7, 0xC1, 0x05, 0x00, 0x00, 0x00, 0x48, 0xF7, 0xE1, 0x48, 0x89, 0xEC, 0x5D, 0xC3 };
	iced::Decoder<true> decoder{ code, sizeof(code), 0};
	while (decoder.canDecode()) {
		auto ip = decoder.ip();
		auto instruction = decoder.decode();
		std::cout << std::hex << ip << " " << instruction.toString() << std::endl;
		std::cout << "\tOperand count: " << (int)instruction.operandCount() << std::endl;
		for (auto i = 0u; i < instruction.operandCount(); ++i) {
			auto op = instruction.operands;
			std::cout << std::dec << "\t\t[" << i << "]Operand width : " << (int)op->width() << std::endl;
		}
		std::cout << "\tisMov: " << instruction.isMov() << std::endl;
		std::cout << "\tisBranching: " << instruction.isBranching() << std::endl;
	}
}