#include "cpuBFS.h"

int T1 = 64;	
int T2 = 262144;
int T3 = 2048;
float alpha = 2.0;
float beta = 2.0;
int MAX_THRESHOLD = 20;

int* hybridBFS(bool* graph, int numNodes, int root) 
{
	vector<int> C;
	vector<int> N;
	bool* V = new bool[numNodes];	// 记录访问位图
	int* lev = new int[numNodes];	// 每一层的结点数
	int level = 0;
	int numCurrent = 0;		// 当前层级结点的数量
	int numNext = 0;		// 下一层结点数量
	int nextState = SEQ;		// 状态

	omp_lock_t queue_lock;
	omp_init_lock(&queue_lock);

	#pragma omp parallel for
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// 加入根节点
	V[root] = true;
	N.push_back(root);
	numNext++;
	lev[root] = 0;

	bool flag = true;
	while (flag) {
		flag = false;
		numCurrent = numNext;
		numNext = 0;

		switch (nextState) {
		// 串行执行
		case SEQ:
			printf("层级 %d 到 %d: SEQ算法\n", level, level + 1);
			C.clear();
			C.swap(N);

			for (int i = 0; i < C.size(); i++) {
				int c = C.at(i);
				vector<int> nbrs = findNeighNodes(graph, numNodes, c);
				for (int j = 0; j < nbrs.size(); j++) {
					int n = nbrs.at(j);
					if (!V[n]) {
						N.push_back(n);
						V[n] = true;
						lev[n] = level + 1;
						flag = true;
					}
				}
			}

			numNext = N.size();

			if (numNext > T1) {
				nextState = QUEUE;
			}
			level++;
			break;

		// 基于队列的BFS
		case QUEUE:
			printf("层级 %d 到 %d: QUEUE算法\n", level, level + 1);
			C.clear();
			C.swap(N);

			#pragma omp parallel
			{
				queue<int> private_queue;

				#pragma omp for
				for (int i = 0; i < C.size(); i++) {
					int c = C.at(i);

					vector<int> nbrs = findNeighNodes(graph, numNodes, c);

					#pragma omp parallel for
					for (int j = 0; j < nbrs.size(); j++) {
						int n = nbrs.at(j);

						if (!V[n]) {
							V[n] = true;
							lev[n] = level + 1;
							flag = true;

							private_queue.push(n);

							if (private_queue.size() > MAX_THRESHOLD) {
								if (omp_test_lock(&queue_lock)) {
									while (!private_queue.empty()) {
										N.push_back(private_queue.front());
										private_queue.pop();
									}
									omp_unset_lock(&queue_lock);
								}
							}
						}
					}
				}
				omp_set_lock(&queue_lock);
				while (!private_queue.empty()) {
					N.push_back(private_queue.front());
					private_queue.pop();
				}
				omp_unset_lock(&queue_lock);
			}

			numNext = N.size();

			if (numNext > T2 || ((numNext > alpha * numCurrent) && numNext > T3)) {
				nextState = QUEUE_TO_READ;
			}

			level++;
			break;

		// 基于队列到基于读的过度状态,则需要保存好过度前的层级结点信息
		case QUEUE_TO_READ:
			printf("层级 %d 到 %d: 等待状态，准备进入READ算法\n", level, level + 1);
			C.clear();
			C.swap(N);
			flag = true;
			nextState = READ;
			//level++;
			break;

		// 基于读的BFS
		case READ:
			printf("层级 %d 到 %d: READ算法\n", level, level + 1);
			#pragma omp parallel for
			// 遍历所有结点
			for (int c = 0; c < numNodes; c++) {
				if (lev[c] == level) {		// 找到属于当前层级的结点
					vector<int> neighbours = findNeighNodes(graph, numNodes, c);
					#pragma omp parallel for
					for (int j = 0; j < neighbours.size(); j++) {
						int n = neighbours.at(j);

						if (!V[n]) {
							V[n] = true;
							lev[n] = level + 1;
							flag = true;

							#pragma omp critical (nextPush)
							{
								numNext++;
							}
						}
					}
				}
			}

			if (numNext <= T2 || (numNext < beta * numCurrent)) {
				nextState = READ_TO_QUEUE;
			}
			level++;
			break;

		// 基于读的BFS到基于队列的BFS的过渡状态,需要继续进行基于读BFS算法，但需要找到下一级的结点并保存到N
		case READ_TO_QUEUE:
			printf("层级 %d 到 %d: 过渡状态，准备进行QUEUE算法\n", level, level + 1);
			N.clear();
			#pragma omp parallel
			{
				queue<int> private_queue;

				#pragma omp for
				for (int c = 0; c < numNodes; c++) {
					if (lev[c] == level) {

						vector<int> nbrs = findNeighNodes(graph, numNodes, c);

						#pragma omp parallel for
						for (int j = 0; j < nbrs.size(); j++) {
							int n = nbrs.at(j);

							if (!V[n]) {
								V[n] = true;
								lev[n] = level + 1;
								flag = true;

								private_queue.push(n);

								if (private_queue.size() > MAX_THRESHOLD) {
									if (omp_test_lock(&queue_lock)) {
										while (!private_queue.empty()) {
											N.push_back(private_queue.front());
											private_queue.pop();
										}
										omp_unset_lock(&queue_lock);
									}
								}
							}
						}
					}
				}
				omp_set_lock(&queue_lock);
				while (!private_queue.empty()) {
					N.push_back(private_queue.front());
					private_queue.pop();
				}
				omp_unset_lock(&queue_lock);
			}

			numNext = N.size();

			if (numNext > T1) {
				nextState = QUEUE;
			}
			else {
				nextState = SEQ;
			}

			level++;
			break;
		default:
			break;
		}
	}

	delete[] V;

	omp_destroy_lock(&queue_lock);

	return lev;
}

