#pragma once

#include "hw1_common.h"

#include <functional>


using value_type = MyInteger;
using iterator = std::vector<MyInteger>::iterator;
using predicate = std::function<bool (const MyInteger &, const MyInteger &)>;


inline bool verify(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	if ( beg == end )
		return true;

	++beg;

	while ( beg != end )
	{
		if ( compare(beg[-1], beg[0]) == false )
			return false;

		++beg;
	}

	return true;
}


inline void bogosort(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	std::default_random_engine eng{ std::random_device{}() };

	while ( verify(beg, end, compare) == false )
		std::shuffle(beg, end, eng);
}


inline void insertion_sort(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	const int length = end - beg;

	// 목표 명제:
	//     [beg, end)			  범위는 이미 잘 정렬되어 있음
	// 반복 도중 유지하려는 명제:
	//     [beg, beg+idx_current) 범위는 이미 잘 정렬되어 있음

	// Initialize: [beg, beg+1) 범위는 이미 잘 정렬되어 있음
	int idx_current = 1;

	while ( idx_current < length )
	{
		// Maintenance: beg[idx_current]를 이미 정렬 완료된 [beg, idx_current) 범위 안에 '끼워' 담음으로써,
		//				이제 beg[0, idx_current+1) 범위가 잘 정렬되어 있게끔 만들기
		MyInteger temp = std::move(beg[idx_current]);

		int idx_target = idx_current - 1;

		while ( idx_target >= 0 && compare(temp, beg[idx_target]) )
		{
			beg[idx_target + 1] = std::move(beg[idx_target]);
			--idx_target;
		}

		beg[idx_target + 1] = std::move(temp);

		++idx_current;
	}

	// Termination: 위 반복 흐름을 idx_current == end - beg가 될 때까지 수행하므로, 전체 반복 수행 이후에는 목표 명제가 항상 참이 됨
}


inline void selection_sort(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	const int length = end - beg;

	// 목표 명제:
	//     [beg, end)			  범위의 각 칸에는 전체 범위의 0등부터 순서대로 잘 담겨 있음
	// 반복 도중 유지하려는 명제:
	//     [beg, beg+idx_current) 범위의 각 칸에는 전체 범위의 0등부터 순서대로 잘 담겨 있음

	// Initialize: 그 어떤 칸도 적절한 값이 잘 담겨 있는지 모름( [beg, beg) 범위에 대해 위 명제 성립 )
	int idx_current = 0;

	while ( idx_current < length )
	{
		// Maintenance: beg[idx_current]에 담기기에 적절한 값을 찾아 swap()함으로써
		//				이제 beg[0, idx_current+1) 범위의 각 칸에 전체 범위의 0등부터 순서대로 잘 담겨 있게끔 만들기


		// TODO 이 부분은 수업시간에 작성해 볼 예정이에요.
		iterator best_pos = beg + idx_current;
		for ( iterator it = beg + idx_current + 1; it != end; ++it )
		{
			if ( compare(*it, *best_pos) )
			{
				best_pos = it;
			}
		}

		// 찾은 원소를 현재 위치(정렬된 부분의 바로 다음)와 교환합니다.
		std::swap(*(beg + idx_current), *best_pos);

		++idx_current;
	}

	// Termination: 위 반복 흐름을 idx_current == end - beg가 될 때까지 수행하므로, 전체 반복 수행 이후에는 목표 명제가 항상 참이 됨
}


void merge(iterator beg, iterator mid, iterator end, predicate compare);

inline void merge_sort(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	// Conquer: 범위의 길이가 1 이하인 경우 정렬 완료되어 있다고 바로 말할 수 있음
	if ( end - beg <= 1 )
		return;

	iterator mid = beg + (end - beg) / 2;

	// Divide: 전체 범위를 둘로 나누어 각각을 (작은 문제에 대한) 입력으로 치고 재귀적 접근
	merge_sort(beg, mid, compare);
	merge_sort(mid, end, compare);

	// Combine: 작은 출력 결과들을 모아 원래 입력에 대한 출력을 만듦으로써 원래 문제 해결
	merge(beg, mid, end, compare);
}

