//Header files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#define SA struct sockaddr

//Error codes and exit status
#define ERR_SOCKET 1
#define ERR_BIND 2
#define ERR_LISTEN 3
#define ERR_ACCEPT 4
#define BUFSIZE 500
#define THREADNUMBER 8

//Structure definitions
struct chatUserInfo 
{
	int ID;
	char Username[15];
	int Port;
	int isLeader;
	int isActive;
	char IP[20];
	int timerBroadcastCheck;
	int timerMsgBroadcastCheck;
	int receivedMsgSeqNo;
	int updatedNewEntryTimeStamper;
};
typedef struct chatUserInfo ChatUserInfo;

struct String
{
	char String[500];
};
typedef struct String ArrayString;

struct QueueNode
{
	char content[500];
	struct QueueNode *next;
}*headSendQ, *tailSendQ, *headRecvQ,*tailRecvQ, *headBackupQ, *tailBackupQ, *headGlobalSendQ, *tailGlobalSendQ, *headAvailableIDQueue, 
*tailAvailableIDQueue;
typedef struct QueueNode QueueNode;

//Function definitions
void copyUserDatabaseTable(ChatUserInfo *);
void controllerLeader(char *, ArrayString *);
void enqueue(char *, QueueNode**, QueueNode**);