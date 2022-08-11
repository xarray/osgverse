#include "nanoid/crypto_random.h"

template<class... T> void unused(T&&...) {}

void NANOID_NAMESPACE::crypto_random_base::next_bytes(std::uint8_t* buffer, std::size_t size)
{
	unused(buffer, size);
}