#include "controller.h"

	
void doController()
/*--------------------------------------------
 * gestisce i comandi alle schede
 * -----------------------------------------*/
{
	time_t inizio,curtime;
	int minutes,seconds;
	struct timeval tv;
	struct tm ts;
	int pioggia=0;
	char writeBuffer[MSGLENGTH];
	int i,j,id_circuito,circuito,id_irrigazione,day,wday;
	struct msgform msg;

	while(1)
	{
		// inizio irrigazione
		inizio=time(NULL);
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;
		seconds=minutes*60+ts.tm_sec;
		day=ts.tm_mday+32*ts.tm_mon+384*ts.tm_year;
		wday=(6+ts.tm_wday)%7;

		if(*id_digital_pioggia!=-1)
		{
			if(digitalTable[*id_digital_pioggia].value!=-1)
				pioggia=(digitalTable[*id_digital_pioggia].value==1);
		}


		id_irrigazione=0;

		for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		{
			if(circuiti_attivi(i)>0)
			{
				
				if((irrigazioneTable[i].attivo==0)&&	// non forzato
						(irrigazioneTable[i].day!=day)&&	// non ancora eseguito 
						(minutes-irrigazioneTable[i].ora_start>=0))	// passato ora_start
				{
					if(pioggia==0)	// non piove
						irrigazione_attiva(i,irrigazioneTable[i].msg_avvio_automativo);
					else // piove, impedisco che parta irrigazione
					{
						if(pioggia==1) // piove
						{
							irrigazioneTable[i].day=day;
							formatMessage(msg.mtext,3,0,irrigazioneTable[i].id_irrigazione,0,irrigazioneTable[i].msg_pioggia_noinizio);
							msg.mtype=1;
							msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
							storeEvent(1,irrigazioneTable[i].id_irrigazione,0,0,irrigazioneTable[i].msg_pioggia_noinizio);
						}
					}
				}
				else
				{
					if((pioggia==1)&&(irrigazioneTable[i].attivo==0)&&(irrigazioneTable[i].current_circuito>0))
						irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_pioggia);
				}

				if(irrigazioneTable[i].current_circuito>0) // significa che in qualche modo (manuale o automatico) irrigazione è avviata
				{
					if(irrigazioneTable[i].current_circuito<=irrigazioneTable[i].num_circuiti)
					{
						id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
						j=id_digitalOutToId(irrigazioneCircuitiTable[id_circuito].id_digital_out);
						if(j!=-1)
						{
							if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))>0) //circuito ancora attivo
							{
								if(digitalOutTable[j].value!=digitalOutTable[j].on_value)
								{
/*									formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito attivato");
									msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
									storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito attivato");*/
								}
								irrigazione_attiva_pompa(i);	// imposto uscita a 1 anche se già lo è per essere certo che sia attiva
								digitalOutTable[j].value=digitalOutTable[j].on_value;
								digitalOutTable[j].req_value=digitalOutTable[j].on_value;
								digitalOutTable[j].po_delay=0;
							}
							else 	//circuito non più attivo
							{
/*								formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito disattivato");
								msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);											
								storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito disattivato");*/
								digitalOutTable[j].value=1-digitalOutTable[j].on_value;
								digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
								digitalOutTable[j].po_delay=0;
								irrigazioneTable[i].ora_started=seconds;
								irrigazioneTable[i].current_circuito++;
								if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
								{
									irrigazione_disattiva_pompa(i);	// imposto uscita pompa a 0
									irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
								}
								else
								{
									id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
									if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
										irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
									else
										irrigazioneTable[i].current_time=0;
								}
							}
						}
						else //id_digital_out non trovato
						{
							irrigazioneTable[i].current_circuito++;
							irrigazioneTable[i].ora_started=seconds;
							if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
								irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
							else
							{
								id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
								printf("circuito %d %d %d\n",id_circuito,i,irrigazioneTable[i].current_circuito);
								if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
									irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
								else
									irrigazioneTable[i].current_time=0;
							}
						}
					}
					else 	// fine circuiti, attendo tempo_off oppure termino
					{
						irrigazione_disattiva_pompa(i);
	/*					if(irrigazioneTable[i].current_circuito==1+irrigazioneTable[i].num_circuiti)
						{
							formatMessage(msg.mtext,3,0,irrigazioneTable[i].id_irrigazione,0,"Fine circuiti");
							msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
							storeEvent(1,irrigazioneTable[i].id_irrigazione,0,0,"Fine circuiti");
							irrigazioneTable[i].current_circuito++;
						}*/
						if(irrigazioneTable[i].count>=irrigazioneTable[i].ripetitivita)	// non ripeto più
							irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_automatico);
						else	//ripeto
						{
							if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))<0) // passato tempo off
							{
								irrigazioneTable[i].count++;
								irrigazioneTable[i].ora_started=seconds;
								irrigazioneTable[i].current_circuito=1;
								id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,1);
								irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
							}
						}
					}
				}
			}
			
		}

//		printf("controllo\n");
/* giorgio
 * 		analogTable[id_analogToId(23)].value;
 *		digitalTable[id_digitalToId(1)].value;
 * 		setOutput(22,0);
 * 	
 * 		formatMessage(writeBuffer,0,0,0,0,"SYSTEM STOPPING");
 *		write(logFileFd,writeBuffer,strlen(writeBuffer));
 * 		
 * 
 * */

		
		usleep(10000);
	}
}
