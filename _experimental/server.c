#include "server.h"


void doServer()
/*--------------------------------------------
 * gestisce le connessioni dei clients
 * -----------------------------------------*/
{
	int sockfd, new_fd,nbytes;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr; /* address information del chiamante */
	int sin_size;	
	unsigned char buf[MAXDATASIZE];
	int out;
	int newpid;
	const int SHM_INT_SIZE = sizeof(int);
	int i;
	int true=1;

	for(i=0;i<MAXCONNECTIONS;i++)
		serverPids[i]=-1;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		my_perror("socket");
		exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &true, sizeof(true)) == -1)
		my_perror("setsockopt");
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
	while(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		my_perror("bind");
		sleep(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		my_perror("listen");
		exit(1);
	}
	my_printf("Listen on Port : %d\n",MYPORT);

	while(1)
	{
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			my_perror("accept");
			continue;
		}
		
		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
				serverPids[i]=-1;
						
		i=0;
		while((i<MAXCONNECTIONS)&&(serverPids[i]!=-1))
			i++;
		if(i<MAXCONNECTIONS)
		{
//			my_printf("i=%d\n",i);
//			my_printf("Connect!! %s is on the Server!\n",inet_ntoa(their_addr.sin_addr));
			if ((newpid=fork())==0)
			{
				do
				{
					out=0;
					// Processo figlio
					if ((nbytes=recv(new_fd,buf,MAXDATASIZE,0)) == -1)
						my_perror("recv");
					if(nbytes==0)
					{
						close(new_fd);
						exit(0);
					}
					buf[nbytes]='\0';

					if((*reloadCommand)==0)
						performAction(new_fd,buf);
					out=(strstr(buf,"quit")!=NULL);
				}
				while(out==0);
				close(new_fd);
				exit(0);
			}
//			my_printf("connessione: %d\n",newpid);
			serverPids[i]=newpid; //padre
			close(new_fd);
		}
		else
		{
			my_printf("too many connections\n");
			close(new_fd);
		}
	}
}


