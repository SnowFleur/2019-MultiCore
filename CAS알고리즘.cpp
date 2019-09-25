
#include <iostream>
#include <thread>
#include <vector>
#include<mutex>
#include <chrono>
#include<atomic>

using namespace std::chrono;
constexpr int MAX_NUMBER = 100000000;
constexpr int MAX_THREAD = 16;
volatile int sum;
std::mutex lock;



int x;  // 0-> Lock이 Free다.
		// 1-> 누군가 Lock을 얻어서 CS를 실행중
bool CAS(int* addr, int exp, int up) {
	return atomic_compare_exchange_strong(reinterpret_cast<std::atomic_int*>(addr), &exp, up);
}

void CASLock() { while (!CAS(&x, 0, 1)); }
void CASUnlock() { CAS(&x, 1, 0); }

void AddFunc(int num_threads, int myid) {
	for (int i = 0; i < MAX_NUMBER / num_threads; ++i) {
		CASLock();
		sum += 2;
		CASUnlock();
	}
}

int main() {
	for (int num_thread = 1; num_thread <= MAX_THREAD; num_thread *= 2) {
		sum = 0;
		g_bakery.InitFlag(num_thread);
		g_bakery.InitTicketNumber(num_thread);
		std::vector<std::thread> threads;

		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i) threads.emplace_back(AddFunc, num_thread, i);

		for (auto &th : threads) th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;

		threads.clear();
		int ex_ms = duration_cast<milliseconds>(exec_t).count();

		std::cout << "Threads [ " << num_thread << " ] Sum = " << sum;
		std::cout << ", Exec Time = " << ex_ms << std::endl;
		g_bakery.DeleteFlag();
		g_bakery.DeleteTicketNumber();
	}

	system("pause");
}









