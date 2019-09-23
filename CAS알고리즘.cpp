#include <iostream>
#include <thread>
#include <vector>
#include<mutex>
#include <chrono>
#include<Windows.h>


#ifdef WIN32
BOOL CAS(LONG volatile* Addr, LONG Old, LONG New) {
	LONG temp = InterlockedCompareExchange(Addr, New, Old);
	return temp == Old;
}
#endif


using namespace std::chrono;
constexpr int MAX_NUMBER = 100000000;
constexpr int MAX_THREAD = 16;
volatile int sum;
std::mutex lock;


int main() { system("pause"); }