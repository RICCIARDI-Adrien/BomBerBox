/* Websocket */
var ws;

/* canvas and his context */
var canvas;
var ctx;

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
    GAME_TILE_ITEM_SHIELD : "",
    GAME_TILE_ITEM_POWER_UP_BOMB_RANGE : "",
    GAME_TILE_ITEM_POWER_UP_BOMBS_COUNT : ""

};

window.onload = function() {

    canvas = document.getElementById('bbb_canvas');
    if(!canvas) {
        alert("Impossible de récupérer le canvas");
        return;
    }

    ctx = canvas.getContext('2d');
    if(!ctx) {
        alert("Impossible de récupérer le context du canvas");
        return;
    }

    ctx.font = "22px Comic Sans MS";
    ctx.textAlign = "center";
}

function canvas_print_text(str) {
    ctx.fillText(str, canvas.width/2, canvas.height/2);
}

function canvas_print_tile(tid, x, y)
{
    var img = new Image();
    img.src = tile_img[Object.keys(tile_img)[tid]];

    img.onload = function() {
        ctx.drawImage(img, x, y);
    };
}

function canvas_clear()
{
    ctx.clearRect(0, 0, canvas.width, canvas.height);
}



function server_connect() {
    var form = document.getElementById("bbb_settings");
    var server = form.server_ip.value;
    var port = form.server_port.value;
    var name = form.player_name.value;
    var avatar = form.player_avatar.value;

    tile_img.GAME_TILE_ID_CURRENT_PLAYER = "sprites/" + avatar + ".png";

    ws = new WebSocket('ws://'+server+':'+port);
    form.connect_btn.disabled = true;


    ws.onmessage = function (evt) {
        //console.log("ws rcv msg: " + evt.data);
        if (evt.data[0] == String.fromCharCode(action_type.NW_ACTION_DISPLAY_STR)) {
            canvas_print_text(evt.data.substring(1));
        } else if(evt.data[0] == String.fromCharCode(action_type.NW_ACTION_DISPLAY_TILE)) {
            var tid = evt.data.charCodeAt(1);
            var x = evt.data.charCodeAt(3) * 32;
            var y = evt.data.charCodeAt(2) * 32;
            canvas_print_tile(tid, x, y);
        }
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
