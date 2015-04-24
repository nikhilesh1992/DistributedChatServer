#include "dchat6.h"
//TODO: 1. table cleanup 2. increase timeouts 3. If somebody elses timestamp is greater than mine, then store him and dequeue him only when I am done.
int main(int argc, char const *argv[])
{
	// Local variable definitions
	int recvlen,i=0;
	char buf[BUFSIZE];
	char tempbuf[BUFSIZE];
	ChatUserInfo bufRecvdTable[20];
	socklen_t len;
	ArrayString arrayString[20];
	ArrayString arrayString2[20];
	ArrayString arrayS[20];
	int createReturn;
	void *state;

	sem_init(&S1, 1, 1);

	pthread_attr_init(&attribute);										//Setting attribute for join and detachable
    pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);	//Setting attribute so that thread can accept join and also detachable state
	createReturn = pthread_create(&threadMessaging[0], &attribute, queueForMessaging,NULL);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting messaging thread %d\n", createReturn);
        exit(-1);
	}
	
	createReturn = pthread_create(&threadMessaging[1], &attribute, queueForPrinting,NULL);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting messaging thread %d\n", createReturn);
         exit(-1);
	}
	char temp[15];
	strcpy(temp,argv[1]);
	createReturn = pthread_create(&threadMessaging[2], &attribute, threadForSendingAccess,temp);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting messaging thread %d\n", createReturn);
         exit(-1);
	}
	//printf("debug2\n");
	createReturn = pthread_create(&threadMessaging[4], &attribute, threadForAckTableReceiveCallback, NULL);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	
	createReturn = pthread_create(&threadMessaging[5], &attribute, threadForAckRecvdBroadcastMsgCallback, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[6], &attribute, threadForAccessRecvdBroadcastMsgCallback, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	//printf("\nbefore argc 3\n");
	if(argc > 3) 
	{
		fprintf(stderr, "Invalid number of arguments\n");
		exit(0);
	}
	if(strlen(argv[1]) > 15 )
	{
		fprintf(stderr, "Username length cannot exceed 15 alphabets\n");
		exit(0);
	}
	
	strcpy(nameOfUser, argv[1]);
	
	//Communication starts
	char ifconfig[20];
	socketIdentifier = createsocket();
	if(argc == 2)
	{
		printf("%s started a new chat, listening on %s:%d\n" ,nameOfUser, getIP(ifconfig) , ntohs(useraddr.sin_port));
		printUsers();
		userEntry = 0;
		printf("Waiting for others to join...\n");
	}
	strcpy(chatUser[iterator].Username,argv[1]);
	chatUser[iterator].Port = ntohs(useraddr.sin_port);
	chatUser[iterator].isActive = 1;
	chatUser[iterator].timerBroadcastCheck = 0;
	chatUser[iterator].timerMsgBroadcastCheck = 0;
	chatUser[iterator].timerMsgBroadcastAccess = 0;
	strcpy(chatUser[iterator].IP,getIP(ifconfig));
	iterator++;
	
	if(argc == 3)
	{
		char temp[30];
		strcpy(temp,argv[2]);
		ipAddPortParsing(temp);
		
		toSendAddr(IP,port);
		printf("%s joining a new chat on %s, listening on %s:%d\n",nameOfUser,argv[2],getIP(ifconfig),ntohs(useraddr.sin_port));
		strcpy(buf,"Add");
		strcat(buf,"~");
		strcat(buf,argv[1]);
		if( sendto(socketIdentifier, buf, BUFSIZE, 0, (SA *)&requesteraddr, sizeof(requesteraddr)) < 0 )
		{
		  perror( "Sending message to server failed" );
		}
		else	//start timer to sense loss of packets in a separate thread
		{
			createReturn = pthread_create(&threadMessaging[3], &attribute, threadForTableReceiveCallback, buf);		//Creating the thread that uses the fprintf 
			if(createReturn)
			{
				fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
				exit(-1);
			}
			timer1 = 1;
			//printf("Connect request sent:%s\n",buf);
		}
	}
	
	while(1)
	{
		sem_wait(&S1);
		len = sizeof( incomingAddr ); 
		if ( recvlen=recvfrom(socketIdentifier, buf, sizeof(buf),0,(SA *)&incomingAddr,&len) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
		{        
			perror( "Error in recvfrom on Server socketIdentifier");
			exit(1);
		}
		else
		{
			//printf("%s\n",buf);
			controller(buf, arrayString2);
		}
		sem_post(&S1);
	}

	sem_destroy(&S1);

	pthread_attr_destroy(&attribute);				// Joining the internal threads with the main thread so that the main terminates only after the internal threads have completed
	for(i=0; i<THREADNUMBER; i++) 
	{
	    createReturn = pthread_join(threadMessaging[i], &state);
	    if (createReturn) 
	    {
		    fprintf(stderr,"In joining threads pthread_join() is %d\n", createReturn);
		    exit(0);
	    }
    }
	return 0;
}

int createsocket()
{
	int sockfd;
	if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0 )) < 0 )
	{
		perror( "Unable to open a socketIdentifier" );
		exit( ERR_SOCKET );
	}
	bzero( &useraddr, sizeof(useraddr));
	useraddr.sin_family = AF_INET;
	useraddr.sin_port = htons( randomPortGenerator() );
	useraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (SA *)&useraddr, sizeof(useraddr)) < 0) 
	{
		perror("Binding failed");
		return 0;
	}
