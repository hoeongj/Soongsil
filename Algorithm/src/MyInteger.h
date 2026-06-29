/*
	☆

	MyInteger.h

	Version 2.0

	C++ object(들)의 인생을 간단히 엿볼 수 있도록
	class MyInteger 및 부가 기능들을 마련해 둔 헤더 파일입니다.

	- default_logger의 메시지 출력 기능을 끄고 싶은 경우
	  main() 정의 내용물로 default_logger->SetLogFunctions() 호출식을 적어 주세요

	- default_logger의 메시지 출력 기능을 끄고 주요 함수 호출에 대한 delay를 부여하고 싶은 경우
	  main() 정의 내용물로 default_logger->SetDelays() 호출식을 적어 주세요

	- 여기서 사용한 모든 이름들, 스킬들을 우리 수업에서 전부 다루지는 않아요.
	  그렇기는 하지만 궁금한 부분이 있다면 언제든 물어봐도 좋아요

	- 이 파일에서만 등장하는 것은 절대 시험에 출제되지 않으니 심각하게 읽지 않아도 좋아요

	- 버전별 변경 이력은 파일 맨 밑에 주석으로 적어 두었어요
*/

#pragma once

#include <iostream>
#include <limits>
#include <exception>
#include <string>
#include <chrono>

class MyInteger;

class MyInteger_Logger
{
public:
	using clock = std::chrono::high_resolution_clock;

	friend class MyInteger;

	int count_normal_constructor_called;
	int count_copy_constructor_called;
	int count_move_constructor_called;
	int count_destructor_called_normal;
	int count_destructor_called_alreadyMoved;
	int count_copy_assignment_called;
	int count_move_assignment_called;
	int count_compare_called;
	int count_swap_called;

	// 이 멤버 변수는 과제 테스트 과정에서 강사 혼자 사용할 용도로 도입해 두었어요.
	int ret_code;

	constexpr static clock::duration delay_zero{ clock::duration::zero() };
	clock::duration delay_normal_constructor_call;
	clock::duration delay_copy_constructor_call;
	clock::duration delay_move_constructor_call;
	clock::duration delay_destructor_call;
	clock::duration delay_destructor_call_alreadyMoved;
	clock::duration delay_copy_assignment_call;
	clock::duration delay_move_assignment_call;
	clock::duration delay_compare_call;
	clock::duration delay_swap_call;

private:
	inline void Log_impl(const char *msg, const MyInteger &obj, clock::duration) const;
	constexpr void NoLog_impl(const char *msg, const MyInteger &obj, clock::duration) const {}
	inline void NoLog_Delay_impl(const char *msg, const MyInteger &obj, clock::duration duration) const;
	inline void Log2_impl(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration) const;
	constexpr void NoLog2_impl(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration) const {}
	inline void NoLog2_Delay_impl(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration duration) const;

	void (MyInteger_Logger:: *Log_ctor_dtor_assign)(const char *msg, const MyInteger &obj, clock::duration) const;
	void (MyInteger_Logger:: *Log_compare)(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration) const;
	void (MyInteger_Logger:: *Log_swap)(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration) const;

public:
	constexpr MyInteger_Logger(bool log_on_ctor_dtor_assign_calls = true, bool log_on_compare_calls = true, bool log_on_swap_calls = true, int ret_code = 0) :
		count_normal_constructor_called{},
		count_copy_constructor_called{},
		count_move_constructor_called{},
		count_destructor_called_normal{},
		count_destructor_called_alreadyMoved{},
		count_copy_assignment_called{},
		count_move_assignment_called{},
		count_compare_called{},
		count_swap_called{},
		ret_code{ ret_code },
		delay_normal_constructor_call{ delay_zero },
		delay_copy_constructor_call{ delay_zero },
		delay_move_constructor_call{ delay_zero },
		delay_destructor_call{ delay_zero },
		delay_destructor_call_alreadyMoved{ delay_zero },
		delay_copy_assignment_call{ delay_zero },
		delay_move_assignment_call{ delay_zero },
		delay_compare_call{ delay_zero },
		delay_swap_call{ delay_zero },
		Log_ctor_dtor_assign{ log_on_ctor_dtor_assign_calls ? &MyInteger_Logger::Log_impl : &MyInteger_Logger::NoLog_impl },
		Log_compare{ log_on_compare_calls ? &MyInteger_Logger::Log2_impl : &MyInteger_Logger::NoLog2_impl },
		Log_swap{ log_on_swap_calls ? &MyInteger_Logger::Log2_impl : &MyInteger_Logger::NoLog2_impl }
	{
	}

