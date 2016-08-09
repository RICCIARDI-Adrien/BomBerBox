
//--------------------------------------------------------------------
// TYPES
//--------------------------------------------------------------------
/** redefine key pressed to cut off from interface **/
enum _ui_keyboard_value {
    UI_KEYBOARD_UP,
    UI_KEYBOARD_DOWN,
    UI_KEYBOARD_LEFT,
    UI_KEYBOARD_RIGHT,
    UI_KEYBOARD_SPACE,
    UI_KEYBOARD_ESCAPE,
};

typedef enum _ui_keyboard_value ui_keyboard_value_t;

/** kind of event received (keyboard, mouse, ...) **/
enum _ui_event_type {
    UI_EVENT_KEYBOARD,
};

typedef enum _ui_event_type ui_event_type_t;

/** type and value of received event **/
struct _ui_event {
    ui_event_type_t type;
    ui_keyboard_value_t  value; // TODO: must an union
};

typedef struct _ui_event ui_event_t;

//--------------------------------------------------------------------
// PUBLIC API
//--------------------------------------------------------------------
int ui_init(void);
int ui_text(char * str);
int ui_tile(int tid, int x, int y);
int ui_event(ui_event_t * event);
