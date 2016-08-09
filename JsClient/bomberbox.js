var connection;

window.onload = function() {

    var canvas = document.getElementById('bbb_canvas');
    if(!canvas) {
        alert("Impossible de récupérer le canvas");
        return;
    }

    var context = canvas.getContext('2d');
    if(!context) {
        alert("Impossible de récupérer le context du canvas");
        return;
    }
}

function server_connect() {
    var form = document.getElementById("bbb_settings");
    var server = form.server_ip.value;
    var port = form.server_port.value;
    var name = form.player_name.value;
    console.log(name);

    connection = new WebSocket('ws://'+server+':'+port);

    connection.onmessage = function () {
        console.log("ws message!");
    }

    connection.onerror = function () {
        console.log("ws error!");
    }

    connection.onopen = function () {
        console.log("ws open!");
        connection.send(name + '\0');
    };

    connection.onclose = function () {
        console.log("ws closed!");
    };
}

function send_msg() {
    console.log("send TEST");
    connection.send("TEST" + '\0');
}

