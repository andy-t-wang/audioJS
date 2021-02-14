function load_script(script_path) {
    /* Create and append a new script */
    var new_script = document.createElement('script');
    new_script.type = 'text/javascript';
    new_script.src = script_path;
    document.getElementsByTagName('head')[0].appendChild(new_script);
    return new_script;
  }

load_script('/static/puffer/js/puffer.js').onload = function() {
    var ws_client = new WebSocketClient(
      session_key, username, settings_debug, port, csrf_token, sysinfo);

    channel_bar.on_channel_change = function(new_channel) {
      ws_client.set_channel(new_channel);
    };

    /* configure play button's onclick event */
    var play_button = document.getElementById('tv-play-button');
    play_button.onclick = function() {
      ws_client.set_channel(channel_bar.get_curr_channel());
      play_button.style.display = 'none';
    };

    ws_client.connect(channel_bar.get_curr_channel());
  };