#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <sys/types.h>
#include <sys/wait.h>
#include "systems.h"
#include "knx.h"
#include "irrigazione.h"

int controllerPid; //pid del controller
void doController();

#endif
