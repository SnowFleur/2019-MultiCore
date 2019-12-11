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
constexpr int CAPACITY = 8;

constexpr unsigned int ST_EMPTY = 0;
constexpr unsigned int ST_WAIT = 0x40000000;
constexpr unsigned int ST_BUSY = 0x80000000;

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

class EXCHANGER {
	volatile int slot;
public:
	EXCHANGER() { slot = 0; }
	bool CAS(unsigned int old_st, int value, unsigned int new_st) {
		//�׳� ī���� �ϸ�ȵ� �ٸ����� �ٲ���� Ȯ���ϸ鼭 ī���� �ؾ��Ѵ�.

		int old_v = (slot & 0x3FFFFFFFF) | old_st; //2��Ʈ ���� �� old_st�̶� OR����
		int new_v = (value & 0x3FFFFFFFF) | new_st;

		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(&slot), &old_v, new_v);
	}

	bool CAS(int* addr, int expected, int update) {
		return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic_int*>(addr), &expected, update);
	}

	int exchange(int value, bool* time_out, int* range) {
		//�����Ҷ����� ����.
		while (true) {
			int old_range = *range;
			switch (slot & 0xC0000000) {

			case ST_EMPTY: {
				int cur_value = slot;
				*time_out = false;
				if (CAS(ST_EMPTY, value, ST_WAIT) == true) {
					for (int i = 0; i < 100; ++i) {
						if (slot & 0xC0000000 != ST_WAIT) {
							int ret = slot & 0x3FFFFFFF;
							slot = ST_EMPTY; //�ٽ� EMPTY�� �����.
							return ret;
						} //ST_EMPTY�� ��ٸ���.
					}


					if (true == CAS(ST_WAIT, 0, ST_EMPTY)) {
						*time_out = true;
						if (old_range > 1) {
							while (CAS(range, old_range, old_range - 1));
						}
						return 0;
					}

					//���������� ��ȯ
					else {
						int ret = slot & 0x3FFFFFFF;
						slot = ST_EMPTY;
						return ret;
					}
				}
				break;
			}

			case ST_WAIT: {
				if (CAS(ST_WAIT, value, ST_BUSY)) {
					return slot & 0x3FFFFFFF;
				}
				break;
			}

			case ST_BUSY: {
				if (old_range < CAPACITY) 
					CAS(range, old_range, old_range + 1);
				break;
			}

			default:
				std::cout << "Invalid State" << std::endl;
				while (true);
				break;
			}
		}
	}
};


class EL_ARRAY {
	int range; //�̰� �ٲ�°͵� �����ؾ��Ѵ�??
	EXCHANGER exchanger[CAPACITY];
public:
	EL_ARRAY() {
		range = 1;
	}
	int visit(int value, bool* time_out) {
		int index = rand()&range;
		return exchanger[index].exchange(value, time_out,&range);
	}
};

class LFELSTACK {
private:
	SPTR top;
	EL_ARRAY el_array;
public:
	LFELSTACK() {
		top.ptr = new NODE(0);
	}
	~LFELSTACK() { Init(); }

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
			else {
				bool time_out;
				int ret = el_array.visit(key, &time_out);


				//���������� ��ȯ
				if (false == time_out) {
					if (0 == ret) return;
				}

				//true �� �ٽ� �õ�
				continue;
			}
		}
	}

	int Pop() {
		while (true) {
			SPTR last = top;
			/* �ƹ��͵� ���� ��찡 �ִ�.
			top���� �ϸ� �ǹ̰� ����... �а� ������ ���̿� �ٸ� �����尡 �ٲ� �� ����
			*/
			if (last.ptr == nullptr)
				return 0;
			NODE* node = last.ptr->next;
			//ī���Ѵ��� ������ �ǹ̰� ����.. �� ���� value�� ���� ��!
			int value = node->key;
			if (CAS(&top.ptr, last.ptr, last.ptr->next)) {
				return value;
			}
			else {
				bool time_out;
				int ret = el_array.visit(0, &time_out);

				//���������� ��ȯ
				if (false == time_out) {
					if (0 == ret) return 0;
				}

				continue;
			}

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
LFELSTACK my_stack;

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