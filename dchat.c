	if(strlen(argv[1]) > 15 )
	{
		fprintf(stderr, "Username length cannot exceed 15 alphabets\n");
		exit(0);
	}
	strcpy(nameOfUser, argv[1]);
	if(argc == 2)
	{
		char ifconfig[20];
		socketIdentifier = createsocket();
		printf("New connection %d , ip is : %s , port : %d \n" , socketIdentifier , getIP(ifconfig) , ntohs(useraddr.sin_port));
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
		char temp[30];
		strcpy(temp,argv[2]);
		ipAddPortParsing(temp);
		socketIdentifier = createsocket();
		toSendAddr(IP,port);
		strcpy(buf,"Add");
		strcat(buf,"~");
		strcat(buf,argv[1]);
		printf("%s joining a new chat on %s, listening on %s:%d\n",nameOfUser,argv[2],getIP(ifconfig),ntohs(useraddr.sin_port));
		if( sendto(socketIdentifier, buf, BUFSIZE, 0, (SA *)&leaderaddr, sizeof(leaderaddr)) < 0 )
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
		if(amILeader(nameOfUser) == 1)
		{
			len = sizeof( incomingAddr ); 
			if ( recvlen=recvfrom(socketIdentifier, buf, BUFSIZE, 0,(SA *)&incomingAddr, &len) < 0)	//receive message from user
			{        
				perror( "Error in recvfrom on Server socketIdentifier");
				exit(1);
			}
			
			controllerLeader(buf, arrayString);
			
		}
		
		else if(amILeader(nameOfUser) == 0)
		{
			if ( recvlen=recvfrom(socketIdentifier, buf, sizeof(buf),0,(SA *)&incomingAddr,&len) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
			{        
				perror( "Error in recvfrom on Server socketIdentifier");
				exit(1);
			}
			strcpy(tempbuf,buf);
			generalisedStringTok(tempbuf,arrayS);
			if(strcmp(buf,"Table") == 0)
			{
				if ( recvlen=recvfrom(socketIdentifier, bufRecvdTable, sizeof(bufRecvdTable), 0,0,0) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
				{        
					perror( "Error in recvfrom on Server socketIdentifier");
					exit(1);
				}
				else
				{
					//acknowledgement in the form of table received, reset timer1
					timer1 = 0;
					copyUserDatabaseTable(bufRecvdTable);
					updateLeaderAddress();
					updateTimeStamper();
					resetAllSeqNum();
					controllerAcknowledgement("AckTable", nameOfUser);
				}
			}
			else if(strcmp(buf,"String") == 0)
			{
				//receive message with timestamper
				if ( recvlen = recvfrom(socketIdentifier, buf, sizeof(buf), 0,0,0) < 0)	// The 0,0 can be used to check if the leader IP address has changed over the time
				{        
					perror( "Error in recvfrom on Server socketIdentifier");
					exit(1);
				}
				controllerNonLeader(buf, arrayString2);
			}
			else if(strcmp(arrayS[0].String,"Add") == 0)
			{
				//receive message with timestamper
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
					perror("Error in sendto from server to client");
					return 0;
				}
			}

		}
	}

	pthread_attr_destroy(&attribute);	// Joining the internal threads with the main thread so that the main terminates only after the internal threads have completed
	for(i=0; i<THREADNUMBER; i++) 
	{
	    createReturn = pthread_join(threadMessaging[i], &state);
	    if (createReturn) 
	    {
		    fprintf(stderr,"In joining threads pthread_join() is %d\n", createReturn);
		    exit(-1);
	    }
    }

    sem_destroy(&S1);

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
	bzero( &leaderaddr, sizeof(leaderaddr));
	leaderaddr.sin_family = AF_INET;
	leaderaddr.sin_port = htons(port);
	if( inet_pton( AF_INET, IP, &leaderaddr.sin_addr ) <= 0 )
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
				perror("Error in sendto from server to client");
			}
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
				broadCastMsg(socketIdentifier,4,arrayString[1].String);
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
				int check = broadCastMsg(socketIdentifier,1,buf);
				if(check == 0)
				{
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
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
		if(atoi(arrayString[3].String) <= getSeqNoOfUser(arrayString[2].String))	//message exists in queue, checking duplicacy
		{
			//printf("Inside Exists and in the less than case\n");
			//resendAck
		}
		else if((atoi(arrayString[3].String)) == (getSeqNoOfUser(arrayString[2].String) + 1))	//if there is a new message coming
		{
			updateUserMsgNo(arrayString[2].String);
			strcpy(bufBroadCast, sequencer(bufBroadCast, &timeStamper));	//assigning timestamper in the order in which leader receives the messages from 	
			// different users
			//store in globalQueue
			enqueue(bufBroadCast, &headGlobalSendQ, &tailGlobalSendQ);	// Note: While sending remove the timestamper from the msg
			int check = broadCastMsg(socketIdentifier,2,bufBroadCast);
			if(check == 0)
			{
				fprintf(stderr, "BroadCast Unsuccessful\n");
			}
			//sendAck
		}
	}
	else if(strcmp(arrayString[0].String,"AckTable")==0)
	{
		//resends table to users from whom ackTable is not received
		//printf("AckTable ack received from user: %s\n",arrayString[1].String);
		resetTimerForGivenUser(arrayString[1].String,1);
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
			perror( "Unable to convert address to inet_pton \n" );
			exit( 99 );
		}
		if( checkIfUsernameExists(arrayString[1].String) == 1)
		{
			char bufIdentifier[BUFSIZE];
			strcpy(bufIdentifier,"String");
			if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
			{
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
				broadCastMsg(socketIdentifier,4,arrayString[1].String);
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
				int check = broadCastMsg(socketIdentifier,1,buf);
				if(check == 0)
				{
					fprintf(stderr, "BroadCast Unsuccessful\n");
				}
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
								printf("Retry#(Table-Leader) : %d\n",retry);
								bzero( &tempAddr, sizeof(tempAddr));
								tempAddr.sin_family = AF_INET;
								tempAddr.sin_port = htons(chatUser[i].Port);
								if( inet_pton( AF_INET, chatUser[i].IP, &tempAddr.sin_addr ) <= 0 )
								{
									perror( "Unable to convert address to inet_pton \n" );
									exit( 99 );
								}
								strcpy(bufIdentifier,"Table");
								if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
								{
									perror("Error in sendto from server to client");
									return 0;
								}
								if (sendto(socketIdentifier,chatUser, sizeof(chatUser), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
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
								broadCastMsg(socketIdentifier,3,chatUser[i].Username);
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
								printf("User:%s timed out\n",nameOfUser);
								exit(0);
							}
							//printf("Leader dead-table\n");
							timer1 = 0;
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
		perror( "Unable to convert address to inet_pton \n" );
		exit( 99 );
	}
	char bufIdentifier[BUFSIZE];
	strcpy(bufIdentifier,"String");
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
	{
		perror("Error in sendto from server to client");
	}
	strcpy(bufIdentifier, "BecomeLeader");
	strcat(bufIdentifier, "~");
	strcat(bufIdentifier, crashedUser);
	//printf("%s\n",bufIdentifier);
	if (sendto(socketIdentifier,bufIdentifier, sizeof(bufIdentifier), 0,(SA *)&tempAddr, sizeof(tempAddr)) < 0)
	{
		perror("Error in sendto from server to client");
	}
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