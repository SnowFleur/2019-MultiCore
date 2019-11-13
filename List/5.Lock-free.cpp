#include<iostream>
#include<thread>
#include<mutex>
#include<chrono>
#include<windows.h>
#include<vector>
#include<atomic>


using namespace std::chrono;
const auto NUM_TEXT = 4000000;
const auto KEY_RANGE = 1000;


class LFNODE;

class MPTR {
private:
	int value;
	bool removed;
public:
	void set(LFNODE* node, bool removed) {
		value = reinterpret_cast<int>(node);
		if (value == true) {
			value = value | 0x01;
		}
		else
			value = value & 0xFFFFFFFE;

	}

	LFNODE* getptr() {
		return reinterpret_cast<LFNODE*>(value & 0xFFFFFFFE); //마킹을 뺸 포인터주소만 리턴한다. 
	}
	LFNODE* getptr(bool* removed) {
		int temp = value;
		if ((temp & 0x1) == 0)
			*removed = false;
		else
			*removed = true;
		return reinterpret_cast<LFNODE*>(temp & 0xFFFFFFFE); //마킹을 뺸 포인터주소만 리턴한다. 

	}
	bool CAS(LFNODE* curr, LFNODE* succ, bool old_mark, bool new_mark) {
		int oldvalue = reinterpret_cast<int>(curr); //주소값
		if (old_mark)oldvalue = oldvalue | 0x01;
		else oldvalue = oldvalue & 0xFFFFFFFE;

		int newvalue = reinterpret_cast<int>(succ);
		if (new_mark)newvalue = newvalue | 0x01;
		else newvalue = newvalue & 0xFFFFFFFE;

		//return CAS(oldvalue, newvalue);
		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic_int*>(&value), &oldvalue, newvalue);

	}

	bool CAS(int old_v, int new_v) {
		return std::atomic_compare_exchange_strong(reinterpret_cast<std::atomic_int*>(this), &old_v, new_v);
	}

	bool AttemptMark(LFNODE* old_node, bool newMark) {
		int oldvalue = reinterpret_cast<int>(old_node);
		int newvalue = oldvalue;
		if (newMark)newvalue = newvalue | 0x01;
		else newvalue = newvalue & 0xFFFFFFFE;
		
		//return CAS(oldvalue, newvalue);

		return std::atomic_compare_exchange_strong(
			reinterpret_cast<std::atomic_int*>(&value), &oldvalue, newvalue);

	}
	void AtomicMarkableReference(LFNODE* node, bool newmark) {}


};

class LFNODE {
public:
	int key;
	MPTR next;

	LFNODE() {
		next.set(nullptr, false);
	}
	LFNODE(int key_value) {
		next.set(nullptr, false);
		key = key_value;
	}
	~LFNODE() {}



};



class LFList {

	LFNODE head, tail;
	LFNODE* freelist, freetail;
public:
	LFList() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next.set(&tail, false);
		//freelist->key = 0x7FFFFFFF;
		//freelist = &freetail;
	}
	~LFList() {}

	void Init() {
		/*싱글스레드 작동*/
		LFNODE* ptr;
		while (head.next.getptr() != &tail) {
			ptr = head.next.getptr();
			head.next = head.next.getptr()->next;
			delete ptr;
		}
	}


	/*void Recycle_freeList() {
	   auto p = freeList;
	   while (p != &freetail) {
		 NODE* n = p->next;
		 delete p;
		 p = n;
	   }
	   freeList = &freetail;
	}*/

	void find(int key, LFNODE* (&pred), LFNODE* (&curr)) {
		//포인터 자체를 바꿔야한다. 포인터를 레퍼런스로 한다.

	retry: //두 단계 빠져나가는 게 없기 때문에 goto를 사용
		pred = &head;
		curr = pred->next.getptr();
		while (true) {
			//계속 전진
			bool removed;
			LFNODE* succ = curr->next.getptr(&removed);

			//removed를 다시 지워준다.
			while (removed == true) {

				if (pred->next.CAS(curr, succ, false, false) == false)
					goto retry; //goto 사용
				curr = succ;
			}

			if (curr->key >= key)return;
			pred = curr;
			curr = curr->next.getptr();
		}
	}

	bool Add(int key) {
		LFNODE* pred, *curr;
		while (true) {

			find(key, pred, curr);
			if (curr->key == key) { return false; }

			else {
				LFNODE* node = new LFNODE(key);
				auto error = GetLastError();
				node->next.set(curr, false);

				if (pred->next.CAS(curr, node, false, false)) { return true; }
				delete node;
			}
		}
	}

	bool Remove(int key) {
		LFNODE* pred, *curr;
		bool snip;
		while (true) {
			find(key, pred, curr);
			if (curr->key != key) { return false; }
			else {
				LFNODE* succ = curr->next.getptr();
				snip = curr->next.AttemptMark(succ, true);
				if (snip == false) { continue; }
				pred->next.CAS(curr, succ, false, false);
				return true;
			}
		}
	}

	bool Contains(int key) {
		LFNODE* curr = &head;
		bool marked;
		while (curr->key < key) {
			curr = curr->next.getptr(&marked);
			//LFNODE succ = curr->next.getptr(marked);
		}
		return curr->key == key && !marked;
	}
	void display20() {
		/*싱글쓰레드에서 작동*/
		int c = 20;
		LFNODE* p = head.next.getptr();
		while (p != &tail) {
			std::cout << p->key << ", ";
			p = p->next.getptr();
			c--;
			if (c == 0)break;
		}
		std::cout << std::endl;
	}
};


LFList clist;

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