
#include "hw1_myheader.h"


int main()
{
	default_logger.SetLogFunctions(false, false, false);

	std::vector<MyInteger> vec;

	while ( true )
	{
		// n 값을 바꾸고 싶다면 아래의 literal을 고치면 돼요:
		fill_random_sequence(vec, 10);

		default_logger.ClearCounts();

		print_sequence(vec);

		// 테스트할 알고리즘을 바꾸고 싶다면 아래의 함수 이름 부분을 고치면 돼요:
		quicksort(vec.begin(), vec.end());

		print_sequence(vec);

		default_logger.PrintCounts();

		// 여기는 간단한 안내 메시지 출력을 위한 부분이에요. 신경쓰지 않아도 돼요.
		if ( default_logger.count_normal_constructor_called )
			std::cout << "주의: 정렬 도중 일반 생성자 호출이 발생했어요. 정렬 도중에는 새 object를 만들면 안 돼요. 선언을 적을 때 std::move() 호출식을 적절히 섞어 가며 적어 주세요." << std::endl;
		else if ( default_logger.count_copy_constructor_called )
			std::cout << "주의: 정렬 도중 복사 생성자 호출이 발생했어요. 정렬 도중에는 새 object를 만들면 안 돼요. emplace_back() 호출식 또는 선언을 적을 때 std::move() 호출식을 적절히 섞어 가며 적어 주세요." << std::endl;
		else if ( default_logger.count_copy_assignment_called )
			std::cout << "주의: 정렬 도중 복사 = 연산자 호출이 발생했어요. 정렬 도중에는 값을 복사하면 안 돼요. = 수식을 적을 때 std::move() 호출식을 적절히 섞어 가며 적어 주세요." << std::endl;
		else if ( default_logger.count_destructor_called_normal )
			std::cout << "주의: 정렬 도중 object 파괴가 발생했어요. 정렬 도중에는 입력으로 들어온 object를 파괴하면 안 돼요. 이 오류 메시지를 볼 가능성은 거의 없을 것 같은데... 입력 Data를 손상시키는 부분이 있지는 않은지 확인해 주세요." << std::endl;

		// Windows 기준 Ctrl + C를 눌러 테스트를 끝낼 수 있어요.
		if ( std::cin.get() == EOF )
			break;
	}

	return 0;
}