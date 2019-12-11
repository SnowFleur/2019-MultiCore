#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>

using namespace std;
using namespace std::chrono;
const auto NUM_TEST = 10000000;
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
	~NODE() {}
};

class SPTR {
public:
	NODE* volatile ptr;
	SPTR() {
		ptr = nullptr;
	}
	SPTR(NODE* p, int v) {
		ptr = p;
	}
};

class LFSTACK {
private:
	SPTR top;
public:
	LFSTACK() {
		top.ptr = new NODE(0);
	}
	~LFSTACK() { Init(); }

	void Init() {
		NODE *ptr;
		while (top.ptr != nullptr) {
			ptr = top.ptr;
			top.ptr = top.ptr->next;
			delete ptr;
		}
		top.ptr = new NODE(0);

	}

	bool CAS(NODE * volatile * addr, NODE *old_node, NODE *new_node) {
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int *>(addr),
			reinterpret_cast<int *>(&old_node),
			reinterpret_cast<int>(new_node));
	}

	bool STAMP_CAS(SPTR* addr, NODE* old_node, int old_stamp, NODE* new_node) {
		SPTR old_ptr{ old_node,old_stamp };
		SPTR new_ptr{ new_node,old_stamp + 1 };

		return atomic_compare_exchange_strong(reinterpret_cast<atomic_llong *>(addr),
			reinterpret_cast<long long*>(&old_ptr),
			*(reinterpret_cast<long long*>(&new_ptr)));
	}

	void Push(int key) {
		NODE *e = new NODE(key);
		while (true) {
			SPTR last = top;
			e->next = last.ptr;
			if (CAS(&top.ptr, last.ptr, e)) {
				return; //break
			}
			else
				continue;
		}
	}

	int Pop() {
		while (true) {
			SPTR last = top;

			/* 아무것도 없을 경우가 있다.
			top으로 하면 의미가 없다... 읽고 내려간 사이에 다른 스레드가 바꿀 수 있음
			*/
			if (last.ptr == nullptr)
				return 0;
			NODE* node = last.ptr->next;
			//카스한다음 읽으면 의미가 없다.. 그 전에 value를 읽을 것!
			int value = node->key;
			if (CAS(&top.ptr, last.ptr, last.ptr->next)) {
				return value;
			}
			else
				continue;
		}
	}
	void display20()
	{
		int c = 20;
		NODE *p = top.ptr;
		while (p != nullptr) {
			cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0) break;
		}
		cout << endl;
	}

};
LFSTACK my_stack;

void ThreadFunc(int num_thread) {
	for (int i = 1; i < NUM_TEST / num_thread; i++) {
		if ((rand() % 2) || (false)) {
			my_stack.Push(i);
		}
		else {
			my_stack.Pop();
		}
	}
}




int main() {
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2) {
		my_stack.Init();
		vector<thread> threads;
		auto s = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back(ThreadFunc, num_thread);

		for (auto& th : threads) th.join();
		threads.clear();
		auto d = high_resolution_clock::now() - s;

		my_stack.display20();
		cout << num_thread << "Threads,  ";
		cout << ",  Duration : " << duration_cast<milliseconds>(d).count() << " msecs.\n\n";


	}
	system("pause");
}