return sockfd;
}
void toSendAddr(char* IP,int port)
{
	bzero( &requesteraddr, sizeof(requesteraddr));
	requesteraddr.sin_family = AF_INET;
	requesteraddr.sin_port = htons(port);
	if( inet_pton( AF_INET, IP, &requesteraddr.sin_addr ) <= 0 )
	{
		perror( "Unable to convert address to inet_pton \n" );
		exit( 99 );
	}
}
void ipAddPortParsing(char *toParse)
{
	int index = 0;
	char* token = strtok(toParse, ":");
	while (token) 
	{
		if(index==0)
		{
			strcpy(IP,token);
		}
		else if(index==1)
		{
			port = atoi(token);
		}
		token = strtok(NULL, ":");
		index++;
	}
}

void generalisedStringTok(char *toParse,ArrayString *arrayString)
{
	char temp[500];
	strcpy(temp,toParse);
	int index = 0;
	char* token = strtok(temp, "~");
	while (token) 
	{
		strcpy(arrayString[index].String,token);
		token = strtok(NULL, "~");
		index++;
	}
}

int randomPortGenerator() 
{
	srand(time(NULL));
	int r = rand() % 600;
	return (11000 + r);
}

void *threadForTableReceiveCallback(void *bufMsg)
{
	time_t start;
	time_t diff;
	int retry = 0;
	char tempMsg[500];
	while(1)
	{
		if(timer1 == 1)
		{
			//printf("Timer-1 has started\n");
			start = time (NULL);	//timer started
			while(timer1 == 1)	//still if timer on	; waiting for timer1 to get 0 when ack is received
			{
				diff = (time (NULL) - start);
				if(diff > 5)	//no response from leader for 60 secs
				{
					if(retry < 3)	//try one more time
					{
						printf("Retry#(Table-Sender) : %d\n",retry);
						if( sendto(socketIdentifier, bufMsg, BUFSIZE, 0, (SA *)&requesteraddr, sizeof(requesteraddr)) < 0 )
						{
						  perror( "Sending message to server failed" );
						}
						//printf("60 seconds elapsed, send message again\n");
						retry++;
						start = time (NULL);	//restart timer
						break;
					}
					else
					{
						printf("Contacting user dead, please try to connect to some other user of the group\n");
						timer1 = 0;
						retry = 0;
						exit(0);
						break;
					}
				}
			}
		}
		//timer1 to be reset on success on receiving acknowledge
	}
}
void *threadForAckTableReceiveCallback(void *bufMsg)
{
	time_t start;
	time_t diff;
	struct sockaddr_in tempAddr;
	char bufIdentifier[BUFSIZE];
	int retry = 0;
	int i;
	while(1)
	{
		for(i=0,retry=0;i<20;i++)
		{
			if(chatUser[i].timerBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
			{
				start = time (NULL);	//timer started
				while(chatUser[i].timerBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])))	//still if timer on	; waiting for timer1 to get 0 when ack is received
				{
					diff = (time (NULL) - start);
					if(diff > 5)	//no response from leader for 60 secs
					{
						if(retry < 3)	//try one more time
						{
							printf("Retry#(Table-Receiver) : %d\n",retry);
							bzero( &tempAddr, sizeof(tempAddr));
							tempAddr.sin_family = AF_INET;
							tempAddr.sin_port = htons(chatUser[i].Port);
							if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
							{
								perror( "Unable to convert address to inet_pton \n" );
								exit( 99 );
							}
							char temp[BUFSIZE];
							strcpy(temp,"Table");
							tableToString(temp);
							if (sendto(socketIdentifier,tableString, sizeof(tableString), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
							{
								perror("Error in sendto from server to client");
								return 0;
							}
							//printf("60 seconds elapsed, send message again\n");
							retry++;
							start = time (NULL);	//restart timer
							break;
						}
						else
						{
							chatUser[i].isActive = 0;
							broadCastMsg(socketIdentifier,6,chatUser[i].Username);
							tableCleanUp();
							chatUser[i].timerBroadcastCheck = 0;
							broadCastMsg(socketIdentifier,1,bufIdentifier);
							retry = 0;
							break;
						}
					}
				}
			}
			//timer1 to be reset on success on receiving acknowledge
		}
	}
}


void* threadForAckRecvdBroadcastMsgCallback(void *bufMsg)
{
	time_t start;
	time_t diff;
	struct sockaddr_in tempAddr;
	int retry = 0;
	int i;
	char bufIdentifier[BUFSIZE];
	while(1)
	{
		for(i=0;i<20;i++)
		{
			if(chatUser[i].timerMsgBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
			{
				start = time (NULL);	//timer started
				while(chatUser[i].timerMsgBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])))	//still if timer on	; waiting for timer1 to get 0 when ack is received
				{
					diff = (time (NULL) - start);
					if(diff > 60)	//no response from leader for 60 secs
					{
						if(retry < 3)	//try one more time
						{
							printf("Retry#(Msg-Sender) : %d\n",retry);
							bzero( &tempAddr, sizeof(tempAddr));
							tempAddr.sin_family = AF_INET;
							tempAddr.sin_port = htons(chatUser[i].Port);
							if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
							{
								perror( "Unable to convert address to inet_pton \n" );
								exit( 99 );
							}
							if( headSendQ != NULL && tailSendQ != NULL)
								strcpy(bufIdentifier,peakQueue(&headSendQ, &tailSendQ));
							if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
							{
								perror("Error in sendto from server to client");
								exit( 99 );
							}
							retry++;
							start = time (NULL);	//restart timer
							break;
						}
						else
						{
							chatUser[i].isActive = 0;
							broadCastMsg(socketIdentifier,6,chatUser[i].Username);
							tableCleanUp();
							chatUser[i].timerMsgBroadcastCheck = 0;
							broadCastMsg(socketIdentifier,1,bufIdentifier); 
							retry = 0;
							break;
						}
					}
				}
			}
			//timer1 to be reset on success on receiving acknowledge
		}
	}
}

