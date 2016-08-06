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
    var name = form.server_port.player_name;

    var connection = new WebSocket('ws://'+server+':'+port);

    connection.onopen = function () {
        connection.send(player_name);
    };
}
