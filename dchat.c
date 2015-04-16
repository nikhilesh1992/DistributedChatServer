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