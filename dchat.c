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