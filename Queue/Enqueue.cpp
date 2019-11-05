#include<iostream>
#include<thread>
#include<mutex>
#include<chrono>
#include<windows.h>
#include<vector>
#include<atomic>

using namespace std::chrono;
const auto NUM_MAX = 10000000;
const auto KEY_RANGE = 1000;



class NODE {
public:
	int key;
	NODE* next;
	NODE() { next = nullptr; }
	NODE(int key_value) {
		next = nullptr;
		key = key_value;
	}
};

class Cqueue {
	NODE* head, *tail;
	std::mutex glock;
public:

	Cqueue() {head = tail = new NODE(0);}

	bool CAS(NODE* volatile* addr, NODE* old_v, NODE* new_v){
		return atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic_int*>(addr), reinterpret_cast<int*>(&old_v), reinterpret_cast<int>(new_v));
	}
	bool CAS(int old_v, int new_v) {
		return std::atomic_compare_exchange_strong(reinterpret_cast<std::atomic_int*>(this), &old_v, new_v);
		
	}


	void Init() {
		/*싱글스레드 작동*/
		NODE* ptr;
		while (head->next != nullptr) {
			ptr = head->next;
			head->next = head->next->next;
			delete ptr;
		}
		tail = head;
	}


	void Enq(int key) {
		NODE* e = new NODE(key);
		while (true) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (nullptr == next) {
				if (CAS(&(last->next), nullptr, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else CAS(&tail, last, next);
		}

	}

	int Deq() {
		while (true) {
			NODE* first = head;
			NODE* last = tail;
			NODE* next = first->next;

			if (first != head) continue;
			if (first == last) {
				if (nullptr == next) return -1;
				CAS(&tail, last, next);
				continue;
			}
			int value{ next->key };
			if (false == CAS(&head, first, next)) continue;

			return value;
		}

	}

	void display20() {
		/*싱글쓰레드에서 작동*/
		int c = 20;
		NODE* p = head->next;
		while (p != nullptr) {
			std::cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0)break;
		}
		std::cout << std::endl;
	}
};

Cqueue my_queue;

void ThreadFunc(int num_thread) {
	int key;

	for (int i = 0; i < NUM_MAX / num_thread; ++i) {
		if ((rand() % 2 == 0) || i < 1000 / num_thread) {
			my_queue.Enq(i);
		}
		else {
			my_queue.Deq();
		}
	}
}

int main() {
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2) {
		std::vector<std::thread> threads;
		my_queue.Init();
		auto start_time = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i) { threads.emplace_back(ThreadFunc, num_thread); }
		for (auto &th : threads)th.join();
		auto end_time = high_resolution_clock::now();
		threads.clear();

		auto exec_time = end_time - start_time;
		int exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();
		std::cout << "Threads[" << num_thread << "]" << " Exec_time" << exec_ms << "ms" << std::endl;
		my_queue.display20();

	}
	system("pause");
}