	MyInteger_Logger(clock::duration delay_normal_constructor_call,
					 clock::duration delay_copy_constructor_call,
					 clock::duration delay_move_constructor_call,
					 clock::duration delay_destructor_call,
					 clock::duration delay_destructor_call_alreadyMoved,
					 clock::duration delay_copy_assignment_call,
					 clock::duration delay_move_assignment_call,
					 clock::duration delay_compare_call,
					 clock::duration delay_swap_call,
					 int ret_code = 0) :
		count_normal_constructor_called{},
		count_copy_constructor_called{},
		count_move_constructor_called{},
		count_destructor_called_normal{},
		count_destructor_called_alreadyMoved{},
		count_copy_assignment_called{},
		count_move_assignment_called{},
		count_compare_called{},
		count_swap_called{},
		ret_code{ ret_code },
		delay_normal_constructor_call{ delay_normal_constructor_call },
		delay_copy_constructor_call{ delay_copy_constructor_call },
		delay_move_constructor_call{ delay_move_constructor_call },
		delay_destructor_call{ delay_destructor_call },
		delay_destructor_call_alreadyMoved{ delay_destructor_call_alreadyMoved },
		delay_copy_assignment_call{ delay_copy_assignment_call },
		delay_move_assignment_call{ delay_move_assignment_call },
		delay_compare_call{ delay_compare_call },
		delay_swap_call{ delay_swap_call },
		Log_ctor_dtor_assign{ &MyInteger_Logger::NoLog_Delay_impl },
		Log_compare{ &MyInteger_Logger::NoLog2_Delay_impl },
		Log_swap{ &MyInteger_Logger::NoLog2_Delay_impl }
	{
	}

	/*
		Log 출력 기능을 켜거나 끌 수 있습니다.

		- true로 지정하는 경우 해당 operation들에 대한 delay 설정은 무시됩니다
	*/
	void SetLogFunctions(bool log_on_ctor_dtor_assign_calls, bool log_on_compare_calls, bool log_on_swap_calls)
	{
		if ( log_on_ctor_dtor_assign_calls )
			Log_ctor_dtor_assign = &MyInteger_Logger::Log_impl;
		else if ( delay_normal_constructor_call == delay_zero &&
				 delay_copy_constructor_call == delay_zero &&
				 delay_move_constructor_call == delay_zero &&
				 delay_destructor_call == delay_zero &&
				 delay_destructor_call_alreadyMoved == delay_zero &&
				 delay_copy_assignment_call == delay_zero &&
				 delay_move_assignment_call == delay_zero )
			Log_ctor_dtor_assign = &MyInteger_Logger::NoLog_Delay_impl;
		else
			Log_ctor_dtor_assign = &MyInteger_Logger::NoLog_impl;

		if ( log_on_compare_calls )
			Log_compare = &MyInteger_Logger::Log2_impl;
		else if ( delay_compare_call == delay_zero )
			Log_compare = &MyInteger_Logger::NoLog2_Delay_impl;
		else
			Log_compare = &MyInteger_Logger::NoLog2_impl;

		if ( log_on_swap_calls )
			Log_swap = &MyInteger_Logger::Log2_impl;
		else if ( delay_swap_call == delay_zero )
			Log_swap = &MyInteger_Logger::NoLog2_Delay_impl;
		else
			Log_swap = &MyInteger_Logger::NoLog2_impl;
	}