int* serialBFS(bool* graph, int numNodes, int root)
{
	vector<int> C;		// 当前当前结点
	vector<int> N;		// 下一层级结点
	bool* V = new bool[numNodes];	// 已访问表，位图表示
	int* lev = new int[numNodes];	// 层级表，表示每个结点的层级
	int level = 0;

	// 初始化已访问位图和层级表
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// 加入根节点
	V[root] = true;
	N.push_back(root);
	lev[root] = 0;
	// BFS
	while (!N.empty()) {
		C.swap(N);
		N.clear();

		while (!C.empty()) {
			int c = C.back();
			C.pop_back();
			vector<int> neighbours = findNeighNodes(graph, numNodes, c);
			for (int i = 0; i < neighbours.size(); i++) {
				int n = neighbours[i];
				if (!V[n]) {
					lev[n] = level + 1;
					N.push_back(n);
					V[n] = true;
				}
			}
		}
		level++;
	}

	delete[] V;
	return lev;
}

int* queueBased(bool* graph, int numNodes, int root)
{
	vector<int> C;		// 保存当前层级的结点
	vector<int> N;		// 保存下一层级的结点
	bool* V = new bool[numNodes];		// 已访问的结点，位图结构表示
	int* lev = new int[numNodes];		// 每个结点对应的层级
	int level = 0;				// 当前层级

	omp_lock_t queue_lock;
	omp_init_lock(&queue_lock);

	// 初始化层级表和已访问表
	#pragma omp parallel for
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// 加入根节点
	V[root] = true;
	N.push_back(root);
	lev[root] = 0;
	// 迭代进行BFS
	while (!N.empty()) {

		C.clear();
		C.swap(N);

		#pragma omp parallel
		{
			queue<int> private_next;	//  线程私有队列
			#pragma omp for
			for (int i = 0; i < C.size(); i++) {
				int c = C.at(i);
				vector<int> nbrs = findNeighNodes(graph, numNodes, c);	// 找到当前结点的邻居
				#pragma omp parallel for
				for (int j = 0; j < nbrs.size(); j++) {
					int n = nbrs.at(j);
					if (!V[n]) {		// 检查是否已经访问过，不用原子操作
						V[n] = true;			// 更新访问表
						lev[n] = level + 1;		// 更新层级表
						private_next.push(n);
						if (private_next.size() > MAX_THRESHOLD) {		// 到达阈值时，将私有队列找到的结点加入N中
							if (omp_test_lock(&queue_lock)) {
								while (!private_next.empty()) {
									N.push_back(private_next.front());
									private_next.pop();
								}
								omp_unset_lock(&queue_lock);
							}
						}
					}
				}
			}
			// 处理剩余的私有队列节点
			omp_set_lock(&queue_lock);
			while (!private_next.empty()) {
				N.push_back(private_next.front());
				private_next.pop();
			}
			omp_unset_lock(&queue_lock);
		}
		level++;
	}
	delete[] V;
	omp_destroy_lock(&queue_lock);
	return lev;
}

int* readBased(bool* graph, int numNodes, int root)
{
	bool* V = new bool[numNodes];		// 
	int level = 0;
	int* lev = new int[numNodes];
	#pragma omp parallel for
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// 加入根节点
	V[root] = true;
	lev[root] = 0;

	// 迭代进行，直到当前层级的结点都找不到邻居
	bool flag = true;
	while (flag) {
		flag = false;

		#pragma omp parallel for
		// 检查所有结点的层级
		for (int c = 0; c < numNodes; c++) {
			if (lev[c] == level) {		// 找到属于当前结点的层级，对这些结点的邻居的层级进行更新
				vector<int> nbrs = findNeighNodes(graph, numNodes, c);
				#pragma omp parallel for
				for (int j = 0; j < nbrs.size(); j++) {
					int neigh = nbrs.at(j);

					if (!V[neigh]) {
						V[neigh] = true;
						lev[neigh] = level + 1;
						flag = true;
					}
				}
			}
		}

		level++;
	}
	delete[] V;
	return lev;
}

vector<int> findNeighNodes(bool* graph, int numNodes, int c) 
{

	long long int lc = (long long int)c;
	long long int ln = (long long int)numNodes;
	vector<int> neighbours;

	omp_lock_t neigh_lock;
	omp_init_lock(&neigh_lock);
	#pragma omp parallel for
	for (long long int i = 0; i < ln; i++) {
		if (graph[lc * ln + i]) {
			omp_set_lock(&neigh_lock);
			neighbours.push_back((int)i);
			omp_unset_lock(&neigh_lock);
		}
	}
	omp_destroy_lock(&neigh_lock);

	return neighbours;
}