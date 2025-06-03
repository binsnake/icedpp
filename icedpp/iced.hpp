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
#include <utility>
#include <vector>

#ifdef ICED_USE_STD_STRING
#include <string>
#define ICED_STR std::string_view
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

#if defined(_MSC_VER)
#define UNREACHABLE() __assume(false)
#elif defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() __builtin_unreachable()
#else
#if __cplusplus >= 202302L || _MSVC_LANG >= 202302L
#define UNREACHABLE() std::unreachable()
#else
#define UNREACHABLE() do {} while (0) // Fallback for older standards
#endif
#endif

extern "C" {
	int disas ( void* obj, const void* code, std::size_t len );
	int disas2 ( void* obj, const void* code, std::size_t len );
}

/* CLASSES */
namespace iced
{
	class Instruction {
	public:
		Instruction ( ) = default;
		Instruction ( const __iced_internal::IcedInstruction& instruction, std::uint64_t ip_ ) : icedInstr ( instruction ), ip ( ip_ ) { }
		~Instruction ( ) { }

		NODISCARD OperandTypeSimple opKindToSimple ( OperandType rawType ) const noexcept {
			const auto map = static_cast< OperandType >( rawType );
			switch ( map ) {
				case OperandType::Memory8:
				case OperandType::Memory16:
				case OperandType::Memory32:
				case OperandType::Memory64:
				case OperandType::Memory128:
				case OperandType::Memory256:
				case OperandType::Memory512:
					return OperandTypeSimple::Memory;
				case OperandType::Immediate8:
				case OperandType::Immediate8_2nd:
				case OperandType::Immediate16:
				case OperandType::Immediate32:
				case OperandType::Immediate64:
					return OperandTypeSimple::Immediate;
				case OperandType::Register8:
				case OperandType::Register16:
				case OperandType::Register32:
				case OperandType::Register64:
				case OperandType::Register128:
				case OperandType::Register256:
				case OperandType::Register512:
					return OperandTypeSimple::Register;
				case OperandType::NearBranch:
					return OperandTypeSimple::NearBranch;
				case OperandType::FarBranch:
					return OperandTypeSimple::FarBranch;
				default:
					return OperandTypeSimple::Invalid;
			}

			UNREACHABLE ( );
		}
		NODISCARD std::size_t opSize ( std::size_t index ) const noexcept {
			const auto map = icedInstr.types [ index ];
			switch ( map ) {
				case OperandType::Memory8:
				case OperandType::Register8:
				case OperandType::Immediate8:
				case OperandType::Immediate8_2nd:
					return 1;
				case OperandType::Memory16:
				case OperandType::Register16:
				case OperandType::Immediate16:
					return 2;
				case OperandType::Memory32:
				case OperandType::Immediate32:
				case OperandType::Register32:
				case OperandType::FarBranch:
					return 4;
				case OperandType::Memory64:
				case OperandType::Register64:
				case OperandType::Immediate64:
				case OperandType::NearBranch:
					return 8;
				case OperandType::Memory128:
				case OperandType::Register128:
					return 16;
				case OperandType::Memory256:
				case OperandType::Register256:
					return 32;
				case OperandType::Memory512:
				case OperandType::Register512:
					return 64;
				default:
					return 0ULL;
			}

			UNREACHABLE ( );
		}

		NODISCARD std::size_t opWidth ( std::size_t index ) const noexcept { return opSize ( index ) * 8ULL; }

		NODISCARD OperandType opKind ( std::size_t index ) const noexcept { return static_cast< OperandType >( icedInstr.types [ index ] ); }
		NODISCARD OperandType op0Kind ( ) const noexcept { return opKind ( 0 ); }
		NODISCARD OperandType op1Kind ( ) const noexcept { return opKind ( 1 ); }
		NODISCARD OperandType op2Kind ( ) const noexcept { return opKind ( 2 ); }
		NODISCARD OperandType op3Kind ( ) const noexcept { return opKind ( 3 ); }

		NODISCARD OperandTypeSimple opKindSimple ( std::size_t index ) const noexcept { return opKindToSimple ( icedInstr.types [ index ] ); }
		NODISCARD OperandTypeSimple op0KindSimple ( ) const noexcept { return opKindSimple ( 0 ); }
		NODISCARD OperandTypeSimple op1KindSimple ( ) const noexcept { return opKindSimple ( 1 ); }
		NODISCARD OperandTypeSimple op2KindSimple ( ) const noexcept { return opKindSimple ( 2 ); }
		NODISCARD OperandTypeSimple op3KindSimple ( ) const noexcept { return opKindSimple ( 3 ); }

