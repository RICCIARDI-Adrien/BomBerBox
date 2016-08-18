#define NW_ACTION_DATA_SIZE 254


/** client command definition **/
enum _nw_action_type {
    NW_ACTION_DISPLAY_TILE    =   0x0,
    NW_ACTION_DISPLAY_STR     =   0x1,
};
typedef enum _nw_action_type nw_action_type_t;

/** client message definition **/
struct _nw_action {
    uint8_t type;
    uint8_t data[NW_ACTION_DATA_SIZE];      // TODO: must be an union
};

typedef struct _nw_action nw_action_t;

/** server command definition **/
enum _nw_command_type {
    NW_COMMAND_CONNECT =   0x2,
    NW_COMMAND_INPUT   =   0x3,
};
typedef enum _nw_command_type nw_command_type_t;

enum _nw_cmd_input_value {
    NW_COMMAND_INPUT_UP = 0x1,
    NW_COMMAND_INPUT_DOWN,
    NW_COMMAND_INPUT_LEFT,
    NW_COMMAND_INPUT_RIGHT,
    NW_COMMAND_INPUT_SPACE,
    NW_COMMAND_INPUT_ESCAPE,
};
typedef enum _ui_cmd_input_value ui_cmd_input_value_t;

int nw_get_action(nw_action_t * action);
int nw_connect(const char * server_addr, const char * server_port);
int nw_send_command(nw_command_type_t type, void * data, size_t size);
