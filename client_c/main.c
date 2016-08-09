#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "ui.h"
#include "network.h"

//--------------------------------------------------------------------
// PRIVATE API
//--------------------------------------------------------------------
int game_process(void)
{
    int rc;
    ui_event_t event;
    nw_action_t action;

    while (1) {
        // process user interface
        rc = ui_event(&event);
        if ( rc > 0 ) {
            if ( event.type == UI_EVENT_KEYBOARD ) {
                // process keyboard event
                _game_keyboard_ingame(event.value);
            }
        }

        // process server commands
        rc = nw_get_action(&action);
        if ( rc > 0 ) {
            if ( action.type == NW_ACTION_DISPLAY_TILE ) {
                ui_tile(action.data[0], 25 + action.data[2] * 32, 87 + action.data[1] * 32);
            } else if ( action.type == NW_ACTION_DISPLAY_STR ) {
                ui_text(action.data);
            }
        }

        usleep(100);
    }
}

//--------------------------------------------------------------------
int _game_keyboard_ingame(ui_keyboard_value_t value)
{
    int rc = 0;
    uint8_t data;

    switch(value) {
        case UI_KEYBOARD_UP:
            data = NW_COMMAND_INPUT_UP;
            rc = nw_send_command(NW_COMMAND_INPUT, &data, 1);
            break;
        case UI_KEYBOARD_DOWN:
            data = NW_COMMAND_INPUT_DOWN;
            rc = nw_send_command(NW_COMMAND_INPUT, &data, 1);
            break;
        case UI_KEYBOARD_LEFT:
            data = NW_COMMAND_INPUT_LEFT;
            rc = nw_send_command(NW_COMMAND_INPUT, &data, 1);
            break;
        case UI_KEYBOARD_RIGHT:
            data = NW_COMMAND_INPUT_RIGHT;
            rc = nw_send_command(NW_COMMAND_INPUT, &data, 1);
            break;
        case UI_KEYBOARD_SPACE:
            data = NW_COMMAND_INPUT_SPACE;
            rc = nw_send_command(NW_COMMAND_INPUT, &data, 1);
            break;
        case UI_KEYBOARD_ESCAPE:
            // TODO: currently dirty exit
            exit(1);
            break;
        default:
            break;
    }

    return rc;
}


//--------------------------------------------------------------------
// PUBLIC API
//--------------------------------------------------------------------
int main(int argc, const char *argv[])
{
    int rc, fd;

    if ( argc < 4 ) {
        fprintf(stderr, "usage");
        exit(1);
    }

    rc = nw_connect(argv[1], argv[2], argv[3]);
    if ( rc ) {
        fprintf(stderr, "cannot connect to server %s:%d", argv[1], atoi(argv[2]));
        exit(1);
    }
  
    rc = ui_init();
    if ( rc < 0 ) {
        fprintf(stderr, "cannot init ui");
        exit(1);
    }
    
    // TODO: must be done as a part of game process
    rc = nw_send_command(NW_COMMAND_CONNECT, (char *)argv[3], strlen(argv[3]));

    rc = game_process();

    return rc;
}