	/*
		각 operation에 대한 delay를 개별적으로 지정할 수 있습니다.

		- 인수 값과 무관하게, 이 함수를 호출하면 모든 operation들에 대한 log 출력 기능은 항상 해제됩니다
	*/
	void SetDelays(clock::duration delay_normal_constructor_call,
				   clock::duration delay_copy_constructor_call,
				   clock::duration delay_move_constructor_call,
				   clock::duration delay_destructor_call,
				   clock::duration delay_destructor_call_alreadyMoved,
				   clock::duration delay_copy_assignment_call,
				   clock::duration delay_move_assignment_call,
				   clock::duration delay_compare_call,
				   clock::duration delay_swap_call)
	{
		this->delay_normal_constructor_call = delay_normal_constructor_call;
		this->delay_copy_constructor_call = delay_copy_constructor_call;
		this->delay_move_constructor_call = delay_move_constructor_call;
		this->delay_destructor_call = delay_destructor_call;
		this->delay_destructor_call_alreadyMoved = delay_destructor_call_alreadyMoved;
		this->delay_copy_assignment_call = delay_copy_assignment_call;
		this->delay_move_assignment_call = delay_move_assignment_call;
		this->delay_compare_call = delay_compare_call;
		this->delay_swap_call = delay_swap_call;

		if ( delay_normal_constructor_call == delay_zero &&
			delay_copy_constructor_call == delay_zero &&
			delay_move_constructor_call == delay_zero &&
			delay_destructor_call == delay_zero &&
			delay_destructor_call_alreadyMoved == delay_zero &&
			delay_copy_assignment_call == delay_zero &&
			delay_move_assignment_call == delay_zero )
			Log_ctor_dtor_assign = &MyInteger_Logger::NoLog_Delay_impl;
		else
			Log_ctor_dtor_assign = &MyInteger_Logger::NoLog_impl;

		if ( delay_compare_call == delay_zero )
			Log_compare = &MyInteger_Logger::NoLog2_Delay_impl;
		else
			Log_compare = &MyInteger_Logger::NoLog2_impl;

		if ( delay_swap_call == delay_zero )
			Log_swap = &MyInteger_Logger::NoLog2_Delay_impl;
		else
			Log_swap = &MyInteger_Logger::NoLog2_impl;
	}

	void PrintCounts() const
	{
		std::cout
			<< "Call counts" << std::endl
			<< " N__ ctor: " << count_normal_constructor_called << std::endl
			<< " _C_ ctor: " << count_copy_constructor_called << std::endl
			<< " __M ctor: " << count_move_constructor_called << std::endl
			<< " n__ dtor: " << count_destructor_called_normal << std::endl
			<< " __m dtor: " << count_destructor_called_alreadyMoved << std::endl
			<< " C_ op=(): " << count_copy_assignment_called << std::endl
			<< " _M op=(): " << count_move_assignment_called << std::endl
			<< "compare(): " << count_compare_called << std::endl
			<< "   swap(): " << count_swap_called << std::endl;
	}

	constexpr void ClearCounts()
	{
		count_normal_constructor_called = 0;
		count_copy_constructor_called = 0;
		count_move_constructor_called = 0;
		count_destructor_called_normal = 0;
		count_destructor_called_alreadyMoved = 0;
		count_copy_assignment_called = 0;
		count_move_assignment_called = 0;
		count_compare_called = 0;
		count_swap_called = 0;
		ret_code = 0;
	}
};

inline MyInteger_Logger default_logger;

class MyInteger
{
private:
	int number;
	bool is_moved;
	bool is_dummy;
	bool is_discovered_dummy;
	bool is_destroyed;
	MyInteger_Logger *logger;

public:
	constexpr static int min_number = std::numeric_limits<int>::min();

	constexpr bool isMoved() const noexcept { return is_moved; }
	constexpr bool isDummy() const noexcept { return is_dummy; }

	constexpr MyInteger() : number{ min_number }, is_moved{ }, is_dummy{ true }, is_discovered_dummy{ }, is_destroyed{ }, logger{ nullptr }
	{
	}

