#ifndef HTML_PAGE_H
#define HTML_PAGE_H

#include <pgmspace.h>

// HTML + JavaScript als PROGMEM
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Chat</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; max-width: 600px; margin: 20px auto; }
    #messages { border: 1px solid #ccc; padding: 10px; height: 300px; overflow-y: scroll; }
    #messages div { margin-bottom: 8px; }
    form { display: flex; margin-top: 10px; }
    input { flex: 1; padding: 8px; font-size: 1em; }
    button { padding: 8px 16px; font-size: 1em; }
  </style>
  <script>
    async function sendMessage() {
      const input = document.getElementById('msg');
      const text = input.value.trim();
      if (!text) return false;
      await fetch('/send', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'msg=' + encodeURIComponent(text)
      });
      input.value = '';
      return false;
    }

    async function fetchMessages() {
      try {
        const res = await fetch('/messages');
        const data = await res.json();
        const container = document.getElementById('messages');
        container.innerHTML = data.map(m => '<div>'+m+'</div>').join('');
        container.scrollTop = container.scrollHeight;
      } catch (e) {
        console.error('Fetch error', e);
      }
    }

    window.addEventListener('load', () => {
      document.getElementById('chatForm').onsubmit = () => sendMessage();
      fetchMessages();
      setInterval(fetchMessages, 2000);
    });
  </script>
</head>
<body>
  <h1>ESP32 Chat</h1>
  <div id="messages"></div>
  <form id="chatForm">
    <input id="msg" autocomplete="off" placeholder="Nachricht eingeben…" />
    <button type="submit">Senden</button>
  </form>
</body>
</html>
)rawliteral";

#endif // HTML_PAGE_H
