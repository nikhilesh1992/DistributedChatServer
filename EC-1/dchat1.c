#include "dchat.h"

int main(int argc, char const *argv[])
{
	// Local variable definitions
	int recvlen,i=0;
	int userEntry =  0;
	char buf[BUFSIZE];
	char tempbuf[BUFSIZE];
	char tempPortString[20];
	ChatUserInfo bufRecvdTable[20];
	socklen_t len;
	ArrayString arrayString[20];
	ArrayString arrayString2[20];
	ArrayString arrayS[20];
	int createReturn;
	void *state;
	char congestionCommand[40];
	strcpy(congestionCommand,"");
	initialiseMap();

	sem_init(&S1, 1, 1);
	sem_init(&S2, 1, 1);
	sem_init(&S2, 1, 1);
	sem_init(&SendQ, 1, 1);
	sem_init(&RecvQ, 1, 1);
	sem_init(&msgSent, 1, 1);
	sem_init(&Congestion,1,1);

	logFile = fopen("Log.txt", "w+");
	fprintf(logFile, "START LOGGING: \n\n");
	
	if(argc > 3 && argc < 2) 
	{
		fprintf(logFile, "ERROR: Invalid number of arguments\n");
		fprintf(stderr, "Invalid number of arguments\n");
		exit(0);
	}
	if(strlen(argv[1]) > 15 )
	{
		fprintf(logFile, "ERROR: Username length cannot exceed 15 alphabets\n");
		fprintf(stderr, "Username length cannot exceed 15 alphabets\n");
		exit(0);
	}

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
	createReturn = pthread_create(&threadMessaging[2], &attribute, threadForSending,temp);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting messaging thread %d\n", createReturn);
         exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[4], &attribute, threadForAckTableReceiveCallback, NULL);		//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[5], &attribute, threadForMessageSentCallback, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[6], &attribute, threadForAckRecvdBroadcastMsgCallback, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[7], &attribute, threadClearGlobalQ, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[9], &attribute, threadForMapRefresh, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[10], &attribute, threadForLeaderBrdCast, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	createReturn = pthread_create(&threadMessaging[11], &attribute, threadForMsgCount, NULL);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	strcpy(nameOfUser, argv[1]);
	if(argc == 2)
	{
		char ifconfig[20];
		socketIdentifier = createsocket();
		printf("%s started a new chat, listening on %s:%d\n" ,nameOfUser, getIP(ifconfig) , ntohs(useraddr.sin_port));
		fprintf(logFile, "INFO: \n");
		chatUser[iterator].ID = ID;
		strcpy(chatUser[iterator].Username,argv[1]);
		chatUser[iterator].Port = ntohs(useraddr.sin_port);
		chatUser[iterator].isActive = 1;
		chatUser[iterator].isLeader = 1;
		chatUser[iterator].receivedMsgSeqNo = 0;
		chatUser[iterator].timerMsgBroadcastCheck = 0;
		chatUser[iterator].timerBroadcastCheck = 0;
		strcpy(chatUser[iterator].IP,getIP(ifconfig));
		toSendAddr(chatUser[iterator].IP,chatUser[iterator].Port);
		printUsers();
		printf("Waiting for others to join...\n");
		iterator++;
		ID--;
	}
	else if(argc == 3)
	{
		char ifconfig[20];
		chatUser[iterator].ID = 100;
		strcpy(chatUser[iterator].Username,nameOfUser);
		chatUser[iterator].isActive = 1;
		chatUser[iterator].isLeader = 0;
		chatUser[iterator].receivedMsgSeqNo = 0;
		chatUser[iterator].timerMsgBroadcastCheck = 0;
		chatUser[iterator].timerBroadcastCheck = 0;
		char temp[30];
		strcpy(temp,argv[2]);
		ipAddPortParsing(temp);
		socketIdentifier = createsocket();
		toSendAddr(IP,port);
		strcpy(buf,"Add");
		strcat(buf,"~");
		strcat(buf,argv[1]);
		fprintf(logFile, "INFO: \n");
		userEntry = 1;
		printf("%s joining a new chat on %s, listening on %s:%d\n",nameOfUser,argv[2],getIP(ifconfig),ntohs(useraddr.sin_port));
		if( sendto(socketIdentifier, buf, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
		{
			fprintf(logFile, "ERROR: \n");
			perror( "Sending message to server failed" );
		}
		else	//start timer to sense loss of packets in a separate thread
		{
			createReturn = pthread_create(&threadMessaging[3], &attribute, threadForTableReceiveCallback, buf);		//Creating the thread that uses the fprintf 
			if(createReturn)
			{
				fprintf(logFile, "ERROR: \n");
				fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
				closeChatServer();
			}
			sem_wait(&S3);
			timer1 = 1;
			sem_post(&S3);
			//printf("Connect request sent:%s\n",buf);
		}
	}
	createReturn = pthread_create(&threadMessaging[8], &attribute, threadForTrafficControl,congestionCommand);	//Creating the thread that uses the fprintf 
	if(createReturn)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stderr,"Error in starting table acknowledgement thread %d\n", createReturn);
		exit(-1);
	}
	while(1)
	{
		if(amILeader(nameOfUser) == 1)
		{
			len = sizeof( incomingAddr ); 
			if ( recvlen=recvfrom(socketIdentifier, buf, BUFSIZE, 0,(SA *)&incomingAddr, &len) < 0)	//receive message from user
			{     
				fprintf(logFile, "ERROR: \n");
				perror( "Error in recvfrom on Server socketIdentifier");
				closeChatServer();
			}
			
			controllerLeader(buf, arrayString);
			
		}
		else if(amILeader(nameOfUser) == 0)
		{
			if ( recvlen=recvfrom(socketIdentifier, buf, sizeof(buf),0,(SA *)&incomingAddr,&len) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
			{     
				fprintf(logFile, "ERROR: \n");
				perror( "Error in recvfrom on Server socketIdentifier");
				closeChatServer();
			}
			strcpy(tempbuf,buf);
			generalisedStringTok(tempbuf,arrayS);
			if(strcmp(buf,"Table") == 0)
			{
				if ( recvlen=recvfrom(socketIdentifier, bufRecvdTable, sizeof(bufRecvdTable), 0,0,0) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
				{      
					fprintf(logFile, "ERROR: \n");
					perror( "Error in recvfrom on Server socketIdentifier");
					closeChatServer();
				}
				else
				{
					//acknowledgement in the form of table received, reset timer1
					timer1 = 0;
					copyUserDatabaseTable(bufRecvdTable);
					resetAllTimers();
					updateLeaderAddress();
					updateTimeStamper();
					resetAllSeqNum();
					if(userEntry == 1)
					{
						printUsers();
						userEntry = 0;
					}
					controllerAcknowledgement("AckTable", nameOfUser);
				}
			}
			else if(strcmp(buf,"String") == 0)
			{
				//receive message with timestamper
				if ( recvlen = recvfrom(socketIdentifier, buf, sizeof(buf), 0,0,0) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
				{   
					fprintf(logFile, "ERROR: \n");
					perror( "Error in recvfrom on Server socketIdentifier");
					closeChatServer();
				}
				controllerNonLeader(buf, arrayString2);
			}
			else if(strcmp(arrayS[0].String,"Add") == 0)
			{
				//receive message with timestamper
				highPriority = 1;
				char tempPort[10];
				strcpy(tempbuf,"Alert");
				strcat(tempbuf,"~");
				strcat(tempbuf, arrayS[1].String);
				strcat(tempbuf,"~");
				strcat(tempbuf,inet_ntoa(incomingAddr.sin_addr));
				strcat(tempbuf,"~");
				sprintf(tempPort,"%d",ntohs(incomingAddr.sin_port));
				strcat(tempbuf,tempPort);
				if (sendto(socketIdentifier,tempbuf, sizeof(tempbuf), 0,(SA *)&leaderaddr, sizeof(leaderaddr)) < 0)	
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					return 0;
				}
			}

		}
	}

	pthread_attr_destroy(&attribute);				// Joining the internal threads with the main thread so that the main terminates only after the internal threads have completed
	for(i=0; i<THREADNUMBER; i++) 
	{
	    createReturn = pthread_join(threadMessaging[i], &state);
	    if (createReturn) 
	    {
			fprintf(logFile, "ERROR: \n");
		    fprintf(stderr,"In joining threads pthread_join() is %d\n", createReturn);
		    closeChatServer();
	    }
    }
	
	return 0;
}

int createsocket()
{
	int sockfd;
	if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0 )) < 0 )
	{
		fprintf(logFile, "ERROR: \n");
		perror( "Unable to open a socketIdentifier" );
		closeChatServer();
	}
	bzero( &useraddr, sizeof(useraddr));
	useraddr.sin_family = AF_INET;
	useraddr.sin_port = htons( randomPortGenerator() );
	useraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (SA *)&useraddr, sizeof(useraddr)) < 0) 
	{
		fprintf(logFile, "ERROR: \n");
		perror("Binding failed");
		closeChatServer();
	}
