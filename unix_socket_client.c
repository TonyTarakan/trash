/*

Tables to move here:

NODES(MESH)
|  ID  |  IPV4  |  RANK  |  PARENT_IPV4  |


LINK
|  ID  |  IPV4  |  NEIGHBOR_IPV4  |  NEIGHBOR_RANK  |  NEIGHBOR_RSSI  |  FLAGS  |


*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>



#define SERVER_PATH     	"/tmp/MeshDataServer"	// file name for data exchange
#define MAX_THREAD_COUNT	5

/*
struct fdStructType 
{
    int 		mainSocket;
    int 		epollFD;
};

void CleanThread(struct fdStructType *threadFD);
void *ServerThreadFunc(void *args);
*/


/*****************************************/
/*************** MAIN FUNC ***************/
/*****************************************/
int main(int argc, char **argv)
{
	/*
	pthread_t thread[MAX_THREAD_COUNT] = {0};
    pthread_attr_t threadAttr; 
    pthread_attr_init(&threadAttr); 
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    */

	int res,
	serv_socket;


    struct sockaddr_un server_addr;

    // create unix socket
	serv_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(serv_socket < 0)	{
		printf("Error: serv_socket\n");
		return serv_socket;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SERVER_PATH);

	res = connect(serv_socket, (struct sockaddr *)&server_addr, SUN_LEN(&server_addr));
	if (res < 0) {
		perror("connect() failed");
		return res;
	}

	sleep(10);

	while(1)
	{
		
	}

	return 0;
}