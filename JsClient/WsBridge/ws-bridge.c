/** @file ws-bridge.c
 * WebSocket Bridge server.
 * This program allow communication between WebSocket client and another TCP server (without support of WebSocket protocol).
 * @author Yannick BILCOT
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
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

// Max number of simultaneous client connections authorized
#define MAX_BRIDGE_CONNECTIONS 50

/* Program structs */

typedef struct {
    int ws_fd; // Websocket fd, we use this value as unique 'id'
    int tcp_fd; // Remote server socket
} t_client;

typedef struct {
    unsigned int client_nb;
    t_client client_socket[MAX_BRIDGE_CONNECTIONS];
    const char *remote_ip;
    const char *remote_port;
} t_ws_bridge;

// polling struct : contain fds to poll and client associated fd
typedef struct {
        struct pollfd pfds[MAX_BRIDGE_CONNECTIONS];
        int sock_fd[MAX_BRIDGE_CONNECTIONS];
} t_wsb_poll_struct;

/* Global variables */

static t_ws_bridge wsb;

static bool dead = false;

/* Signaling stuff to stop the server */

static void intHandler()
{
    dead = true;
}
/* ----------------- */

/* Wrapper functions */

static int server_write(int fd, const char* msg, int msgSize)
{
    int n;

    DBG_STR(msg);
    n = write(fd, msg, msgSize);
    if(n == -1)
        printf("[%s:%d] Error : failed to write to the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
    return n;
}

static int server_read(int fd, char* buffer,int bufferSize)
{
    int n;

    memset(buffer, 0, bufferSize);
    n = read(fd,buffer,bufferSize-1);
    if(n == -1)
        printf("[%s:%d] Error : failed to read from the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
    DBG_STR(buffer);
    return n;
}
/* ----------------- */

/* Add/Delete client functions */
static int add_client(int sock_fd)
{
    unsigned int i;

    if(wsb.client_nb >= MAX_BRIDGE_CONNECTIONS)
    {
        printf("[%s:%d] Error : Maximum number of clients already connected.\n", __FUNCTION__, __LINE__);
        return 1;
    }

    // Find the first free element into the list
    for(i=0; i<MAX_BRIDGE_CONNECTIONS; i++)
    {
        if(wsb.client_socket[i].ws_fd == -1)
        {
            wsb.client_nb++;
            wsb.client_socket[i].ws_fd = sock_fd;
            printf("[%s:%d] Info : new client with id %d is connected.\n", __FUNCTION__, __LINE__, sock_fd);
            printf("[%s:%d] Info : %d client(s) connected.\n", __FUNCTION__, __LINE__, wsb.client_nb);

            return 0;
        }
    }

    printf("[%s:%d] Error : Unknown error.\n", __FUNCTION__, __LINE__);

    return 1;
}

static int set_client_remote_fd(int cli_id, int sock_fd)
{
    unsigned int i;

    // Find fd to delete into wsb struct
    for(i=0; i<MAX_BRIDGE_CONNECTIONS; i++)
    {
        if(wsb.client_socket[i].ws_fd == cli_id)
        {
            wsb.client_socket[i].tcp_fd = sock_fd;
            return 0;
        }
    }

    printf("[%s:%d] Error : client to set remote socket not found.\n", __FUNCTION__, __LINE__);
    return 1;
}

static int delete_client(int sock_fd)
{
    unsigned int i;

    // Find fd to delete into wsb struct
    for(i=0; i<MAX_BRIDGE_CONNECTIONS; i++)
    {
        if(wsb.client_socket[i].ws_fd == sock_fd)
        {
            wsb.client_nb--;
            printf("[%s:%d] Info : client connection with id %d is terminated.\n", __FUNCTION__, __LINE__, sock_fd);
            printf("[%s:%d] Info : %d client(s) connected.\n", __FUNCTION__, __LINE__, wsb.client_nb);
            wsb.client_socket[i].ws_fd = -1;
            wsb.client_socket[i].tcp_fd = -1;
            return 0;
        }
    }

    printf("[%s:%d] Error : client to delete not found.\n", __FUNCTION__, __LINE__);
    return 1;
}


/* Remote server thread handler */
static void *remote_server_thread_handler(void *arg)
{
    int num_events, ws_frame_size;
    unsigned int nb, i, rd_bytes;
    unsigned int events_found = 0;
    char server_msg[1024], data[1024];
    t_wsb_poll_struct ps;

    // Clear/Init all file descriptors
    for(i=0; i< MAX_BRIDGE_CONNECTIONS; i++)
    {
        ps.pfds[i].fd = -1;
    }

    while(!dead)
    {
        // Refresh list of fds to poll
        nb = 0;
        for(i=0; nb<wsb.client_nb; i++)
        {
            if(wsb.client_socket[i].tcp_fd != -1)
            {
                ps.pfds[nb].fd = wsb.client_socket[i].tcp_fd;
                ps.pfds[nb].events = POLLIN | POLLPRI;
                ps.sock_fd[nb] = wsb.client_socket[i].ws_fd;
                nb++;
            }
        }

        // Poll the client sockets without blocking the thread
        num_events = poll(ps.pfds, wsb.client_nb, 0);

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
                if(ps.pfds[i].revents & POLLIN)
                {
                    // Clear recv buffers
                    server_msg[0] = '\0';
                    data[0] = '\0';

                    // Receive message from remote server
                    rd_bytes = server_read(ps.pfds[i].fd, server_msg, sizeof(server_msg));

                    // Server connection disconnected or Error => send conn close msg to Websocket client
                    if(rd_bytes <= 0)
                    {
                        server_write(ps.sock_fd[i], WEBSOCKET_CONN_CLOSE, sizeof(WEBSOCKET_CONN_CLOSE));
                        delete_client(ps.sock_fd[i]);
                        close(ps.pfds[i].fd);
                        close(ps.sock_fd[i]);
                        ps.pfds[i].fd = -1;
                    }
                    else
                    {
#if DEBUG
                        printf("[%s:%d] remote server send msg to client id=%d : %s\n", __FUNCTION__, __LINE__, ps.sock_fd[i], server_msg);
#endif
                        // Encode msg to Websocket format
                        ws_frame_size = WEBSOCKET_set_content(server_msg, sizeof(server_msg), (unsigned char*) data, sizeof(data));

                        // Send message to the client
                        if(ws_frame_size > 0)
                        {
                            server_write(ps.sock_fd[i], data, ws_frame_size);
                        }
                        else
                        {
                            printf("[%s:%d] Error : WebSocket msg to send is empty.\n", __FUNCTION__, __LINE__);
                        }
                    }
                    events_found++;
                }
            }
        }
    }
    printf("[%s:%d] Exit websocket thread\n", __FUNCTION__, __LINE__);

    return NULL;
}

