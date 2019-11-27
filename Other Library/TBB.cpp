#include<iostream>
#include<tbb/tbb.h>  //Nuget을 사용했기 때문에 알아서 디렉토리가 생성된다. "" 사용할필요없음
#include<array>
using namespace tbb;
using namespace std;


void F(int n) {
	std::cout << "[" << n << "]" <<" ";
}

int main() {
	array<int,9> data{ 1,2,4,6,16,32,64,7,55 };



	parallel_for(static_cast<size_t>(0), data.size(), [&](int i) {
		F(data[i]);
	});


	system("pause");

}

