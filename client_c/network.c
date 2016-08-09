#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "network.h"

#define NW_COMMAND_BUFFER_SIZE  255

/** server message definition **/
struct _nw_message {
    uint8_t cmd;
    char buffer[NW_COMMAND_BUFFER_SIZE];
}__attribute__ ((packed)) ;
typedef struct _nw_message nw_message_t;

//--------------------------------------------------------------------
// PROTOTYPES
//--------------------------------------------------------------------
static int sockfd;

//--------------------------------------------------------------------
int nw_connect(const char * server_addr, const char * server_port)
{
    int numbytes;  
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

    return 0;
}

//--------------------------------------------------------------------
int nw_send_command(nw_command_type_t type, void * data, size_t size)
{
    int rc;
    nw_message_t msg;

    // sanity check
    if ( ! data || ! size ) {
        return -1;
    }

    if ( type == NW_COMMAND_CONNECT ) {
        if ( size > NW_COMMAND_BUFFER_SIZE )
            size = NW_COMMAND_BUFFER_SIZE ;

        msg.cmd = NW_COMMAND_CONNECT;
        memcpy(msg.buffer, data, size);
        ++size; // add command size
    } else if ( type == NW_COMMAND_INPUT ) {
        msg.cmd = NW_COMMAND_INPUT;
        msg.buffer[0] = ((uint8_t*)data)[0];
        size = 2;
    }

    rc = send(sockfd, (char *)&msg, size, 0);
    if ( rc == -1 ) {
        perror("send");
    }

    return rc;
}

//--------------------------------------------------------------------
int nw_get_action(nw_action_t * action)
{
    uint8_t len;
    int numbytes;

    // sanity check
    if ( ! action ) {
        return -1;
    }

    numbytes = recv(sockfd, &action->type, 1, MSG_DONTWAIT);
    if ( numbytes <= 0 ) {
        if ( errno == EINVAL || errno == EWOULDBLOCK ) {
            return 0;
        }

        perror("recv");
        return -1;
    }
  
    switch(action->type) {
        case NW_ACTION_DISPLAY_TILE:
            recv(sockfd, &action->data[0], 3, 0);
            break;
        case NW_ACTION_DISPLAY_STR:
            recv(sockfd, &len, 1, 0);
            recv(sockfd, &action->data[0], len, 0);
            break;
        default:
            return -1;
    }

    return 1;
}
