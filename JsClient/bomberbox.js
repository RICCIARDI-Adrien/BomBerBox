/* Websocket */
var ws;

/* canvas context */
var canvas;

/* tile set png */
var tile_set;

/* client command definition */
var action_type = {
    ACTION_DISPLAY_TILE : 0x0,
    ACTION_DISPLAY_STR  : 0x1
};

/* server command definition */
var command_type = {
    COMMAND_CONNECT : 0x2,
    COMMAND_INPUT   : 0x3
};

/* tile item coordonates */
var tile_xy = {
    TILE_ID_GROUND : {x: 192, y: 0},
    TILE_ID_WALL : {x: 64, y: 0},
    TILE_ID_DESTRUCTIBLE_WALL : {x: 32, y: 0},
    TILE_ID_CURRENT_PLAYER : {x: 128, y:32},
    TILE_ID_OTHER_PLAYER : {x: 128, y: 0},
    TILE_BOMB : {x: 96, y: 0},
    TILE_FLAME : {x: 160, y: 0},
    TILE_PLAYER_SHIELD : {x: 224, y: 0},
    TILE_ITEM_SHIELD : {x: 64, y: 32},
    TILE_ITEM_POWER_UP_BOMB_RANGE : {x: 32, y: 32},
    TILE_ITEM_POWER_UP_BOMBS_COUNT : {x: 0, y: 32}
};

/* Keyboard input value */
var kbd = {
    UP : 0x1,
    DOWN : 0x2,
    LEFT : 0x3,
    RIGHT : 0x4,
    SPACE : 0x5,
    ESCAPE : 0x6,
};

window.onload = function() {

    var canvas = document.getElementById('bbb_canvas');
    if(!canvas) {
        alert("Impossible de récupérer le canvas");
        return;
    }

    ctx = canvas.getContext('2d');
    if(!ctx) {
        alert("Impossible de récupérer le context du canvas");
        return;
    }

    tile_set = new Image();
    tile_set.src = "sprites/tile.png"

    document.addEventListener('keydown', keyboard_event);
}

function bbb_console_log(str) {
    var textarea = document.getElementById("bbb_console");
    textarea.value += "> " + str + "\n";
    textarea.scrollTop = textarea.scrollHeight;
}

function canvas_print_tile(tid, x, y) {
    var tile_x = tile_xy[Object.keys(tile_xy)[tid]].x;
    var tile_y = tile_xy[Object.keys(tile_xy)[tid]].y;
    //console.log("tile x=" + tile_x + " y=" + tile_y + " x=" + x +" y=" + y);
    ctx.drawImage(tile_set, tile_x, tile_y, 32, 32, x, y, 32, 32);
}

function send_kbd_msg(code) {
    if (typeof ws != 'undefined') {
        if(ws.readyState == ws.OPEN) {
            var command = String.fromCharCode(command_type.COMMAND_INPUT);
            ws.send(command + String.fromCharCode(code) + '\0');
        }
    }
}

function keyboard_event(event) {
    if(event.keyCode == 37) {
        // Left was pressed
        send_kbd_msg(kbd.LEFT);
    }
    else if(event.keyCode == 38) {
        // Up was pressed
        send_kbd_msg(kbd.UP);
    }
    else if(event.keyCode == 39) {
        // Right was pressed
        send_kbd_msg(kbd.RIGHT);
    }
    else if(event.keyCode == 40) {
        // Down was pressed
        send_kbd_msg(kbd.DOWN);
    }
    else if(event.keyCode == 27) {
        // Escape was pressed
        send_kbd_msg(kbd.ESCAPE);
    }
    else if(event.keyCode == 32) {
        // Space was pressed
        send_kbd_msg(kbd.SPACE);
    }

}

function server_connect() {
    var form = document.getElementById("bbb_settings");
    var server = form.server_ip.value;
    var port = form.server_port.value;
    var name = form.player_name.value;
    var avatar = form.player_avatar.value;

    if (avatar == "ninja_red") {
        tile_xy.TILE_ID_CURRENT_PLAYER.x = 192;
        tile_xy.TILE_ID_CURRENT_PLAYER.y = 32;
    }
    else if (avatar == "ninja_blue") {
        tile_xy.TILE_ID_CURRENT_PLAYER.x = 160;
        tile_xy.TILE_ID_CURRENT_PLAYER.y = 32;
    }
    else if (avatar == "alien") {
        tile_xy.TILE_ID_CURRENT_PLAYER.x = 0;
        tile_xy.TILE_ID_CURRENT_PLAYER.y = 0;
    }

    bbb_console_log("Trying to connect to BomBerBox server.");

    ws = new WebSocket('ws://'+server+':'+port);
    form.connect_btn.disabled = true;


    ws.onmessage = function (evt) {
        var i = 0;
        do {
            if (evt.data[i] == String.fromCharCode(action_type.ACTION_DISPLAY_STR)) {
                bbb_console_log(evt.data.substring(i+2, i+2+evt.data.charCodeAt(i+1)));
                i += evt.data.charCodeAt(i+1) + 2;
            } else if(evt.data[i] == String.fromCharCode(action_type.ACTION_DISPLAY_TILE)) {
                var tid = evt.data.charCodeAt(i+1);
                var x = evt.data.charCodeAt(i+3) * 32;
                var y = evt.data.charCodeAt(i+2) * 32;
                i += 4;
                canvas_print_tile(tid, x, y);
            } else {
                // Bad message: check next byte
                //console.log("bad msg!");
                i++;
            }
        } while (i < evt.data.length);
    }

    ws.onerror = function () {
        //console.log("ws error!");
    }

    ws.onopen = function () {
        //console.log("ws open!");
        var command = String.fromCharCode(command_type.COMMAND_CONNECT);
        ws.send(command + name + '\0');
    };

    ws.onclose = function (evt) {
        //console.log("ws closed!");
        // abnormal socket closure, maybe handcheck failed ...
        if (evt.code == 1006) {
            // try to reconnect
            setTimeout(function(){server_connect()}, 500);
        }
        else {
            form.connect_btn.disabled = false;
        }
    };
}
