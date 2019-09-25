
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

class Bakery {
private:
	bool*volatile m_flag=nullptr;
	int*volatile  m_ticketNumber=nullptr;
public:
	void InitFlag(const int);
	void DeleteFlag();
	void InitTicketNumber(const int);
	void DeleteTicketNumber();
	int GetTickNumber(const int);
	void Lock(const int,const int);
	void UnLock(const int);
	~Bakery();
};

Bakery g_bakery;


void Bakery::InitFlag(const int number) {
	if (m_flag == nullptr) {
		m_flag = new bool[number];
		for (int i = 0; i < number; ++i) { m_flag[i] = false; }
	}
	else
		std::cout << "Error:InitFlag() flag is not nullptr" << std::endl;
}
void Bakery::DeleteFlag() {
	if (m_flag != nullptr) {
		delete[] m_flag;
		m_flag = nullptr;
	}
	else
		std::cout << "Error:DeleteFlag() flag is nullptr" << std::endl;

}
void Bakery::InitTicketNumber(const int number) {
	if (m_ticketNumber == nullptr) {
		m_ticketNumber = new int[number];
		for (int i = 0; i < number; ++i) { m_ticketNumber[i] = 0; }
	}
	else
		std::cout << "Error:InitTicketNumber() m_ticketNumber is not nullptr" << std::endl;
}
void Bakery::DeleteTicketNumber() {
	if (m_ticketNumber != nullptr) {
		delete[] m_ticketNumber;
		m_ticketNumber = nullptr;
	}
	else
		std::cout << "Error:DeleteTicketNumber() m_ticketNumber is nullptr" << std::endl;

}

void Bakery::Lock(const int num_thread, const int thread_id) {

	m_flag[thread_id] = true; //들어온 Thread 의 Flag를 킨다.
	m_ticketNumber[thread_id] = GetTickNumber(num_thread); //번호표 부여
	m_flag[thread_id] = false;
	for (int i = 0; i < num_thread; ++i) {
		while (m_flag[i]); //내 flag 넘버가 true가 아니면 무한대기한다.

		//번호표를 가지고 있고 번호표가 더 크다면 대기
		while (m_ticketNumber[i] && m_ticketNumber[i] < m_ticketNumber[thread_id]);
	}
}
int Bakery::GetTickNumber(const int number_thread) {
	int max = 0;
	for (int i = 0; i < number_thread; ++i) {
		if (max <= m_ticketNumber[i])max = m_ticketNumber[i];
	}
	return max + 1;
}
void Bakery::UnLock(const int thread_id) {
	m_flag[thread_id] = false;
	m_ticketNumber[thread_id] = 0;
}
Bakery::~Bakery() {
	if (m_flag != nullptr) {
		delete[] m_flag;
		m_flag = nullptr;
	}
	else
		std::cout << "Error:~Bakery() flag is nullptr" << std::endl;
}


void AddFunc(int num_threads, int myid) {
	for (int i = 0; i < MAX_NUMBER / num_threads; ++i) {
		g_bakery.Lock(num_threads, myid);
		sum+=2;
		g_bakery.UnLock(myid);
	}
}

int main() {
	for (int num_thread = 1; num_thread <= MAX_THREAD; num_thread *= 2) {
		sum = 0;
		g_bakery.InitFlag(num_thread);
		g_bakery.InitTicketNumber(num_thread);
		std::vector<std::thread> threads;

		auto start_t =high_resolution_clock::now();
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









