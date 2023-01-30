#ifndef SHARED_H_
#define SHARED_H_

#include <iostream>			//For cerr
#include <cstdlib>			//For exit
#include <sys/types.h> 		//For System V IPC
#include <sys/ipc.h> 		//For System V IPC
#include <sys/shm.h>		//For shared memory
#include <sys/sem.h>		//For semaphores

#define MAX_LINE_SIZE 256

using namespace std;

enum sem_name
{
	SEMREQ,
	SEMVALUES
};

union semun
{
	int val;
	struct semid_ds *buff;
	unsigned short *array;
};

struct Request
{
	int id, sectionId, lineId;
	int maxSection, maxLine;
	Request(int maxSection=1, int maxLine=1, int id=0) { this->maxLine = maxLine; this->maxSection = maxSection; this->id = id;
		id = 0; sectionId = rand() % maxSection; lineId = rand() % maxLine; }
	void renew()
	{
		lineId = rand() % (maxLine-1) + 1;
		if (rand() % 100 < 30)
			sectionId = rand() % maxSection;
	}
	void print(bool stderr = false)
	{
		if (stderr)
			cerr << "[" << id << "," << sectionId << "," << lineId << "] ";
		else
			cout << "[" << id << "," << sectionId << "," << lineId << "] ";
	}
};

struct ShmemInfo
{
	int finished;
	Request r;
};

int create_shmem_region (int, int s = 0, int p = 0);
int access_shmem_region (int, int s = 0, int p = 0);
void destroy_shmem_region (int);
char *attach_shmem_region (int);
void detach_shmem_region (char*);
int create_semaphores (int, int s = 0, int p = 0);
void destroy_semaphores (int);
int access_semaphores (int, int s = 0, int p = 0);
void up (int, int);
void upN (int, int, int);
void down (int, int);

#endif
