#include <stdio.h>
#include <iostream>

struct icedObj {
  uint16_t mnemonic;

  // we only care about explicit operands in this struct

  // we can do this because x86 allows maximum of one mem operand

  uint8_t mem_base;
  uint8_t mem_index;
  uint8_t mem_scale;

  uint8_t stack_growth;

  uint8_t regs[4];
  uint8_t types[4];

  // instruction prefix, attributes
  uint8_t attributes;

  uint8_t length;
  uint8_t operand_count_visible;

  // TODO : 32 bit
  uint64_t immediate; //
  union {
    uint64_t mem_disp;
    uint64_t immediate2;
  };
  char* text;
};

static_assert(offsetof(icedObj, mnemonic) == 0, "invalid offset");
static_assert(offsetof(icedObj, mem_base) == 2, "invalid offset");
static_assert(offsetof(icedObj, mem_index) == 3, "invalid offset");
static_assert(offsetof(icedObj, mem_scale) == 4, "invalid offset");
static_assert(offsetof(icedObj, stack_growth) == 5, "invalid offset");
static_assert(offsetof(icedObj, regs) == 6, "invalid offset");
static_assert(offsetof(icedObj, types) == 10, "invalid offset");
static_assert(offsetof(icedObj, attributes) == 14, "invalid offset");
static_assert(offsetof(icedObj, length) == 15, "invalid offset");
static_assert(offsetof(icedObj, operand_count_visible) == 16, "invalid offset");
static_assert(offsetof(icedObj, immediate) == 24, "invalid offset");
static_assert(offsetof(icedObj, immediate2) == 32, "invalid offset");
static_assert(offsetof(icedObj, text) == 40, "invalid offset");

extern "C" {
int disas(void* obj, const void* code, size_t len);
int disas2(void* obj, const void* code, size_t len);
}

int main() {
	icedObj obj;
	unsigned char code[] = { 0x90 };
	disas2(&obj, code,15);
	printf("%s", obj.text);
}