#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>

#include "cWebSockets.h"

#if DEBUG
#define DBG_STR(buf) printf("[DEBUG] %s: %s\n", __FUNCTION__, buf);
#else
#define DBG_STR(buf)
#endif

#define MAX_BRIDGE_CONNECTIONS 50

static int client_socket[MAX_BRIDGE_CONNECTIONS];
static int client_nb = 0;

/* Signaling stuff to stop the server */
static bool dead = false;

void intHandler()
{
    dead = true;
}
/* ----------------- */

/* Wrapper functions */

int server_recv(int fd, char* buffer, int bufferSize)
{
    int n;

    n = recv(fd, buffer, bufferSize, 0);
    return n;
}


int server_write(int fd, const char* msg, int msgSize)
{
    int n;

    DBG_STR(msg);
    n = write(fd, msg, msgSize);
    if (n == -1)
        printf("[%s:%d] Error : failed to write to the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
    return n;
}

int server_read(int fd, char* buffer,int bufferSize)
{
    int n;

    memset(buffer, 0, bufferSize);
    n = read(fd,buffer,bufferSize-1);
    if (n == -1)
        printf("[%s:%d] Error : failed to read from the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
    DBG_STR(buffer);
    return n;
}
/* ----------------- */

/* Websocket thread handler */
void *websocket_thread_handler(void *arg)
{
    int i, num_events;
    int events_found = 0;
    char client_ws_msg[1024], data[1024];
    struct pollfd pfds[MAX_BRIDGE_CONNECTIONS];

    // Clear/Init all file descriptors
    for(i=0; i< MAX_BRIDGE_CONNECTIONS; i++)
    {
        pfds[i].fd = -1;
    }

    while(!dead)
    {
        for(i=0; i<client_nb; i++)
        {
            pfds[i].fd = client_socket[i];
            pfds[i].events = POLLIN | POLLPRI;
        }

        // Poll the client sockets without blocking the thread
        num_events = poll(pfds, client_nb, 0);

        if(num_events == -1)
        {
            printf("[%s:%d] Error : poll() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        }
        else if(num_events > 0)
        {
            /* Service all the sockets with input pending. */
            events_found = 0;
            for(i=0; events_found < num_events; i++)
            {
                if(pfds[i].revents & POLLIN)
                {
                    // Receive Ws message from client
                    server_read(client_socket[i], client_ws_msg, sizeof(client_ws_msg));
                    WEBSOCKET_get_content(client_ws_msg, sizeof(client_ws_msg), (unsigned char*) data, sizeof(data));
                    printf("[%s:%d] Receive msg : %s\n", __FUNCTION__, __LINE__, data);
                    events_found++;
                }
            }
        }
    }
    printf("[%s:%d] Exit websocket thread\n", __FUNCTION__, __LINE__);

    return NULL;
}


int server_wait_client(int sockfd)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    char buffer[1024], response[1024];

    client_socket[client_nb] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if(client_socket[client_nb] == -1)
    {
        printf("[%s:%d] Error : connect() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    server_read(client_socket[client_nb], buffer, sizeof(buffer));

    if(WEBSOCKET_client_version(buffer) != 13)
    {
        printf("[%s:%d] Error : unsupported Websocket client version.\n", __FUNCTION__, __LINE__);
        goto err;
    }

    if(!WEBSOCKET_valid_connection(buffer))
    {
        printf("[%s:%d] Error : not valid Websocket connection.\n", __FUNCTION__, __LINE__);
        goto err;
    }

    // Calculate response handshake
    if(WEBSOCKET_generate_handshake(buffer, response, sizeof(response)) != 0)
    {
        printf("[%s:%d] Error : generate Websocket handshake failed.\n", __FUNCTION__, __LINE__);
        goto err;
    }

    // Send handshake response
    server_write(client_socket[client_nb], response, strlen(response));
    client_nb++;
    printf("[%s:%d] Info : Ws client number %d connected.\n", __FUNCTION__, __LINE__, client_nb);

    return 0;

err:
    server_write(client_socket[client_nb], WEBSOCKET_CONN_CLOSE, sizeof(WEBSOCKET_CONN_CLOSE));
    close(client_socket[client_nb]);
    return 1;
}


int main(int argc, const char *argv[])
{
    int bridge_sockfd;
    int option_Value = 1;
    struct sockaddr_in server;
    //const char *remote_ip;
    unsigned short bridge_port; // remote_port;
    pthread_t ws_thread;

    // Check parameters
    if (argc != 4)
    {
        printf("Usage : %s  local_port remote_ip remote_port\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Bridge Websocket server to create
    bridge_port = atoi(argv[1]);

    // Remote TCP server to connect
    //remote_ip = argv[2];
    //remote_port = atoi(argv[3]);

    // Create TCP socket dedicated to the bridge server
    bridge_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(bridge_sockfd < 0)
    {
        printf("[%s:%d] Error : failed to create the socket server (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // Make the socket instantly reusable
    if (setsockopt(bridge_sockfd, SOL_SOCKET, SO_REUSEADDR, &option_Value, sizeof(option_Value)) == -1)
    {
        printf("[%s:%d] Error : failed to set the server socket as reusable (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // Try to bind the server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(bridge_port);
    if(bind(bridge_sockfd, (const struct sockaddr *) &server, sizeof(server)) != 0)
    {
        printf("[%s:%d] Error : failed to bind the server (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        close(bridge_sockfd);
        return 1;
    }

    if((listen(bridge_sockfd, MAX_BRIDGE_CONNECTIONS)) != 0)
    {
        printf("[%s:%d] Error : listen() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // register signal handler
    signal(SIGINT, intHandler);

    // Wait for clients
    while(!dead && client_nb < MAX_BRIDGE_CONNECTIONS)
    {
        if(server_wait_client(bridge_sockfd) == 0)
        {
            /* 1st client connected => create Websocket thread to handle all clients messages */
            if(client_nb == 1)
            {
                if(pthread_create(&ws_thread , NULL, websocket_thread_handler, NULL) < 0)
                {
                    printf("[%s:%d] Error : pthread_create() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
                    return 1;
                }
                printf("[%s:%d] Info : WebSocket thread created.\n", __FUNCTION__, __LINE__);
            }
        }
    }

    // Waiting thread terminating
    if(pthread_join(ws_thread, NULL) != 0)
    {
        printf("[%s:%d] Error : pthread_join() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // Close the program server socket
    if(close(bridge_sockfd) != 0){
        printf("[%s:%d] Error : failed to close the server (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    printf("Succesfully exited program\n");

    return 0;
}
