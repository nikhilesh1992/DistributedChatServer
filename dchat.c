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