/* Websocket thread handler */
static void *websocket_thread_handler(void *arg)
{
    int num_events, ret;
    unsigned int nb, i;
    unsigned int events_found = 0;
    char client_ws_msg[1024], data[1024];
    t_wsb_poll_struct ps;

    // Clear/Init all file descriptors
    for(i=0; i< MAX_BRIDGE_CONNECTIONS; i++)
    {
        ps.pfds[i].fd = -1;
    }

    while(!dead)
    {
        // Refresh list of fds to poll
        nb = 0;
        for(i=0; nb<wsb.client_nb; i++)
        {
            if(wsb.client_socket[i].ws_fd != -1)
            {
                ps.pfds[nb].fd = wsb.client_socket[i].ws_fd;
                ps.pfds[nb].events = POLLIN | POLLPRI;
                ps.sock_fd[nb] = wsb.client_socket[i].tcp_fd;
                nb++;
            }
        }

        // Poll the client sockets without blocking the thread
        num_events = poll(ps.pfds, wsb.client_nb, 0);

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
                if(ps.pfds[i].revents & POLLIN)
                {
                    // Clear recv buffers
                    client_ws_msg[0] = '\0';
                    data[0] = '\0';

                    // Receive Ws message from client
                    server_read(ps.pfds[i].fd, client_ws_msg, sizeof(client_ws_msg));
                    ret = WEBSOCKET_get_content(client_ws_msg, sizeof(client_ws_msg), (unsigned char*) data, sizeof(data));

                    // Client disconnected => close also the socket with the remote server
                    if(ret == -2)
                    {
                        delete_client(ps.pfds[i].fd);
                        close(ps.pfds[i].fd);
                        close(ps.sock_fd[i]);
                        ps.pfds[i].fd = -1;
                    }
                    else if(ret == -1)
                    {
                        printf("[%s:%d] Unknown error get webSocket content\n", __FUNCTION__, __LINE__);
                    }
                    else
                    {
#if DEBUG
                        printf("[%s:%d] client id=%d Receive msg : %s\n", __FUNCTION__, __LINE__, ps.pfds[i].fd, data);
#endif
                        // Send the msg to the remote server
                        if(ps.sock_fd[i] != -1)
                        {
                            server_write(ps.sock_fd[i], data, strlen(data));
                        }
                    }
                    events_found++;
                }
            }
        }
    }
    printf("[%s:%d] Exit websocket thread\n", __FUNCTION__, __LINE__);

    return NULL;
}


