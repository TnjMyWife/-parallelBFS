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
	bool* V = new bool[numNodes];	// ��¼����λͼ
	int* lev = new int[numNodes];	// ÿһ��Ľ����
	int level = 0;
	int numCurrent = 0;		// ��ǰ�㼶��������
	int numNext = 0;		// ��һ��������
	int nextState = SEQ;		// ״̬

	omp_lock_t queue_lock;
	omp_init_lock(&queue_lock);

	#pragma omp parallel for
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// ������ڵ�
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
		// ����ִ��
		case SEQ:
			printf("�㼶 %d �� %d: SEQ�㷨\n", level, level + 1);
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

		// ���ڶ��е�BFS
		case QUEUE:
			printf("�㼶 %d �� %d: QUEUE�㷨\n", level, level + 1);
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

		// ���ڶ��е����ڶ��Ĺ���״̬,����Ҫ����ù���ǰ�Ĳ㼶�����Ϣ
		case QUEUE_TO_READ:
			printf("�㼶 %d �� %d: �ȴ�״̬��׼������READ�㷨\n", level, level + 1);
			C.clear();
			C.swap(N);
			flag = true;
			nextState = READ;
			//level++;
			break;

		// ���ڶ���BFS
		case READ:
			printf("�㼶 %d �� %d: READ�㷨\n", level, level + 1);
			#pragma omp parallel for
			// �������н��
			for (int c = 0; c < numNodes; c++) {
				if (lev[c] == level) {		// �ҵ����ڵ�ǰ�㼶�Ľ��
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

		// ���ڶ���BFS�����ڶ��е�BFS�Ĺ���״̬,��Ҫ�������л��ڶ�BFS�㷨������Ҫ�ҵ���һ���Ľ�㲢���浽N
		case READ_TO_QUEUE:
			printf("�㼶 %d �� %d: ����״̬��׼������QUEUE�㷨\n", level, level + 1);
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
	vector<int> C;		// ��ǰ��ǰ���
	vector<int> N;		// ��һ�㼶���
	bool* V = new bool[numNodes];	// �ѷ��ʱ�λͼ��ʾ
	int* lev = new int[numNodes];	// �㼶����ʾÿ�����Ĳ㼶
	int level = 0;

	// ��ʼ���ѷ���λͼ�Ͳ㼶��
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// ������ڵ�
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
	vector<int> C;		// ���浱ǰ�㼶�Ľ��
	vector<int> N;		// ������һ�㼶�Ľ��
	bool* V = new bool[numNodes];		// �ѷ��ʵĽ�㣬λͼ�ṹ��ʾ
	int* lev = new int[numNodes];		// ÿ������Ӧ�Ĳ㼶
	int level = 0;				// ��ǰ�㼶

	omp_lock_t queue_lock;
	omp_init_lock(&queue_lock);

	// ��ʼ���㼶����ѷ��ʱ�
	#pragma omp parallel for
	for (int i = 0; i < numNodes; i++) {
		V[i] = false;
		lev[i] = -1;
	}

	// ������ڵ�
	V[root] = true;
	N.push_back(root);
	lev[root] = 0;
	// ��������BFS
	while (!N.empty()) {

		C.clear();
		C.swap(N);

		#pragma omp parallel
		{
			queue<int> private_next;	//  �߳�˽�ж���
			#pragma omp for
			for (int i = 0; i < C.size(); i++) {
				int c = C.at(i);
				vector<int> nbrs = findNeighNodes(graph, numNodes, c);	// �ҵ���ǰ�����ھ�
				#pragma omp parallel for
				for (int j = 0; j < nbrs.size(); j++) {
					int n = nbrs.at(j);
					if (!V[n]) {		// ����Ƿ��Ѿ����ʹ�������ԭ�Ӳ���
						V[n] = true;			// ���·��ʱ�
						lev[n] = level + 1;		// ���²㼶��
						private_next.push(n);
						if (private_next.size() > MAX_THRESHOLD) {		// ������ֵʱ����˽�ж����ҵ��Ľ�����N��
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
			// ����ʣ���˽�ж��нڵ�
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

	// ������ڵ�
	V[root] = true;
	lev[root] = 0;

	// �������У�ֱ����ǰ�㼶�Ľ�㶼�Ҳ����ھ�
	bool flag = true;
	while (flag) {
		flag = false;

		#pragma omp parallel for
		// ������н��Ĳ㼶
		for (int c = 0; c < numNodes; c++) {
			if (lev[c] == level) {		// �ҵ����ڵ�ǰ���Ĳ㼶������Щ�����ھӵĲ㼶���и���
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