void* threadForAccessRecvdBroadcastMsgCallback(void *bufMsg)
{
	time_t start;
	time_t diff;
	struct sockaddr_in tempAddr;
	int retry = 0;
	int i;
	char bufIdentifier[BUFSIZE];
	while(1)
	{
		if(wantCriticalSection == 1)
		{
			for(i=0;i<20;i++)
			{
				if(chatUser[i].timerMsgBroadcastAccess == 1 && !(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
				{
					start = time (NULL);	//timer started
					while(chatUser[i].timerMsgBroadcastAccess == 1 && !(isTableEntryEmpty(chatUser[i])))	//still if timer on	; waiting for timer1 to get 0 when ack is received
					{
						diff = (time (NULL) - start);
						if(diff > 5)	//no response from leader for 60 secs
						{
							if(retry < 3)	//try one more time
							{
								//printf("Retry#(Access) : %d-%s\n",retry,chatUser[i].Username);
								bzero( &tempAddr, sizeof(tempAddr));
								tempAddr.sin_family = AF_INET;
								tempAddr.sin_port = htons(chatUser[i].Port);
								if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
								{
									perror( "Unable to convert address to inet_pton \n" );
									exit( 99 );
								}
								if( headBackupQ != NULL && tailBackupQ != NULL)
									strcpy(bufIdentifier, peakQueue(&headBackupQ, &tailBackupQ));
								if (sendto(socketIdentifier, bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									perror("Error in sendto from server to client");
									exit( 99 );
								}
								// printf("60 seconds elapsed, send message again\n");
								retry++;
								start = time (NULL);	//restart timer
								break;
							}
							else
							{	
								chatUser[i].isActive = 0;
								broadCastMsg(socketIdentifier,6,chatUser[i].Username);
								tableCleanUp();
								chatUser[i].timerMsgBroadcastAccess = 0;
								broadCastMsg(socketIdentifier,1,bufIdentifier); 
								retry = 0;
								break;
							}
						}
					}
				}
				retry = 0;
			}
		}
	}
}

char* getIP(char *IPAdd)
{
	FILE *fp;
	fp = popen("/sbin/ifconfig em1 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'", "r");
	while (fgets(IPAdd,20, fp) != NULL)
	{
	    //printf("%s", IP);
	}
	pclose(fp);	
	IPAdd[strlen(IPAdd)-1] = '\0';
	return IPAdd;	
}

void controller(char *buf, ArrayString *arrayString1)
{
	char bufPrint[BUFSIZE];
	strcpy(bufPrint, buf);
	char tempMsg[BUFSIZE];
	struct sockaddr_in tempAddr;
	
	generalisedStringTok(buf,arrayString1);
	
	if(strcmp(arrayString1[0].String,"Message")==0)
	{
		controllerAcknowledgement("AckRecvdBroadcastMsg", nameOfUser);
		enqueue(bufPrint, &headRecvQ, &tailRecvQ);
	}
	else if(strcmp(arrayString1[0].String,"Table")==0)
	{
		timer1 = 0;
		controllerAcknowledgement("AckTable", nameOfUser);
		stringToTable(arrayString1[1].String);
		if(userEntry == 1)
		{
			printUsers();
			userEntry = 0;
		}
		copyFromTable();
		resetAllTimers();
	}
	else if(strcmp(arrayString1[0].String,"Clean")==0)
	{
		timer1 = 0;
		int i;
		controllerAcknowledgement("AckTable", nameOfUser);
		for(i = 0; i < 20; i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])))
			{
				clearTableEntry(chatUser, i);
			}
		}
		stringToTable(arrayString1[1].String);
		//printTable();
		if(userEntry == 1)
		{
			printUsers();
			userEntry = 0;
		}
		resetAllTimers();
	}
	else if(strcmp(arrayString1[0].String,"Access")==0)
	{
		char bufPrintX[BUFSIZE];
		//printf("received access: %s\n", bufPrint);
		controllerAcknowledgement("AckAccess", nameOfUser);
		printQ(&headSendQ, &tailSendQ);
		ArrayString arrayStringX[20];
		strcpy(bufPrintX, buf);
		generalisedStringTok(bufPrintX,arrayStringX);
		if(headSendQ == NULL && tailSendQ == NULL)
		{
			bzero( &tempAddr, sizeof(tempAddr));
			tempAddr.sin_family = AF_INET;
			tempAddr.sin_port = htons(chatUser[findIndexOfUserName(arrayStringX[1].String)].Port);
			if( inet_pton( AF_INET, chatUser[findIndexOfUserName(arrayStringX[1].String)].IP, &tempAddr.sin_addr ) <= 0 )
			{
				//printTable();
				//printf("whole message is: %s %s %s\n", arrayStringX[0].String);
				//printf("username is: %s ip:%s : %s\n", arrayStringX[1].String, chatUser[findIndexOfUserName(arrayStringX[1].String)].IP, chatUser[findIndexOfUserName(arrayStringX[1].String)].Port);
				perror( "Unable to convert address to inet_pton 1\n" );
				exit( 99 );
			}
			char tbuf[500];
			strcpy(tbuf, "OK");
			strcat(tbuf, "~");
			strcat(tbuf, nameOfUser);
			if (sendto(socketIdentifier, tbuf, sizeof(tbuf), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
			{
				perror("Error in sendto from server to client");
			}
			//printf("sending ok immediately to %s\n", arrayString1[1].String);
		}
		else
		{
			//printf("Request from other user to send message as well\n");
			if(atol(arrayString1[2].String) > myTimeSec || (atol(arrayString1[2].String) == myTimeSec && atol(arrayString1[3].String) > myTime))
			{
				//wait for n-1 OKs and then broadcast the message
				printf("user %s has to wait as I have higher priority\n", arrayString1[1].String);
				char pendingRequestUser[500];
				strcpy(pendingRequestUser, arrayString1[1].String);
				strcat(pendingRequestUser, "~");
				strcat(pendingRequestUser, arrayString1[2].String);
				updateHoldBackList(pendingRequestUser, &headWaitList, &tailWaitList);
			}
			else
			{
				//send OK to that user
				char tbuf[500];
				strcpy(tbuf, "OK");
				strcat(tbuf, "~");
				strcat(tbuf, nameOfUser);
				if (sendto(socketIdentifier, tbuf, sizeof(tbuf), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
				{
					perror("Error in sendto from server to client");
				}
				printf("sending ok as my priority is lower to %s\n", arrayString1[1].String);
			}
		}
	}
	else if(strcmp(arrayString1[0].String,"Add")==0)
	{
		if( checkIfUsernameExists(arrayString1[1].String) == 1)
		{
			char bufIdentifier[BUFSIZE];
			strcpy(bufIdentifier,"Error-2");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)	
			{
				perror("Error in sendto from server to client");
			}
		}
		else
		{
			if( iterator < 20 )
			{
				//Filling up table
				broadCastMsg(socketIdentifier,4,arrayString1[1].String);													//Updating the table with new User
				strcpy(chatUser[iterator].Username,arrayString1[1].String);
				chatUser[iterator].Port = ntohs(incomingAddr.sin_port);
				chatUser[iterator].isActive = 1;
				chatUser[iterator].timerBroadcastCheck = 0;
				chatUser[iterator].timerMsgBroadcastCheck = 0;
				chatUser[iterator].timerMsgBroadcastAccess = 0;
				//setTimeStamper();
				strcpy(chatUser[iterator].IP,inet_ntoa(incomingAddr.sin_addr));
				iterator++;
				int check = broadCastMsg(socketIdentifier,1,buf);
				if(check == 0)
				{
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
				//printf("after receiving add request from user:\n");
				//printTable();
			}
			else
			{
				sendMessageForUserLimit();
			}
		}		
	}
	else if(strcmp(arrayString1[0].String,"AckTable")==0)
	{
		//printf("Received acknowledgement for table\n");
		resetTimerForGivenUser(arrayString1[1].String,1);
		//printf("after receiving acktable:\n");
		//printTable();
	}
	else if(strcmp(arrayString1[0].String,"AckRecvdBroadcastMsg")==0)
	{
		//printf("Received acknowledgement for broadcasted message\n");
		resetTimerForGivenUser(arrayString1[1].String,2);
	}
	else if(strcmp(arrayString1[0].String,"AckAccess")==0)
	{
		ackAccessCount++;
		//printf("%s\n",bufPrint);
		resetTimerForGivenUser(arrayString1[1].String, 3);
		if( headBackupQ != NULL && tailBackupQ != NULL)				//flushing out the head of backUp Queue
		{
			dequeue(&headBackupQ, &tailBackupQ);
		}
		
		if(ackAccessCount == numOfChatUsers() - 1)
		{
			wantCriticalSection = 0;
		}
	}
	
	else if(strcmp(arrayString1[0].String,"OK")==0)
	{
		ackCount++;
		//printf("Received OK from %d. %s\n", ackCount, arrayString1[1].String);
		setOKForUser(arrayString1[1].String);
		//printRecvOK();
		//printf("%d\n", checkAllOK());
		if(checkAllOK() == 1)
		{
			//printf("I have the critical section\n");
			
			//printf("dequeue\n");
			while(headSendQ != NULL && tailSendQ != NULL)
			{
				dequeue(&headSendQ, &tailSendQ);
				//printQ(&headSendQ, &tailSendQ);
				//printf("removed message from sendQ\n");
				//printf("received n-1 ok for %s\n", dequeuedMsg);
				broadCastMsg(socketIdentifier, 2, dequeuedMsg);
			}
			
			emptyHoldBackList();
			//printf("I release the critical section\n");
			resetRecvOK();
			ackCount = 0;
			myTime = 0;
		}
		
	}
	else if(strcmp(arrayString1[0].String,"Error-1")==0)
	{
		fprintf(stderr,"Maximum number of users in the group can be atmost 20\n");
		exit(-1);
	}
	else if(strcmp(arrayString1[0].String,"Error-2")==0)
	{
		fprintf(stderr,"Username already exists\n");
		exit(-1);
	}
	else if(strcmp(arrayString1[0].String,"Notice")==0)
	{
		fprintf(stdout,"%s\n",arrayString1[1].String);
	}
	else if(strcmp(arrayString1[0].String,"Alert")==0)
	{
		fprintf(stdout,"%s\n",arrayString1[1].String);
		exit(0);
	}
}

void emptyHoldBackList()
{
	char otherUser[15];
	char bufIdentifier[BUFSIZE];
	struct sockaddr_in tempAddr;
	char tbuf[500];
	int indexOfUser;
	int myIndex;
	ArrayString arrayString4[20];

	
	while(headWaitList != NULL && tailWaitList != NULL)
	{
		//printf("I no longer have the critical section, hence sending OK to pending users\n");
		strcpy(otherUser, deleteNode(otherUser, &headWaitList, &tailWaitList));
		
		if(strcmp(otherUser, "") != 0)
		{
			generalisedStringTok(otherUser, arrayString4);
			indexOfUser = findIndexOfUserName(arrayString4[0].String);

			bzero( &tempAddr, sizeof(tempAddr));
			tempAddr.sin_family = AF_INET;
			tempAddr.sin_port = htons(chatUser[indexOfUser].Port);
			
			if( inet_pton( AF_INET, chatUser[indexOfUser].IP, &tempAddr.sin_addr ) <= 0 )
			{
				perror( "Unable to convert address to inet_pton 2\n" );
				exit( 99 );
			}
			strcpy(tbuf, "OK");
			strcat(tbuf, "~");
			strcat(tbuf, nameOfUser);
			//printf("sending ok to %s\n", arrayString4[0].String);
			if (sendto(socketIdentifier, tbuf, sizeof(tbuf), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
			{
				perror("Error in sendto from server to client");
			}
			//printf("sending ok as my work is over now to %s\n", arrayString4[0].String);
			strcpy(otherUser, "");
		}
	}
}
int checkIfUsernameExists(char *username)
{
	int i;
	for(i=0;i<20;i++)
	{
		if( strcmp(chatUser[i].Username,username) == 0)
		{
			return 1;
		}
	}
	return 0;
}

int broadCastMsg(int sock, int identifier,char *msg)
{
	char bufBroadCast[BUFSIZE];
	ArrayString arrayString[20];
	char msg2[500];
	char tempTable[BUFSIZE];
	strcpy(msg2, msg);
	generalisedStringTok(msg2, arrayString);
	int i;
	if(identifier == 0) 	//connection establishment
	{
		strcpy(tempTable,"Clean");
		struct sockaddr_in tempAddr;
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 3\n" );
					exit( 99 );
				}
				chatUser[i].timerBroadcastCheck = 1;

				tableToString(tempTable);
				if (sendto(sock,tableString, sizeof(tableString), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					perror("Error in sendto from server to client");
					return 0;
				}
				//printf("Table broadcasted\n");
			}
		}	
	}
	else if(identifier == 1) 	//connection establishment
	{
		strcpy(tempTable,"Table");
		struct sockaddr_in tempAddr;
		//resetAllSeqNum();
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 3\n" );
					exit( 99 );
				}
				chatUser[i].timerBroadcastCheck = 1;

				tableToString(tempTable);
				if (sendto(sock,tableString, sizeof(tableString), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					perror("Error in sendto from server to client");
					return 0;
				}
				//printf("Table broadcasted\n");
			}
		}	
	}
	else if(identifier == 2) //normal messages
	{
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast, nameOfUser);
		strcat(bufBroadCast,":: ");
		strcat(bufBroadCast, msg);	//message
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		strcpy(msg, "Message");
		strcat(msg, "~");
		strcat(msg,nameOfUser);
		strcat(msg,"~");
		strcat(msg, bufBroadCast);
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i]))&& i!=findIndexOfUserName(nameOfUser))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 4\n" );
					exit( 99 );
				}
				char bufTemp[BUFSIZE];
				strcpy(bufTemp, msg);
				chatUser[i].timerMsgBroadcastCheck = 1;
				//printf("sending %s to %s\n", bufTemp, chatUser[i].Username);
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					perror("Error in sendto from server to client");
					return 0;
				}
			}
		}
	}
	else if(identifier == 3) //crashed messages
	{
		char bufTemp[BUFSIZE];
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," left the chat or crashed");
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser) && i!=findIndexOfUserName(msg))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 5\n" );
					exit( 99 );
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					perror("Error in sendto from server to client");
					return 0;
				}
			}
		}
	}
	else if(identifier == 4) //new user messages
	{
		char bufTemp[BUFSIZE];
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," has joined the chat");
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 6\n" );
					exit( 99 );
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					perror("Error in sendto from server to client");
					return 0;
				}
			}
		}
	}
	else if(identifier == 5) //access message
	{
		
		struct sockaddr_in tempAddr;
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser)) //IMP
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 7\n" );
					exit( 99 );
				}
				char bufTemp[BUFSIZE];
				strcpy(bufTemp, msg);
				
				chatUser[i].timerMsgBroadcastAccess = 1;
				//printf("sending access to %s for message no: %s of user %s\n", chatUser[i].Username, arrayString[2].String, arrayString[1].String);
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					perror("Error in sendto from server to client");
					return 0;
				}
			}
		}
		sleep(0.2);
	}
	else if(identifier == 6) //crashed messages
	{
		char bufTemp[BUFSIZE];
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," left the chat or crashed");
		printf("%s\n",bufBroadCast);
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && i!=findIndexOfUserName(nameOfUser) && i!=findIndexOfUserName(msg))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					perror( "Unable to convert address to inet_pton 8\n" );
					exit( 99 );
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					perror("Error in sendto from server to client");
					return 0;
				}
			}
		}
	}
}
int isTableEntryEmpty(ChatUserInfo tableEntry)
{
	if(tableEntry.Port == 0 && tableEntry.isActive == 0 && strcmp(tableEntry.Username,"") == 0 && strcmp(tableEntry.IP,"") == 0 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void copyUserDatabaseTable(ChatUserInfo *table)
{
	int i;
	for(i=0;i<20;i++)
	{
		chatUser[i] = table[i];
		//printf("%d-ID:%d-User:%s-isLeader-%d-Port#-%d\n",(i+1),chatUser[i].ID,chatUser[i].Username,chatUser[i].isLeader,chatUser[i].Port);
	}
}
void controllerAcknowledgement(char *ack, char *user)
{
	//send Acknowledgement
	char acknowledgementType[500];
	char tempMsg[500];
	struct sockaddr_in tempAddr;
	if(strcmp(ack, "AckTable") == 0)
	{
		resetTimerForGivenUser(nameOfUser,1);
		strcpy(acknowledgementType, "AckTable");
		strcat(acknowledgementType, "~");
		strcat(acknowledgementType, user);
		if (sendto(socketIdentifier, acknowledgementType, sizeof(acknowledgementType), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
		{
			perror("Error in sendto from server to client");
		}
	}
	
	if(strcmp(ack, "AckAccess") == 0)
	{
		bzero( &tempAddr, sizeof(tempAddr));
		tempAddr.sin_family = AF_INET;
		tempAddr.sin_port = htons(chatUser[findIndexOfUserName(user)].Port);
		if( inet_pton( AF_INET, chatUser[findIndexOfUserName(user)].IP, &tempAddr.sin_addr ) <= 0 )
		{
			perror( "Unable to convert address to inet_pton 9\n" );
			exit( 99 );
		}
		strcpy(acknowledgementType, "AckAccess");
		strcat(acknowledgementType, "~");
		strcat(acknowledgementType, user);
		if (sendto(socketIdentifier, acknowledgementType, sizeof(acknowledgementType), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
		{
			perror("Error in sendto from server to client");
		}
	}
	else if(strcmp(ack, "AckRecvdBroadcastMsg") == 0)
	{
		bzero( &tempAddr, sizeof(tempAddr));
		tempAddr.sin_family = AF_INET;
		tempAddr.sin_port = htons(chatUser[findIndexOfUserName(user)].Port);
		if( inet_pton( AF_INET, chatUser[findIndexOfUserName(user)].IP, &tempAddr.sin_addr ) <= 0 )
		{
			perror( "Unable to convert address to inet_pton 10\n" );
			exit( 99 );
		}
		strcpy(acknowledgementType, "AckRecvdBroadcastMsg");
		strcat(acknowledgementType, "~");
		strcat(acknowledgementType, user);
		if (sendto(socketIdentifier,acknowledgementType, sizeof(acknowledgementType), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
		{
			perror("Error in sendto from server to client");
		}
	}
}
void resetTimerForGivenUser(char *User,int identifier)
{
	int i;
	for(i=0;i<20;i++)
	{
		if(identifier == 1)
		{
			if(strcmp( chatUser[i].Username,User) == 0)
			chatUser[i].timerBroadcastCheck = 0;
		}
		else if(identifier == 2)
		{
			if(strcmp( chatUser[i].Username,User) == 0)
			chatUser[i].timerMsgBroadcastCheck = 0;
		}
		else if(identifier == 3)
		{
			if(strcmp( chatUser[i].Username, User) == 0)
			{
				chatUser[i].timerMsgBroadcastAccess = 0;	
			}
		}
	}
}
void sendMessageForUserLimit()
{
	char bufIdentifier[BUFSIZE];
	strcpy(bufIdentifier,"Error-1");
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
	{
		perror("Error in sendto from server to client");
		exit( 99 );
	}
}
void tableCleanUp()
{
	int i;
	int count = 0;
	char tempIndex[500];
	for(i = 0; i < 20; i++)
	{
		if(!(isTableEntryEmpty(chatUser[i])))
		{
			if(chatUser[i].isActive == 0)
			{
				clearTableEntry(chatUser, i);
				// chatUser[i] = chatUser[iterator - 1];
				// clearTableEntry(chatUser, iterator - 1);
				// iterator--;
			}
		}
	}
}
void clearTableEntry(ChatUserInfo *tableEntry,int rowNum)
{
	tableEntry[rowNum].Port = 0;
	tableEntry[rowNum].isActive = 0;
	tableEntry[rowNum].timerMsgBroadcastCheck = 0;
	tableEntry[rowNum].timerBroadcastCheck = 0;
	strcpy(tableEntry[rowNum].Username,"");
	strcpy(tableEntry[rowNum].IP,"");
}

void *queueForMessaging(void *arg)
{
	//printf("inside queueForMessaging\n");
	char buf[500];
	//Inputting message sequence from stdin to place in sendq
	for(;(fgets(buf, sizeof(buf),stdin) != NULL);)
	{
		if(strlen(buf) != 0)
		{
			int i = strlen(buf) - 1;
			if( buf[i] == '\n' )
				buf[i] = '\0';
			if(strlen(buf) > 220)
			{
				fprintf(stderr, "Message length exceeded[220]\n");
			}
			else if( buf[0] != '\0' && buf[0] != '\n')
			{
				//printf("taking in input\n");
				enqueue(buf, &headSendQ, &tailSendQ);	//TODO:Use a mutex as this is used by multiple threads
				flag = 1;	//set as there is some message typed by the user to send, used to send access to all users
				wantCriticalSection = 1;
			}
		}
	}

	printf("Exiting the chat group\n");			//TODO: Updated table has to be sent to all the users, might be handled from somewhere else
	strcpy(buf,nameOfUser);
	chatUser[findIndexOfUserName(buf)].isActive = 0;
	tableCleanUp();
	//printTable();
	broadCastMsg(socketIdentifier,3 , buf);
	broadCastMsg(socketIdentifier,0 , buf);
	exit(0); 
}

void *queueForPrinting(void *arg)
{
	
	ArrayString arrayString3[20];
	char buf[500];
	char tempMsg[500];
	//printf("inside queueForPrinting\n");
	//Printing out the message out on the console by dequeuing the recvQ
	while(1)
	{
		if(headRecvQ != NULL && tailRecvQ != NULL)
		{
			dequeue(&headRecvQ, &tailRecvQ);
			strcpy(tempMsg, dequeuedMsg);
			generalisedStringTok(tempMsg, arrayString3);
			printf("%s\n", arrayString3[2].String);
		}
	}
	pthread_exit(NULL);
}

void *threadForSendingAccess(void *Username)
{
	char tempMsg[500];
	while(1)
	{
		if(flag == 1)
		{
			
			if(numOfChatUsers() == 1)
			{
				while(headSendQ != NULL && tailSendQ != NULL)
				{
					dequeue(&headSendQ, &tailSendQ);
					broadCastMsg(socketIdentifier, 2, dequeuedMsg);
				}
				flag = 0;
			}

			strcpy(tempMsg, "Access~");
			strcat(tempMsg, nameOfUser);
			strcat(tempMsg, "~");
			strcpy(tempMsg, sequencing(tempMsg));	//assigning currentTime to the message at which it is sending the message
			enqueue(tempMsg, &headBackupQ, &tailBackupQ);	//making connection reliable by waiting for ack and then removing from backUpQ once receive
			broadCastMsg(socketIdentifier, 5, tempMsg);
			flag = 0;
		}
	}
	pthread_exit(NULL);
}

int isEmpty(QueueNode **head, QueueNode **tail)
{
	
	if( (*head)==NULL && (*tail)==NULL )
		return 1;
	else
	{
		return 0;
	}
}

void enqueue(char *message, QueueNode **head, QueueNode **tail)
{
	QueueNode *addNode=NULL;
	addNode = (QueueNode *)malloc(1*sizeof(QueueNode));
	strcpy(addNode -> content,message);
	addNode -> next = NULL;
	if( *head==NULL && *tail==NULL )
	{
		*head = addNode;
		*tail = addNode;
	}
	else
	{
		(*tail) -> next = addNode;
		*tail = addNode;
	}
}

void dequeue(QueueNode **head, QueueNode **tail)
{
	QueueNode *temp;
	
	if( *head == *tail )
	{
		temp = *head;
		strcpy(dequeuedMsg,temp -> content);
		*head = NULL;
		temp -> next = NULL;
		*tail = NULL;
		free(temp);
	}
	else
	{
		temp = *head;
		strcpy(dequeuedMsg,temp -> content);
		temp = temp -> next;
		free(*head);
		*head = temp;
	}
}
char* sequencing(char *tempMsg)
{
	char buf[50];
	struct timeval start;
	struct timezone tzp;
	gettimeofday(&start, &tzp);
	myTime = (long int)start.tv_usec;
	myTimeSec = (long int)start.tv_sec;
	sprintf(buf, "%ld", myTimeSec);
	strcat(tempMsg, buf);	//Access~username~sec~usec
	sprintf(buf, "%ld", myTime);
	strcat(tempMsg, "~");
	strcat(tempMsg, buf);
	return tempMsg;
}

char* peakQueue(QueueNode **head, QueueNode**tail)
{
	return (*head)->content;
}

int findIndexOfUserName(char *userName)
{
	int k;
	for(k = 0; k < 20; k++)
	{
		if( strcmp( chatUser[k].Username,userName) == 0)
			return k;
	}
	return -1;
}
void printTable()
{
	int i;
	for(i=0;i<20;i++)
	{
		if( !(isTableEntryEmpty(chatUser[i])))
		{
			printf("Username:%s,IP:%s,Port#:%d,isActive:%d,timerBroadcastCheck: %d\n",chatUser[i].Username,chatUser[i].IP,chatUser[i].Port,chatUser[i].isActive, chatUser[i].timerBroadcastCheck);
		}
	}
}

int numOfChatUsers()
{
	int i,count=0;
	for(i=0;i<20;i++)
	{
		if(!(isTableEntryEmpty(chatUser[i])))
		{
			count++;
		}
	}
	return count;
}


void updateHoldBackList(char *msg, InsertionList **head, InsertionList **tail)
{
	//msg: username~time
	//TO DO: optimization of if-else temp->next ==NULL
	char tempbuf[500];
	ArrayString arrayString[20];
	strcpy(tempbuf, msg);
	generalisedStringTok(tempbuf, arrayString);
	InsertionList *addNode = (InsertionList *)malloc(sizeof(InsertionList));
	strcpy(addNode->otherUserName, msg); //otherUserName now has username~time
	addNode->seqTime = atol(arrayString[1].String); //UserName~TimeOfOtherUser
	addNode->next = NULL;
	if(*head == NULL && *tail == NULL)
	{
		//printf("first message added out of order\n");
		*head = addNode;
		*tail = addNode;
	}
	else
	{
		//printf("message added to holdbacklist\n");
		InsertionList *temp;
		temp = *head;
		while(temp != NULL)
		{
			if(addNode->seqTime < temp->seqTime)
			{
				*head = addNode;
				addNode->next = temp;
				break;
			}
			else if(temp == *tail)
			{
				(*tail)->next = addNode;
				(*tail) = addNode;
				break;
			}	
			else if((addNode->seqTime > temp->seqTime) && (addNode->seqTime < (temp->next->seqTime)))
			{
				addNode->next = temp->next;
				temp->next = addNode;
				break;
			}
			temp = temp->next;
		}
	}
}

char *deleteNode(char *msgbuf, InsertionList **head, InsertionList **tail)
{
	InsertionList *removeNode;
	if( *head == *tail)
	{
		removeNode = *head;
		strcpy(msgbuf, (*head)->otherUserName);
		free(removeNode);
		*head = NULL;
		*tail = NULL;
	}
	else
	{
		removeNode = *head;
		*head = removeNode->next;
		strcpy(msgbuf, removeNode->otherUserName);
		free(removeNode);
	}
	return msgbuf;
}

void printQ(QueueNode **head, QueueNode **tail)
{
	QueueNode *temp;
	temp = *head;
	if(*head == NULL && *tail == NULL)
	{
		//printf("Queue is empty\n");
	}
	else
	{
		while(temp != NULL)
		{
			//printf("%s - ", temp->content);
			temp = temp->next;
		}
	}
	
}

void resetAllTimers()
{
	int i;
	for(i=0;i<20;i++)
	{
		if( !(isTableEntryEmpty(chatUser[i])))
		{
			chatUser[i].timerBroadcastCheck = 0;
			chatUser[i].timerMsgBroadcastCheck = 0;
			chatUser[i].timerMsgBroadcastAccess = 0;
		}
	}
}

void copyFromTable()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(!(isTableEntryEmpty(chatUser[i])))
		{
			strcpy(recvOK[i].name,chatUser[i].Username);
		}
		else
		{
			strcpy(recvOK[i].name,"");
		}
	}
}

void setOKForUser(char *User)
{
	int i;
	for(i=0;i<20;i++)
	{
		if(strcmp(recvOK[i].name,User) == 0)
		{
			recvOK[i].check = 1;
		}
	}
}

void resetRecvOK()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(strcmp(recvOK[i].name,"") != 0)
		{
			recvOK[i].check = 0;
		}
	}
}

int checkAllOK()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(strcmp(recvOK[i].name,nameOfUser) != 0 && (strcmp(recvOK[i].name,"") != 0))
		{
			if(recvOK[i].check == 0)
			{
				return 0;
			}			
		}
	}
	return 1;
}

void printRecvOK()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(strcmp(recvOK[i].name,"") != 0)
		{
			printf("%s-%d\n",recvOK[i].name,recvOK[i].check );	
		}
	}
}