		NODISCARD ICED_STR opKindSimpleStr ( std::size_t operandIndex ) const noexcept {
			switch ( opKindSimple ( operandIndex ) ) {
				case OperandTypeSimple::Register:
					return "Register";
				case OperandTypeSimple::Memory:
					return "Memory";
				case OperandTypeSimple::Immediate:
					return "Immediate";
				case OperandTypeSimple::NearBranch:
					return "NearBranch";
				case OperandTypeSimple::FarBranch:
					return "FarBranch";
				default:
					return "Invalid";
			}

			UNREACHABLE ( );
		}

		NODISCARD ICED_STR op0KindSimpleStr ( ) const noexcept { return opKindSimpleStr ( 0 ); }
		NODISCARD ICED_STR op1KindSimpleStr ( ) const noexcept { return opKindSimpleStr ( 1 ); }
		NODISCARD ICED_STR op2KindSimpleStr ( ) const noexcept { return opKindSimpleStr ( 2 ); }
		NODISCARD ICED_STR op3KindSimpleStr ( ) const noexcept { return opKindSimpleStr ( 3 ); }

		NODISCARD IcedReg opReg ( std::size_t index ) const noexcept { return icedInstr.regs [ index ]; }
		NODISCARD IcedReg op0Reg ( ) const noexcept { return opReg ( 0 ); }
		NODISCARD IcedReg op1Reg ( ) const noexcept { return opReg ( 1 ); }
		NODISCARD IcedReg op2Reg ( ) const noexcept { return opReg ( 2 ); }
		NODISCARD IcedReg op3Reg ( ) const noexcept { return opReg ( 3 ); }

		NODISCARD std::size_t op0Size ( ) const noexcept { return opSize ( 0 ); }
		NODISCARD std::size_t op1Size ( ) const noexcept { return opSize ( 1 ); }
		NODISCARD std::size_t op2Size ( ) const noexcept { return opSize ( 2 ); }
		NODISCARD std::size_t op3Size ( ) const noexcept { return opSize ( 3 ); }

		NODISCARD std::size_t op0Width ( ) const noexcept { return opWidth ( 0 ); }
		NODISCARD std::size_t op1Width ( ) const noexcept { return opWidth ( 1 ); }
		NODISCARD std::size_t op2Width ( ) const noexcept { return opWidth ( 2 ); }
		NODISCARD std::size_t op3Width ( ) const noexcept { return opWidth ( 3 ); }

		NODISCARD std::uint64_t immediate ( ) const noexcept { return icedInstr.immediate; }
		NODISCARD std::uint64_t displacement ( ) const noexcept { return icedInstr.mem_disp; }
		NODISCARD std::uint64_t memIndex ( ) const noexcept { return icedInstr.mem_index; }
		NODISCARD IcedReg memBase ( ) const noexcept { return static_cast< IcedReg >( icedInstr.mem_base ); }
		//NODISCARD std::uint64_t memIndex ( std::size_t operandIndex ) const noexcept { return icedInstr.mem_index; }

		NODISCARD __iced_internal::IcedInstruction& internalInstruction ( ) { return icedInstr; }
		NODISCARD std::uint8_t operandCount ( ) const noexcept { return icedInstr.operand_count_visible; }
		NODISCARD std::uint8_t instructionLength ( ) const noexcept { return icedInstr.length; }
		NODISCARD IcedMnemonic mnemonic ( ) const noexcept { return static_cast< IcedMnemonic >( icedInstr.mnemonic ); }
		NODISCARD bool valid ( ) const noexcept { return icedInstr.mnemonic != 0; }
		NODISCARD std::uint8_t stackGrowth ( ) const noexcept { return icedInstr.stack_growth; }
		NODISCARD bool isLea ( ) const noexcept { return idEquals ( IcedMnemonic::Lea ); }
		NODISCARD bool isMov ( ) const noexcept { return idEquals ( IcedMnemonic::Mov ); }
		NODISCARD bool isBp ( ) const noexcept { return idEquals ( IcedMnemonic::Int3 ); }
		NODISCARD bool isNop ( ) const noexcept { return idEquals ( IcedMnemonic::Nop ); }
		NODISCARD bool isRet ( ) const noexcept { return idEquals ( IcedMnemonic::Nop ); }
		NODISCARD bool isCall ( ) const noexcept { return idEquals ( IcedMnemonic::Call ); }
		NODISCARD bool isJmp ( ) const noexcept { return idEquals ( IcedMnemonic::Jmp ); }
		NODISCARD bool isJcc ( ) const noexcept {
			switch ( mnemonic ( ) ) {
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
					return true;
				default:
					return false;
			}
		}
		NODISCARD bool isJump ( ) const noexcept { return isJmp ( ) || isJcc ( ); }
		NODISCARD bool isBranching ( ) const noexcept { return isCall ( ) || isJump ( ); }
		NODISCARD bool isConditionalBranch ( ) const noexcept { return isJcc ( ); }
		NODISCARD bool isUnconditionalBranch ( ) const noexcept { return isCall ( ) || isJmp ( ); }
		NODISCARD bool isIndirectCall ( ) const noexcept {
			if ( !isCall ( ) ) {
				return false;
			}

			return op0KindSimple ( ) == OperandTypeSimple::Register || op0KindSimple ( ) == OperandTypeSimple::Memory;
		}

