#ifndef PANELS_H
#define PANELS_H

#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"

int NUMPANELS;
int **panels;
void loadPanels();

#endif
