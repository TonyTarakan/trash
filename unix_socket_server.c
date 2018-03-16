#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/epoll.h>


#define NOTE_CNT_INFO		0x01
#define DATA_TRANSFER_BYTE	0x11

#define EPOLL_TIMEOUT		-1
#define EPOLL_EVENT_LIMIT	10

#define IN_BUFF_SIZE		32
#define OUT_BUFF_SIZE		4096

#define SERVER_PATH     	"/tmp/MeshDataServer"	// file name for data exchange
#define MAX_CONN			2


#pragma pack(push, 1)
typedef struct
{
	uint32_t ipv4;
	uint16_t rank;
	uint32_t parent_ipv4;
} node_note_t;

typedef struct
{
	uint32_t ipv4;
	uint32_t neighbor_ipv4;
	uint16_t neighbor_rank;
	int8_t   neighbor_rssi;
	uint16_t flags;	
} link_note_t;
#pragma pack(pop)


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
	send_bytes_cnt,
	i;

	const uint32_t nodes_cnt = 500000;
	node_note_t nodes_array[nodes_cnt];
	uint32_t first_index,
	last_index;
	for (int i = 0; i < nodes_cnt; ++i)
	{
		nodes_array[i].ipv4 = i;
		nodes_array[i].rank = 128+i;
		nodes_array[i].parent_ipv4 = i;
	}
/*
	const uint32_t links_cnt = 500000;
	node_note_t links_array[links_cnt];
	for (int i = 0; i < links_cnt; ++i)
	{
		links_array[i].ipv4 = i;
		links_array[i].neighbor_ipv4 = i;
		links_array[i].neighbor_rank = i;
		links_array[i].neighbor_rssi = i;
		links_array[i].flags = i;
	}
*/
	struct epoll_event epoll_event_arr[EPOLL_EVENT_LIMIT];
	uint8_t data_buffer_in[IN_BUFF_SIZE];
	uint8_t data_buffer_out[OUT_BUFF_SIZE];

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
				read_bytes_cnt = recv(epoll_source, data_buffer_in, IN_BUFF_SIZE, 0);
				if(read_bytes_cnt > 0)
				{
					printf("GOT: ");
					for(int i = 0; i < read_bytes_cnt; i++)
						printf("%02x ", *((uint8_t * )data_buffer_in+i));
					printf("\n");


					data_buffer_out[0] = data_buffer_in[0];
					switch(data_buffer_in[0]){

						case NOTE_CNT_INFO: 
							memcpy(data_buffer_out+1, &nodes_cnt, 4);
							send_bytes_cnt = 5;
							send(epoll_source, data_buffer_out, send_bytes_cnt, 0);
							break;

						case DATA_TRANSFER_BYTE: 
							memcpy(&first_index, data_buffer_in+1, 4);	// send notes from here
							memcpy(&last_index,  data_buffer_in+5, 4);	// to here

							send_bytes_cnt = sizeof(node_note_t)*(last_index - first_index + 1);

							memcpy((uint8_t * )(data_buffer_out + 1), (const uint8_t * )(&nodes_array[first_index]), send_bytes_cnt);

							send_bytes_cnt++; // including first byte(type)

							printf("SEND %d NOTES (%d bytes)\n", (last_index - first_index + 1), send_bytes_cnt);

							send(epoll_source, data_buffer_out, send_bytes_cnt, 0);
							break;

						default:
							data_buffer_out[0] = 'X';
							send_bytes_cnt = 1;
							send(epoll_source, data_buffer_out, send_bytes_cnt, 0);
							break;
					}
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
