#include "shared.h"	//For prototypes
#include <unistd.h>

using namespace std;

int access_shmem_region (int size, int set, int p)
{
	const char *path = "/home";
	char letter = 'M';
	int perms = 0600 | IPC_CREAT;
	if (p != 0)
		perms = p;
	int shmid;
	key_t key = ftok(path, letter+(set*10));
	shmid = shmget(key, (size_t)size, perms);
	if (shmid < 0)
	{
		cerr << "Error in shmget" << endl;
		exit(1);
	}
	else
		return shmid;
}

void destroy_shmem_region (int shmid)
{
	if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) <= -1)
	{
		cerr << "Error in shmctl" << endl;
		exit(1);
	}
}

int create_shmem_region (int size, int set, int p)
{
	const char *path = "/home";
	char letter = 'M';
	int perms = 0600 | IPC_CREAT;
	if (p != 0)
		perms = p;
	int shmid;
	key_t key = ftok(path, letter+(set*10));
	shmid = shmget(key, (size_t)size, perms);
	if (shmid < 0)
	{
		cerr << "Error in shmget" << endl;
		exit(1);
	}
	else
		return shmid;
}

char *attach_shmem_region (int shmid)
{
	string msg = "Error in shmat";
	char *shmem;
	if ((shmem = (char*)shmat(shmid, (char *) 0, 0)) == (void*)-1)
	{
		cerr << msg << endl;
		exit(1);
	}
	return shmem;
}

void detach_shmem_region (char* shmem)
{
	if (!shmem) return;
	else
		shmdt(shmem);
}

void destroy_semaphores (int semid)
{
	string msg = "Error in semctl";
	if (semctl(semid, 0, IPC_RMID, 0) <= -1)
	{
		cerr << msg << endl;
		exit(1);
	}
}

int create_semaphores (int n, int set, int p)
{
	const char *path = "/tmp";
	char letter = 'S';
	int i = 0;
	int perms = 0600 | IPC_CREAT;
	if (p != 0)
		perms = p;
	union semun arg;
	arg.val = 1;
	int semid = 0, randNum = 100;
	key_t key;

	key = ftok(path, letter+(set*randNum));
	semid = semget(key, n, perms);
	if (semid < 0)
	{
		cerr << "Error in semget" << endl;
		exit(2);
	}

	for (i = 0; i < n; )
	{
		if (semctl(semid, i, SETVAL, arg) <= -1)
		{
			cerr << "Error in semctl" << endl;
			exit(1);
		}
		i++;
	}
	return semid;
}

void down (int semid, int semaphore)
{
	struct sembuf oper[1] = {(short unsigned)semaphore, (short)-1, (short)0};
	if (semop(semid, &oper[0], 1) == -1)
	{
		cerr << "Error in down of " << semaphore << endl;
		cerr << getpid() << endl;
		exit(2);
	}
}

void up (int semid, int semaphore)
{
	struct sembuf oper[1] = {(short unsigned)semaphore, (short)1, (short)0};
	if (semop(semid, &oper[0], 1) == -1)
	{
		cerr << "Error in up of " << semaphore << endl;
		cerr << getpid() << endl;
		exit(2);
	}
}

void upN (int semid, int semaphore, int count)
{
	struct sembuf oper[1] = {(short unsigned)semaphore, (short)count, (short)0};
	if (semop(semid, &oper[0], 1) == -1)
	{
		cerr << "Error in up" << endl;
		exit(2);
	}
}

int access_semaphores (int n, int set, int p)
{
	const char *path = "/tmp";
	char letter = 'S';
	key_t key;
	int perms = 0600;
	if (p != 0)
		perms = p;
	int semid;

	key = ftok(path, letter+(set*100));
	if ((semid = semget(key, n, perms)) < 0)
	{
		cerr << "Error in semget" << endl;
		exit(2);
	}
	return semid;
}
