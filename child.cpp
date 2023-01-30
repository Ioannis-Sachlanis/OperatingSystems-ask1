#include <iostream>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "shared.h"

using namespace std;

void appendToLog(long, long, Request, string);

int main(int argc, char** argv)
{
	int totalRequests = atoi(argv[1]);
	int linesPerSection = atoi(argv[2]);
	int totalSections = atoi(argv[3]);
	int id = atoi(argv[4]);
	int N = atoi(argv[5]);

	srand(id*1000);
	int uses[totalSections];
	int memorySize = MAX_LINE_SIZE + sizeof(Request) + sizeof(int) + sizeof(uses);
	int shmid = access_shmem_region(memorySize, 1);
	int semid = access_semaphores(N+2, 1);
	char* shmem = attach_shmem_region(shmid);

	cout << "Child with PID " << getpid() << " (id = " << id << ") is starting..." << endl;

	char lineStr[MAX_LINE_SIZE];
	Request r(totalSections, linesPerSection, id), rTemp = r;
	for (int i = 0; i < totalRequests; i++)
	{
		r.renew();
		timeval start, end;
		long int time1;
		bool written = false;
		while(written == false)
		{
			down(semid, SEMREQ);
			memcpy(&rTemp, shmem + MAX_LINE_SIZE, sizeof r);
			if (rTemp.lineId == 0 && rTemp.sectionId == 0 && rTemp.id == 0)
			{
				memcpy(shmem + MAX_LINE_SIZE, &r, sizeof r);
				written = true;
			}
			up(semid, SEMREQ);
			usleep(100);
		}
		gettimeofday(&start, NULL);
		time1 = start.tv_sec * 1000 + start.tv_usec / 1000; //get current timestamp in milliseconds
		cerr << "Process submitted (" << i+1 << "/" << totalRequests << "): "; r.print(true); cout << endl;

		down(semid, id + 2);
		cerr << "Received reply for: "; r.print(true); cout << endl;

		gettimeofday(&end, NULL);
		long int time2 = end.tv_sec * 1000 + end.tv_usec / 1000; //get current timestamp in milliseconds
		memcpy(lineStr, shmem, sizeof lineStr);
		cerr << lineStr << endl;
		usleep(rand() % 10000 + 50000);
		down(semid, SEMVALUES);
		memcpy(uses, shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), sizeof uses);
		uses[r.sectionId]--;
		memcpy(shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), uses, sizeof uses);
		up(semid, SEMVALUES);

		appendToLog(time1, time2, r, lineStr);
	}

	//mark finished
	int finished = 0;
	down(semid, SEMVALUES);
	memcpy(&finished, shmem+MAX_LINE_SIZE + sizeof(Request), sizeof(int));
	finished++;
	memcpy(shmem+MAX_LINE_SIZE + sizeof(Request), &finished, sizeof(int));
	up(semid, SEMVALUES);

	shmdt(shmem);
	cout << "Child with PID " << getpid() << " (id = " << id << ") is exiting..." << endl;

	return 0;
}

void appendToLog(long int requestedTime, long int receivedResponse, Request r, string line)
{
	char filename[20];
	sprintf(filename, "logCh%d.txt", getpid());
	ofstream fout;
	fout.open(filename, ios::app);
	fout << "[EVENT PID " << getpid() << "]: " << requestedTime << " " << receivedResponse
			<< " for " << r.sectionId << ", " << r.lineId << " " << line << endl;
	fout.close();
}
