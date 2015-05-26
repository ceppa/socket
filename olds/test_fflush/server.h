#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scenari.h"
#include "irrigazione.h"
#include "analog.h"
#include "digital.h"
#include "digital_out.h"
#include "knx.h"
#include "multimeter.h"
#include "panels.h"
#include "common.h"




int serverPid;	//pid del server (padre di serverPids)
int serverPids[MAXCONNECTIONS];		//connessioni esterne (web,telnet)
void doServer();
void performAction(int fd,char *buf);
void server_termination_handler (int signum);

#endif
