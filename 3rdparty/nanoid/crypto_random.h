#pragma once

#include "nanoid/settings.h"
#include <cstdint>
#include <limits>
#include <utility>

namespace NANOID_NAMESPACE
{
	class crypto_random_base
	{
	public:
		virtual void next_bytes(std::uint8_t* buffer, std::size_t size); //Cannot be pure virtual due to std::async semantics
		virtual ~crypto_random_base()= default;
	};

	template <class UniformRandomBitGenerator>
	class crypto_random : public crypto_random_base
	{
	private:
		UniformRandomBitGenerator _generator;
	public:
		using result_type = std::uint32_t;

		crypto_random& operator=(const crypto_random& other) = delete;
		crypto_random(const crypto_random&) = delete;
		crypto_random() : _generator() {}
		template <class ...Args>
		explicit crypto_random(Args... args) : _generator(std::forward<Args>(args)...) {}

		static constexpr result_type(min)() {
			return UniformRandomBitGenerator::min();
		}

		static constexpr result_type(max)() {
			return UniformRandomBitGenerator::max();
		}

		result_type operator()() {
			return _generator();
		}

		void next_bytes(std::uint8_t* buffer, std::size_t size) override
		{
			std::size_t reps = (size / sizeof(result_type)) * sizeof(result_type);
			std::size_t i = 0;
			for (; i < reps; i += sizeof(result_type))
			{
				*(result_type*)(buffer + i) = operator()();
			}
			if (i == size) return;
			result_type last = operator()();
			for (; i < size; ++i)
			{
				*(buffer + i) = (std::uint8_t)((last >> (8 * (i - reps))));
			}
		}
	};
}