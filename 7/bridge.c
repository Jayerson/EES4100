#include <stdio.h>
#include <stdlib.h>
#include <modbus-tcp.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

//added new libraries:
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

//defined server
#define SERVER_PORT 10000
#define SERVER_ADDR "127.0.0.1"
#define DATA_LENGTH 256

struct list_object_s {
    char *string;                   /* 8 bytes */
    int strlen;                     /* 4 bytes */
    struct list_object_s *next;     /* 8 bytes */
};
/* list_head is initialised to NULL on application launch as it is located in 
 * the .bss */
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t list_data_ready = PTHREAD_COND_INITIALIZER;
static pthread_cond_t list_data_flush = PTHREAD_COND_INITIALIZER;
static struct list_object_s *list_head;



static void add_to_list(char *input) {
    /* Allocate memory */
    struct list_object_s *last_item;
    struct list_object_s *new_item = malloc(sizeof(struct list_object_s));
    if (!new_item) {
        fprintf(stderr, "Malloc failed\n");
        exit(1);
    }

    /* Set up the object */
    new_item->string = strdup(input);
    new_item->strlen = strlen(input);
    new_item->next = NULL;

//list head shared between threads, lock before access
    pthread_mutex_lock(&list_lock);
    
    if (list_head == NULL) {
        /* Adding the first object */
        list_head = new_item;
    } else {
        /* Adding the nth object */
        last_item = list_head;
        while (last_item->next) last_item = last_item->next;
        last_item->next = new_item;
    }
// inform print_and_free, then release lock
    pthread_cond_signal(&list_data_ready);
    pthread_mutex_unlock(&list_lock);
}



// automatically pulls list item for p&f
static struct list_object_s *list_get_first(void)
{
    struct list_object_s *first_item;
    first_item = list_head;
    list_head = list_head->next;

    return first_item;
}



//forking
static void *print_and_free(void *arg) {
    struct list_object_s *cur_object;
    printf("thread is starting\n");
while (1) {
//wait until data available
//lock
	pthread_mutex_lock(&list_lock);

	while(!list_head)
	    pthread_cond_wait(&list_data_ready, &list_lock);
        cur_object = list_get_first();
//unlock
	pthread_mutex_unlock(&list_lock);

        printf("t2: String is: %s\n", cur_object->string);
        printf("t2:String length is %i\n", cur_object->strlen);
        free(cur_object->string);
        free(cur_object);

// inform list_flush of work order
	pthread_cond_signal(&list_data_flush);
    }
}



//holds lock while flushing
static void list_flush(void)
{
	pthread_mutex_lock(&list_lock);
	while (list_head)
		{
		pthread_cond_signal(&list_data_flush);
		pthread_cond_wait(&list_data_flush, &list_lock);
		}
	pthread_mutex_unlock(&list_lock);	
}



static void server(void)
{
int sock, bytes;
pthread_t print_thread;
struct sockaddr_in server_loc;
struct sockaddr_in client_loc;
socklen_t client_llen;
char data [DATA_LENGTH];

printf("starting server - \n");
pthread_create(&print_thread, NULL, print_and_free, NULL);

if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	printf("Socket error\n");
	exit(EXIT_FAILURE);
	}

memset(&server_loc, 0, sizeof(struct sockaddr_in)); // sets aside memory
server_loc.sin_family = AF_INET;
server_loc.sin_port = htons(SERVER_PORT);
server_loc.sin_addr.s_addr = INADDR_ANY;

if (bind(sock, (struct sockaddr *) &server_loc, 
	sizeof(struct sockaddr_in)) <0)
	{
	printf("binding error\n");
	exit(EXIT_FAILURE);
	}

while(1)
	{
	client_llen = sizeof(client_loc);
	bytes = recvfrom(sock, data, sizeof(data), 0,
		(struct sockaddr *) &client_loc, &client_llen);

	if (bytes <0)
		{
		printf("receipt error\n");
		exit(EXIT_FAILURE);
		}
	add_to_list(data);
	}
list_flush();
}



static void client(int counter_given, int counter)
{
int sock;
struct sockaddr_in loc; // socket location
char input[256]; // on stack
if ((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	printf("socket error\n");
	exit(EXIT_FAILURE);
	}
memset(&loc, 0, sizeof(loc));
loc.sin_family = AF_INET;
loc.sin_port = htons(SERVER_PORT);
loc.sin_addr.s_addr = inet_addr(SERVER_ADDR);

if(connect(sock, (struct sockaddr *) &loc, sizeof(loc)) <0)
	{
	printf("server error\n");
	exit(EXIT_FAILURE);
	}
while (scanf("%256s", input) !=EOF) // on the stack
	{
	if (send (sock, input, strlen(input) + 1, 0) <0)
		{
		printf("send error\n");
		exit(EXIT_FAILURE);
		}
	if (counter_given)
		{
		counter--;
		if(!counter) break;
		}
	}
}



//forking main
int main(int argc, char **argv) {
    modbus_t *ctx;
int option_index, c, counter, counter_given = 0, run_server = 0;
//stack input shifted to client from here

    struct option long_options[] = {
        { "count",      required_argument,  0, 'c' },
        { "directive",  no_argument,        0, 'd' },
	{ "server",	no_argument,	    0, 's' },
        { 0 }
    };

    while (1) {
        c = getopt_long(argc, argv, "c:ds", long_options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 'c':
                printf("Got count argument with value %s\n", optarg);
                counter = atoi(optarg);
                counter_given = 1;
                break;
            case 'd':
                printf("Got directive argument\n");
                break;
	    case 's':
		run_server = 1;
		break;
        }
    }

/* Deprecated/ shifted
    // Print out all items of linked list and free them
    pthread_create(&print_thread, NULL, print_and_free, NULL);

    while (scanf("%256s", input) != EOF) {
        // Add word to the bottom of a linked list
        add_to_list(input);
        if (counter_given) {
            counter--;
            if (!counter) break;
        }
    }

    printf("Linked list object is %li bytes long\n",
                    sizeof(struct list_object_s));

//block until printed
    list_flush();
*/
	if (run_server) server();
	else client (counter_given, counter);
    return 0;
}
