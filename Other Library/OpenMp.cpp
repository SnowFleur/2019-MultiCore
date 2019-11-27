#include<omp.h>
#include<iostream>

int main() {

	int nthread;
#pragma omp parallel 
	{

		nthread = omp_get_num_threads();
		int tid = omp_get_thread_num();
		std::cout << "Hello OMP From Thread:" << tid << std::endl;


	}


	std::cout << "Number of Thread=" << nthread << std::endl;



	system("pause");
}