#include "controller.h"

	
void doController()
/*--------------------------------------------
 * gestisce i comandi alle schede
 * -----------------------------------------*/
{

	while(1)
	{
		// irrigazione
		doIrrigazione();
		// simulazione presenze
		doScenariPresenze();
		// anti intrusione
		doAntiIntrusione();

//		my_printf("controllo\n");
/* giorgio
 * 		analogTable[id_analogToId(23)].value;
 *		digitalTable[id_digitalToId(1)].value;
 * 		setOutput(22,0);
 * 	
 * 		handleMessage(0,0,0,0,"SYSTEM STOPPING",0,ALARM_NO_STORE);
 * 		
 * 
 * */

		usleep(10000);
	}
}


