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