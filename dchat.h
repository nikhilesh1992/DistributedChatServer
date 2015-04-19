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
int timer1 =0, timer2 =0;
pthread_t threadMessaging[THREADNUMBER];
pthread_attr_t attribute;

//Function definitions
int createSocket(); 
void ipAddPortParsing(char *);
void toSendAddr(char *, int);
void generalisedStringTok(char *, ArrayString *);
void copyUserDatabaseTable(ChatUserInfo *);
void controllerLeader(char *, ArrayString *);
char *sequencer(char *, int*);
int getSeqNoOfUser(char *);
void controllerNonLeader(char *, ArrayString *);
void updateUserMsgNo(char *);
void enqueue(char *, QueueNode**, QueueNode**);
void updateTimeStamper();
void setTimeStamper();
void updateHoldBackList(char*, InsertionList**, InsertionList**);
void *threadForAckTableReceiveCallback(void *);
void *threadForTableReceiveCallback(void *);
char *getIP(char *);
void conductLeaderElection();
void tableCleanUp();
int numOfChatUsers();