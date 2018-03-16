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
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define NOTE_CNT_INFO		0x01
#define DATA_TRANSFER_BYTE	0x11

#define OUT_BUFF_SIZE		32
#define IN_BUFF_SIZE		4096

#define SERVER_PATH     	"/tmp/MeshDataServer"	// file name for data exchange
#define MAX_THREAD_COUNT	5


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
	serv_socket,
	read_bytes_cnt,
	send_bytes_cnt;

	uint8_t data_buffer_in[IN_BUFF_SIZE];
	uint8_t data_buffer_out[OUT_BUFF_SIZE];

	uint32_t nodes_cnt,
	first_index, get_from,
	last_index, get_to,
	notes_per_msg;

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

	sleep(1);

	data_buffer_out[0] = NOTE_CNT_INFO;
	send_bytes_cnt = 1;
	send(serv_socket, data_buffer_out, send_bytes_cnt, 0);

	while(1)
	{
		read_bytes_cnt = recv(serv_socket, data_buffer_in, IN_BUFF_SIZE, 0);
		if(read_bytes_cnt) {
			memcpy(&nodes_cnt, data_buffer_in + 1, 4);
			printf("ANSWER: %d\n", nodes_cnt);
			break;
		}
	}

	sleep(1);

	data_buffer_out[0] = DATA_TRANSFER_BYTE;
	get_from = 0;
	get_to = 499999;
	first_index = 0;
	last_index = -1;
	notes_per_msg = (IN_BUFF_SIZE - 1) / sizeof(node_note_t);
	printf("notes_per_msg: %d\n", notes_per_msg);
	send_bytes_cnt = 9;



	first_index = last_index + 1;
	last_index = first_index + notes_per_msg - 1;
	if(last_index > get_to) last_index = get_to;
	printf("first_index: %d\n", first_index);
	printf("last_index: %d\n", last_index);
	

	while(1)
	{
		memcpy(data_buffer_out+1, &first_index, 4);
		memcpy(data_buffer_out+5, &last_index, 4);
		send(serv_socket, data_buffer_out, send_bytes_cnt, 0);

		usleep(100);

		read_bytes_cnt = recv(serv_socket, data_buffer_in, IN_BUFF_SIZE, 0);

		if(read_bytes_cnt) {
			volatile node_note_t note;
			for(int i = 0; i < last_index-first_index + 1; i++)
			{
				memcpy((uint8_t * )&note, data_buffer_in + 1 + sizeof(node_note_t)*i, sizeof(node_note_t));
				//printf("-- NOTE #%d:\n", first_index+i);
				//printf("ipv4: %d\n", note.ipv4);
				/*printf("rank: %d\n", note.rank);
				printf("parent_ipv4: %d\n", note.parent_ipv4);*/
			}

			printf("read_bytes_cnt: %d\n", read_bytes_cnt);
			
		}
		
		first_index = last_index + 1;
		if(first_index > get_to) break;
		last_index = first_index + notes_per_msg - 1;
		if(last_index > get_to) last_index = get_to;
		printf("first_index: %d\n", first_index);
		printf("last_index: %d\n", last_index);
	}

	return 0;
}