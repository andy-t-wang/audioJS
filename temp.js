
var audioCodec = "audio/webm; codecs=\"opus\"";
function check_buffer(){
    console.log(audio.readyState);
}

window.onload = function(e){
    // request();
    audio = document.getElementById('audio');
    if (window.MediaSource) {
        var mediaSource = new MediaSource();
        audio.src = URL.createObjectURL(mediaSource);
        mediaSource.addEventListener('sourceopen', sourceOpen);
        console.log('done');
    } else {
        console.log("The Media Source Extensions API is not supported.")
    }
}
function request(){
    const Http = new XMLHttpRequest();
    const url='http://127.0.0.1:43952/';
    Http.open("GET", url);
    Http.send();
    Http.onprogress = (e) => {
        console.log(Http.response)
    }
}
setInterval(request(), 100);
function sourceOpen(e) {
    var audio = document.getElementById('audio');
    URL.revokeObjectURL(audio.src);
    var mime = 'audio/webm; codecs="opus"';
    var mediaSource = e.target;
    var sourceBuffer = mediaSource.addSourceBuffer(mime);
    var videoUrl = 'http://127.0.0.1:8080';
    fetchAB(videoUrl, function (buf) {
        sourceBuffer.addEventListener('updateend', function (_) {
          mediaSource.endOfStream();
          console.log(mediaSource.activeSourceBuffers);
          // will contain the source buffer that was added above,
          // as it is selected for playing in the video player
          video.play();
          //console.log(mediaSource.readyState); // ended
        });
        sourceBuffer.appendBuffer(buf);
    });
}
function fetchAB (url, cb) {
    var xhr = new XMLHttpRequest;
    xhr.open('get', url);
    xhr.onprogress = function () {
        // cb(xhr.response);
        console.log(xhr.readyState);
        console.log(xhr.responseText);
    };
    xhr.send();
  };

