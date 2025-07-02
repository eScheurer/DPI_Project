#ifndef CHAT_HTML_H
#define CHAT_HTML_H

#include <Arduino.h>

// HTML-Seite für das Webinterface
static const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Chat</title>
  <style>
    body { font-family: sans-serif; max-width:600px; margin:20px auto; }
    #history { border:1px solid #ccc; height:200px; overflow-y:scroll; padding:5px; margin-bottom:10px; }
    input { width:80%; padding:8px; font-size:1em; }
    button { padding:8px 16px; font-size:1em; }
  </style>
  <script>
    // ruft alle 2s die letzten Nachrichten ab und aktualisiert das Chat-Fenster
    async function fetchMessages() {
      try {
        const res = await fetch('/messages'); if (!res.ok) return;
        const data = await res.json();
        const container = document.getElementById('history');
        container.innerHTML = data.map(m => '<div>'+m+'</div>').join('');
        container.scrollTop = container.scrollHeight;
      } catch (e) { console.error('Fetch error', e); }
    }
    // sendet neue Nachricht an den Server
    async function sendMessage(e) {
      e.preventDefault();
      const input = document.getElementById('msg');
      const text = input.value.trim(); if (!text) return;
      await fetch('/send',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'msg='+encodeURIComponent(text)
      });
      input.value = '';
      await fetchMessages();
    }
    // Event-Handler für Formular und Intervall-Updates
    window.addEventListener('load', () => {
      document.getElementById('chatForm').onsubmit = sendMessage;
      fetchMessages(); setInterval(fetchMessages, 2000);
    });
  </script>
</head>
<body>
  <h1>ESP32 Chat</h1>
  <div id="history"></div>
  <form id="chatForm">
    <input id="msg" autocomplete="off" placeholder="Enter message..." />
    <button type="submit">Send</button>
  </form>
</body>
</html>
)rawliteral";

#endif // CHAT_HTML_H
