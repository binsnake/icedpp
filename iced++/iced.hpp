#ifndef __ICED_DEF
#define __ICED_DEF

/* Required links if you're not using cmake (and on Windows) */
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "userenv")
#pragma comment(lib, "Iced_Wrapper.lib")
/* DEFINES */

#define ICED_USE_STD_STRING // Wrappers will include a std::string instead of a char*

/* INCLUDES */
#include <cstdint>
#include <cstddef>

#ifdef ICED_USE_STD_STRING
#include <string>
#define ICED_STR std::string
#else
#define ICED_STR char*
#endif

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
	void free_rust_string(char* ptr);
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
		union {
			std::uint64_t disp;
			std::uint64_t imm2;
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

		std::uint8_t width() {
			return size * 8;
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
				case OperandType::Register128:
					operands[i].setAsRegister(icedInstr.regs[i], 16);
					break;				
				case OperandType::Register256:
					operands[i].setAsRegister(icedInstr.regs[i], 32);
					break;				
				case OperandType::Register512:
					operands[i].setAsRegister(icedInstr.regs[i], 64);
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
				case OperandType::Memory128:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 16);
					break;				
				case OperandType::Memory256:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 32);
					break;				
				case OperandType::Memory512:
					operands[i].setAsMemory(icedInstr.mem_base, icedInstr.mem_index, icedInstr.mem_scale, 64);
					break;
				default:
					break;
				}
			}
		}
		~Instruction() {
			free_rust_string(icedInstr.text);
		}

		NODISCARD __iced_internal::IcedInstruction& internalInstruction() { return icedInstr; }
		NODISCARD std::uint8_t operandCount() const noexcept { return icedInstr.operand_count_visible; }
		NODISCARD std::uint8_t instructionLength() const noexcept { return icedInstr.length; }
		NODISCARD IcedMnemonic mnemonic() const noexcept { return static_cast<IcedMnemonic>(icedInstr.mnemonic); }
		NODISCARD bool valid() const noexcept { return icedInstr.mnemonic != 0; }
		NODISCARD std::uint8_t stackGrowth() const noexcept { return icedInstr.stack_growth; }
		NODISCARD bool isLea() const noexcept { return idEquals(IcedMnemonic::Lea); }
		NODISCARD bool isMov() const noexcept { return idEquals(IcedMnemonic::Mov); }
		NODISCARD bool isBp() const noexcept { return idEquals(IcedMnemonic::Int3); }
		NODISCARD bool isNop() const noexcept { return idEquals(IcedMnemonic::Nop); }
		NODISCARD bool isRet() const noexcept { return idEquals(IcedMnemonic::Nop); }
		NODISCARD bool isCall() const noexcept { return idEquals(IcedMnemonic::Call); }
		NODISCARD bool isJmp() const noexcept { return idEquals(IcedMnemonic::Jmp); }
		NODISCARD bool isJcc() const noexcept { 
			switch (mnemonic()) {
			case IcedMnemonic::Jb:
			case IcedMnemonic::Jae:
			case IcedMnemonic::Jbe:
			case IcedMnemonic::Ja:
			case IcedMnemonic::Je:
			case IcedMnemonic::Jne:
			case IcedMnemonic::Jl:
			case IcedMnemonic::Jge:
			case IcedMnemonic::Jle:
			case IcedMnemonic::Jg:
			case IcedMnemonic::Jo:
			case IcedMnemonic::Jno:
			case IcedMnemonic::Jp:
			case IcedMnemonic::Jnp:
			case IcedMnemonic::Js:
			case IcedMnemonic::Jns:
			case IcedMnemonic::Jcxz:
			case IcedMnemonic::Jecxz:
			case IcedMnemonic::Jrcxz:
			case IcedMnemonic::Loop:
			case IcedMnemonic::Loope:
			case IcedMnemonic::Loopne:
				return true;
			default:
				return false;
			}
		}
		NODISCARD bool isJump() const noexcept { return isJmp() || isJcc(); }
		NODISCARD bool isBranching() const noexcept { return isCall() || isJump(); }
		NODISCARD bool isIndirectCall() const noexcept {
			if (!isCall()) {
				return false;
			}

			return operands[0].type == OperandTypeSimple::Register || operands[0].type == OperandTypeSimple::Memory;
		}

		NODISCARD std::uint64_t computeMemoryAddress(int operandIndex) const noexcept {
			const auto& op = operands[operandIndex];
			if (op.mem.base == static_cast<std::uint8_t>(IcedReg::RIP)) {
				return ip + instructionLength() + op.disp;
			}

			if (!op.mem.base || !op.mem.index) { // Displacement holds absolute address
				return op.disp;
			}

			return 0ULL; // Unresolvable statically
		}

		NODISCARD std::uint64_t branchTarget() const noexcept {
			const auto& op = operands[0];
			if (op.type == OperandTypeSimple::Immediate) {
				return op.imm2 ? op.imm2 : op.imm;
			}
			else if (op.type == OperandTypeSimple::Register && op.reg == IcedReg::RIP) {
				return ip + instructionLength() + op.disp;
			}
			else {
				return computeMemoryAddress(0);
			}

			return 0ULL;
		}

		NODISCARD std::uint64_t resolveMemoryTarget() const noexcept {
			for (auto i = 0u; i < operandCount(); ++i) {
				const auto& op = operands[i];
				if (op.type == OperandTypeSimple::Memory) {
					return computeMemoryAddress(i);
				}
			}

			return 0ULL;
		}

		NODISCARD ICED_STR toString() const noexcept {
			if (!valid()) {
				return "Invalid instruction";
			}

			return icedInstr.text;
		}
		Operand operands[4]{ };
		std::uint64_t ip;
	private:
		NODISCARD bool idEquals(IcedMnemonic mnemonic) const noexcept { return icedInstr.mnemonic == static_cast<uint16_t>(mnemonic); }
		__iced_internal::IcedInstruction icedInstr;
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

			currentInstruction_ = Instruction{ icedInstruction, ip_ };
			lastSuccessfulIp_ = ip_;             // Store IP *before* advancing
			lastSuccessfulLength_ = icedInstruction.length; // Store length
			ip_ += icedInstruction.length;
			offset_ += icedInstruction.length;
			remainingSize_ -= icedInstruction.length;
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