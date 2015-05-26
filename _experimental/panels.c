#include "panels.h"

void loadPanels()
{
	int curPanel=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,j;
	int temp;
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return;
	}

	
	temp=NUMPANELS;
	NUMPANELS=0;
	for(i=0;i<temp;i++)
		free(panels[i]);
	if(panels)
		free(panels);
	panels=NULL;
	
	state = mysql_query(conn, "SELECT max(panel_num) FROM panel");
	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	if( ( row = mysql_fetch_row(result)) != NULL )
		NUMPANELS=atoi(row[0]);
		
//	my_printf("%d\n",NUMPANELS);
	mysql_free_result(result);
	panels=(int **)malloc(NUMPANELS);
	for(i=0;i<NUMPANELS;i++)
	{
		panels[i]=(int *)malloc(16*sizeof(int));
		for(j=0;j<16;j++)
			panels[i][j]=0;
	}
	

	state = mysql_query(conn, "SELECT panel_num,id_analog FROM panel ORDER BY panel_num,id");
	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		if(atoi(row[0])!=curPanel)
		{
			curPanel=atoi(row[0]);
			i=0;
		}
		panels[curPanel-1][i]=atoi(row[1]);
		i++;
	}
	mysql_free_result(result);
	mysql_close(conn);
}
