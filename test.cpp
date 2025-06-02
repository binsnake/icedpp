#include <stdio.h>
#include <iostream>

#include "iced++/iced.hpp"

int main() {
	unsigned char code[] = { 0x90 };
	iced::Decoder<true> decoder{ code, 1, 0 };
	auto instruction = decoder.decode();
	printf("%s", instruction.icedInstr.text);
}