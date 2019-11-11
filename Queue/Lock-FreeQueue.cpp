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

//Steamp pointer
class SPTR {
public:
	NODE* volatile ptr;
	volatile int stamp;
	SPTR() {
		ptr = nullptr;
		stamp = 0;
	}
	SPTR(NODE* p, int v) {
		ptr = p;
		stamp = v;
	}
};

class SLFQUEUE {
	SPTR  tail;
	SPTR  head;
public:
	SLFQUEUE() {
		head.ptr = tail.ptr = new NODE(0);
	}
	~SLFQUEUE() {}

	void Init() {
		NODE *ptr;
		while (head.ptr->next != nullptr) {
			ptr = head.ptr->next;
			head.ptr->next = head.ptr->next->next;
			delete ptr;
		}
		tail = head;
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

	void Enq(int key) {
		NODE *e = new NODE(key);
		while (true) {
			SPTR last = tail;
			NODE *next = last.ptr->next;
			if (last.ptr != tail.ptr) continue;

			if (next != nullptr) {
				//CAS(&tail, last, next);
				STAMP_CAS(&tail, last.ptr, last.stamp, next);
				continue;

			}
			else {
			if (false == CAS(&last.ptr->next, nullptr, e)) continue;
			//CAS(&tail, last, e);
			STAMP_CAS(&tail, last.ptr, last.stamp, e);
			return; //break;
			}
		}
	}

	int Deq(){
		while (true) {
			SPTR first = head;
			SPTR last = tail;
			NODE *next = first.ptr->next;   //next는 ABA문제랑 연관이 없다 왜? 로컬변수니까  next값을 CAS넣지도 않는다.
			NODE *lastnext = last.ptr->next;
			if (first.ptr != head.ptr) continue;
			if (last.ptr == first.ptr) {
				if (lastnext == nullptr) {
				//	cout << "EMPTY!!!\n";
				//	this_thread::sleep_for(1ms);
					return -1;
				}
				else
				{
					STAMP_CAS(&tail, last.ptr, last.stamp, lastnext);
					continue;
				}
			}

			if (nullptr == next) continue;
			int result = next->key;
			if (false == STAMP_CAS(&head, first.ptr, first.stamp, next)) continue;
			
			first.ptr->next = nullptr;
			delete first.ptr;
			return result;
		}
	}

	void display20()
	{
		int c = 20;
		NODE *p = head.ptr->next;
		while (p != nullptr)
		{
			cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0) break;
		}
		cout << endl;
	}
};


SLFQUEUE my_queue;
void ThreadFunc(int num_thread){
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		if ((rand() % 2) || i < 100 / num_thread) {
			my_queue.Enq(i);
		}
		else {
			int key = my_queue.Deq();
		}
	}
}


int main() {
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2){
		my_queue.Init();
		vector<thread> threads;
		auto s = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
			threads.emplace_back(ThreadFunc, num_thread);

		for (auto& th : threads) th.join();
		threads.clear();
		auto d = high_resolution_clock::now() - s;

		my_queue.display20();
		//my_queue.recycle_freelist();
		cout << num_thread << "Threads,  ";
		cout << ",  Duration : " << duration_cast<milliseconds>(d).count() << " msecs.\n\n";


	}
	system("pause");
}