return sockfd;
}

void toSendAddr(char* IP,int port)
{
	bzero( &leaderaddr, sizeof(leaderaddr));
	leaderaddr.sin_family = AF_INET;
	leaderaddr.sin_port = htons(port);
	if( inet_pton( AF_INET, IP, &leaderaddr.sin_addr ) <= 0 )
	{
		fprintf(logFile, "ERROR: \n");
		perror( "Unable to convert address to inet_pton \n" );
		closeChatServer();
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

int broadCastMsg(int sock, int identifier,char *msg)
{
	char bufBroadCast[BUFSIZE];
	ArrayString arrayString[20];
	int i;
	if(identifier == 1) 	//connection establishment
	{
		struct sockaddr_in tempAddr;
		resetAllSeqNum();
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					fprintf(logFile, "ERROR: \n");
					perror( "Unable to convert address to inet_pton \n" );
					closeChatServer();
				}
				char bufIdentifier[BUFSIZE];
				strcpy(bufIdentifier,"Table");
				if (sendto(sock,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				chatUser[i].timerBroadcastCheck = 1;
				if (sendto(sock,chatUser, sizeof(chatUser), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				//printf("Table broadcasted\n");
			}
		}	
	}
	else if(identifier == 2) //normal messages
	{
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast, msg);
		generalisedStringTok(bufBroadCast, arrayString);
		strcpy(bufBroadCast, arrayString[2].String); //username
		strcat(bufBroadCast,":: ");
		strcat(bufBroadCast, arrayString[1].String);	//message
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		readyToSend = 1;
		strcpy(msg, "Message");
		strcat(msg, "~");
		strcat(msg, bufBroadCast);
		strcat(msg, "~");
		strcat(msg, arrayString[4].String);	//msg - Message:Nick:: (message):timestamper
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					fprintf(logFile, "ERROR: \n");
					perror( "Unable to convert address to inet_pton \n" );
					closeChatServer();
				}
				strcpy(bufIdentifier,"String");
				if (sendto(sock,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				char bufTemp[BUFSIZE];
				strcpy(bufTemp, msg);
				chatUser[i].timerMsgBroadcastCheck = 1;
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
			}
		}
	}
	else if(identifier == 3) //normal messages
	{
		char bufTemp[BUFSIZE];
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," left the chat or crashed");
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		strcat(bufTemp,"~");
		strcat(bufTemp,"0");
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					fprintf(logFile, "INFO: \n");
					perror( "Unable to convert address to inet_pton \n" );
					closeChatServer();
				}
				strcpy(bufIdentifier,"String");
				if (sendto(sock,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
			}
		}
	}
	else if(identifier == 4) //normal messages
	{
		char bufTemp[BUFSIZE];
		char portTemp[10];
		sprintf(portTemp,"%d",chatUser[findIndexOfUserName(msg)].Port);
		//chatUser[findIndexOfUserName(msg)].Port
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," has joined chat on ");
		strcat(bufBroadCast,chatUser[findIndexOfUserName(msg)].IP);
		strcat(bufBroadCast,":");
		strcat(bufBroadCast,portTemp);
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0) && (strcmp(chatUser[i].Username,msg) != 0))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					fprintf(logFile, "ERROR: \n");
					perror( "Unable to convert address to inet_pton \n" );
					closeChatServer();
				}
				strcpy(bufIdentifier,"String");
				if (sendto(sock,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
			}
		}
	}
	else if(identifier == 5) //normal messages
	{
		char bufTemp[BUFSIZE];
		struct sockaddr_in tempAddr;
		strcpy(bufBroadCast,"NOTICE ");
		strcat(bufBroadCast,msg);
		strcat(bufBroadCast," left the chat or crashed");
		printf("%s\n",bufBroadCast);	//Print at the broadcast end (self) (without timestamper)
		strcpy(bufTemp,"Notice");
		strcat(bufTemp,"~");
		strcat(bufTemp,bufBroadCast);
		strcat(bufTemp,"~");
		strcat(bufTemp,"1");
		char bufIdentifier[BUFSIZE];
		for(i=0;i<20;i++)
		{
			if(!(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0))
			{
				bzero( &tempAddr, sizeof(tempAddr));
				tempAddr.sin_family = AF_INET;
				tempAddr.sin_port = htons(chatUser[i].Port);
				if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
				{
					fprintf(logFile, "INFO: \n");
					perror( "Unable to convert address to inet_pton \n" );
					closeChatServer();
				}
				strcpy(bufIdentifier,"String");
				if (sendto(sock,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
				if (sendto(sock,bufTemp, sizeof(bufTemp), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
				{
					fprintf(logFile, "ERROR: \n");
					perror("Error in sendto from server to client");
					closeChatServer();
				}
			}
		}
	}
}

void clearTableEntry(ChatUserInfo *tableEntry,int rowNum)
{
	tableEntry[rowNum].ID = 0;
	tableEntry[rowNum].Port = 0;
	tableEntry[rowNum].isLeader = 0;
	tableEntry[rowNum].timerBroadcastCheck = 0;
	tableEntry[rowNum].isActive = 0;
	tableEntry[rowNum].receivedMsgSeqNo = 0;
	tableEntry[rowNum].timerMsgBroadcastCheck = 0;
	tableEntry[rowNum].timerBroadcastCheck = 0;
	strcpy(tableEntry[rowNum].Username,"");
	strcpy(tableEntry[rowNum].IP,"");
}

int isTableEntryEmpty(ChatUserInfo tableEntry)
{
	if(tableEntry.ID == 0 && tableEntry.Port == 0 && tableEntry.isLeader == 0 && tableEntry.isActive == 0 && strcmp(tableEntry.Username,"") == 0 && strcmp(tableEntry.IP,"") == 0 && tableEntry.receivedMsgSeqNo == 0 )
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
	}
}

void controllerLeader(char *buf, ArrayString *arrayString)
{
	char bufBroadCast[BUFSIZE];
	strcpy( bufBroadCast,buf);
	generalisedStringTok(buf,arrayString);
	int createReturn;
	if(strcmp(arrayString[0].String,"Add")==0)
	{
		if( checkIfUsernameExists(arrayString[1].String) == 1)
		{
			char bufIdentifier[BUFSIZE];
			strcpy(bufIdentifier,"String");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
			{
				fprintf(logFile, "ERROR: \n");
				perror("Error in sendto from server to client");
			}
			strcpy(bufIdentifier,"Error-2");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)	
			{
				fprintf(logFile, "ERROR: \n");
				perror("Error in sendto from server to client");
			}
		}
		else
		{
			if( iterator < 20 )
			{
				chatUser[iterator].ID = ID;																//Updating the table with new User
				strcpy(chatUser[iterator].Username,arrayString[1].String);
				chatUser[iterator].Port = ntohs(incomingAddr.sin_port);
				chatUser[iterator].isActive = 1;
				chatUser[iterator].isLeader = 0;
				chatUser[iterator].receivedMsgSeqNo = 0;
				chatUser[iterator].timerBroadcastCheck = 0;
				chatUser[iterator].timerMsgBroadcastCheck = 0;
				setTimeStamper();
				strcpy(chatUser[iterator].IP,inet_ntoa(incomingAddr.sin_addr));
				iterator++;
				ID--;
				broadCastMsg(socketIdentifier,4,arrayString[1].String);
				int check = broadCastMsg(socketIdentifier,1,buf);
				if(check == 0)
				{
					fprintf(logFile, "ERROR: \n");
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
				highPriority = 1;
			}
			else
			{
				if( headAvailableIDQueue != NULL && tailAvailableIDQueue != NULL)
				{
					broadCastMsg(socketIdentifier,4,arrayString[1].String);
					dequeue(&headAvailableIDQueue,&tailAvailableIDQueue);
					int index = atoi(dequeuedMsg);
					chatUser[index].ID = ( 20 - index );																//Updating the table with new User
					strcpy(chatUser[index].Username,arrayString[1].String);
					chatUser[index].Port = ntohs(incomingAddr.sin_port);
					chatUser[index].isActive = 1;
					chatUser[index].isLeader = 0;
					chatUser[index].receivedMsgSeqNo = 0;
					chatUser[index].timerBroadcastCheck = 0;
					chatUser[index].timerMsgBroadcastCheck = 0;
					setTimeStamper();
					strcpy(chatUser[index].IP,inet_ntoa(incomingAddr.sin_addr));
				}
				else
				{
					//20 users in group no more ca be added
					sendMessageForUserLimit();
				}
			}
		}		
	}
	else if(strcmp(arrayString[0].String,"Exists")==0)
	{
		updateMap(arrayString[2].String);
		//timer2 = 0;	//Ressting the timer when the leader sends it to himself
		//printf("Inside Exists string (%s) and incoming seq#:%d and table seq#:%d\n",bufBroadCast,(atoi(arrayString[3].String)),getSeqNoOfUser(arrayString[2].String));
		//printf("%s\n",bufBroadCast);
		sem_wait(&SendQ);
		sem_post(&SendQ);
		if(atoi(arrayString[3].String) <= getSeqNoOfUser(arrayString[2].String))	//message exists in queue, checking duplicacy
		{
			//printf("Inside Exists and in the less than case\n");
			//resendAck
		}
		else if((atoi(arrayString[3].String)) == (getSeqNoOfUser(arrayString[2].String) + 1))	//if there is a new message coming
		{
			char temp[BUFSIZE];
			updateUserMsgNo(arrayString[2].String);
			sem_wait(&msgSent);
			enqueue(bufBroadCast,&headLeaderQ, &tailLeaderQ);
			sem_post(&msgSent);
			//printQ(&headLeaderQ, &tailLeaderQ);
			//sendAck
		}
	}
	else if(strcmp(arrayString[0].String,"AckTable")==0)
	{
		//resends table to users from whom ackTable is not received
		//printf("AckTable ack received from user: %s\n",arrayString[1].String);
		resetTimerForGivenUser(arrayString[1].String,1);
		highPriority = 0;
		//printf("Timer stopped for user: %s\n",arrayString[1].String );
	}
	else if(strcmp(arrayString[0].String,"AckRecvdBroadcastMsg")==0)
	{
		//printf("AckRecvdBroadcastMsg ack received from user: %s\n",arrayString[1].String);
		resetTimerForGivenUser(arrayString[1].String,2);
	}
	else if(strcmp(arrayString[0].String,"Exiting")==0)
	{
		chatUser[findIndexOfUserName(arrayString[1].String)].isActive = 0;
		tableCleanUp();
		broadCastMsg(socketIdentifier,1,bufBroadCast);
		if(numOfChatUsers() == 1)
			highPriority = 0;
		else
			highPriority = 1;
		strcpy(bufBroadCast,arrayString[1].String);
		broadCastMsg(socketIdentifier,3,bufBroadCast);
	}
	else if(strcmp(arrayString[0].String,"Alert")==0)
	{
		struct sockaddr_in tempAddr;
		bzero( &tempAddr, sizeof(tempAddr));
		tempAddr.sin_family = AF_INET;
		tempAddr.sin_port = htons(atoi(arrayString[3].String));
		if( inet_pton( AF_INET, arrayString[2].String, &tempAddr.sin_addr ) <= 0 )
		{
			fprintf(logFile, "ERROR: \n");
			perror( "Unable to convert address to inet_pton \n" );
			closeChatServer();
		}
		if( checkIfUsernameExists(arrayString[1].String) == 1)
		{
			char bufIdentifier[BUFSIZE];
			strcpy(bufIdentifier,"String");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
			{
				fprintf(logFile, "ERROR: \n");
				perror("Error in sendto from server to client");
			}
			strcpy(bufIdentifier,"Error-2");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)	
			{
				perror("Error in sendto from server to client");
			}
		}
		else
		{
			if( iterator < 20 )
			{
				chatUser[iterator].ID = ID;																//Updating the table with new User
				strcpy(chatUser[iterator].Username,arrayString[1].String);
				chatUser[iterator].Port = atoi(arrayString[3].String);
				chatUser[iterator].isActive = 1;
				chatUser[iterator].isLeader = 0;
				chatUser[iterator].receivedMsgSeqNo = 0;
				chatUser[iterator].timerBroadcastCheck = 0;
				chatUser[iterator].timerMsgBroadcastCheck = 0;
				setTimeStamper();
				strcpy(chatUser[iterator].IP,arrayString[2].String);
				iterator++;
				ID--;
				broadCastMsg(socketIdentifier,4,arrayString[1].String);
				int check = broadCastMsg(socketIdentifier,1,buf);
				if(check == 0)
				{
					fprintf(logFile, "ERROR: \n");
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
				highPriority = 1;
			}
			else
			{
				if( headAvailableIDQueue != NULL && tailAvailableIDQueue != NULL)
				{
					broadCastMsg(socketIdentifier,4,arrayString[1].String);
					dequeue(&headAvailableIDQueue,&tailAvailableIDQueue);
					int index = atoi(dequeuedMsg);
					chatUser[index].ID = ( 20 - index );								//Updating the table with new User
					strcpy(chatUser[index].Username,arrayString[1].String);
					chatUser[index].Port = ntohs(incomingAddr.sin_port);
					chatUser[index].isActive = 1;
					chatUser[index].isLeader = 0;
					chatUser[index].receivedMsgSeqNo = 0;
					chatUser[index].timerBroadcastCheck = 0;
					chatUser[index].timerMsgBroadcastCheck = 0;
					setTimeStamper();
					strcpy(chatUser[index].IP,inet_ntoa(incomingAddr.sin_addr));
				}
				else
				{
					//20 users in group no more ca be added
					sendMessageForUserLimit();
				}
			}
		}
	}
}

void controllerNonLeader(char *buf, ArrayString *arrayString)
{
	char bufPrint[BUFSIZE];
	strcpy(bufPrint, buf);
	char tempMsg[BUFSIZE];
	
	generalisedStringTok(buf,arrayString);
	
	if(strcmp(arrayString[0].String,"Message")==0)
	{
		timer2 = 0;	//reset timer2 to 0 on receipt of broadcasted message
		sem_wait(&SendQ);
		sem_post(&SendQ);
		//printf("%s\n",bufPrint);
		controllerAcknowledgement("AckRecvdBroadcastMsg", nameOfUser);
		if( headBackupQ != NULL && tailBackupQ != NULL)
		{
			dequeue(&headBackupQ,&tailBackupQ);
			if( headBackupQ != NULL && tailBackupQ != NULL)
			{
				strcpy(tempMsg,peakQueue(&headBackupQ, &tailBackupQ));
				// strcpy(tempMsg, sequencer(tempMsg, &sequenceNumber));	//assigning sequenceNumber to messages from a particular user
				if( sendto(socketIdentifier, tempMsg, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
				{
				  perror( "Sending message to server failed" );
				}
			}
		}
		sem_wait(&RecvQ);
		updateHoldBackList(bufPrint, &headHoldBackList, &tailHoldBackList);
		sem_post(&RecvQ);
		readyToSend = 1;
	}
	else if(strcmp(arrayString[0].String,"BecomeLeader")==0)
	{
		char tempMsg[BUFSIZE];
		timer2 = 0;
		timer1 = 0;
		chatUser[findIndexOfUserName(arrayString[1].String)].isActive = 0;
		chatUser[findIndexOfUserName(nameOfUser)].isLeader = 1;
		iterator = atoi(arrayString[2].String);
		ID = atoi(arrayString[3].String);
		tableCleanUp();
		//printTable();
		updateLeaderAddress();
		int check = broadCastMsg(socketIdentifier,1,buf);
		if(check == 0)
		{
			fprintf(logFile, "ERROR: \n");
			fprintf(stderr, "BroadCast Unsuccessful\n");
		}
		strcpy(bufPrint,arrayString[1].String);
		broadCastMsg(socketIdentifier,5,bufPrint);
		while(headBackupQ != NULL && tailBackupQ != NULL)
		{

			strcpy(tempMsg,peakQueue(&headBackupQ, &tailBackupQ));
			// strcpy(tempMsg, sequencer(tempMsg, &sequenceNumber));	//assigning sequenceNumber to messages from a particular user
			if( sendto(socketIdentifier, tempMsg, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
			{
				fprintf(logFile, "ERROR: \n");
				perror( "Sending message to server failed" );
			}
			dequeue(&headBackupQ, &tailBackupQ);
		}
		readyToSend = 1;
		//sendtable
	}
	else if(strcmp(arrayString[0].String,"Error-1")==0)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stderr,"Maximum number of users in the group can be atmost 20\n");
		closeChatServer();
	}
	else if(strcmp(arrayString[0].String,"Error-2")==0)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stderr,"Username already exists\n");
		closeChatServer();
	}
	else if(strcmp(arrayString[0].String,"Notice")==0)
	{
		fprintf(logFile, "ERROR: \n");
		fprintf(stdout,"%s\n",arrayString[1].String);
		if(atoi(arrayString[2].String) == 1)
		{
			readyToSend = 1;
		}
	}
	else if(strcmp(arrayString[0].String,"Congestion")==0)
	{
		fprintf(logFile, "ERROR: \n");
		//printf("****************************************\n");
		slowDownMsg = 1;
	}
}

void *queueForMessaging(void *arg)
{
	char buf[500];
	char *temp;
	int k;
	//Inputting message sequence from stdin to place in sendq
	for(;;)
	{
		temp = fgets(buf, sizeof(buf),stdin);
		if( temp != NULL)
		{
			int i = strlen(buf) - 1;
			if( buf[i] == '\n' )
				buf[i] = '\0';
			if(strlen(buf) > 220)
			{
				fprintf(logFile, "ERROR: \n");
				fprintf(stderr, "Message length exceeded[220]\n");
			}
			else if( strlen(buf) != 0 )
			{
				//printf("%s\n",buf);
				for(k=0;k<strlen(buf);k++)
				{
					if(buf[k] == '~')
						buf[k] = ',';
				}
				sem_wait(&SendQ);
				enqueue(buf, &headSendQ, &tailSendQ);	//TODO:Use a mutex as this is used by multiple threads
				//printQ(&headSendQ, &tailSendQ);
				sem_post(&SendQ);
			}
		}
		else
		{
			break;
		}
	}
	//Case when the user is exiting the group
	if(amILeader(nameOfUser) == 1)
	{
		fprintf(logFile, "INFO: Leader has crashed\n");
		printf("Exiting the chat group\n",nameOfUser);
		if(numOfChatUsers() > 1)
		{
			conductLeaderElection();
		}
		closeChatServer();
	}
	else
	{
		//printQ(&headSendQ, &tailSendQ);
		fprintf(logFile, "INFO: \n");
		printf("Exiting the chat group\n");			//TODO: Updated table has to be sent to all the users, might be handled from somewhere else
		//notify the leader
		strcpy(buf,"Exiting");
		strcat(buf,"~");
		strcat(buf,nameOfUser);
		if( sendto(socketIdentifier, buf, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
		{
			fprintf(logFile, "ERROR: \n");
			perror( "Sending message to server failed" );
		}
		closeChatServer(); 
	}
	//Printing out the message out on the console by dequeuing the recvQ
	pthread_exit(NULL);
}

void *queueForPrinting(void *arg)
{
	ArrayString arrayString3[20];
	char buf[500];
	checkTimeStamper = 0;
	//Printing out the message out on the console by dequeuing the recvQ
	while(1)
	{
		sem_wait(&RecvQ);
		if(headHoldBackList != NULL, tailHoldBackList != NULL)
		{
			if(1)	//(checkTimeStamper + 1) == headHoldBackList->seqNum
			{
				//printf("i am in holdback list\n");
				char tempMsg[500];
				strcpy(tempMsg, deleteNode(tempMsg, &headHoldBackList, &tailHoldBackList));
				generalisedStringTok(tempMsg, arrayString3);
				printf("%s\n", arrayString3[1].String);
				checkTimeStamper++;
			}
			else
			{
				//printf("i am in holdback list\n");
			}
		}
		sem_post(&RecvQ);
	}
	pthread_exit(NULL);
}

void *threadForSending(void *Username)
{
	char tempMsg[500];
	char temp[500];
	while(1)
	{
		if(readyToSend == 1)
		{
			sem_wait(&SendQ);
			if(!(isEmpty(&headSendQ, &tailSendQ)))
			{
				//printQ(&headSendQ, &tailSendQ);
				//printQ(&headBackupQ, &tailBackupQ);
				strcpy(tempMsg, "Exists~");
				strcpy(temp,dequeueStandby(temp,&headSendQ,&tailSendQ));	//dequeue stores the msg into global array dequeuedMsg
				//printf("IN thread for sending\n");
				strcat(tempMsg, temp);
				strcat(tempMsg, "~");
				strcat(tempMsg, Username);
				strcpy(tempMsg, sequencer(tempMsg, &sequenceNumber));	//assigning sequenceNumber to messages from a particular user
				enqueue(tempMsg, &headBackupQ, &tailBackupQ);	//making connection reliable by waiting for ack and then removing from backUpQ once received
				//printf("%s\n",tempMsg);
				if( sendto(socketIdentifier, tempMsg, sizeof(tempMsg), 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )	//Exists~(Message)~Username~sequencer
				{
					fprintf(logFile, "ERROR: \n");
			  		perror( "Sending message to server failed" );
				}
				else	//start timer to sense loss of packets in a separate thread
				{
					timer2 = 1;
					readyToSend = 0;
					if(slowDownMsg == 1)
						sleep(1.5);
					else
						sleep(0.1);
				}
				//recv ack and remove from backUpQ
			}
			sem_post(&SendQ);
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

char* sequencer(char *unseqString, int *number)
{
	(*number)++;
	char numberOfMsgs[4];
	strcat(unseqString, "~");
	sprintf(numberOfMsgs, "%d", *number);
	strcat(unseqString, numberOfMsgs);
	return unseqString;
}

int getSeqNoOfUser(char *userName)
{
	int i = 0;
	char user[15];
	strcpy(user, userName);
	//printf("isnide get: %s\n", user);
	for(i= 0; i < 20; i++)
	{
		//printf("name: %s", chatUser[i].Username);
		if(strcmp(chatUser[i].Username, user) == 0)
			return chatUser[i].receivedMsgSeqNo;
	}
	//printf("here\n");
	return -1;
}

void updateUserMsgNo(char *nameOfUser)
{
	int j = 0;
	for(j = 0; j < 20; j++)
	{
		if(strcmp(chatUser[j].Username, nameOfUser) == 0)
		{
			(chatUser[j].receivedMsgSeqNo)++;
		}
	}
}

void updateTimeStamper()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(chatUser[i].isLeader == 1)
			checkTimeStamper = chatUser[i].updatedNewEntryTimeStamper;
	}
}

void setTimeStamper()
{
	int i;
	for(i=0;i<20;i++)
	{
		if(chatUser[i].isLeader == 1)
			chatUser[i].updatedNewEntryTimeStamper = timeStamper;
	}
}

void updateHoldBackList(char *msg, InsertionList **head, InsertionList **tail)
{
	//TO DO: optimization of if-else temp->next ==NULL
	char tempbuf[500];
	ArrayString arrayString[20];
	strcpy(tempbuf, msg);
	generalisedStringTok(tempbuf, arrayString);
	InsertionList *addNode = (InsertionList *)malloc(sizeof(InsertionList));
	strcpy(addNode->msg, msg);
	addNode->seqNum = atoi(arrayString[2].String); //Message~Sam:: Hi~timestamper
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
			if(addNode->seqNum < temp->seqNum)
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
			else if((addNode->seqNum > temp->seqNum) && (addNode->seqNum < (temp->next->seqNum)))
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
		strcpy(msgbuf, (*head)->msg);
		free(removeNode);
		*head = NULL;
		*tail = NULL;
	}
	else
	{
		removeNode = *head;
		*head = removeNode->next;
		strcpy(msgbuf, removeNode->msg);
		free(removeNode);
	}
	return msgbuf;
}

void controllerAcknowledgement(char *ack, char *user)
{
	//send Acknowledgement
	char acknowledgementType[500];
	char tempMsg[500];
	 
	if(strcmp(ack, "AckTable") == 0)
	{
		strcpy(acknowledgementType, "AckTable");
		strcat(acknowledgementType, "~");
		strcat(acknowledgementType, user);
		//printTable();
		//printf("AckTable ack sent : %s\n",acknowledgementType);
		if (sendto(socketIdentifier, acknowledgementType, sizeof(acknowledgementType), 0,(SA *)&leaderaddr, sizeof(leaderaddr)) < 0)
		{
			fprintf(logFile, "ERROR: \n");
			perror("Error in sendto from server to client");
		}
		if( headBackupQ != NULL && tailBackupQ != NULL)
		{
			strcpy(tempMsg,peakQueue(&headBackupQ, &tailBackupQ));
			// strcpy(tempMsg, sequencer(tempMsg, &sequenceNumber));	//assigning sequenceNumber to messages from a particular user
			if( sendto(socketIdentifier, tempMsg, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
			{
				fprintf(logFile, "ERROR: \n");
				perror( "Sending message to server failed" );
			}
		}
	}
	else if(strcmp(ack, "AckRecvdBroadcastMsg") == 0)
	{
		//printf("sending AckRecvdBroadcastMsg\n");
		strcpy(acknowledgementType, "AckRecvdBroadcastMsg");
		strcat(acknowledgementType, "~");
		strcat(acknowledgementType, user);
		if (sendto(socketIdentifier,acknowledgementType, sizeof(acknowledgementType), 0,(SA *)&leaderaddr, sizeof(leaderaddr)) < 0)
		{
			fprintf(logFile, "ERROR: \n");
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
		if(amILeader(nameOfUser) == 1)
		{
			for(i=0,retry=0;i<20;i++)
			{
				if(chatUser[i].timerBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])) && (amILeader(chatUser[i].Username) == 0))
				{
					start = time (NULL);	//timer started
					//printf("Timer:%s has started\n",chatUser[i].Username);
					while(chatUser[i].timerBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])))	//still if timer on	; waiting for timer1 to get 0 when ack is received
					{
						diff = (time (NULL) - start);
						if(diff > 5)	//no response from leader for 60 secs
						{
							if(retry < 3)	//try one more time
							{
								printf("Retry#(Table-Leader):%d for %s\n",retry,chatUser[i].Username);
								bzero( &tempAddr, sizeof(tempAddr));
								tempAddr.sin_family = AF_INET;
								tempAddr.sin_port = htons(chatUser[i].Port);
								if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
								{
									fprintf(logFile, "ERROR: \n");
									perror( "Unable to convert address to inet_pton \n" );
									closeChatServer();
								}
								strcpy(bufIdentifier,"Table");
								if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									fprintf(logFile, "ERROR: \n");
									perror("Error in sendto from server to client");
									closeChatServer();
								}
								if (sendto(socketIdentifier,chatUser, sizeof(chatUser), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									fprintf(logFile, "ERROR: \n");
									perror("Error in sendto from server to client");
									closeChatServer();
								}
								//printf("60 seconds elapsed, send message again\n");
								retry++;
								start = time (NULL);	//restart timer
								break;
							}
							else
							{
								chatUser[i].isActive = 0;
								tableCleanUp();
								broadCastMsg(socketIdentifier,3,chatUser[i].Username);
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
}

void *threadForTableReceiveCallback(void *bufMsg)
{
	time_t start;
	time_t diff;
	int retry = 0;
	char tempMsg[500];
	while(1)
	{
		if(amILeader(nameOfUser) == 0)
		{
			if(timer1 == 1)
			{
				//printf("Timer-1 has started\n");
				start = time (NULL);	//timer started
				sem_wait(&S3);
				while(timer1 == 1)	//still if timer on	; waiting for timer1 to get 0 when ack is received
				{
					diff = (time (NULL) - start);
					if(diff > 5)	//no response from leader for 60 secs
					{
						if(retry < 3)	//try one more time
						{
							printf("Retry#(Table-User) : %d\n",retry);
							if( sendto(socketIdentifier, bufMsg, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
							{
								fprintf(logFile, "INFO: \n");
								perror( "Sending message to server failed" );
							}
							//printf("60 seconds elapsed, send message again\n");
							retry++;
							start = time (NULL);	//restart timer
							break;
						}
						else
						{
							//conduct leader election if it is not the first time
							if(isTableEmpty() > 1)
								conductLeaderElection();
							else
							{
								printf("Sorry no chat is active on %s:%d, try again later\n",IP,port);
								closeChatServer();
							}
							//printf("Leader dead-table\n");
							timer1 = 0;
							retry = 0;
							break;
						}
					}
				}
			}
			sem_post(&S3);
			//timer1 to be reset on success on receiving acknowledge
		}
	}
}

void* threadForMessageSentCallback(void *arg)
{
	time_t start;
	time_t diff;
	char tempMsg[500];
	int retry = 0;
	while(1)
	{
		if(amILeader(nameOfUser) == 0)
		{
			if(timer2 == 1)
			{
				start = time (NULL);	//timer started
				while(timer2 == 1)	//still if timer on	; waiting for timer1 to get 0 when ack is received
				{
					diff = (time (NULL) - start);
					if(diff > 5)	//no response from leader for 60 secs
					{
						if(retry < 3)	//try one more time
						{
							printf("Retry#(Msg-User) : %d\n",retry);
							if( headBackupQ != NULL && tailBackupQ != NULL)
							{
								strcpy(tempMsg,peakQueue(&headBackupQ, &tailBackupQ));
								// strcpy(tempMsg, sequencer(tempMsg, &sequenceNumber));
							}
							if( sendto(socketIdentifier, tempMsg, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
							{
								fprintf(logFile, "ERROR: \n");
								perror( "Sending message to server failed" );
							}
							//printf("60 seconds elapsed, send message again\n");
							retry++;
							start = time (NULL);	//restart timer
							break;
						}
						else
						{
							//conduct leader election if it is not the first time
							if(isTableEmpty() > 1)
								conductLeaderElection();
							else
							{
								printf("Sorry no chat is active on %s:%d, try again later\n",IP,port);
								closeChatServer();
							}
							//printf("Leader dead-Msg\n");
							timer2 = 0;
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
		if(amILeader(nameOfUser) == 1)
		{
			for(i=0;i<20;i++)
			{
				if(chatUser[i].timerMsgBroadcastCheck == 1 && (amILeader(chatUser[i].Username) == 0) && !(isTableEntryEmpty(chatUser[i])))
				{
					start = time (NULL);	//timer started
					while(chatUser[i].timerMsgBroadcastCheck == 1 && !(isTableEntryEmpty(chatUser[i])))	//still if timer on	; waiting for timer1 to get 0 when ack is received
					{
						diff = (time (NULL) - start);
						if(diff > 3)	//no response from leader for 60 secs
						{
							if(retry < 3)	//try one more time
							{
								printf("Retry#(Msg-Leader) : %d\n",retry);
								bzero( &tempAddr, sizeof(tempAddr));
								tempAddr.sin_family = AF_INET;
								tempAddr.sin_port = htons(chatUser[i].Port);
								if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
								{
									fprintf(logFile, "ERROR: \n");
									perror( "Unable to convert address to inet_pton \n" );
									closeChatServer();
								}
								strcpy(bufIdentifier,"String");
								if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									fprintf(logFile, "ERROR: \n");
									perror("Error in sendto from server to client");
									closeChatServer();
								}
								if( headGlobalSendQ != NULL && tailGlobalSendQ != NULL)
									strcpy(bufIdentifier,peakQueue(&headGlobalSendQ, &tailGlobalSendQ));
								if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									fprintf(logFile, "ERROR: \n");
									perror("Error in sendto from server to client");
									closeChatServer();
								}
								//printf("60 seconds elapsed, send message again\n");
								retry++;
								start = time (NULL);	//restart timer
								break;
							}
							else
							{
								chatUser[i].isActive = 0;
								tableCleanUp();
								broadCastMsg(socketIdentifier,3,chatUser[i].Username);
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
}

char* peakQueue(QueueNode **head, QueueNode**tail)
{
	return (*head)->content;
}

void* threadClearGlobalQ(void *arg)
{
	int i,flag=0;
	while(1)
	{
		sleep(5000);
		for(i=0;i<20;i++)
		{
			if(chatUser[i].timerMsgBroadcastCheck == 0)
				flag = 1;
			else
			{
				flag = 0;
				break;
			}
		}
		if(flag == 1)
		{
			dequeue(&headGlobalSendQ,&tailGlobalSendQ);
			flag = 0;
		}
	}
}

int amILeader(char *userName)
{
	int i;
	int leadership;
	for(i=0;i<20;i++)
	{
		if(strcmp(chatUser[i].Username,userName) == 0)
		{
			leadership = chatUser[i].isLeader;
			break;
		}
		leadership = -1;
	}
	return leadership;
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

void conductLeaderElection()
{
	//printf("Inside conduct leader election\n");
	int k, maxId, maxIDIndex, flag = 0;
	struct sockaddr_in tempAddr;
	char crashedUser[15];
	for(k = 0; k < 20; k++)
	{
		if(chatUser[k].isActive == 1 && !(isTableEntryEmpty(chatUser[k])))
		{
			if( flag == 0)
			{
				maxId = chatUser[k].ID;
				maxIDIndex = k;
				flag = 1;
			}
			else
			{
				if(chatUser[k].ID > maxId)
				{
					maxId = chatUser[k].ID;
					maxIDIndex = k;
				}
			}
		}
	}
	strcpy(crashedUser,chatUser[maxIDIndex].Username);
	int max2Id, max2IDIndex;
	for(k = 0,flag=0; k < 20; k++)
	{
		if(chatUser[k].isActive == 1 && !(isTableEntryEmpty(chatUser[k])))
		{
			if( flag == 0 && k != maxIDIndex)
			{
				max2Id = chatUser[k].ID;
				max2IDIndex = k;
				flag = 1;
			}
			else
			{
				if(chatUser[k].ID > max2Id && k != maxIDIndex)
				{
					max2Id = chatUser[k].ID;
					max2IDIndex = k;
				}
			}
		}
	}
	//printf("Next Leader:%s,IP:%s,Port:%d\n",chatUser[max2IDIndex].Username,chatUser[max2IDIndex].IP,chatUser[max2IDIndex].Port);
	
	bzero( &tempAddr, sizeof(tempAddr));
	tempAddr.sin_family = AF_INET;
	tempAddr.sin_port = htons(chatUser[max2IDIndex].Port);
	if( inet_pton( AF_INET, chatUser[max2IDIndex].IP, &tempAddr.sin_addr ) <= 0 )
	{
		fprintf(logFile, "ERROR: \n");
		perror( "Unable to convert address to inet_pton \n" );
		closeChatServer();
	}
	char bufIdentifier[BUFSIZE];
	strcpy(bufIdentifier,"String");
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
	{
		fprintf(logFile, "ERROR: \n");
		perror("Error in sendto from server to client");
	}
	strcpy(bufIdentifier, "BecomeLeader");
	strcat(bufIdentifier, "~");
	strcat(bufIdentifier, crashedUser);
	strcat(bufIdentifier,"~");
	char tempIterator[5];
	sprintf(tempIterator,"%d",iterator);	//BecomeLeader~Username~Iterator~ID
	strcat(bufIdentifier,tempIterator);
	strcat(bufIdentifier,"~");
	sprintf(tempIterator,"%d",ID);
	strcat(bufIdentifier,tempIterator);
	//printf("%s\n",bufIdentifier);
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
	{
		fprintf(logFile, "ERROR: \n");
		perror("Error in sendto from server to client");
	}
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

void tableCleanUp()
{
	int i;
	char tempIndex[500];
	for(i = 0; i < 20; i++)
	{
		if(!(isTableEntryEmpty(chatUser[i])))
		{
			if(chatUser[i].isActive == 0)
			{
				clearTableEntry(chatUser, i);
				//enqueue
				sprintf(tempIndex, "%d", i);
				enqueue(tempIndex, &headAvailableIDQueue, &tailAvailableIDQueue);
			}
		}
	}
}

void sendMessageForUserLimit()
{
	char bufIdentifier[BUFSIZE];
	strcpy(bufIdentifier,"String");
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
	{
		fprintf(logFile, "ERROR: \n");
		perror("Error in sendto from server to client");
		closeChatServer();
	}
	strcpy(bufIdentifier,"Error-1");
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&incomingAddr, sizeof(incomingAddr)) < 0)
	{
		fprintf(logFile, "ERROR: \n");
		perror("Error in sendto from server to client");
		closeChatServer();
	}
}

void updateLeaderAddress()
{
	int i,tempPort;
	char tempIP[20];

	for(i=0;i<20;i++)
	{
		if( chatUser[i].isLeader == 1)
		{
			strcpy(tempIP, chatUser[i].IP);
			tempPort = chatUser[i].Port;
		}
	}
	//printf("%s,%d\n",tempIP,tempPort);
	toSendAddr(tempIP,tempPort);
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

void printTable()
{
	int i;
	for(i=0;i<20;i++)
	{
		if( !(isTableEntryEmpty(chatUser[i])))
		{
			printf("Username:%s,IP:%s,Port#:%d,isActive:%d,isLeader:%d\n",chatUser[i].Username,chatUser[i].IP,chatUser[i].Port,chatUser[i].isActive,chatUser[i].isLeader);
		}
	}
}

void resetAllSeqNum()
{
	sequenceNumber = 0;
	checkTimeStamper = 0;
	timeStamper = 0;
	int i;
	for(i=0;i<20;i++)
	{
		chatUser[i].receivedMsgSeqNo = 0;
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

int isTableEmpty()
{
	int i,count=0;
	for(i=0;i<20;i++)
	{
		if((isTableEntryEmpty(chatUser[i])) == 0)
		{
			count++;
		}
	}
	return count;
}

void *threadForTrafficControl(void *arg)
{
	FILE *fp;
	char recQ[10];
	while(1)
	{
		if(amILeader(nameOfUser) == 1)
		{
			if(strcmp(arg,"") != 0)
			{

				fp = popen(arg , "r");	//netstat -au | grep '<Port#>' | awk '{ print $2}'
				while (fgets(recQ,10, fp) != NULL)
				{
				}
				if(atoi(recQ) > CONGESTIONTHRESHOLD)
				{
						fprintf(stdout, "NOTICE: Congestion occurred at the leader\n");
						flagCongestion = 1;
				}
				//printMap();
				sleep(5);		
			}
		}
	}
}

char *congestionCommandConstruct(char *command,int port)
{
	char tempPort[10];
	strcpy(command,"netstat -au | grep '");
	sprintf(tempPort,"%d",port);
	strcat(command,tempPort);
	strcat(command,"' | awk '{ print $2}'");
	return command;
}

void *threadForMapRefresh(void *arg)
{
	int i;
	time_t start;
	time_t diff;
	while(1)
	{
		if((amILeader(nameOfUser) == 1))	//(flagCongestion == 1) && 
		{
			start = time (NULL);
			while((time (NULL) - start) < 2)
			{

			}
			//printMap();
			for (i = 0; i < 20; i++)
			{
				if(strcmp(map[i].User,"") != 0)
				{
					sem_wait(&Congestion);
					map[i].totalMsgs = 0;
					sem_post(&Congestion);
				}
					
			}
		}
	}
}

void *threadForMsgCount(void *arg)
{
	struct sockaddr_in tempAddr;
	char bufIdentifier[BUFSIZE];
	int i;
	while(1)
	{
		if((amILeader(nameOfUser) == 1))	//(flagCongestion == 1) && 
		{
			for (i = 0; (i < 20) ; i++)
			{
				if(strcmp(map[i].User,"") != 0)
				{
					sem_wait(&Congestion);
					if( map[i].totalMsgs > 100 )
					{
						//printf("Msg Sent\n");
						bzero( &tempAddr, sizeof(tempAddr));
						tempAddr.sin_family = AF_INET;
						tempAddr.sin_port = htons(chatUser[findIndexOfUserName(map[i].User)].Port);
						if( inet_pton( AF_INET, chatUser[findIndexOfUserName(map[i].User)].IP, &tempAddr.sin_addr ) <= 0 )
						{
							perror( "Unable to convert address to inet_pton \n" );
							exit( 99 );
						}
						strcpy(bufIdentifier,"String");
						if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
						{
							perror("Error in sendto from server to client");
							exit( 99 );
						}
						strcpy(bufIdentifier,"Congestion");
						if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
						{
							perror("Error in sendto from server to client");
							exit( 99 );
						}
						map[i].totalMsgs = 0;
					}
					sem_post(&Congestion);
				}
			}
		}
	}
}

void updateMap(char *userName)
{
	int i;
	for(i=0;i<20;i++)
	{
		if((strcmp(map[i].User,"") == 0) && (map[i].totalMsgs == 0))
		{
			strcpy(map[i].User,userName);
			(map[i].totalMsgs)++;
			// printf("%d\n",i);
			break;
		}
		else if(strcmp(map[i].User,userName) == 0)
		{
			(map[i].totalMsgs)++;
			break;
		}
	}
}

void printMap()
{
	int i;
	for(i=0;i<20;i++)
	{
		if((strcmp(map[i].User,"") != 0))
		{
			printf("%s-%d\n",map[i].User,map[i].totalMsgs );
		}
	}
}

void initialiseMap()
{
	int i;
	for(i=0;i<20;i++)
	{
		strcpy(map[i].User,"");
		map[i].totalMsgs = 0;
	}
}

void closeChatServer()
{
	int createReturn,i;
	sem_destroy(&S1);
    sem_destroy(&S2);
    sem_destroy(&S3);
    sem_destroy(&SendQ);
    sem_destroy(&RecvQ);
    sem_destroy(&msgSent);
    sem_destroy(&Congestion);
    fclose(logFile);
	for(i=0; i<THREADNUMBER; i++) 
	{
	    createReturn = pthread_kill(threadMessaging[i], SIGTERM);
	    if (createReturn) 
	    {
		    fprintf(stderr,"In ending threads pthread_kill() is %d\n", createReturn);
		    exit(-1);
	    }
    }
    exit(0);
}

void printUsers()
{
	int i;
	printf("Succeeded, Current Users:\n");
	for(i=0;i<20;i++)
	{
		if(isTableEntryEmpty(chatUser[i]) == 0)
		{
			if(chatUser[i].isLeader == 1)
			{
				printf("%s %s:%d (Leader)\n",chatUser[i].Username,chatUser[i].IP,chatUser[i].Port);
			}
			else
			{
				printf("%s %s:%d\n",chatUser[i].Username,chatUser[i].IP,chatUser[i].Port);
			}
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
		}
	}
}

void printQ(QueueNode **head, QueueNode **tail)
{
	QueueNode *temp;
	temp = *head;
	if(*head == NULL && *tail == NULL)
	{
		printf("Queue is empty\n");
	}
	else
	{
		while(temp != NULL)
		{
			printf("%s - ", temp->content);
			temp = temp->next;
		}
	}
	
}

char *dequeueStandby(char *storage,QueueNode **head, QueueNode **tail)
{
	QueueNode *temp;
	
	if( *head == *tail )
	{
		temp = *head;
		strcpy(storage,temp -> content);
		*head = NULL;
		temp -> next = NULL;
		*tail = NULL;
		free(temp);
	}
	else
	{
		temp = *head;
		strcpy(storage,temp -> content);
		temp = temp -> next;
		free(*head);
		*head = temp;
	}
	return storage;	
}

void *threadForLeaderBrdCast(void *arg)
{
	char temp[BUFSIZE];
	while(1)
	{
		if(amILeader(nameOfUser) == 1)
		{
			while(headLeaderQ != NULL && tailLeaderQ != NULL && highPriority == 0 && readyToBroadCast() == 1)		//&& readyToBroadCast() == 1
			{
				sem_wait(&msgSent);
				strcpy(temp,dequeueStandby(temp,&headLeaderQ, &tailLeaderQ));
				strcpy(temp, sequencer(temp, &timeStamper));	//assigning timestamper in the order in which leader receives the messages from 	
				
				enqueue(temp, &headGlobalSendQ, &tailGlobalSendQ);	// Note: While sending remove the timestamper from the msg
				//printf("In threadForLeaderBrdCast %s\n",temp);
				int check = broadCastMsg(socketIdentifier,2,temp);
				if(check == 0)
				{
					fprintf(logFile, "ERROR: \n");
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
				sem_post(&msgSent);
			}
		}
	}
}

int readyToBroadCast()
{
	int i;
	for (i = 0; i < 20; i++)
	{
		if(numOfChatUsers() == 1)
		{
			return 1;
		}
		else
		{
			if(!(isTableEntryEmpty(chatUser[i])))
			{
				if(chatUser[i].timerMsgBroadcastCheck == 1)
				{
					return 0;
				}
			}
		}
	}
	return 1;
}