		NODISCARD std::uint64_t computeMemoryAddress ( ) const noexcept {
			if ( icedInstr.mem_base == IcedReg::RIP ) {
				return ip + instructionLength ( ) + icedInstr.mem_disp;
			}

			if ( icedInstr.mem_base == IcedReg::None || !icedInstr.mem_index ) { // Displacement holds absolute address
				return icedInstr.mem_disp;
			}

			return 0ULL; // Unresolvable statically
		}
		NODISCARD std::uint64_t resolveMemoryTarget ( ) const noexcept { return computeMemoryAddress ( ); }

		NODISCARD std::uint64_t branchTarget ( ) const noexcept {
			switch ( op0KindSimple ( ) ) {
				case OperandTypeSimple::Immediate:
					return icedInstr.immediate2 ? icedInstr.immediate2 : icedInstr.immediate;
				case OperandTypeSimple::Memory:
					return resolveMemoryTarget ( );
				case OperandTypeSimple::NearBranch:
				case OperandTypeSimple::FarBranch:
					return ip + instructionLength ( ) + icedInstr.mem_disp;
				default:
					return 0ULL;
			}

			UNREACHABLE ( );
		}

		NODISCARD ICED_STR toString ( ) const noexcept {
			if ( !valid ( ) ) {
				return "Invalid instruction";
			}

			return icedInstr.text;
		}
		std::uint64_t ip;
	private:
		NODISCARD bool idEquals ( IcedMnemonic mnemonic ) const noexcept { return icedInstr.mnemonic == static_cast< uint16_t >( mnemonic ); }
		__iced_internal::IcedInstruction icedInstr;
	};

	template <bool IcedDebug = true>
	class Decoder {
	public:
		Decoder ( ) = delete;
		Decoder ( const std::uint8_t* buffer = nullptr, std::size_t size = 15ULL, std::uint64_t baseAddress = 0ULL )
			: data_ ( buffer ), ip_ ( baseAddress ), baseAddr_ ( baseAddress ), size_ ( size ),
			offset_ ( 0UL ), remainingSize_ ( size ) { }

		Decoder ( const Decoder& ) = delete;
		Decoder& operator=( const Decoder& ) = delete;
		Decoder ( Decoder&& other ) : data_ ( other.data_ ), ip_ ( other.ip_ ), baseAddr_ ( other.baseAddr_ ), size_ ( other.size_ ),
			offset_ ( other.offset_ ), remainingSize_ ( other.size_ ) { }
		Decoder& operator=( Decoder&& other ) {
			if ( this != &other ) {
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
		~Decoder ( ) = default;

		NODISCARD std::uint64_t ip ( ) const noexcept { return ip_; }
		NODISCARD Instruction& getCurrentInstruction ( ) const noexcept { return currentInstruction_; }
		NODISCARD bool canDecode ( ) const noexcept { return remainingSize_ > 0 && offset_ < size_; }
		NODISCARD std::uint64_t lastSuccessfulIp ( ) const noexcept { return lastSuccessfulIp_; }
		NODISCARD std::uint16_t lastSuccessfulLength ( ) const noexcept { return lastSuccessfulLength_; }

		void setIp ( std::uint64_t ip ) noexcept {
			ip_ = ip;
			offset_ = ip - baseAddr_;
			remainingSize_ = size_ - offset_;
		}

		void reconfigure ( const std::uint8_t* buffer = nullptr, std::size_t size = 15ULL, std::uint64_t baseAddress = 0ULL ) noexcept {
			data_ = buffer;
			size_ = size;
			baseAddr_ = baseAddress;
			remainingSize_ = size;
			currentInstruction_ = { 0 };
			offset_ = lastSuccessfulIp_ = lastSuccessfulLength_ = 0;
		}

		NODISCARD Instruction& decode ( ) noexcept {
			const auto* current_ptr = data_ + offset_;
			const auto code_size = remainingSize_;
			const auto address = ip_;

			__iced_internal::IcedInstruction icedInstruction { 0 };
			if constexpr ( IcedDebug ) {
				disas2 ( &icedInstruction, current_ptr, 15 );
			}
			else {
				disas ( &icedInstruction, current_ptr, 15 );
			}
			const auto& len = icedInstruction.length;
			currentInstruction_ = Instruction { std::move ( icedInstruction ), ip_ };
			lastSuccessfulIp_ = ip_;             // Store IP *before* advancing
			lastSuccessfulLength_ = len; // Store length
			ip_ += len;
			offset_ += len;
			remainingSize_ -= len;
			return currentInstruction_;
		}

	private:
		Instruction currentInstruction_ {};
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