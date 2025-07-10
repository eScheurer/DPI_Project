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
  <title>Crisis-Chatt</title>

  <style>
    body { 
      font-family: 
      sans-serif; 
      max-width:600px; 
      margin:20px auto; 
      background-color: lightsteelblue;
      }
    h1 {
      margin-bottom: 4px;
    }
    h2 {
      font-size: 1em;
      margin-bottom: 20px;
      margin-top: 0px;
    }
    #history { 
      border:1px solid #ccc; 
      height:260px; 
      overflow-y:scroll; 
      padding:5px; 
      margin-bottom:10px; 
      border-radius: 4px;
      background-color: whitesmoke;
    }
    input { 
      width:80%; 
      padding:8px; 
      font-size:1em; 
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
      background-color: whitesmoke;
    }
    form {
      margin-bottom: 10px;
    }
    button { 
      padding:8px 16px; 
      font-size:1em;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      transition: background-color 0.3s ease; 
      background-color: cornflowerblue;
    }
    button:hover {
      background-color: royalblue;
    }
    mark {
      background-color: lightpink;
      font-weight: bold;
    }
  </style>


  <script>
    let allMessages = [] // safes all msgs, for searching

    // ruft alle 2s die letzten Nachrichten ab und aktualisiert das Chat-Fenster
    async function fetchMessages() {
      try {
        const res = await fetch('/messages'); 
        if (!res.ok) return;
        const data = await res.json();

        allMessages = data;
        const currentlySearching = document.getElementById('search').value.trim();

        if (currentlySearching){
          searchMessages();
        } else {
          const container = document.getElementById('history');
          container.innerHTML = data.map(m => '<div>'+m+'</div>').join('');
          container.scrollTop = container.scrollHeight;
        }
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
    
    // searches input in text msgs
    async function searchMessages() {
      const searchText = document.getElementById('search').value.toLowerCase();
      const result = allMessages.filter(m => m.toLowerCase().includes(searchText));

      const box = document.getElementById('history');
      box.innerHTML = result.map(m => '<div>'+ highlight(m, searchText) +'</div>').join('');
      box.scrollTop = box.scrollHeight;
    }

    // highlights search hits
    function highlight(text, search) {
      const escaped = search.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'); 
      const regex = new RegExp(escaped, 'gi'); //gi means global&case-insensitive
      return text.replace(regex, match => `<mark>${match}</mark>`);
    }

    // clears search bar
    function clearSearch() {
      const input = document.getElementById('search');
      input.value = '';
      fetchMessages(); // so that msgs are shown again
    }

    // Event-Handler für Formular und Intervall-Updates
    window.addEventListener('load', () => {
      document.getElementById('chatForm').onsubmit = sendMessage;
      fetchMessages(); setInterval(fetchMessages, 2000);
    });
  </script>

</head>


<body>
  <h1>Crisis-Chat</h1>
    <h2>(a Project from the DPI Lecture FS25)</h2>
  <div id="history"></div>
  <form id="chatForm">
    <input id="msg" autocomplete="off" placeholder="Enter message..." />
    <button type="submit">Send</button>
  </form>
    <input id="search" placeholder="search in chat.." oninput="searchMessages()">
    <button type="button" onclick="clearSearch()">X</button>
</body>
</html>
)rawliteral";

#endif // CHAT_HTML_H
