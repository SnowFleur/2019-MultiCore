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
constexpr int MAXHEIGHT = 10;

class LFNODE;
class SLNODE {
public:
	int key;
	SLNODE* next[MAXHEIGHT];
	int height; //몇 층이냐
	SLNODE(int key,int height):key(key),height(height) {
		for (auto&p : next)p = nullptr;
	}
	SLNODE(int x):key(x),height(MAXHEIGHT) {
		for (auto&p : next)p = nullptr;
	}
	SLNODE() :key(0), height(MAXHEIGHT) {
		for (auto&p : next)p = nullptr;
	}
	~SLNODE() {}
};

class SKLIST {
public:
	SLNODE head, tail;
	std::mutex glock;
	SKLIST() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.height = tail.height = MAXHEIGHT; //감싸야한다 
		for (auto& p : head.next) p = &tail;

	}
	~SKLIST() {
		Init();
	}
	void Init() {
		SLNODE *ptr;
		while (head.next[0] != &tail)
		{
			ptr = head.next[0];
			head.next[0] = head.next[0]->next[0];
			delete ptr;
		}
		for (auto &p : head.next)
			p = &tail;
	}
	void Find(int key, SLNODE*preds[MAXHEIGHT], SLNODE* currs[MAXHEIGHT]) {

		int cl = MAXHEIGHT - 1; //맨 위 레벨부터 찾는다.
		while (true) {

			if (MAXHEIGHT - 1 == cl)
				preds[cl] = &head;
			else {
				preds[cl] = preds[cl + 1];
			}
				currs[cl] = preds[cl]->next[cl];

					//전진
					while (currs[cl]->key < key) {
						preds[cl] = currs[cl];
						currs[cl] = currs[cl]->next[cl];
					}

			//커런트 레벨이 0이면 검색이 다 끝난것
			if (0 == cl) return;
			cl--; //아니면 계속 돈다.
		}
	}




	bool Add(int key) {

		SLNODE* preds[MAXHEIGHT], *currs[MAXHEIGHT];
			glock.lock();
		Find(key, preds, currs);

		if (key == currs[0]->key) {
			glock.unlock();
			return false;
		}
		
		else {
			int height = 1;
			while (rand() % 2 == 0) { //1/2확률로 늘린다.
				++height;
				if (MAXHEIGHT == height)break;
			}

			SLNODE* node = new SLNODE(key,height);
			//여길해야함

			for (int i = 0; i < height; ++i) {
				node->next[i] = currs[i];
				preds[i]->next[i] = node;
			}


			glock.unlock();
			return true;

		}



	}

	bool Remove(int key) {

		SLNODE* preds[MAXHEIGHT], *currs[MAXHEIGHT];
		glock.lock();
		Find(key, preds, currs);


		if (key == currs[0]->key) {
			////삭제
			//pred->next = curr->next;
			int height = preds[0]->height;

			for (int i = 0; i < currs[0]->height; ++i)
				preds[i]->next[i] = currs[i]->next[i];
			//preds[0]->next[0] = currs[0]->next[0];
			
			delete currs[0];
			glock.unlock();
			return true;
		}

		else {
			glock.unlock();
			return false;
		}
	}

	bool Contains(int key) {

		SLNODE* preds[MAXHEIGHT], *currs[MAXHEIGHT];
		glock.lock();

		Find(key, preds, currs);

		if (key == currs[0]->key) {
			glock.unlock();
			return true;
		}
		else {
			glock.unlock();
			return false;
		}

	}
	void display20() {
		int c = 20;
		SLNODE* p = head.next[0];
		while (p != &tail) {
			std::cout << p->key << ", ";
			p = p->next[0];
			c--;
			if (c == 0)break;
		}
		std::cout << std::endl;
	}
};


SKLIST clist;

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