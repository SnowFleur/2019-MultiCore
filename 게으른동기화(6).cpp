#include<iostream>
#include<thread>
#include<mutex>
#include<chrono>
#include<vector>


using namespace std::chrono;
const auto NUM_TEXT = 4000000;
const auto KEY_RANGE = 1000;

class NODE {
public:
	int key;
	NODE* next;
	std::mutex n_lock;
	bool marked= false;
	NODE() { next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}
	void Lock() { n_lock.lock(); };
	void UnLock() { n_lock.unlock(); };
};



class lList {

	NODE head, tail;

public:
	lList() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	bool Validate(NODE* pred, NODE* curr) {
		return !pred->marked && !curr->marked && pred->next == curr;
	}

	void Init() {
		/*싱글스레드 작동*/
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			ptr->marked = false;
			head.next = head.next->next;
			delete ptr;
		}
	}
	bool Add(int key) {
		NODE* pred, *curr;
		pred = &head;
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
				NODE* node = new NODE(key);
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
		}

	}

	bool Remove(int key) {
		NODE* pred, *curr;
		pred = &head;
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
		}
	}

	bool Contains(int key) {
		NODE* curr = &head;
		while (curr->key < key) {
			curr = curr->next;
		}
		return curr->key == key && !curr->marked;
	}

	void display20() {
		/*싱글쓰레드에서 작동*/
		int c = 20;
		NODE* p = head.next;
		while (p != &tail) {
			std::cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0)break;
		}
		std::cout << std::endl;
	}
};


lList clist;

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