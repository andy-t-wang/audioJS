const WS_OPEN = 1;

var last_msg_recv_ts = null;
var init = true;
var port = 8080;
var hostname = "127.0.0.1";
var nonsecure = true;
const ws_host_port = hostname + ':' + port;
const ws_addr = nonsecure ? 'ws://' + ws_host_port
                            : 'wss://' + ws_host_port;
var queue = [];
var abuf = null;
var socket = null;


queue.push = function( buffer ) { 
    // in case the message comes late
    if ( !abuf.updating && this.length == 0) { 
        abuf_update();
    } else { 
        Array.prototype.push.call( this, buffer );
    } 
};

window.onload = function(e){
    socket = new WebSocket(ws_addr);

    socket.onopen = function(e) {
        console.log("[open] Connection established");
        console.log("Sending to server");
        sendClientInit();
    };

    socket.onmessage = handle_ws_msg;

    socket.onclose = function(event) {
        if (event.wasClean) {
           console.log("closed cleanly");
        } else {
            // e.g. server process killed or network down
            // event.code is usually 1006 in this case
            alert('[close] Connection died');
        }
    };

    socket.onerror = function(error) {
        console.log(error.message);
    };
};

function handle_ws_msg(e) {
    if(init){
        //server init
        init = false;
        var audio = document.getElementById('audio');
        if (window.MediaSource) {
            var mediaSource = new MediaSource();
            mediaSource.addEventListener('sourceopen', sourceOpen);
            audio.src = URL.createObjectURL(mediaSource);
            audio.load();
            audio.play();
        } else {
            console.log("The Media Source Extensions API is not supported.");
        }
    } else {
        last_msg_recv_ts = Date.now();
        // console.log(last_msg_recv_ts);
        // const msg_ts = e.timeStamp;
        const server_msg = e.data;
        queue.push(server_msg);
    }
  }

function sourceOpen(e) {
    var mime = "audio/webm; codecs=\"opus\"";
    var mediaSource = this;
    var abuf = mediaSource.addSourceBuffer(mime);
    abuf.addEventListener('abort', function(e) {
        console.log('audio source buffer abort:', e);
    });
    abuf.addEventListener('error', function(e) {
        console.log('audio source buffer error:', e);
    });
    abuf.addEventListener('updateend', function(e) {
        abuf_update();
    });
    socket.send('init success');

}

function abuf_update() {
    if (abuf && !abuf.updating && queue.length > 0) {
      var next_data= queue.shift();
      abuf.appendBuffer(next_data);
      // send back last message added to current buffer as ack 
      socket.send(JSON.stringify({cur_buf: abuf.buffered.end(0)}));
    }
  }


function sendClientInit() {
    if (!(socket && socket.readyState === WS_OPEN)) {
      return;
    }
    // some init message
    var init_id = 1;
    var msg = {
      initId: init_id,
    };
    socket.send(format_client_msg('client-init', msg));
  }
 
  
function format_client_msg(msg_type, data) {
    data.type = msg_type;
    return JSON.stringify(data);
}