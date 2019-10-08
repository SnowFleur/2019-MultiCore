#include<iostream>
#include<thread>
#include<mutex>
#include<chrono>
#include<vector>
#include<memory.h>

using namespace std::chrono;
const auto NUM_TEXT = 40000;
const auto KEY_RANGE = 1000;

class SPNODE {
public:
	int key;
	std::shared_ptr<SPNODE>next;
	std::mutex n_lock;
	bool marked = false;
	SPNODE() { next = nullptr; }
	SPNODE(int key_value) {next = nullptr;key = key_value;}
	~SPNODE() {}
	void Lock() { n_lock.lock(); };
	void UnLock() { n_lock.unlock(); };
};


class spzList {
	std::shared_ptr<SPNODE> head;
	std::shared_ptr<SPNODE> tail;
public:
	spzList() {
		head = std::make_shared<SPNODE>(0x80000000);
		tail = std::make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}
	~spzList() {
		head = nullptr;
		tail = nullptr;
	}

	bool Validate(std::shared_ptr<SPNODE> pred, std::shared_ptr<SPNODE> curr) {
		return !pred->marked && !curr->marked && pred->next == curr;
	}
	
	void Init() {
		/*싱글스레드 작동*/
		head->next = tail;
	}

	bool Add(int key) {
		std::shared_ptr<SPNODE> pred, curr;
		while (true) {
			pred = head;
			curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;

			}
			pred->Lock();
			curr->Lock();

			if (Validate(pred, curr)) {
				if (key == curr->key) {
					curr->UnLock();
					pred->UnLock();
					return false;
				}
				else {
					std::shared_ptr<SPNODE> node;
					node = std::make_shared<SPNODE>(key);
					node->next = curr;
					pred->next = node;
					curr->UnLock();
					pred->UnLock();
					return true;
				}
			}
			else {
				curr->UnLock();
				pred->UnLock();
				continue;
			}
		}

	}

	bool Remove(int key) {
		std::shared_ptr<SPNODE> pred, curr;
		while (true) {
			pred = head;
			curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}
			pred->Lock();
			curr->Lock();

			if (Validate(pred, curr)) {

				if (curr->key != key) {
					curr->UnLock();
					pred->UnLock();
					return false;
				}
				else {
					curr->marked = true;
					pred->next = curr->next;
					curr->UnLock();
					pred->UnLock();
					return true;
				}
			}
			else {
				curr->UnLock();
				pred->UnLock();
				continue;
			}
		}
	}

	bool Contains(int key) {
		std::shared_ptr<SPNODE> curr;
		curr = head;
		while (curr->key < key) {
			curr = curr->next;
		}
		return curr->key == key && !curr->marked;
	}

	void display20() {
		/*싱글쓰레드에서 작동*/
		int c = 20;
		std::shared_ptr<SPNODE>p = head->next;
		while (p != tail) {
			std::cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0)break;
		}
		std::cout << std::endl;
	}
};


spzList clist;

void ThreadFunc(int num_thread) {
	int key;
	for (int i = 0; i < NUM_TEXT / num_thread; ++i) {
		switch (rand() % 3) {
		case 0: {
			key = rand() % KEY_RANGE;
			clist.Add(key);
			break;
		}
		case 1: {
			key = rand() % KEY_RANGE;
			clist.Remove(key);
			break;
		}
		case 2: {
			key = rand() % KEY_RANGE;
			clist.Contains(key);
			break;

		}
		default:
			std::cout << "Error" << std::endl;
			break;

		}
	}
}



int main() {
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2) {
		std::vector<std::thread> threads;
		clist.Init();
		auto start_time = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i) { threads.emplace_back(ThreadFunc, num_thread); }
		for (auto &th : threads)th.join();
		auto end_time = high_resolution_clock::now();
		threads.clear();
		auto exec_time = end_time - start_time;
		int exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_time).count();
		std::cout << "Threads[" << num_thread << "]" << " Exec_time" << exec_ms << "ms" << std::endl;
		clist.display20();
	}
	system("pause");
}