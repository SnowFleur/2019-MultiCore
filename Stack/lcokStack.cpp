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

class LSTACK {
private:
	NODE* top;
	mutex glock;
public:
	LSTACK() {
		top = new NODE(0);
	}
	~LSTACK() { Init(); }

	void Init() {
		NODE *ptr;
		while (top != nullptr) {
			ptr = top;
			top = top->next;
			delete ptr;
		}
		top = new NODE(0);

	}

	void Push(int key) {
		NODE *e = new NODE(key);
		glock.lock();
		e->next = top;
		top = e;
		glock.unlock();
	}

	int Pop() {
		glock.lock();
		if (top == nullptr) {
			glock.unlock();
			return 0;
		}

		NODE* e = top;
		int result = top->key;
		top = top->next;
		delete e;
		glock.unlock();
		return  result;
	}
	void display20()
	{
		int c = 20;
		NODE *p = top;
		while (p != nullptr) {
			cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0) break;
		}
		cout << endl;
	}

};
LSTACK my_stack;

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
