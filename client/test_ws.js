import WebSocket from 'ws';
const ws = new WebSocket('ws://localhost:3000');
ws.on('open', () => {
    console.log("Connected. Sending order...");
    ws.send('B 10 50500');
});
ws.on('message', (data) => {
    console.log("Received:", data.toString());
    process.exit(0);
});
