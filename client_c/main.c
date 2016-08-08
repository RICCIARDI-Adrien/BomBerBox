#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <SDL/SDL.h>

#include "display.h"

/** server command definition **/
enum _server_command {
    SRV_CMD_CONNECT =   0x2,
    SRV_CMD_INPUT   =   0x3,
};
typedef enum _server_command server_command_t;

/** server message definition **/
struct _server_message {
    uint8_t cmd;
    char buffer[63];
}__attribute__ ((packed)) ;
typedef struct _server_message server_message_t;

/** client command definition **/
enum _client_command {
    CLT_CMD_DISPLAY_TILE    =   0x0,
    CLT_CMD_DISPLAY_STR     =   0x1,
};
typedef enum _client_command client_command_t;

/** client message definition **/
struct _client_message {
    uint8_t cmd;
    char buffer[254];
}__attribute__ ((packed)) ;
typedef struct _client_message client_message_t;

//--------------------------------------------------------------------
// PROTOTYPES
//--------------------------------------------------------------------
int game_process(int sockfd);
int connect_server(const char * server_addr, const char * server_port);

//--------------------------------------------------------------------
// PUBLIC API
//--------------------------------------------------------------------
int main(int argc, const char *argv[])
{
    int rc, fd;

    if ( argc < 3 ) {
        fprintf(stderr, "usage");
        exit(1);
    }

    fd = connect_server(argv[1], argv[2]);
    if ( fd < 0 ) {
        fprintf(stderr, "cannot connect to server %s:%d", argv[1], atoi(argv[2]));
        exit(1);
    }
  
    rc = display_init();
    if ( rc < 0 ) {
        fprintf(stderr, "cannot init display");
        exit(1);
    }

    game_process(fd);

    close(rc);

    return 0;
}

//--------------------------------------------------------------------
int connect_server(const char * server_addr, const char * server_port)
{
    int sockfd, numbytes;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if ( ! server_addr || ! server_port) {
        fprintf(stderr,"usage: client hostname\n");
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_addr, server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if ( ! p ) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

//--------------------------------------------------------------------
int game_process(int sockfd)
{
    int rc, numbytes;
    char command;
    uint8_t buffer[255];
    server_message_t srv_msg;
    SDL_Event event;

    // sanity check
    if ( sockfd <= 0 ) {
        return -1;
    }

    printf("Enter your name: ");
    srv_msg.cmd = SRV_CMD_CONNECT;
    rc = scanf("%62s", &srv_msg.buffer[0]);
    if ( rc <= 0 ) {
        fprintf(stderr, "cannot get username\n");
        return -1;
    }

    rc = send(sockfd, (char *)&srv_msg, sizeof(server_message_t), 0);
    if ( rc == -1 ) {
        perror("send");
        return -1;
    }

    while (1) {
        rc = SDL_PollEvent(&event);
        if ( rc ) {
            if ( event.type == SDL_KEYDOWN )
            {
                SDLKey keyPressed = event.key.keysym.sym;
                srv_msg.cmd = SRV_CMD_INPUT;

                switch (keyPressed)
                {
                    case SDLK_UP:
                        srv_msg.buffer[0] = 0;
                        break;
                    case SDLK_DOWN:
                        srv_msg.buffer[0] = 1;
                        break;
                    case SDLK_LEFT:
                        srv_msg.buffer[0] = 2;
                        break;
                    case SDLK_RIGHT:
                        srv_msg.buffer[0] = 3;
                        break;
                    case SDLK_SPACE:
                        srv_msg.buffer[0] = 4;
                        break;
                    case SDLK_ESCAPE:
                        exit(1);
                        break;
                }
                
                rc = send(sockfd, (char *)&srv_msg, sizeof(uint16_t), 0);
            }
        }

        numbytes = recv(sockfd, &command, 1, MSG_DONTWAIT);
        if ( numbytes < 0 ) {
            if ( errno == EAGAIN || errno == EWOULDBLOCK ) continue;
            perror("recv");
            return -1;
        }
       

 
        switch(command) {
            case CLT_CMD_DISPLAY_TILE:
            {
                recv(sockfd, &buffer[0], 3, 0);
                display_tile(buffer[0], 25 + buffer[2] * 32, 87 + buffer[1] * 32);
                break;
            }
            case CLT_CMD_DISPLAY_STR:
            {
                printf("read string\n");
                recv(sockfd, &buffer[0], 1, 0);
                printf("read %d\n", buffer[0]);
                recv(sockfd, &buffer[0], buffer[0], 0);
                printf("%s\n", buffer);
                break;
            }
            default:
            {
                continue;
            }
        }
    }
}

