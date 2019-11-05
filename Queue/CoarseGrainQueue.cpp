#include<iostream>
#include<thread>
#include<mutex>
#include<chrono>
#include<vector>

using namespace std::chrono;
const auto NUM_TEXT = 10000000;
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
	NODE* head,* tail;
	std::mutex glock;
public:

	Cqueue() {
		head = tail = new NODE(0);
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
		NODE* e=nullptr;
		e = new NODE(key);
		glock.lock();
		tail->next = e;
		tail = e;
		glock.unlock();
	}
	int Deq() {
		NODE* node;
		int result;
		glock.lock();
		if (head->next == nullptr) {
			return false;
		}
		node = head;
		result = head->next->key;
		head = head->next;
		delete node;
		
		glock.unlock();
		return result;

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

	for (int i = 0; i < NUM_TEXT / num_thread; ++i) {
		if ((rand() % 2 == 0) || i < 10000 / num_thread){
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