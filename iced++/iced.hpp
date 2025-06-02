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
	class Instruction {
	public:
		Instruction() = delete;
		Instruction(const __iced_internal::IcedInstruction& instruction) {

		}
	};

	template <bool IcedDebug = true>
	class Decoder {
	public:
		Decoder(const std::uint8_t* buffer = nullptr, std::size_t size = 15ULL, std::uint64_t baseAddress = 0ULL)
			: data_(buffer), ip_(baseAddress), baseAddr_(baseAddress), size_(size),
			offset_(0UL), remainingSize_(size) {
		}

		Decoder(const Decoder&) = delete;
		Decoder& operator=(const Decoder&) = delete;
		Decoder(Decoder&&) = default;
		Decoder& operator=(Decoder&&) = default;
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
		}

	private:
		Instruction currentInstruction_{ 0 };
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