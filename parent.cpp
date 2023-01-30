#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <list>
#include <sys/time.h>
#include <sys/wait.h>
#include "shared.h"

using namespace std;

const string logfile = "logParent.txt";
int countLines(string);
void getSectionFromMemory(string, int, string localMemory[], int);
void clearSectionFromMemory(int, string localMemory[], int);
void appendToLog(string);

int main(int argc, char** argv)
{
	//Syntax: ./argv[0] N totalRequests filename linesPerSection
	if (argc != 5)
	{
		cerr << "Usage: " << argv[0] << " N totalRequests filename linesPerSection" << endl;
		exit(1);
	}

	string filename = argv[3];
	int linesPerSection = atoi(argv[4]);
	string localMemory[linesPerSection];

	unlink(logfile.c_str());
	int totalLines = countLines(filename);
	int totalSections = totalLines / linesPerSection;

	int N = atoi(argv[1]);
	int currentSection = -1;
	int uses[totalSections];
	list<Request> requests;

	int semid = create_semaphores(N+2, 1);
	down(semid, SEMREQ);
	for (int i = 0; i < N; i++)
		down(semid, i + 2);

	int memorySize = MAX_LINE_SIZE + sizeof(Request) + sizeof(int) + sizeof(uses);
	int shmid = create_shmem_region(memorySize, 1);
	char* shmem = attach_shmem_region(shmid);
	bzero(shmem, memorySize);

	//init uses with -1
	for (int i = 0; i < totalSections; i++)
		uses[i] = -1;
	memcpy(shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), uses, sizeof uses);

	char linesPerSectionStr[10], totalSectionsStr[10];
	sprintf(linesPerSectionStr, "%d", linesPerSection);
	sprintf(totalSectionsStr, "%d", totalSections);
	//Create N child processes
	for (int i = 0; i < N; i++)
	{
		int pid;
		char id[10];
		if ((pid = fork()) == -1)
		{
			perror("fork");
			exit(1);
		}
		else if (pid == 0)
		{
			srand(i);
			sprintf(id, "%d", i);
			execlp("./child", "child", argv[2], linesPerSectionStr, totalSectionsStr, id, argv[1], NULL);
			perror("execlp");
			exit(1);
		}
	}
	usleep(100);
	up(semid, SEMREQ);

	Request r;
	int finished = 0;
	while(true)
	{
		if (requests.size())
		{
			for(list<Request>::iterator it = requests.begin(); it != requests.end(); )
			{
				if (currentSection == -1)
				{
					r = *it;
					getSectionFromMemory(filename, r.sectionId, localMemory, linesPerSection);
					currentSection = r.sectionId;
					down(semid, SEMVALUES);
					memcpy(shmem, localMemory[r.lineId].c_str(), MAX_LINE_SIZE);
					memcpy(uses, shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), sizeof uses);
					uses[r.sectionId] = 1;
					memcpy(shmem + MAX_LINE_SIZE + sizeof(Request) + sizeof(int), uses, sizeof uses);
					up(semid, SEMVALUES);
					up(semid, r.id + 2);
					requests.erase(it);
					break;
				}
				else
				{
					if (it->sectionId == currentSection)
					{
						down(semid, SEMVALUES);
						memcpy(shmem, localMemory[it->lineId].c_str(), MAX_LINE_SIZE); //just copy line
						memcpy(uses, shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), sizeof uses);
						uses[it->sectionId]++;
						memcpy(shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), uses, sizeof uses);
						up(semid, SEMVALUES);
						up(semid, it->id + 2);
						requests.erase(it);
						it = requests.begin();
						continue;
					}
				}
				it++;
			}
		}

		//read request from shmem
		while(1)
		{
			down(semid, SEMREQ);
			bzero(&r, sizeof r);
			memcpy(&r, shmem + MAX_LINE_SIZE, sizeof(r));
			if (r.lineId != 0 || r.sectionId != 0)
			{
				requests.push_back(r);
				bzero(shmem + MAX_LINE_SIZE, sizeof(Request));
				bzero(&r, sizeof r);
				up(semid, SEMREQ);
			}
			else
			{
				up(semid, SEMREQ);
				break;
			}
		}

		//scan for ended usage
		down(semid, SEMVALUES);
		memcpy(uses, shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), sizeof uses);
		for (int j = 0; j < totalSections; j++)
		{
			if (uses[j] == 0) //just stopped using section
			{
				clearSectionFromMemory(currentSection, localMemory, linesPerSection);
				currentSection = -1;
				uses[j] = -1;
				memcpy(shmem+MAX_LINE_SIZE + sizeof(Request) + sizeof(int), uses, sizeof uses);
			}
		}
		up(semid, SEMVALUES);

		//check finished
		down(semid, SEMVALUES);
		memcpy(&finished, shmem+MAX_LINE_SIZE + sizeof(Request), sizeof(int));
		if (finished == N)
		{
			up(semid, SEMVALUES);
			break;
		}
		up(semid, SEMVALUES);
//		usleep(1000);
	}

	for (int i = 0; i < N; i++)
		wait(NULL);

	shmdt(shmem);
	destroy_semaphores(semid);
	destroy_shmem_region(shmid);
	cout << "Parent process is exiting..." << endl;
	return 0;
}

int countLines(string filename)
{
	ifstream fin;
	fin.open(filename.c_str());
	if (!fin.is_open())
	{
		perror("fopen");
		exit(1);
	}
	string line;
	getline(fin, line);
	fin.seekg(0, fin.end);
	int length = fin.tellg();
	fin.close();
	return length / (line.length()+1);
}

void getSectionFromMemory(string filename, int sectionId, string localMemory[], int linesPerSection)
{
	ifstream fin;
	fin.open(filename.c_str());
	string line;
	getline(fin, line);
	int offset = ((line.length() + 1) * linesPerSection) * sectionId;
	fin.seekg(offset, fin.beg); //goto correct position of file
	for (int i = 0; i < linesPerSection; i++) //Get lines to local memory
	{
		getline(fin, line);
		localMemory[i] = line;
	}
	fin.close();

	//Log event to logfile
	char sectionStr[10];
	sprintf(sectionStr, "%d", sectionId);
	appendToLog("get section " + string(sectionStr));
}

void clearSectionFromMemory(int sectionId, string localMemory[], int linesPerSection)
{
	//Clear memory
	for (int i = 0; i < linesPerSection; i++)
		localMemory[i].clear();

	//Log event to logfile
	char sectionStr[10];
	sprintf(sectionStr, "%d", sectionId);
	appendToLog("clear section " + string(sectionStr));
}

void appendToLog(string str)
{
	ofstream fout;
	fout.open(logfile.c_str(), ios::app);

	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
	fout << "[EVENT PID " << getpid() << "]: " << ms << " " << str << endl;
	fout.close();
}