void tableToString(char *identifier)
{
	int i;
	char temp[20];
	strcpy(tableString,identifier);
	strcat(tableString,"~");
	//printf("converting table to string\n");
	for(i=0;i<20;i++)
	{
		if(!(isTableEntryEmpty(chatUser[i])))
		{
			strcat(tableString,chatUser[i].Username);
			strcat(tableString,",");
			sprintf(temp,"%d",chatUser[i].Port);
			strcat(tableString,temp);
			strcat(tableString,",");
			sprintf(temp,"%d",chatUser[i].isActive);
			strcat(tableString,temp);
			strcat(tableString,",");
			strcat(tableString,chatUser[i].IP);
			strcat(tableString,",");
			sprintf(temp,"%d",chatUser[i].timerBroadcastCheck);
			strcat(tableString,temp);
			strcat(tableString,",");
			sprintf(temp,"%d",chatUser[i].timerMsgBroadcastCheck);
			strcat(tableString,temp);
			strcat(tableString,",");
			sprintf(temp,"%d",chatUser[i].timerMsgBroadcastAccess);
			strcat(tableString,temp);
			strcat(tableString,"|");
		}
	}
}

void stringToTable(char *stringTable)
{
	char temp[500];
	char tempRow[500];
	ArrayString arrayString[20];
	strcpy(temp,stringTable);
	int index = 0,i;
	int col = 0;
	//printf("converting string to table\n");
	char* token = strtok(temp, "|");
	while (token) 
	{
		strcpy(arrayString[index].String,token);
		token = strtok(NULL, "|");
		index++;
	}
	for(i=0;i<index;i++)
	{
		col = 0;
		char* tokenElement = strtok(arrayString[i].String, ",");
		while(tokenElement)
		{
			if(col == 0)
			{
				strcpy(chatUser[i].Username,tokenElement);
			}
			else if(col == 1)
			{
				chatUser[i].Port = atoi(tokenElement);
			}
			else if(col == 2)
			{
				chatUser[i].isActive = atoi(tokenElement);
			}
			else if(col == 3)
			{
				strcpy(chatUser[i].IP,tokenElement);
			}
			else if(col == 4)
			{
				chatUser[i].timerBroadcastCheck = atoi(tokenElement);
			}
			else if(col == 5)
			{
				chatUser[i].timerMsgBroadcastCheck = atoi(tokenElement);
			}
			else if(col == 6)
			{
				chatUser[i].timerMsgBroadcastAccess = atoi(tokenElement);
			}
			col++;
			tokenElement = strtok(NULL, ",");
		}
	}
}

void printUsers()
{
	int i;
	printf("Succeeded, Current Users:\n");
	for(i=0;i<20;i++)
	{
		if(isTableEntryEmpty(chatUser[i]) == 0)
		{
			printf("%s %s:%d\n",chatUser[i].Username,chatUser[i].IP,chatUser[i].Port);
		}
	}
}