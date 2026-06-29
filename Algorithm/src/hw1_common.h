#pragma once

#include "MyInteger.h"

#include <random>
#include <algorithm>


void fill_random_sequence(std::vector<MyInteger> &vec, int count, bool unique, int min, int max_exclusive)
{
	if ( count <= 0 )
		throw std::exception{ "fill_random_sequence()에 실패했습니다. count 값이 0 또는 음수입니다." };

	if ( unique )
	{
		if ( max_exclusive < min + count )
			throw std::exception{ "fill_random_sequence()에 실패했습니다. unique == true임에도 max_exclusive 값이 너무 작습니다." };

		std::default_random_engine eng{ std::random_device{}() };

		vec.clear();
		vec.reserve(count);

		int *numbers = new int[max_exclusive - min];

		for ( int idx = 0; idx < max_exclusive - min; ++idx )
			numbers[idx] = min + idx;

		std::shuffle(numbers, numbers + max_exclusive - min, eng);

		while ( count-- )
			vec.emplace_back(numbers[count]);

		delete[] numbers;
	}
	else
	{
		if ( max_exclusive <= min )
			throw std::exception{ "fill_random_sequence()에 실패했습니다. max_exclusive 값이 min 값보다 작거나 같습니다." };

		std::default_random_engine eng{ std::random_device{}() };
		std::uniform_int_distribution dist{ min, max_exclusive - 1 };

		vec.clear();
		vec.reserve(count);

		while ( count-- )
			vec.emplace_back(dist(eng));
	}
}

void fill_random_sequence(std::vector<MyInteger> &vec, int count, bool unique = true, int min = 0)
{
	fill_random_sequence(vec, count, unique, min, min + count);
}


void print_sequence(std::ostream &os, const std::vector<MyInteger> &vec)
{
	os << '[';
	for ( auto &elem : vec )
		os << elem << ", ";
	os << "\b\b] " << std::endl;
}

void print_sequence(const std::vector<MyInteger> &vec)
{
	print_sequence(std::cout, vec);
}