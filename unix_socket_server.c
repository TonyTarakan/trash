/*

Tables to move here:

NODES(MESH)
|  ID  |  IPV4  |  RANK  |  PARENT_IPV4  |


LINK
|  ID  |  IPV4  |  NEIGHBOR_IPV4  |  NEIGHBOR_RANK  |  NEIGHBOR_RSSI  |  FLAGS  |


*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/epoll.h>


#define EPOLL_TIMEOUT		-1
#define EPOLL_EVENT_LIMIT	10

#define BUFF_SIZE			256

#define SERVER_PATH     	"/tmp/MeshDataServer"	// file name for data exchange
#define MAX_CONN			2


/******************************************
				MAIN FUNC
******************************************/
int main(int argc, char **argv) {


	int epoll_fd, 
	serv_socket, 
	res, 
	epoll_event_cnt, 
	epoll_event_type, 
	epoll_source,
	read_bytes_cnt, 
	i;

	struct epoll_event epoll_event_arr[EPOLL_EVENT_LIMIT];
	char data_buffer[BUFF_SIZE];

	struct sockaddr_un server_addr;

	// create unix socket
	serv_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(serv_socket < 0)	{
		printf("Error: serv_socket\n");
		return serv_socket;
	}

	// assign the file path to our server socket
	unlink(SERVER_PATH);							// remove old file if it exists
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SERVER_PATH);

	res = bind(serv_socket, (struct sockaddr *)&server_addr, SUN_LEN(&server_addr));
	if (res < 0) {
		printf("Error: bind() failed");
		return res;
	}



	struct epoll_event epoll_config;
	struct epoll_event epoll_events[EPOLL_EVENT_LIMIT];

	epoll_fd = epoll_create(EPOLL_EVENT_LIMIT);
	if (epoll_fd < 0) {
		printf("Error: epoll_fd\n");
		return epoll_fd;
	}
	epoll_config.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
	epoll_config.data.fd = serv_socket;



	// add listening UNIX socket to epoll
	res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serv_socket, &epoll_config);
	if(res < 0)
	{
		printf("Error: epoll regisration\n");
		return res;
	}

	// wait for requests
	res = listen(serv_socket, MAX_CONN);
	if (res < 0) {
		printf("Error: listen() failed\n");
		return res;
	}

	printf("Listening...\n");

	while(1)
	{
		epoll_event_cnt = epoll_wait(epoll_fd, epoll_event_arr, EPOLL_EVENT_LIMIT, EPOLL_TIMEOUT);

		for(i = 0; i < epoll_event_cnt; i++) {

			epoll_source = epoll_event_arr[i].data.fd;
			epoll_event_type = epoll_event_arr[i].events;

			// if we have a new connection
			if(epoll_source == serv_socket)   
			{
				res = accept(serv_socket, NULL, NULL);  			// any client
				if(res < 0) {
					printf("Error: accept()\n");
				}

				fcntl(res, F_SETFL, O_NONBLOCK);   // set listening socket as nonblocking
				printf("New connection\n");

				epoll_config.data.fd = res;
				epoll_config.events = EPOLLIN | EPOLLET;			// message, edge trigger
				res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, res, &epoll_config);
				if(res < 0)
				{
					printf("Error: epoll event regisration\n");
				}
			}


			// data on existing connection
			else if(epoll_event_type & EPOLLIN) 
			{
				read_bytes_cnt = recv(epoll_source, data_buffer, BUFF_SIZE, 0);
				if(read_bytes_cnt > 0)
				{
					
				}
				else // someone wants to close connection
				{
					close(epoll_source);
					epoll_config.data.fd = epoll_source;
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_source, &epoll_config);
					printf("Connection closed\n");
				}
			}

			else
			{
				printf("Unknown epoll event\n");
			}
		}
	}

	return 0;
}
