#include "iced.hpp"

// C++23
int main2 ( ) {
	unsigned char code [ ] = { 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x10, 0x48, 0x89, 0xC8, 0x48, 0x01, 0xD0, 0x4C, 0x01, 0xC0, 0x4C, 0x01, 0xC8, 0x48, 0xC7, 0xC1, 0x05, 0x00, 0x00, 0x00, 0x48, 0xF7, 0xE1, 0x48, 0x89, 0xEC, 0x5D, 0xC3 };
	auto decoder = iced::makeDecoder<true>(code, sizeof ( code ), 0);
	while ( decoder.canDecode ( ) ) {
		auto ip = decoder.ip ( );
		auto instruction = decoder.decode ( );
		std::println ( "{:#x} -> {}", instruction.ip, instruction.toString ( ) );
		std::println ( "{:6}Operand count: {}", "", instruction.operandCount ( ) );
		for ( auto i = 0u; i < instruction.operandCount ( ); ++i ) {
			std::println ( "{:12}[{}] Operand width: {:d}", "", i, instruction.opWidth ( i ) );
			std::println ( "{:12}[{}] Operand type: {}", "", i, instruction.opKindSimpleStr ( i ) );
		}
	}

	return 0;
}