void performAction(int fd,char *buf)
/*--------------------------------------------
 * esegue operazione richiesta dal client (web o telnet)
 * int fd 	 - descrittore del socket
 * char *buf - stringa con il comando
 * attualmente implementate:

 * 	checkBuonGiorno
 * 	checkBuonaNotte 
 * 	presenze_attiva 
 * 	presenze_disattiva
 * 	irrigazione_attiva id_circuito
 * 	irrigazione_disattiva id_circuito
 * 	bg_attiva
 * 	bn_attiva
 * 	bg_disattiva	
 * 	bn_disattiva
 * 	get
 * 		channels	(restituisce i descrittori dei canali)
 * 		values		(restituisce i valori dei canali)
 * 		latest_values	(restituisce gli ultimi valori eng dei canali)
 * 		outchannels
 * 		outputs
 * 		mm_names
 * 		mm_values
 * 		mm_values_rt
 * 		knx_values
 * 		bacnet_values
 * 		value adId channel_n
 * 		panel panel_id
 * 		presenze_attivo
 * 		irrigazione_attivo irrigazione_n
 * 		buongiorno_attivo
 * 		buonanotte_attivo
 *		ai_attivo
 * 		ai_attivo_ss id_sistema
 * 		irrigazione_rt irrigazione_n (real time table)
 * 		irrigazione irrigazione_n (mysql tabel)
 * 		pioggia
 * 		effemeridi
 * 		sottostato inizio_estate inizio_inverno (restituisce antiintrusione,pioggia,giorno,estate)
 * 	set	
 * 		knx_value id_knx value
 * 		output system_n channel_n value 
 * 		output ch_id value
 * 		irrigazione id_irrigazione circuito ora_start ripetitivita tempo_off durata validita
 * 		toggle system_n channel_n
 * 		reset system_n (sends reset signal)
 * 		status
 * 	reload	(spegne e riaccende tutti i sistemi)
 * 		system	(ricarica tabella system)
 * 		analog	(ricarica tabella analog)
 * 		digital	(ricarica tabella digital)
 * 		out		(ricarica tabella digitalOut)
 * 		presenze	(ricarica scenari_presenze)
 * 		mm_reading	(ricarica mm_reading)
 * 		bgbn 	(ricarica scenari bgbn)
 * 	dig system_n channel_n	(restituisce corrispondente canale digitale)
 * 	ana system_n channel_n	(restituisce corrispondente canale analogico)
 * 	out system_n channel_n	(restituisce corrispondente uscita digitale)
 * 	update
 * 		id type device_num ch_num
 * 		analog channel_id
 * 		digital channel_id
 * 		out channel_id
 * 		bg
 * 		bn
 * 		presenze
 * 		irrigazione
 * 	system
 * 		update system_n		(ricarica scheda system_n)
 * 		disable system_n	(disabilita scheda system_n)
 * 		enable system_n		(abilita scheda system_n)
 * 	chn
 * 		disable system_n adId channel_n	(disabilita canale channel_n tipo adId su scheda system_n)
 * 		enable system_n adId channel_n	(abilita canale channel_n tipo adId su scheda system_n)
 *-----------------------------------------*/
{
	char buffer[4096];
	char tempstring[80];
	float tempfloat;
	int value;
	int i,j,k,l;
	int state;
	MYSQL *conn=NULL;
	int systemId;
	int chnId;
	int adId;
	char query[255];
	char *c,*d;
	struct timeval tim;
	double t1;
	int rem_time;
	int *lock;
	int po_delay_rem;
	int pon_time_rem;
	int on_time_rem;
	int poff_time_rem;

	if(strstr(buf,"checkBuonGiorno")==buf)
	{
		*buonGiornoAttivo=checkBuonGiornoAttivo();
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"checkBuonaNotte")==buf)
	{
		*buonaNotteAttivo=checkBuonaNotteAttivo();
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"presenze_attiva")==buf)
	{
		*scenariPresenzeAttivo=1;
		sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"irrigazione_attiva ")==buf)
	{
		i=atoi(&buf[19]);
		j=id_irrigazioneToId(i);

		if(j>=0)
		{
			if(circuiti_attivi(j)>0)
			{
				irrigazioneTable[j].attivo_forzato=1;
				irrigazione_attiva(j,irrigazioneTable[j].msg_avvio_manuale);
			}
			sprintf(buffer,"%d %s",irrigazioneTable[j].attivo_forzato,TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
		}
		else
		{
			sprintf(buffer,"-1 %s",TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
		}
		return;
	}
	if(strstr(buf,"bg_attiva")==buf)
	{
		bg_attiva();
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"bn_attiva")==buf)
	{
		bn_attiva();
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"bg_disattiva")==buf)
	{
		bg_disattiva();
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"bn_disattiva")==buf)
	{
		bn_disattiva();
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"presenze_disattiva")==buf)
	{
		*scenariPresenzeAttivo=0;
		sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"irrigazione_disattiva ")==buf)
	{
		i=atoi(&buf[21]);
		i=id_irrigazioneToId(i);
		if(i>=0)
		{
			irrigazioneTable[i].attivo_forzato=0;
			irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_manuale);
			
			sprintf(buffer,"%d %s",irrigazioneTable[i].attivo_forzato,TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
		}
		else
		{
			sprintf(buffer,"-1 %s",TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
		}
		return;
	}
	if(strstr(buf,"set")==buf)
	{
		if(strstr(&buf[3]," ai_attivo ")==&buf[3])
		{
			strcpy(buffer,"");
			lock=(int *)malloc(DIGITALOUTCHANNELS*sizeof(int));
			if(value=ai_system_can_start(atoi(&buf[14]),lock)==0)
			{
				set_active_ai(atoi(&buf[14]),1);
				sprintf(buffer,"%d",0);	//0 = ok
			}
			else
			{
				if(value>0)	//qualche input impedisce di avviare antiintrusione
				{
					i=0;
					for(i=0;i<value;i++)
						sprintf(buffer,"%s,%d,",buffer,lock[i]);
					buffer[strlen(buffer)-1]='\0';
				}
				else
				{	//c'è stato un errore
					sprintf(buffer,"%d",-1);	//-1 = errore
				}
			}
			sprintf(buffer,"%s %s",buffer,TERMINATOR);
			free(lock);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," ai_attivo_ss ")==&buf[3])
		{
			if(c=strchr(&buf[17],' '))
			{
				*c='\0';
				c++;
				set_active_ai_ss(atoi(&buf[17]),atoi(c),1);
			}
		}		
		if(strstr(&buf[3]," ai_disattivo ")==&buf[3])
		{
			value=atoi(&buf[17]);
			set_active_ai(value,0);
		}
		if(strstr(&buf[3]," ai_disattivo_ss ")==&buf[3])
		{
			if(c=strchr(&buf[20],' '))
			{
				*c='\0';
				c++;
				set_active_ai_ss(atoi(&buf[20]),atoi(c),0);
			}
		}		
		if(strstr(&buf[3]," knx_value ")==&buf[3])
		{
			if(c=strchr(&buf[14],' '))
			{
				*c='\0';
				c++;
				setKnx(atoi(&buf[14]),atof(c));
			}
		}
		if(strstr(&buf[3]," output ")==&buf[3])
		{
			if(c=strchr(&buf[11],' '))
			{
				*c='\0';
				c++;
				if(d=strchr(c,' '))
				{
					*d='\0';
					d++;
					systemId=systemNumToId(atoi(&buf[11]),NUMSYSTEMS);
					chnId=atoi(c)-1;
					my_printf("%d %d\n",digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out, atoi(d));
					if(((systemId<NUMSYSTEMS)&&(chnId<systems[systemId].out_ch_d))
							&&(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out!=-1))
						setOutput(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out,atoi(d));
				}
				else
					setOutput(atoi(&buf[11]),atoi(c));
				return;
			}
		}
		if(strstr(&buf[3]," irrigazione")==&buf[3])
		{
			set_irrigazione(fd,&buf[15]);
			return;
		}
		return;
	}
	if(strstr(buf,"toggle ")==buf)
	{
		if(c=strchr(&buf[7],' '))
		{
			*c='\0';
			c++;
			systemId=systemNumToId(atoi(&buf[7]),NUMSYSTEMS);
			chnId=atoi(c)-1;
			if(((systemId<NUMSYSTEMS)&&(chnId<systems[systemId].out_ch_d))
					&&(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out!=-1)
					&&(digitalOutTable[deviceToid[2][systemId][chnId]].value!=-1))
				setOutput(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out,
					(1-digitalOutTable[deviceToid[2][systemId][chnId]].value));
		}
		return;
	}
	if(strstr(buf,"reset ")==buf)
	{
		sendResetSignal(systemNumToId(atoi(&buf[6]),NUMSYSTEMS));
		return;
	}
	if(strstr(buf,"status")==buf)
	{
		k=0;
		for(i=0;i<NUMSYSTEMS;i++)
			buffer[i]=systems[i].status;
		k+=NUMSYSTEMS;
		for(i=0;i<NUMMULTIMETERS;i++)
			buffer[i+k]=multimeters[i].status;
		k+=NUMMULTIMETERS;
		for(i=0;i<NUMKNXGATEWAYS;i++)
			buffer[i+k]=knxGateways[i].status;
		k+=NUMKNXGATEWAYS;
		for(i=0;i<NUMBACNETDEVICES;i++)
			buffer[i+k]=bacnetDevices[i].status;
		k+=NUMBACNETDEVICES;
		for(i=0;i<NUMSMSDEVICES;i++)
			buffer[i+k]=sms_devices[i].status;
		k+=NUMSMSDEVICES;

		for(j=0;j<strlen(TERMINATOR);j++)
			buffer[j+k]=TERMINATOR[j];
		k+=strlen(TERMINATOR);

		if (send(fd, buffer, k, 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"sms_status")==buf)
	{
		for(i=0;i<NUMSMSDEVICES;i++)
			buffer[i]=sms_devices[i].status;

		for(j=0;j<strlen(TERMINATOR);j++)
			buffer[j+i]=TERMINATOR[j];
		k+=strlen(TERMINATOR);

		if (send(fd, buffer, j+i, 0) == -1)
			my_perror("send");
		return;
	}
	if(strstr(buf,"reload ")==buf)
	{
		if(strstr(&buf[7]," system")==&buf[7])
		{
			loadSystemTable(1);
			return;
		}
		if(strstr(&buf[7]," analog")==&buf[7])
		{
			loadAnalogTable(1);
			return;
		}
		if(strstr(&buf[7]," digital")==&buf[7])
		{
			loadDigitalTable(1);
			return;
		}
		if(strstr(&buf[7]," out")==&buf[7])
		{
			loadDigitalOutTable(1);
			return;
		}
		if(strstr(&buf[7]," presenze")==&buf[7])
		{
			loadScenariPresenzeTable(1);
			return;
		}
		if(strstr(&buf[7]," bgbn")==&buf[7])
		{
			loadScenariBgBnTable(1);
			return;
		}
		return;
		if(strstr(&buf[7]," mm_reading")==&buf[7])
		{
			loadMultimeterTable(1);
			return;
		}
		return;
	}
	if(strstr(buf,"reload")==buf)
	{
		*reloadCommand=1;
		return;
	}
	if(strstr(buf,"dig ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			*c='\0';
			c++;
			i=deviceToid[1][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			my_printf("id_digital\tdescription\tdevice_num\tch_num\tenabled\n");
			my_printf("%d\t%s\t%d\t%d\t%d\n",digitalTable[i].id_digital,
						digitalTable[i].description,digitalTable[i].device_num,
						digitalTable[i].ch_num,digitalTable[i].enabled);
		}
		return;
	}
	if(strstr(buf,"ana ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			*c='\0';
			c++;
			i=deviceToid[0][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			my_printf("id_analog\tdescription\tdevice_num\tch_num\tenabled\n");
			my_printf("%d\t%s\t%d\t%d\t%d\n",analogTable[i].id_analog,
						analogTable[i].description,analogTable[i].device_num,
						analogTable[i].ch_num,analogTable[i].enabled);
		}
		return;
	}
	if(strstr(buf,"out ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			*c='\0';
			c++;
			i=deviceToid[2][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			my_printf("id_digital_out\tdescription\tdevice_num\tch_num\tvalue\tpo_delay\tstart\n");
			my_printf("%d\t%s\t%d\t%d\t%d\t%d\t%d\n",digitalOutTable[i].id_digital_out,
						digitalOutTable[i].description,digitalOutTable[i].device_num,
						digitalOutTable[i].ch_num,digitalOutTable[i].value,
						digitalOutTable[i].po_delay,(int)digitalOutTable[i].start_time);
		}
		return;
	}
	
	
	if(strstr(buf,"update")==buf)
	{
		if(strstr(&buf[6]," analog ")==&buf[6])
		{
			updateAnalogChannel(atoi(&buf[14]));
			return;
		}
		if(strstr(&buf[6]," digital ")==&buf[6])
		{
			updateDigitalChannel(atoi(&buf[15]));
			return;
		}
		if(strstr(&buf[6]," out ")==&buf[6])
		{
			updateDigitalOutChannel(fd,atoi(&buf[11]));
			return;
		}
		if(strstr(&buf[6]," bacnet ")==&buf[6])
		{
			i=updateBacnetChannel(atoi(&buf[14]));
			sprintf(buffer,"%d%s",i,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[6]," knx ")==&buf[6])
		{
			i=updateKnxLine(atoi(&buf[11]));
			sprintf(buffer,"%d%s",i,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[6]," mm ")==&buf[6])
		{
			i=updateMultimeterLine(atoi(&buf[10]));
			sprintf(buffer,"%d%s",i,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[6]," bg")==&buf[6])
		{
			updateDayNight(fd);
			return;
		}
		if(strstr(&buf[6]," bn")==&buf[6])
		{
			updateDayNight(fd);
			return;
		}
		if(strstr(&buf[6]," presenze")==&buf[6])
		{
			updatePresenze(fd);
			return;
		}
		if(strstr(&buf[6]," irrigazione")==&buf[6])
		{
			int id_irrigazione=atoi(&buf[18]);
			if(id_irrigazione<1)
				id_irrigazione=-1;
			updateIrrigazioneCircuiti(fd,id_irrigazione);
			return;
		}
		return;
	}
	if(strstr(buf,"get")==buf)
	{
		if(strstr(&buf[3]," sms")==&buf[3])
		{
			if(NUMSMSDEVICES>0)
			{
				sprintf(buffer,"sms status: %c - sms socket: %d\n",
					sms_devices[0].status,sms_devices[0].sockfd);
				if (send(fd, buffer, strlen(buffer), 0) == -1)
					my_perror("send");					
			}
			return;
		}
		if(strstr(&buf[3]," channels")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].in_ch_an;j++)
					{
						if(analogTable[deviceToid[0][i][j]].enabled)
						{
							k=deviceToid[0][i][j];
							sprintf(buffer,"0 %d %d %s`",analogTable[k].device_num,
									analogTable[k].ch_num,analogTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
					for(j=0;j<systems[i].in_ch_d;j++)
					{
						if(digitalTable[deviceToid[1][i][j]].enabled)
						{
							k=deviceToid[1][i][j];
							sprintf(buffer,"1 %d %d %s`",digitalTable[k].device_num,
									digitalTable[k].ch_num,digitalTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," values")==&buf[3])
		{
			for(i=0;i<ANALOGCHANNELS;i++)
			{
				j=systemNumToId(analogTable[i].device_num,NUMSYSTEMS);
				if((j!=-1)&&(analogTable[i].enabled && systems[j].enabled))
				{
					if(analogTable[i].value_valid)
						sprintf(buffer,"0 %d %f %d`",
							analogTable[i].id_analog,
							analogTable[i].value_eng,
							analogTable[i].value);
					else
						sprintf(buffer,"0 %d - -`",
							analogTable[i].id_analog);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");					
				}
			}
			for(i=0;i<DIGITALCHANNELS;i++)
			{
				j=systemNumToId(digitalTable[i].device_num,NUMSYSTEMS);
				if((j!=-1)&&(digitalTable[i].enabled && systems[j].enabled))
				{
					if(digitalTable[i].value_valid)
						sprintf(buffer,"1 %d %d %d`",
							digitalTable[i].id_digital,
							digitalTable[i].value,
							digitalTable[i].value);
					else
						sprintf(buffer,"1 %d - -`",
							digitalTable[i].id_digital);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");
				}
			}
			for(i=0;i<DIGITALOUTCHANNELS;i++)
			{
				j=systemNumToId(digitalOutTable[i].device_num,NUMSYSTEMS);
				if((j!=-1)&&(systems[j].enabled))
				{
					if(digitalOutTable[i].value_valid)
						sprintf(buffer,"2 %d %d %d`",
							digitalOutTable[i].id_digital_out,
							digitalOutTable[i].value,
							digitalOutTable[i].value);
					else
						sprintf(buffer,"2 %d - -`",
							digitalOutTable[i].id_digital_out);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");
				}
			}
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							strcpy(tempstring,"");
							k=deviceToid[3][i][j];
							for(l=0;l<readingTable[k].raw_length;l++)
								sprintf(tempstring,"%s%02x",tempstring,readingTable[k].raw[l]);
							if(readingTable[k].value_valid)
								sprintf(buffer,"3 %d %f %s`",
									readingTable[k].id_reading,
									readingTable[k].value,
									tempstring);
							else
								sprintf(buffer,"3 %d - -`",
									readingTable[k].id_reading);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			for(i=0;i<NUMKNXCHANNELS;i++)
			{
				j=id_knx_gatewayToId(knxTable[i].id_knx_gateway);
				if((j!=-1)&&(knxGateways[j].enabled && knxTable[i].enabled))
				{
					k=(knxTable[i].input_output?4:5);
					if(k==4)
					{
						if(knxTable[i].value_valid)
						{
							if(knxTable[i].data_type=='F')
								sprintf(buffer,"%d %d %f %d`",k,
									knxTable[i].id_knx_line,
									knxTable[i].value_eng,
									knxTable[i].value);
							else
								sprintf(buffer,"%d %d %d %d`",k,
									knxTable[i].id_knx_line,
									(int)knxTable[i].value_eng,
									knxTable[i].value);
						}
						else
							sprintf(buffer,"%d %d - -`",k,
								knxTable[i].id_knx_line);

						if (send(fd, buffer, strlen(buffer), 0) == -1)
							my_perror("send");
					}
				}
			}
			for(i=0;i<NUMBACNETLINES;i++)
			{
				j=id_bacnet_deviceToId(bacnetTable[i].id_bacnet_device);
				if((j!=-1)&&(bacnetDevices[j].enabled))
				{
					tempfloat=formatBacnetValue(bacnetTable[i].object_type,bacnetTable[i].value);
					if(bacnetTable[i].object_type==0)
						sprintf(tempstring,"%f",tempfloat);
					else
						sprintf(tempstring,"%d",(int)tempfloat);

					if(bacnetTable[i].value_valid)
						sprintf(buffer,"6 %d %s %lu`",
							bacnetTable[i].id,
							tempstring,
							bacnetTable[i].value);
					else
						sprintf(buffer,"6 %d - -`",
							bacnetTable[i].id);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");
				}
			}


			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," latest_values")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].in_ch_an;j++)
					{
						if(analogTable[deviceToid[0][i][j]].enabled)
						{
							k=deviceToid[0][i][j];
							sprintf(buffer,"0 %d %d %f %d`",
								analogTable[k].device_num,
								analogTable[k].ch_num,
								analogTable[k].value_eng,
								(analogTable[k].enabled && systems[i].enabled));

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
					for(j=0;j<systems[i].in_ch_d;j++)
					{
						if(digitalTable[deviceToid[1][i][j]].enabled)
						{
							k=deviceToid[1][i][j];
							sprintf(buffer,"1 %d %d %d %d`",
								digitalTable[k].device_num,
								digitalTable[k].ch_num,
								digitalTable[k].value,
								(digitalTable[k].enabled && systems[i].enabled));
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						k=deviceToid[2][i][j];
						sprintf(buffer,"2 %d %d %d %d`",
							digitalOutTable[k].device_num,
							digitalOutTable[k].ch_num,
							digitalOutTable[k].value,
							systems[i].enabled);
						if (send(fd, buffer, strlen(buffer), 0) == -1)
							my_perror("send");
					}
				}
			}
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"3 %d %d %f %d`",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								(readingTable[k].enabled && multimeters[i].enabled));
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}

			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," mm_names")==&buf[3])
		{
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"%d %d %s`",readingTable[k].multimeter_num,
									readingTable[k].ch_num,readingTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," mm_values_rt")==&buf[3])
		{
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];

							sprintf(buffer,"%d %d %f %f %ld %f %ld %ld\n",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								readingTable[k].value_min,
								readingTable[k].ts_min,
								readingTable[k].value_max,
								readingTable[k].ts_max,
								time(NULL));

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," mm_values")==&buf[3])
		{
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"%d %d %f %d`",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								(readingTable[k].enabled && multimeters[i].enabled));

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," knx_values")==&buf[3])
		{
			for(i=0;i<NUMKNXCHANNELS;i++)
			{
				j=id_knx_gatewayToId(knxTable[i].id_knx_gateway);
				if((knxTable[i].input_output==1)&&
					(knxGateways[j].enabled))
				{
					if(knxTable[i].data_type=='F')
						sprintf(buffer,"%d %d %f %d`",
							knxTable[i].id_knx_gateway,
							knxTable[i].id_knx_line,
							knxTable[i].value_eng,
							knxTable[i].enabled);
					else
						sprintf(buffer,"%d %d %d %d`",
							knxTable[i].id_knx_gateway,
							knxTable[i].id_knx_line,
							(int)knxTable[i].value_eng,
							knxTable[i].enabled);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," bacnet_values")==&buf[3])
		{
			for(i=0;i<NUMBACNETLINES;i++)
			{
				tempfloat=formatBacnetValue(bacnetTable[i].object_type,bacnetTable[i].value);
				if(bacnetTable[i].object_type==0)
					sprintf(tempstring,"%f",tempfloat);
				else
					sprintf(tempstring,"%d",(int)tempfloat);

				sprintf(buffer,"%d %lx %s\n",bacnetTable[i].id,bacnetTable[i].value,tempstring);

				if (send(fd, buffer, strlen(buffer), 0) == -1)
					my_perror("send");
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," outchannels")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
						{
							k=deviceToid[2][i][j];
							sprintf(buffer,"%d %d %s`",digitalOutTable[k].device_num,
								digitalOutTable[k].ch_num,digitalOutTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								my_perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," outputs")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						k=deviceToid[2][i][j];
						if(digitalOutTable[k].id_digital_out!=-1)
						{
							getDigitalOutTimings(&digitalOutTable[k],
								&po_delay_rem,&pon_time_rem,&on_time_rem,&poff_time_rem);
							
							sprintf(buffer,"%d %d %d %d %d %d %d`",
								digitalOutTable[k].device_num,
								digitalOutTable[k].ch_num,
								digitalOutTable[k].value_real,
								po_delay_rem,
								pon_time_rem,
								on_time_rem,
								poff_time_rem);

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," value ")==&buf[3])
		{
			if(strlen(&buf[10])>2)
			{
				buf[11]='\0';
				
				adId=atoi(&buf[10]);
				if(adId==0)
				{
					i=id_analogToId(atoi(&buf[12]));
					sprintf(buffer,"%f",analogTable[i].value_eng);
				}
				else
				{
					i=id_digitalToId(atoi(&buf[12]));
					sprintf(buffer,"%d",digitalTable[i].value);
				}
				if (send(fd, buffer, strlen(buffer), 0) == -1)
					my_perror("send");
				if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
					my_perror("send");
			}
			return;
		}
		if(strstr(&buf[3]," panel ")==&buf[3])
		{
			if((strlen(&buf[10])>=0)&&(atoi(&buf[10])<NUMPANELS))
			{
				for(i=0;i<16;i++)
				{
					sprintf(buffer,"%d %f`",panels[atoi(&buf[10])][i],
							analogTable[id_analogToId(panels[atoi(&buf[10])][i])].value_eng);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						my_perror("send");
				}
				if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
					my_perror("send");
			}
			return;
		}
		if(strstr(&buf[3]," presenze_attivo")==&buf[3])
		{
			sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," irrigazione_attivo ")==&buf[3])
		{
			i=atoi(&buf[23]);
			i=id_irrigazioneToId(i);
			if(i>=0)
			{
				sprintf(buffer,"%d %s",(irrigazioneTable[i].current_circuito>0),TERMINATOR);
				if (send(fd, buffer, strlen(buffer), 0) == -1)
					my_perror("send");
			}
			else
			{
				sprintf(buffer,"-1 %s",TERMINATOR);
				if (send(fd, buffer,strlen(buffer), 0) == -1)
					my_perror("send");
			}
			return;
		}
		if(strstr(&buf[3]," buongiorno_attivo")==&buf[3])
		{
			*buonGiornoAttivo=checkBuonGiornoAttivo();
			sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," buonanotte_attivo")==&buf[3])
		{
			*buonaNotteAttivo=checkBuonaNotteAttivo();
			sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
 		if(strstr(&buf[3]," ai_attivo")==&buf[3])
 		{
			value=get_active_ai();
			sprintf(buffer,"%d %s",value,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," ai_attivo_ss ")==&buf[3])
		{
			value=get_active_ai_ss(atoi(&buf[17]));
			sprintf(buffer,"%d %s",value,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		} 
		if(strstr(&buf[3]," irrigazione_rt ")==&buf[3])
		{
			get_irrigazione_rt(fd,&buf[18]);
			return;
		}
		if(strstr(&buf[3]," irrigazione ")==&buf[3])
		{
			get_irrigazione(fd,&buf[15]);
			return;
		}
		if(strstr(&buf[3]," pioggia")==&buf[3])
		{
			int pioggia=-1;
			if(*id_digital_pioggia!=-1)
			{
				if(digitalTable[*id_digital_pioggia].value!=-1)
					pioggia=(digitalTable[*id_digital_pioggia].value==1);
			}
			sprintf(buffer,"%d %s",pioggia,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," effemeridi")==&buf[3])
		{
			time_t timer;
			struct tm now;
			char dawn[6];
			char sunset[6];
			timer=time(NULL);
			now=*localtime(&timer);
	
			i2h(dawn,effemeridiTable[now.tm_mon][now.tm_mday - 1].dawn);
			i2h(sunset,effemeridiTable[now.tm_mon][now.tm_mday - 1].sunset);
			sprintf(buffer,"%s %s %s",dawn,sunset,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				my_perror("send");
			return;
		}
		if(strstr(&buf[3]," sottostato ")==&buf[3])
		{
			get_sottostato(fd,&buf[15]);
			return;
		}
		return;
	}
	if(strstr(buf,"system")==buf)
	{
		if(strstr(&buf[6]," update ")==&buf[6])
		{
			strcpy(buf,&buf[14]);
			while(c=strchr(buf,' '))
			{
				*c='\0';
				i=atoi(buf);
				strcpy(buf,c+1);
				j=atoi(buf);
				switch(i)
				{
					case 0:
						updateSystem(j);
						printf("updating system %d\n",j);
						break;
					case 3:
						updateMultimeter(j);
						printf("updating multimeter %d\n",j);
						break;
					case 4:
						updateKnx(j);
						printf("updating knx %d\n",j);
						break;
					case 6:
						updateBacnet(j);
						printf("updating bacnet %d\n",j);
						break;
				}
			}
		}
		if(strstr(&buf[6]," disable")==&buf[6])
		{
			if(strlen(&buf[14]))
			{
				systemId=atoi(&buf[14]);
				conn=mysqlConnect();
				if(!conn)
					return;
				sprintf(query,"UPDATE system SET enabled=0 WHERE device_num=%d",systems[systemId].device_num);
				state = mysql_query(conn, query);
				if(state==0)
				{
					systems[systemId].enabled=0;
					systems[systemId].status='d';
				}
				mysql_close(conn);
				return;
			}
		}
		if(strstr(&buf[6]," enable")==&buf[6])
		{
			if(strlen(&buf[13]))
			{
				systemId=atoi(&buf[13]);
				conn=mysqlConnect();
				sprintf(query,"UPDATE system SET enabled=1 WHERE device_num=%d",systems[systemId].device_num);
				state = mysql_query(conn, query);
				if(state==0)
					systems[systemId].enabled=1;
				mysql_close(conn);		
				return;
			}
		}
	}
	if(strstr(buf,"dump "))
	{
		if(strcmp(&buf[5],"analog"))
			dumpAnalogTable();
		return;
	}
	if(strstr(buf,"store ")==buf)
	{
		storeSystemTables(atoi(&buf[6]));
		return;
	}
	if(strstr(buf,"chn")==buf)
	{
		if(strstr(&buf[3]," disable ")==&buf[3])
		{
			if(strlen(&buf[12])&&(c=strchr(&buf[12],' '))&&(d=strchr(c+1,' ')))
			{
				*c='\0';
				*d='\0';
				systemId=atoi(&buf[12]);
				adId=atoi(c+1);
				chnId=atoi(d+1);
				conn=mysqlConnect();
				if(!conn)
					return;
				if(adId==0)
					sprintf(query,"UPDATE analog SET enabled=0 WHERE device_num=%d AND ch_num=%d",analogTable[deviceToid[0][systemId][chnId]].device_num,analogTable[deviceToid[0][systemId][chnId]].ch_num);
				else
					sprintf(query,"UPDATE digital SET enabled=0 WHERE device_num=%d AND ch_num=%d",digitalTable[deviceToid[1][systemId][chnId]].device_num,digitalTable[deviceToid[1][systemId][chnId]].ch_num);
				state = mysql_query(conn, query);
				my_printf("%s\n",query);
				if(state==0)
				{
					if(adId==0)
						analogTable[deviceToid[adId][systemId][chnId]].enabled=0;
					else
						digitalTable[deviceToid[adId][systemId][chnId]].enabled=0;
				}
				mysql_close(conn);
				return;
			}
		}
		if(strstr(&buf[3]," enable ")==&buf[3])
		{
			if(strlen(&buf[11])&&(c=strchr(&buf[11],' '))&&(d=strchr(c+1,' ')))
			{
				*c='\0';
				*d='\0';
				systemId=atoi(&buf[11]);
				adId=atoi(c+1);
				chnId=atoi(d+1);
				conn=mysqlConnect();
				if(!conn)
					return;
				if(adId==0)
					sprintf(query,"UPDATE analog SET enabled=1 WHERE device_num=%d AND ch_num=%d",analogTable[deviceToid[0][systemId][chnId]].device_num,analogTable[deviceToid[0][systemId][chnId]].ch_num);
				else
					sprintf(query,"UPDATE digital SET enabled=1 WHERE device_num=%d AND ch_num=%d",digitalTable[deviceToid[1][systemId][chnId]].device_num,digitalTable[deviceToid[1][systemId][chnId]].ch_num);
				state = mysql_query(conn, query);
				my_printf("%s\n",query);
				if(state==0)
				{
					if(adId==0)
						analogTable[deviceToid[adId][systemId][chnId]].enabled=1;
					else
						digitalTable[deviceToid[adId][systemId][chnId]].enabled=1;
				}
				mysql_close(conn);
				return;
			}
		}
	}
	return;
}

void server_termination_handler (int signum)
{
	my_printf("-----------%d-----------------\n",getpid());
	register int i;
	for(i=0;i<MAXCONNECTIONS;i++)
		if(serverPids[i]!=-1)
			kill(serverPids[i],SIGTERM);
	exit(1);
}