inline void merge(iterator beg, iterator mid, iterator end, predicate compare)
{
	std::vector<value_type> left;
	left.reserve(mid - beg);

	std::vector<value_type> right;
	right.reserve(end - mid);

	iterator pos_origin = beg;

	while ( pos_origin != mid )
	{
		left.emplace_back(std::move(*pos_origin));
		++pos_origin;
	}

	while ( pos_origin != end )
	{
		right.emplace_back(std::move(*pos_origin));
		++pos_origin;
	}

	pos_origin = beg;
	iterator pos_left = left.begin();
	iterator pos_right = right.begin();

	while ( pos_origin != end )
	{
		if ( pos_left != left.end() )
		{
			if ( pos_right != right.end() )
			{
				if ( compare(*pos_left, *pos_right) )
				{
					*pos_origin = std::move(*pos_left);
					++pos_left;
				}
				else
				{
					*pos_origin = std::move(*pos_right);
					++pos_right;
				}
			}
			else
			{
				*pos_origin = std::move(*pos_left);
				++pos_left;
			}
		}
		else
		{
			*pos_origin = std::move(*pos_right);
			++pos_right;
		}

		++pos_origin;
	}
}

iterator partition(iterator beg, iterator end, predicate compare);

inline void quicksort(iterator beg, iterator end, predicate compare = std::less<MyInteger>{})
{
	// Conquer: 범위의 길이가 1 이하인 경우 정렬 완료되어 있다고 바로 말할 수 있음
	if ( end - beg <= 1 )
		return;

	iterator mid = partition(beg, end, compare);

	// Divide: 전체 범위를 둘로 나누어 각각을 (작은 문제에 대한) 입력으로 치고 재귀적 접근
	quicksort(beg, mid, compare);
	quicksort(mid + 1, end, compare); // pivot은 '전체 범위'에서의 자신이 있을 칸에 잘 담겨 있을 테니 더 고려하지 않아도 돼요

	// Combine: 각 작은 범위들에 대한 정렬이 모두 끝나면 반드시 전체 범위에 대한 정렬이 끝남(추가로 할 일 없음)
}


inline iterator partition(iterator beg, iterator end, predicate compare)
{
	/*
		TODO

		목표:

		- [beg, end) 범위 내의 값 하나를 pivot으로 삼고, 일단 pivot 값을 맨 뒤로 옮겨 두기
		- 범위 내의 각 값들이 'pivot보다 우월한 값들' - pivot - 'pivot보다 우월하지 않은 값들' 순으로 담겨 있도록 swap() 수행
		- pivot의 위치에 대한 iterator를 return
	*/

	// 범위의 마지막 원소를 pivot으로 선택합니다.
	iterator pivot_val = end - 1;

	// pivot보다 "우월한"(compare 결과가 true인) 원소들이 위치할 경계를 설정합니다.
	iterator store_idx = beg;

	// pivot을 제외한 모든 원소를 순회합니다.
	for ( iterator i = beg; i != pivot_val; ++i )
	{
		// 현재 원소가 pivot보다 우월하다면
		if ( compare(*i, *pivot_val) )
		{
			// 해당 원소를 경계 위치(store_idx)의 원소와 교환하고, 경계를 한 칸 뒤로 이동시킵니다.
			std::swap(*i, *store_idx);
			++store_idx;
		}
	}

	// 순회가 끝난 후, 경계 위치(store_idx)에 pivot을 위치시킵니다.
	// 이 위치를 기준으로 왼쪽은 모두 pivot보다 우월하고, 오른쪽은 그렇지 않습니다.
	std::swap(*store_idx, *pivot_val);

	// pivot의 최종 위치를 반환합니다.
	return store_idx;
}