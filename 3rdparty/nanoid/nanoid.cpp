#include "nanoid/nanoid.h"
#include <cmath>
#include <future>
#include <random>
#include <type_traits>
#include <vector>

namespace impl
{
    static std::string default_dict = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static std::size_t default_size = 21;
    static NANOID_NAMESPACE::crypto_random<std::mt19937> random(std::random_device{}());
    int clz32(int x);

    using gen_func_type = std::string(*)(NANOID_NAMESPACE::crypto_random_base&, const std::string&, std::size_t);
}

int impl::clz32(int x)
{
	const int numIntBits = sizeof(int) * 8; //compile time constant
	//do the smearing
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	//count the ones
	x -= x >> 1 & 0x55555555;
	x = (x >> 2 & 0x33333333) + (x & 0x33333333);
	x = (x >> 4) + x & 0x0f0f0f0f;
	x += x >> 8;
	x += x >> 16;
	return numIntBits - (x & 0x0000003f); //subtract # of 1s from 32
}

std::string NANOID_NAMESPACE::generate()
{
	return generate(impl::random, impl::default_dict, impl::default_size);
}
std::string NANOID_NAMESPACE::generate(const std::string& alphabet)
{
	return generate(impl::random, alphabet, impl::default_size);
}
std::string NANOID_NAMESPACE::generate(std::size_t size)
{
	return generate(impl::random, impl::default_dict, size);
}
std::string NANOID_NAMESPACE::generate(const std::string& alphabet, std::size_t size)
{
	return generate(impl::random, alphabet, size);
}

std::future<std::string> NANOID_NAMESPACE::generate_async()
{
	return std::async
	( 
		(impl::gen_func_type)(generate),
		std::ref(static_cast<NANOID_NAMESPACE::crypto_random_base&>(impl::random)),
		std::cref(impl::default_dict), impl::default_size
	);
}
std::future<std::string> NANOID_NAMESPACE::generate_async(const std::string& alphabet)
{
	return std::async
	(
		(impl::gen_func_type)(generate),
		std::ref(static_cast<NANOID_NAMESPACE::crypto_random_base&>(impl::random)),
		alphabet, impl::default_size
	);
}
std::future<std::string> NANOID_NAMESPACE::generate_async(std::size_t size)
{
	return std::async
	(
		(impl::gen_func_type)(generate),
		std::ref(static_cast<NANOID_NAMESPACE::crypto_random_base&>(impl::random)),
		std::cref(impl::default_dict), size
	);
}
std::future<std::string> NANOID_NAMESPACE::generate_async(const std::string& alphabet, std::size_t size)
{
	return std::async
	(
		(impl::gen_func_type)(generate),
		std::ref(static_cast<NANOID_NAMESPACE::crypto_random_base&>(impl::random)),
		alphabet, size
	);
}

std::string NANOID_NAMESPACE::generate(crypto_random_base& random)
{
	return generate(random, impl::default_dict, impl::default_size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, const std::string& alphabet)
{
	return generate(random, alphabet, impl::default_size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, std::size_t size)
{
	return generate(random, impl::default_dict, size);
}
std::string NANOID_NAMESPACE::generate(crypto_random_base& random, const std::string& alphabet, std::size_t size)
{
    if (alphabet.empty() || alphabet.size() >= 256)
    {
        throw std::invalid_argument("alphabet must contain between 1 and 255 symbols.");
    }

    if (size <= 0)
    {
        throw std::invalid_argument("size must be greater than zero.");
    }

	std::size_t alphSize = alphabet.size();

    // See https://github.com/ai/nanoid/blob/master/format.js for
    // explanation why masking is use (`random % alphabet` is a common
    // mistake security-wise).
    const std::size_t mask = (  2 << (31 - impl::clz32(    (int)((alphSize - 1) | 1)    ))  ) - 1;
    const auto step = (std::size_t)std::ceil(1.6 * (double)mask * (double)size / (double)alphSize);

    auto idBuilder = std::string(size, '_');
    auto bytes = std::vector<std::uint8_t>(step);

	std::size_t cnt = 0;

    while (true)
    {

        random.next_bytes(bytes.data(), bytes.size());

        for (std::size_t i = 0; i < step; i++)
        {

			std::size_t alphabetIndex = bytes[i] & mask;

            if (alphabetIndex >= alphSize) continue;
            idBuilder[cnt] = alphabet[alphabetIndex];
            if (++cnt == size)
            {
                return  { idBuilder };
            }

        }

    }
}