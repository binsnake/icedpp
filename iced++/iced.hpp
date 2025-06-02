#ifndef __ICED_DEF
#define __ICED_DEF

/* INCLUDES */
#include <cstdint>
#include <cstddef>
#include "iced_internal.hpp"

/* MACROS */
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

extern "C" {
	int disas(void* obj, const void* code, std::size_t len);
	int disas2(void* obj, const void* code, std::size_t len);
}

/* CLASSES */
namespace iced {
	struct MemOperand {
		std::uint8_t base;
		std::uint8_t index;
		std::uint8_t scale;
	};
	struct Operand {
		OperandTypeSimple type;
		union {
			MemOperand mem;
			IcedReg reg;
			std::uint64_t imm;
		};
		std::uint8_t size;

		void setAsRegister(std::uint8_t _reg, std::uint8_t _size) {
			type = OperandTypeSimple::Register;
			reg = static_cast<IcedReg>(_reg);
			size = _size;
		}		

		void setAsImmediate(std::uint64_t _imm, std::uint8_t _size) {
			type = OperandTypeSimple::Immediate;
			imm = _imm;
			size = _size;
		}		

		void setAsMemory(std::uint8_t base, std::uint8_t index, std::uint8_t scale, std::uint8_t _size) {
			type = OperandTypeSimple::Memory;
			mem.base = base;
			mem.index = index;
			mem.scale = scale;
			size = _size;
		}
	};
	class Instruction {
	public:
		Instruction() = default;
		Instruction(const __iced_internal::IcedInstruction& instruction, std::uint64_t ip_) : icedInstr(instruction), ip(ip_) {
			for (auto i = 0u; i < 4; ++i) {
				switch (static_cast<OperandType>(icedInstr.types[i])) {
				case OperandType::Register8:
					operands[i].setAsRegister(icedInstr.regs[i], 1);
					break;			
				case OperandType::Register16:
					operands[i].setAsRegister(icedInstr.regs[i], 2);
					break;				
				case OperandType::Register32:
					operands[i].setAsRegister(icedInstr.regs[i], 4);
					break;				
				case OperandType::Register64:
					operands[i].setAsRegister(icedInstr.regs[i], 8);
					break;
				case OperandType::Immediate8:
					operands[i].setAsImmediate(icedInstr.immediate, 1);
					break;				
				case OperandType::Immediate8_2nd:
					operands[i].setAsImmediate(icedInstr.immediate2, 1);
					break;
				case OperandType::Immediate16:
					operands[i].setAsImmediate(icedInstr.immediate2, 2);
					break;
				case OperandType::Immediate32:
					operands[i].setAsImmediate(icedInstr.immediate2, 4);
					break;				
				case OperandType::Immediate64:
					operands[i].setAsImmediate(icedInstr.immediate2, 8);
					break;
				case OperandType::Memory8:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 1);
					break;				
				case OperandType::Memory16:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 2);
					break;				
				case OperandType::Memory32:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 4);
					break;				
				case OperandType::Memory64:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 8);
					break;
				}
			}
		}

		NODISCARD __iced_internal::IcedInstruction& internalInstruction() { return icedInstr; }
		NODISCARD std::uint8_t operandCount() const noexcept { return icedInstr.operand_count_visible; }
		NODISCARD std::uint8_t instructionLength() const noexcept { return icedInstr.length; }
		NODISCARD IcedMnemonic mnemonic() const noexcept { return static_cast<IcedMnemonic>(icedInstr.mnemonic); }

	private:
		__iced_internal::IcedInstruction icedInstr;
		std::uint64_t ip;
		Operand operands[4]{ };
	};

	template <bool IcedDebug = true>
	class Decoder {
	public:
		Decoder() = delete;
		Decoder(const std::uint8_t* buffer = nullptr, std::size_t size = 15ULL, std::uint64_t baseAddress = 0ULL)
			: data_(buffer), ip_(baseAddress), baseAddr_(baseAddress), size_(size),
			offset_(0UL), remainingSize_(size) {
		}

		Decoder(const Decoder&) = delete;
		Decoder& operator=(const Decoder&) = delete;
		Decoder(Decoder&& other) : data_(other.data_), ip_(other.ip_), baseAddr_(other.baseAddr_), size_(other.size_),
			offset_(other.offset_), remainingSize_(other.size_) {
		}
		Decoder& operator=(Decoder&& other) {
			if (this != &other) {
				data_ = other.data_;
				ip_ = other.ip_;
				baseAddr_ = other.baseAddr_;
				size_ = other.size_;
				offset_ = other.offset_;
				remainingSize_ = other.remainingSize_;
				lastSuccessfulIp_ = other.lastSuccessfulIp_;
				lastSuccessfulLength_ = other.lastSuccessfulLength_;
			}
			return *this;
		};
		~Decoder() = default;

		NODISCARD std::uint64_t ip() const noexcept { return ip_; }
		NODISCARD Instruction& getCurrentInstruction() const noexcept { return currentInstruction_; }
		NODISCARD bool canDecode() const noexcept { return remainingSize_ > 0 && offset_ < size_; }
		NODISCARD std::uint64_t lastSuccessfulIp() const noexcept { return lastSuccessfulIp_; }
		NODISCARD std::uint16_t lastSuccessfulLength() const noexcept { return lastSuccessfulLength_; }

		void setIp(std::uint64_t ip) noexcept {
			ip_ = ip;
			offset_ = ip - baseAddr_;
			remainingSize_ = size_ - offset_;
		}

		void reconfigure(const std::uint8_t* buffer = nullptr, std::size_t size = 15ULL, std::uint64_t baseAddress = 0ULL) noexcept {
			data_ = buffer;
			size_ = size;
			baseAddr_ = baseAddress;
			remainingSize_ = size;
			currentInstruction_ = { 0 };
			offset_ = lastSuccessfulIp_ = lastSuccessfulLength_ = 0;
		}

		NODISCARD Instruction& decode() noexcept {
			const auto* current_ptr = data_ + offset_;
			const auto code_size = remainingSize_;
			const auto address = ip_;

			__iced_internal::IcedInstruction icedInstruction{ 0 };
			if constexpr (IcedDebug) {
				disas2(&icedInstruction, current_ptr, 15);
			}
			else {
				disas(&icedInstruction, current_ptr, 15);
			}

			currentInstruction_ = Instruction{ icedInstruction };
			return currentInstruction_;
		}

	private:
		Instruction currentInstruction_;
		const std::uint8_t* data_ = nullptr;
		std::uint64_t ip_ = 0;
		std::uint64_t baseAddr_ = 0;
		const std::size_t size_ = 0;
		std::size_t remainingSize_ = 0;
		std::size_t offset_ = 0;
		std::uint64_t lastSuccessfulIp_ = 0;
		std::uint16_t lastSuccessfulLength_ = 0;
	};
};
#endif