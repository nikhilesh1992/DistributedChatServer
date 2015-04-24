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
#define STRING_SIZE 500

//Error codes and exit status
#define ERR_SOCKET 1
#define ERR_BIND 2
#define ERR_LISTEN 3
#define ERR_ACCEPT 4
#define BUFSIZE 500
#define THREADNUMBER 9
#define CONGESTIONTHRESHOLD 100

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
*tailAvailableIDQueue,*headLeaderQ, *tailLeaderQ;
typedef struct QueueNode QueueNode;

struct InsertionList
{
	int seqNum;
	char msg[500];
	struct InsertionList *next;
}*headHoldBackList, *tailHoldBackList;
typedef struct InsertionList InsertionList;

typedef char string[STRING_SIZE];
//Global Variables
ChatUserInfo chatUser[20];
char IP[20];
int port,socketIdentifier;
struct sockaddr_in leaderaddr,useraddr,incomingAddr;
int ID=20,iterator=0;
char nameOfUser[15];
char dequeuedMsg[500];
int timeStamper = 0;
int sequenceNumber = 0;
int checkTimeStamper;
int timer1 =0, timer2 =0, readyToSend = 1,highPriority = 0;
pthread_t threadMessaging[THREADNUMBER];
pthread_attr_t attribute;

//Semaphores
sem_t S1;	//Semaphore used while printing and sending messages
sem_t S2;	//Semaphore used while updating map during congestion
sem_t S3;	//Semaphore used while updating map during congestion
sem_t SendQ;	//Semaphore used while updating map during congestion
sem_t RecvQ;	//Semaphore used while updating map during congestion
sem_t msgSent;	//Semaphore used while updating map during congestion

//File
FILE *logFile;

//Function Declarations
int createSocket(); 
void ipAddPortParsing(char *);
void toSendAddr(char *, int);
void generalisedStringTok(char *, ArrayString *);
int randomPortGenerator();
int broadCastMsg(int,int,char *);	//1-Successful broadcast, 2- Broadcast failed
void clearTableEntry(ChatUserInfo* , int);
int isTableEntryEmpty(ChatUserInfo);	// 1- If empty entry, 0- Non empty entry
void copyUserDatabaseTable(ChatUserInfo *);
void controllerLeader(char *, ArrayString *);
int isEmpty(QueueNode**, QueueNode**);
void enqueue(char *, QueueNode**, QueueNode**);
void dequeue(QueueNode**, QueueNode**);
char *dequeueStandby(char *,QueueNode**, QueueNode**);
void *queueForMessaging(void *);
void *queueForPrinting(void *);
void *threadForSending(void *);
char *sequencer(char *, int*);
int getSeqNoOfUser(char *);
void controllerNonLeader(char *, ArrayString *);
void updateUserMsgNo(char *);
void updateTimeStamper();
void setTimeStamper();
void updateHoldBackList(char*, InsertionList**, InsertionList**);
char *deleteNode(char *msgbuf, InsertionList**, InsertionList**);
void *threadForAckTableReceiveCallback(void *);
void *threadForTableReceiveCallback(void *);
void controllerAcknowledgement(char *, char *);
void resetTimerForGivenUser(char *,int);
void *threadForMessageSentCallback(void *);
void *threadForAckRecvdBroadcastMsgCallback(void *);
char *peakQueue(QueueNode**, QueueNode**);
void *threadClearGlobalQ(void *);
int amILeader(char *);	//Checks whether the given user is a leader. Return 1 if it is leader, 0 if it is not leader and -1 if not in table
char *getIP(char *);
void conductLeaderElection();
int findIndexOfUserName(char *);
void sendMessageForUserLimit();
void tableCleanUp();
void updateLeaderAddress();
int checkIfUsernameExists(char *);
void printTable();
void resetAllSeqNum();
int numOfChatUsers();
int isTableEmpty();
void closeChatServer();
void printUsers();
void resetAllTimers();
void printQ(QueueNode **, QueueNode **);
void *threadForLeaderBrdCast(void *);
int readyToBroadCast();
char* encryptMsg( char* );
char* decryptMsg( char *);