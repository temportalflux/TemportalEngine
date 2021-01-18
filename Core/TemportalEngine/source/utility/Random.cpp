#include "utility/Random.hpp"

using namespace utility;

Random::Random(ui32 seed)
{
	srand(seed);
}

i32 Random::next()
{
	return rand();
}

i32 Random::nextIn(i32 min, i32 max)
{
	return next() % (max - min) + min;
}