	MyInteger(int number) : number{ number }, is_moved{ }, is_dummy{ }, is_discovered_dummy{ }, is_destroyed{ }, logger{ &default_logger }
	{
		++logger->count_normal_constructor_called;
		(logger->*logger->Log_ctor_dtor_assign)("N__ ctor>", *this, logger->delay_normal_constructor_call);
	}

	constexpr MyInteger(MyInteger_Logger &logger, int number) : number{ number }, is_moved{ }, is_dummy{ }, is_discovered_dummy{ }, is_destroyed{ }, logger{ &logger }
	{
		++logger.count_normal_constructor_called;
		(logger.*logger.Log_ctor_dtor_assign)("N__ ctor>", *this, logger.delay_normal_constructor_call);
	}

	constexpr MyInteger(const MyInteger &other) : number{ other.number }, is_moved{ other.is_moved }, is_dummy{ other.is_dummy }, is_discovered_dummy{ other.is_discovered_dummy }, is_destroyed{ }, logger{ other.logger }
	{
		if ( other.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 복사 생성자 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( other.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 복사 생성자 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		++logger->count_copy_constructor_called;
		(logger->*logger->Log_ctor_dtor_assign)("_C_ ctor>", *this, logger->delay_copy_constructor_call);
	}

	constexpr MyInteger(MyInteger &&other) noexcept : number{ other.number }, is_moved{ other.is_moved }, is_dummy{ other.is_dummy }, is_discovered_dummy{ other.is_discovered_dummy }, is_destroyed{ }, logger{ other.logger }
	{
		if ( other.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 이동 생성자 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( other.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 이동 생성자 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		other.number = std::numeric_limits<int>::min();
		other.is_moved = true;

		++logger->count_move_constructor_called;
		(logger->*logger->Log_ctor_dtor_assign)("__M ctor>", *this, logger->delay_move_constructor_call);
	}

	constexpr ~MyInteger() noexcept
	{
		if ( is_dummy && !is_discovered_dummy )
			return;

		if ( is_destroyed )
		{
			std::cerr << "주의! 부적절한 MyInteger 파괴자 호출이 발생했습니다. 이미 파괴된 object를 재차 파괴했거나, 애초에 object를 생성하지 않은 메모리 영역에 대해 파괴자 호출이 발생했을 수 있습니다." << std::endl;
			std::terminate();
		}

		if ( is_moved )
		{
			++logger->count_destructor_called_alreadyMoved;
			if ( !is_discovered_dummy )
				(logger->*logger->Log_ctor_dtor_assign)("__m dtor>", *this, logger->delay_destructor_call_alreadyMoved);
		}
		else
		{
			++logger->count_destructor_called_normal;
			if ( !is_discovered_dummy )
				(logger->*logger->Log_ctor_dtor_assign)("n__ dtor>", *this, logger->delay_destructor_call);
		}

		is_destroyed = true;
	}

	explicit operator int() const noexcept { return number; }

	explicit operator std::string() const
	{
		if ( is_destroyed )
			return "(Invalid)";

		if ( is_dummy )
			return "(Dummy)";

		if ( is_moved )
			return "(Moved)";

		return std::to_string(number);
	}

	constexpr MyInteger &operator =(const MyInteger &rhs)
	{
		if ( rhs.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 복사 = 수식 계산에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( rhs.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 복사 = 수식 계산에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( this != &rhs )
		{
			if ( is_dummy && !is_discovered_dummy )
				++rhs.logger->count_normal_constructor_called;

			number = rhs.number;
			is_moved = rhs.is_moved;
			is_dummy = false;
			logger = rhs.logger;

			++logger->count_copy_assignment_called;
			(logger->*logger->Log_ctor_dtor_assign)(" C_ op=()>", *this, logger->delay_copy_assignment_call);
		}
		else
		{
			(logger->*logger->Log_ctor_dtor_assign)(" C_ op=()> (called with same object)", *this, logger->delay_zero);
		}


		return *this;
	}

	constexpr MyInteger &operator =(MyInteger &&rhs) noexcept
	{
		if ( rhs.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 이동 = 수식 계산에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( rhs.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 이동 = 수식 계산에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( this != &rhs )
		{
			if ( is_dummy && !is_discovered_dummy )
				++rhs.logger->count_normal_constructor_called;

			number = rhs.number;
			is_moved = rhs.is_moved;
			is_dummy = false;
			logger = rhs.logger;

			rhs.number = min_number;
			rhs.is_moved = true;

			++logger->count_move_assignment_called;
			(logger->*logger->Log_ctor_dtor_assign)(" _M op=()>", *this, logger->delay_move_assignment_call);
		}
		else
		{
			(logger->*logger->Log_ctor_dtor_assign)(" _M op=()> (called with same object)", *this, logger->delay_zero);
		}


		return *this;
	}

	constexpr std::strong_ordering operator <=>(const MyInteger &other) const noexcept
	{
		if ( is_dummy || other.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 compare() 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( logger != other.logger )
		{
			std::cerr << "주의! 서로 다른 logger를 사용하는 두 MyInteger object들에 대한 compare() 호출이 발생했습니다." << std::endl;
			std::terminate();
		}

		if ( is_destroyed || other.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 compare() 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( this != &other )
		{
			++logger->count_compare_called;
			(logger->*logger->Log_compare)("#compare()>", *this, other, logger->delay_compare_call);
		}
		else
		{
			(logger->*logger->Log_compare)("#compare()> (called with same object)", *this, other, logger->delay_zero);
		}

		return number <=> other.number;
	}

	constexpr bool operator ==(const MyInteger &other) const noexcept
	{
		if ( is_dummy || other.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 MyInteger object가 compare() 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( logger != other.logger )
		{
			std::cerr << "주의! 서로 다른 logger를 사용하는 두 MyInteger object들에 대한 compare() 호출이 발생했습니다." << std::endl;
			std::terminate();
		}

		if ( is_destroyed || other.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 compare() 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( this != &other )
		{
			++logger->count_compare_called;
			(logger->*logger->Log_compare)("#compare()>", *this, other, logger->delay_compare_call);
		}
		else
		{
			(logger->*logger->Log_compare)("#compare()> (called with same object)", *this, other, logger->delay_zero);
		}

		return number == other.number;
	}

	constexpr void swap(MyInteger &other) noexcept
	{
		if ( is_dummy && other.is_dummy )
		{
			std::cerr << "주의! 유효하지 않은 두 MyInteger object들에 대한 swap() 호출이 발생했습니다." << std::endl;
			std::terminate();
		}

		if ( logger != nullptr && other.logger != nullptr && logger != other.logger )
		{
			std::cerr << "주의! 서로 다른 logger를 사용하는 두 MyInteger object들에 대한 swap() 호출이 발생했습니다." << std::endl;
			std::terminate();
		}

		if ( is_destroyed || other.is_destroyed )
		{
			std::cerr << "주의! 이미 파괴된 MyInteger object, 또는 애초에 object를 생성하지 않은 메모리 영역이 swap() 호출에 사용되었습니다." << std::endl;
			std::terminate();
		}

		if ( this != &other )
		{
			// (1도 안 중요)추가 칸 없이 xor 연산만으로 swap()을 수행할 수 있어요
			number ^= other.number;
			other.number ^= number;
			number ^= other.number;

			is_moved ^= other.is_moved;
			other.is_moved ^= is_moved;
			is_moved ^= other.is_moved;

			// xor 연산이 가능할까 걱정되는 경우에는 표준 swap<>()을 사용하면 돼요
			std::swap(is_dummy, other.is_dummy);
			std::swap(is_discovered_dummy, other.is_discovered_dummy);
			std::swap(logger, other.logger);

			if ( is_dummy && !is_discovered_dummy )
			{
				is_discovered_dummy = true;
				++other.logger->count_normal_constructor_called;
			}

			if ( other.is_dummy && !other.is_discovered_dummy )
			{
				other.is_discovered_dummy = true;
				++logger->count_normal_constructor_called;
			}

			if ( logger != nullptr )
			{
				++logger->count_swap_called;
				(logger->*logger->Log_swap)("$   swap()>", *this, other, logger->delay_swap_call);
			}
			else
			{
				++other.logger->count_swap_called;
				(other.logger->*other.logger->Log_swap)("$   swap()>", *this, other, other.logger->delay_swap_call);
			}
		}
		else
		{
			(logger->*logger->Log_swap)("$   swap()> (called with same object)", *this, other, logger->delay_zero);
		}
	}

	std::string to_string() const
	{
		return std::string(*this);
	}

	std::uint64_t hash() const noexcept
	{
		return number;
	}

	friend std::ostream &operator <<(std::ostream &os, const MyInteger &rhs)
	{
		if ( rhs.is_destroyed )
			return os << "(Invalid)";

		if ( rhs.is_dummy )
			return os << "(Dummy)";

		if ( rhs.is_moved )
			return os << "(Moved)";

		return os << rhs.number;
	}
};

template<>
constexpr inline void std::swap<MyInteger>(MyInteger &lhs, MyInteger &rhs) noexcept
{
	lhs.swap(rhs);
}

namespace std
{
	inline std::string to_string(const MyInteger &obj)
	{
		return obj.to_string();
	}

	template <>
	struct hash<MyInteger>
	{
		auto operator ()(const MyInteger &obj) const noexcept { return obj.hash(); }
	};
}

inline void MyInteger_Logger::Log_impl(const char *msg, const MyInteger &obj, clock::duration) const
{
	std::cout << msg << " obj: " << obj << std::endl;
}

inline void MyInteger_Logger::Log2_impl(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration) const
{
	std::cout << msg << " lhs: " << lhs << ", rhs: " << rhs << std::endl;
}

inline void MyInteger_Logger::NoLog_Delay_impl(const char *msg, const MyInteger &obj, clock::duration duration) const
{
	auto start_time = std::chrono::steady_clock::now();
	auto end_time = start_time + duration;

	while ( std::chrono::steady_clock::now() < end_time )
		;
}

inline void MyInteger_Logger::NoLog2_Delay_impl(const char *msg, const MyInteger &lhs, const MyInteger &rhs, clock::duration duration) const
{
	auto start_time = std::chrono::steady_clock::now();
	auto end_time = start_time + duration;

	while ( std::chrono::steady_clock::now() < end_time )
		;
}



/*
	변경 이력:

	v1.0
		- 초기 버전

	v1.1 (이전 버전과 호환 안 됨)
		- == 연산자 정의를 추가했습니다. 이제 std::vector<MyInteger>째로 비교해도 compare() 횟수가 정상적으로 카운트됩니다.

		- 로그 출력 기능을 개편했습니다.
			> 출력되는 메시지를 눈으로 구분하기 쉽도록 글자들의 열을 맞추어 두었습니다
			> 이제 operator =(), compare(), swap() 호출이 발생할 때도 메시지를 출력할 수 있습니다
			> 이제 메시지 출력 기능을 부분적으로 on/off할 수 있습니다(생성/파괴/assign, compare(), swap())
			> 횟수 집계 및 메시지 출력을 위한 class MyInteger_Logger를 별도로 추가했습니다
			> 이제 모든 메시지 출력 기능을 off한 automatic MyInteger_Logger object를 사용하면 compile time에 횟수 집계를 진행할 수 있습니다
			  (default_logger는 static object기 때문에 compile time에는 사용할 수 없어요)

	v1.2
		- 기본 생성자를 사용하여 만든 MyInteger object를 더이상 유효하게 사용할 수 없도록 만들어 두었습니다(이하 dummy object로 지칭)
			> Dummy object에 담긴 값을 사용하는 시점에 오류 메시지를 출력하고 프로그램 실행을 중단하도록 만들어 두었어요
			> 새 구현체를 구성할 때, 특히 과제 코드를 작성할 때는 std::allocator<>를 도입하는 것을 적극적으로 고려해 주세요.
			  (이걸 도입하지 않고 그냥 진행하는 경우 과제 제출 전 테스트에 통과하지 못 할 가능성이 높아요.
			   우리 수업에서 다루는 구현체들은 모두 std::vector<>처럼 compile time에도 사용할 수 있어야 하기 때문이에요)

	v1.3
		- Dummy object가 = 수식이나 swap() 호출의 대상이 되었을 때 해당 object를 '발견'한 것으로 간주, 일반 생성자 호출 횟수를 카운트하는 기능을 추가했습니다.

	v1.3.1
		- Apple clang에서 friend 선언이 새 클래스 이름을 도입하지 않는 이슈에 대응할 수 있도록 코드를 수정했습니다(클래스 이름 소개를 위한 선언을 별도로 적어 두었어요)

	v1.4
		- MyInteger object의 현재 상태를 확인하기 위한 멤버 함수 isMoved(), isDummy()를 추가했습니다.
		  (여전히 MyInteger object에 담긴 실제 숫자는 외부에서 확인하기 어렵도록 가려 두었어요)

	v1.5
		- 부적절한 object 사용을 추가로 감지하기 위한 멤버 변수 is_destroyed를 추가했습니다.
			> 이 멤버 변수의 값은 생성시 false로, 파괴자 호출시 true로 지정됩니다
			> 또한 아직 생성자 호출을 하지 않은 대부분의 상황에서 이 멤버 변수 자리에는 0이 아닌 다른 값이 들어 있기에 true로 간주될 수 있습니다

		- explicit operator int()를 추가했습니다.
			> 이 연산자는 constexpr로 지정되어 있지 않으므로 과제용 구현체를 만들 때는 사용할 수 없습니다
			> 물론 나중에 별도 코드를 작성해 가며 내 구현체를 실제로 사용해보고 싶을 때는 int(number)와 같은 느낌으로 사용할 수 있습니다

	v1.6
		- 동일한 object를 사용하여 operator =(), compare(), swap()을 호출했을 때 카운트하지 않도록 변경했습니다.
			> log 메시지는 여전히 출력되며, 해당 상황이 발생했다는 것을 알려줍니다

	v1.7
		- explicit operator std::string(), to_string() 및 비멤버 버전 std::to_string()을 추가했습니다.
			> 이 연산자, 함수들은 constexpr로 지정되어 있지 않으므로 과제용 구현체를 만들 때는 사용할 수 없습니다
			> 물론 나중에 별도 코드를 작성해 가며 내 구현체를 실제로 사용해보고 싶을 때는 std::string(number), number.to_string(), std::to_string(number)와 같은 느낌으로 사용할 수 있습니다

	v1.8
		- Runtime 성능을 재어볼 수 있도록 주요 operation들에 대한 최소 수행 시간(delay)을 지정하는 기능을 추가했습니다.
			> MyInteger object(들)에 대한 생성, 파괴, operator =(), compare(), swap()에 걸리는 시간을 개별적으로 지정할 수 있습니다
			> 이 시간을 크게(예: 1ms) 지정하는 경우 object에 대한 작업을 자주 수행하는 자료구조 및 알고리즘이 불리해집니다
			> 결과적으로 현재 버전의 logger는 log 출력, delay 적용, '둘 다 안 함'의 세 가지 방식을 제공하고 있습니다.
			  이들은 각각 용도가 다르므로(세부 내용 확인, runtime 성능 측정, compile time에 구현체 무결성 확인) log 출력과 delay 적용을 둘 다 하는 버전은 제공하지 않습니다

		- std::unordered_set<> 등과 함께 사용하기 위한 struct std::hash<MyInteger> 를 추가했습니다.

	v2.0
		- 새 학기를 맞이하며 전체 코드 내용을 다소 정돈해 두었습니다.
*/