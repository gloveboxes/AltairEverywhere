// https://www.eclipse.org/paho/files/jsdoc/index.html

(async function () {
  "use strict";
  var mqtt_client_id = "";
  var sub_topic = "";
  var pub_topic_control = "";
  var pub_topic = "";
  var pub_topic_paste = "";
  var channelId = "";
  var channelSet = false;
  var client = null;
  var character_mode = false;

  const term = new Terminal({
    cursorBlink: "block",
    fontFamily: 'Courier New',
    fontSize: 18,
    rows: 30,
    cols: 140,
  });

  var curr_line = "";

  term.open(document.getElementById("terminal"));

  const userReq = await fetch("/.auth/me");
  const { clientPrincipal } = await userReq.json();

  term.write(`WELCOME TO ALTAIR TERMINAL ${clientPrincipal.userDetails}\r\n\r\n`);

  const params = new URLSearchParams(window.location.search);

  function getRandomInt(max) {
    return Math.floor(Math.random() * Math.floor(max));
  }

  const getChannelId = async (msg) => {
    const words = msg.trim().split(" ");

    if (words.length == 2) {
      const url = `api/getchannelid?displayname=${words[1]}`;
      term.write("\r\n\r\nCONNECTING...");
      const response = await fetch(url);
      term.write("\r\n");

      if (response.ok) { // if HTTP-status is 200-299
        const json = await response.json();

        if (json['DesiredChannelId']) {

          channelId = json.DesiredChannelId;

          channelSet = true;
          generateTopics();
          mqttInit();

        }

      } else {
        alert("HTTP-Error: " + response.status);
      }
    } else {
      alert("Expected HELLO followed by Azure Sphere Device ID");
      term.write("\r\n");
    }
  }

  function generateTopics() {
    mqtt_client_id = `${channelId}`
    sub_topic = `altair/${channelId}/web`;
    pub_topic = `altair/${channelId}/dev`;
    pub_topic_control = `altair/${channelId}/dev/ctrl`;
    pub_topic_paste = `altair/${channelId}/dev/paste`
  }

  async function mqttInit() {
    // generateTopics();
    const res = await fetch("/api/mqtt-settings");
    const mqttCredentials = await res.json();

    // https://stackoverflow.com/questions/53051679/how-can-i-use-tls-with-paho-mqtt-over-javascript
    client = new Paho.MQTT.Client(
      mqttCredentials.broker,
      parseInt(mqttCredentials.brokerPort, 10) || 8083,
      "",
      "altair" + new Date().getTime()
    );

    // set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    // connect the client
    // https://www.eclipse.org/paho/files/jsdoc/index.html
    client.connect({
      keepAliveInterval: 30,
      mqttVersion: 4,
      cleanSession: true,
      useSSL: true,
      onSuccess: onConnect,
      userName: mqttCredentials.userName,
      password: mqttCredentials.password,
    });
  }

  function sendControl(msg) {
    if (channelSet) {
      const message = new Paho.MQTT.Message(msg);
      message.destinationName = pub_topic_control;
      client.send(message);
    }
  }

  // called when the client connects
  function onConnect() {
    console.log("onConnect");
    client.subscribe(sub_topic);
    document.getElementById("connectState").innerHTML = "MQTT Broker: Connected";
  }

  // called when the client loses its connection
  function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
      console.log("onConnectionLost:" + responseObject.errorMessage);
      document.getElementById("connectState").innerHTML = "MQTT Broker: Not connected";
      alert("Connection lost to the MQTT broker. Refresh page to reconnect");
    }
  }

  // called when a message arrives
  function onMessageArrived(message) {
    console.log("onMessageArrived:" + message.payloadString);
    term.write(message.payloadString);
  }

  function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  term.prompt = (msg) => {
    if (curr_line.toUpperCase().startsWith("HELLO")) {
      if (!channelSet) {
        getChannelId(curr_line);
        curr_line = "";
      }
    } else if (curr_line.toUpperCase().startsWith("BYE")) {
      sleep(3000).then(() => (window.location.href = "/.auth/logout"));
    } else {
      if (channelSet) {
        const message = new Paho.MQTT.Message(msg);
        message.destinationName = pub_topic;
        client.send(message);
      }
    }
  };

  term.prompt("");

  if (character_mode) {
    document.getElementById("inputmode").innerHTML = "Character input mode : Wordmaster (Ctrl+L to toggle)";
  } else {
    document.getElementById("inputmode").innerHTML = "Line input mode: Default (Ctrl+L to toggle)";
  }

  term.on("key", function (key, ev) {

    if (ev.ctrlKey) {    // ctrl keys
      if (ev.keyCode === 76) {   // hook ctrl L as toggel between line mode and wordmaster mode
        character_mode = !character_mode;
        if (character_mode) {
          document.getElementById("inputmode").innerHTML = "Character input mode : Wordmaster (Ctrl+L to toggle)";
        } else {
          document.getElementById("inputmode").innerHTML = "Line input mode: Default (Ctrl+L to toggle)";
        }
        return;
      }
      sendControl(String.fromCharCode(ev.keyCode));
      return;
    }

    if (ev.keyCode === 27) { // escape
      term.prompt(key);
      term.write(key);
      return;
    }

    if (character_mode) {
      switch (ev.keyCode) {
        case 39:  // cursor right
          sendControl(String.fromCharCode(68)); // ctrl d
          return;
        case 37:  // cursor left
          sendControl(String.fromCharCode(83)); // ctrl s
          return;
        case 38:  // cursor up
          sendControl(String.fromCharCode(69)); // ctrl e
          return;
        case 40:  // cursor down
          sendControl(String.fromCharCode(88)); // ctrl x
          return;
        case 45:  // insert toggle
          sendControl(String.fromCharCode(79)); // ctrl o
          return;
        case 46:  // delete
          sendControl(String.fromCharCode(71)); // ctrl g
          return;
        case 13:  // Enter
          term.prompt("\r");
          curr_line = "";
          return;
        case 45:  // Insert
          sendControl(String.fromCharCode(79));
          return;
        case 8:  // Backspace
          sendControl(String.fromCharCode(72));
          return;
        default:
          curr_line += key;
          term.prompt(key);
          term.write(key);
          return;
      }
    }

    if (!character_mode) {

      switch (ev.keyCode) {
        case 39:  // cursor right
          break;
        case 37:  // cursor left
          break;
        case 38:  // cursor up
          break;
        case 40:  // cursor down
          break;
        case 13:  // Enter
          curr_line += "\r";
          term.prompt(curr_line);
          term.write("\r");
          curr_line = "";
          return;
        case 8:   // Backspace
          if (curr_line) {
            curr_line = curr_line.slice(0, curr_line.length - 1);
            term.write("\b \b");
          }
          return;
        default:
          curr_line += key;
          term.write(key);
      }
    }
  });

  function chunkSubstr(str, size) {
    const numChunks = Math.ceil(str.length / size)
    const chunks = new Array(numChunks)

    for (let i = 0, o = 0; i < numChunks; ++i, o += size) {
      chunks[i] = str.substr(o, size)
    }
    return chunks
  }

  // paste value
  term.on("paste", function (data) {
    if (character_mode) { return; }

    if (data.length < 256) {
      curr_line += data;
      term.write(data);
      return;
    }

    data += "\r"
    var chunks = chunkSubstr(data, 256);
    chunks.forEach(element => {
      if (channelSet) {
        const message = new Paho.MQTT.Message(element);
        message.destinationName = pub_topic_paste;
        client.send(message);
        term.write(element);
      }
    });
    curr_line = "";
  });

  if (params.has('deviceid')) {
    var msg = `hello ${params.get('deviceid')}`;
    getChannelId(msg);
  }
})();
