# iced++

Iced++ is a C++ wrapper around https://github.com/icedland/iced/tree/master

Iced++ does not aim to directly re-implement the Rust SDK.

Work in progress, only basic functionality is implemented so far.

`icedpp_rust_lib` contains a Rust proxy module that runs the decoder and translates them into a C-structure.

`icedpp` is a header-only library providing a friendly wrapper around the decoder & instruction classes.

The code is formatted mostly in camelCase.

## Standard

Requires C++17 or above.

# Building

Building is done with CMake.
`cmake -B build`

If you want to build with a different build system, like MSBuild, you will have to compile the Rust library with cargo, link the `Iced_Wrapper.lib` with your executable and include the `icedpp` headers.

## Example

```cpp
#include "iced.hpp"
int main ( ) {
	unsigned char code [ ] = { 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x10, 0x48, 0x89, 0xC8, 0x48, 0x01, 0xD0, 0x4C, 0x01, 0xC0, 0x4C, 0x01, 0xC8, 0x48, 0xC7, 0xC1, 0x05, 0x00, 0x00, 0x00, 0x48, 0xF7, 0xE1, 0x48, 0x89, 0xEC, 0x5D, 0xC3 };
	iced::Decoder<true> decoder { code, sizeof ( code ), 0 };
	while ( decoder.canDecode ( ) ) {
		auto ip = decoder.ip ( );
		auto instruction = decoder.decode ( );
		std::println ( "{:#x} -> {}", instruction.ip, instruction.toString ( ) );
		std::println ( "{:6}Operand count: {}", "", instruction.operandCount ( ) );
		for ( auto i = 0u; i < instruction.operandCount ( ); ++i ) {
			auto op = instruction.operands[i];
			std::println ( "{:12}[{}] Operand width: {}", "", i, op.width ( ) );
			std::println ( "{:12}[{}] Operand type: {}", "", i, op.typeStr ( ) );
		}
	}
}
```

Output:
```
0x0 -> push rbp
      Operand count: 1
            [0] Operand width: 64
            [0] Operand type: Register
0x1 -> mov rbp,rsp
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0x4 -> sub rsp,0x10
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 8
            [1] Operand type: Immediate
0x8 -> mov rax,rcx
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0xb -> add rax,rdx
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0xe -> add rax,r8
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0x11 -> add rax,r9
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0x14 -> mov rcx,0x5
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 32
            [1] Operand type: Immediate
0x1b -> mul rcx
      Operand count: 1
            [0] Operand width: 64
            [0] Operand type: Register
0x1e -> mov rsp,rbp
      Operand count: 2
            [0] Operand width: 64
            [0] Operand type: Register
            [1] Operand width: 64
            [1] Operand type: Register
0x21 -> pop rbp
      Operand count: 1
            [0] Operand width: 64
            [0] Operand type: Register
0x22 -> ret
      Operand count: 0
```