/* Remote TCP server connect */
static int connect_to_remote_server(int *sockfd)
{
    int rv;
    struct addrinfo hints, *servinfo, *p;

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd < 0)
    {
        printf("[%s:%d] Error : failed to create the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv = getaddrinfo(wsb.remote_ip, wsb.remote_port, &hints, &servinfo)) != 0)
    {
        printf("[%s:%d] Error : getaddrinfo(%s, %s, ...) failed (%s).\n", __FUNCTION__, __LINE__, wsb.remote_ip, wsb.remote_port, gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            printf("[%s:%d] Error : failed to create the socket (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
            continue;
        }

        if(connect(*sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*sockfd);
            printf("[%s:%d] Error : connect() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
            continue;
        }
        break;
    }

    if(!p)
    {
        printf("[%s:%d] Error : failed to connect to the remote server.\n", __FUNCTION__, __LINE__);
        return 1;
    }

    freeaddrinfo(servinfo);

    return 0;
}

/* Function to wait client */
static int server_wait_client(int sockfd)
{
    int cli_fd;
    int remote_fd = -1;
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    char buffer[1024], response[1024];

    cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if(cli_fd == -1)
    {
        printf("[%s:%d] Error : accept() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    server_read(cli_fd, buffer, sizeof(buffer));

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
    server_write(cli_fd, response, strlen(response));

    // Add client to the list
    if(add_client(cli_fd) != 0)
        goto err;

    // No we can connect the client to the remote server
    if(connect_to_remote_server(&remote_fd) != 0)
    {
        delete_client(cli_fd);
        goto err;
    }

    set_client_remote_fd(cli_fd, remote_fd);

    return 0;

err:
    // Close connection
    server_write(cli_fd, WEBSOCKET_CONN_CLOSE, sizeof(WEBSOCKET_CONN_CLOSE));
    close(cli_fd);
    return 1;
}


int main(int argc, const char *argv[])
{
    int i, bridge_sockfd;
    int option_Value = 1;
    struct sockaddr_in server;
    pthread_t ws_thread;
    pthread_t tcp_thread;

    // Check parameters
    if(argc != 4)
    {
        printf("Usage : %s  local_port remote_ip remote_port\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Init Server struct, clear all fds
    wsb.client_nb = 0;
    for(i=0; i<MAX_BRIDGE_CONNECTIONS; i++)
    {
        wsb.client_socket[i].ws_fd = -1;
        wsb.client_socket[i].tcp_fd = -1;
    }

    // Remote TCP server to connect
    wsb.remote_ip = argv[2];
    wsb.remote_port = argv[3];

    // Create TCP socket dedicated to the bridge server
    bridge_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(bridge_sockfd < 0)
    {
        printf("[%s:%d] Error : failed to create the socket server (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // Make the socket instantly reusable
    if(setsockopt(bridge_sockfd, SOL_SOCKET, SO_REUSEADDR, &option_Value, sizeof(option_Value)) == -1)
    {
        printf("[%s:%d] Error : failed to set the server socket as reusable (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

    // Try to bind the server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

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

    // Start thread to handle websocket client msg
    if(pthread_create(&ws_thread , NULL, websocket_thread_handler, NULL) < 0)
    {
        printf("[%s:%d] Error : pthread_create() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }
    printf("[%s:%d] Info : WebSocket thread created.\n", __FUNCTION__, __LINE__);

    // Start thread to handle remote server msg
    if(pthread_create(&tcp_thread , NULL, remote_server_thread_handler, NULL) < 0)
    {
        printf("[%s:%d] Error : pthread_create() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }
    printf("[%s:%d] Info : Remote server thread created.\n", __FUNCTION__, __LINE__);

    // Loop : wait for clients
    while(!dead)
    {
        server_wait_client(bridge_sockfd);
    }

    // Waiting thread terminating
    if(pthread_join(tcp_thread, NULL) != 0)
    {
        printf("[%s:%d] Error : pthread_join() failed (%s).\n", __FUNCTION__, __LINE__, strerror(errno));
        return 1;
    }

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

    printf("[%s:%d] Info : Succesfully exited program\n", __FUNCTION__, __LINE__);

    return 0;
}
