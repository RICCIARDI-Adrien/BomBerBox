/* Websocket */
var ws;

/* canvas context */
var canvas;

/* client command definition */
var action_type = {
    NW_ACTION_DISPLAY_TILE : 0x0,
    NW_ACTION_DISPLAY_STR  : 0x1
};

/* server command definition */
var command_type = {
    NW_COMMAND_CONNECT : 0x2,
    NW_COMMAND_INPUT   : 0x3
};

/* img associated with tiles */
var tile_img = {
    GAME_TILE_ID_EMPTY : "sprites/ground.png",
    GAME_TILE_ID_WALL : "sprites/block_solid.png",
    GAME_TILE_ID_DESTRUCTIBLE_OBSTACLE : "sprites/block_explodable.png",
    GAME_TILE_ID_CURRENT_PLAYER : "sprites/ninja_black.png",
    GAME_TILE_ID_OTHER_PLAYER : "sprites/creep.png",
    GAME_TILE_BOMB : "sprites/bomb.png",
    GAME_TILE_EXPLOSION : "sprites/flame.png",
    GAME_TILE_ITEM_GHOST : "",
    GAME_TILE_ITEM_SHIELD : "sprites/item_shield.png",
    GAME_TILE_ITEM_POWER_UP_BOMB_RANGE : "sprites/item_flame_powerup.png",
    GAME_TILE_ITEM_POWER_UP_BOMBS_COUNT : "sprites/item_bomb_powerup.png"

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

    document.addEventListener('keydown', keyboard_event);
}

function bbb_console_log(str) {
    var textarea = document.getElementById("bbb_console");
    textarea.value += "> " + str + "\n";
    textarea.scrollTop = textarea.scrollHeight;
}

function canvas_print_tile(tid, x, y) {
    var img = new Image();
    img.src = tile_img[Object.keys(tile_img)[tid]];
    console.log(img.src + "x=" + x +" y=" + y);

    img.onload = function() {
        ctx.drawImage(img, x, y);
    };
}

function send_kbd_msg(code) {
    if (typeof ws != 'undefined') {
        if(ws.readyState == ws.OPEN) {
            var command = String.fromCharCode(command_type.NW_COMMAND_INPUT);
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

    tile_img.GAME_TILE_ID_CURRENT_PLAYER = "sprites/" + avatar + ".png";

    bbb_console_log("Trying to connect to BomBerBox server.");

    ws = new WebSocket('ws://'+server+':'+port);
    form.connect_btn.disabled = true;


    ws.onmessage = function (evt) {
        var i = 0;
        do {
            if (evt.data[i] == String.fromCharCode(action_type.NW_ACTION_DISPLAY_STR)) {
                bbb_console_log(evt.data.substring(i+2, i+2+evt.data.charCodeAt(i+1)));
                i += evt.data.charCodeAt(i+1) + 2;
            } else if(evt.data[i] == String.fromCharCode(action_type.NW_ACTION_DISPLAY_TILE)) {
                var tid = evt.data.charCodeAt(i+1);
                var x = evt.data.charCodeAt(i+3) * 32;
                var y = evt.data.charCodeAt(i+2) * 32;
                i += 4;
                canvas_print_tile(tid, x, y);
            } else {
                // Bad message: check next byte
                console.log("bad msg!");
                i++;
            }
        } while (i < evt.data.length);
    }

    ws.onerror = function () {
        console.log("ws error!");
    }

    ws.onopen = function () {
        console.log("ws open!");
        var command = String.fromCharCode(command_type.NW_COMMAND_CONNECT);
        ws.send(command + name + '\0');
    };

    ws.onclose = function (evt) {
        console.log("ws closed!");
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
