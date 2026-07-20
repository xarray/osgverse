#include "bitSet.hpp"


bool bitSet::get(int pos) {
		return pos < _bits.size() && _bits[pos];
}

void bitSet::set(int pos) {
		ensure(pos);
		_bits[pos] = true;
}

void bitSet::ensure(int pos) {
	if (pos >= _bits.size())
	{
		_bits.resize(pos + 64);
	}
}