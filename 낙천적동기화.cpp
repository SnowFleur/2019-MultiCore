#define ADD 0
#define REMOVE 1
#define CONTAINS 2
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
	NODE() { next = NULL; }
	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}
	void Lock() { n_lock.lock(); };
	void UnLock() { n_lock.unlock(); };
};

class OList {
	NODE head, tail;
	NODE* freeList;
	NODE freetail;
	std::mutex fl_l;
public:
	OList() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
		freetail.key = 0x7FFFFFFF;
		freeList= &freetail;
	}
	bool Validate(NODE* pred, NODE* curr) {
		NODE* node = &head;

		while (node->key <= pred->key) {

			if (node == pred) {
				return pred->next == curr;
			}
			node = node->next;
		}
		return false;
	}

	void Init() {
		/*싱글스레드 작동*/
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}
	void Recycle_freeList(){
		auto p = freeList;
		while (p != &freetail) {
			NODE* n = p->next;
			delete p;
			p = n;
		}
		freeList = &freetail;
	}
	bool Add(int key) {
		NODE* pred, *curr;
		while (true) {
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
				continue;
			}
		}

	}

	bool Remove(int key) {
		NODE* pred, *curr;
		while (true) {
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
					pred->next = curr->next;
					curr->next = freeList;
					fl_l.lock();
					freeList = curr;
					fl_l.unlock();
					curr->UnLock();
					pred->UnLock();
					return true;
				}

				else {
					curr->UnLock();
					pred->UnLock();
					return false;
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
			if (key = curr->key) {
				curr->UnLock();
				pred->UnLock();
				return true;
			}
			else {
				curr->UnLock();
				pred->UnLock();
				return false;
			}
		}
		else {
			curr->UnLock();
			pred->UnLock();
		}
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


OList clist;

void ThreadFunc(int num_thread) {
	int key;
	for (int i = 0; i < NUM_TEXT / num_thread; ++i) {
		switch (rand() % 3) {
		case ADD: {
			key = rand() % KEY_RANGE;
			clist.Add(key);
			break;
		}
		case REMOVE: {
			key = rand() % KEY_RANGE;
			clist.Remove(key);
			break;
		}
		case CONTAINS: {
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
		clist.Recycle_freeList();
	}
	system("pause");
}