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
#define SA struct sockaddr

//Error codes and exit status
#define ERR_SOCKET 1
#define ERR_BIND 2
#define ERR_LISTEN 3
#define ERR_ACCEPT 4
#define BUFSIZE 500
#define THREADNUMBER 8

struct chatUserInfo 
{
	char Username[15];
	int Port;
	int isActive;
	char IP[20];
	int timerBroadcastCheck;
	int timerMsgBroadcastCheck;
	int timerMsgBroadcastAccess;
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
}*headSendQ, *tailSendQ, *headRecvQ,*tailRecvQ, *headBackupQ, *tailBackupQ;
typedef struct QueueNode QueueNode;

struct InsertionList
{
	int seqTime;
	char otherUserName[500];
	struct InsertionList *next;
}*headWaitList, *tailWaitList;
typedef struct InsertionList InsertionList;

struct MapOK
{
	char name[20];
	int check;
};
typedef struct MapOK RecvOK;

//Global Variables
ChatUserInfo chatUser[20];
RecvOK recvOK[20];
char IP[20];
int port,socketIdentifier;
struct sockaddr_in requesteraddr,useraddr,incomingAddr;
int iterator=0;
char nameOfUser[15];
int timer1 =0, timer2 =0;
long int myTime;
long int myTimeSec;
pthread_t threadMessaging[THREADNUMBER];
pthread_attr_t attribute;
int ackCount;
char dequeuedMsg[500];
int flag; 
int wantCriticalSection; //to be set when
int ackAccessCount;
char tableString[500];
long int globalTimerIncrementer;
int userEntry=1;
//Semaphores
sem_t S1;	//Semaphore used while printing and sending messages

//Function Declarations
int createSocket(); 
void ipAddPortParsing(char *toParse);
void toSendAddr(char *, int);
char *getIP(char *);
void generalisedStringTok(char *, ArrayString *);
int randomPortGenerator();
void *threadForTableReceiveCallback(void *);
void *threadForAckTableReceiveCallback(void *);
void *threadForAckRecvdBroadcastMsgCallback(void *);
void *threadForAccessRecvdBroadcastMsgCallback(void *);
void *queueForMessaging(void *);
void *queueForPrinting(void *);
void *threadForSendingAccess(void *);
void controller(char *, ArrayString *);
int checkIfUsernameExists(char *);
int broadCastMsg(int, int, char *);
int isTableEntryEmpty(ChatUserInfo);	// 1- If empty entry, 0- Non empty entry
void copyUserDatabaseTable(ChatUserInfo *);
void controllerAcknowledgement(char *, char *);
void resetTimerForGivenUser(char *,int);
void sendMessageForUserLimit();
void tableCleanUp();
void clearTableEntry(ChatUserInfo* , int);
void enqueue(char *, QueueNode**, QueueNode**);
void dequeue(QueueNode**, QueueNode**);
int isEmpty(QueueNode**, QueueNode**);
char* sequencing(char *);
char *peakQueue(QueueNode**, QueueNode**);
int findIndexOfUserName(char *);
void printTable();
int numOfChatUsers();
void updateHoldBackList(char*, InsertionList**, InsertionList**);
char *deleteNode(char *msgbuf, InsertionList**, InsertionList**);
void emptyHoldBackList();
void printQ(QueueNode**, QueueNode**);
void resetAllTimers();
void copyFromTable();
void setOKForUser(char *);
void resetRecvOK();
int checkAllOK();
void printRecvOK();
void tableToString(char *);
void stringToTable(char *);
void printUsers();
