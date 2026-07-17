import { WebSocketServer } from 'ws';
import net from 'net';

const wss = new WebSocketServer({ port: 3000 });

console.log("WebSocket Bridge running on ws://localhost:3000");

wss.on('connection', (ws) => {
    console.log("Browser connected to bridge.");

    // Open a raw TCP connection to your C++ engine
    const tcpClient = new net.Socket();
    tcpClient.connect(8080, '127.0.0.1', () => {
        console.log("Bridge connected to C++ Engine.");
    });

    // Forward messages from Browser -> Bridge -> C++
    ws.on('message', (message) => {
        const payload = message.toString().padEnd(64, ' ');
        tcpClient.write(payload);
    });

    // Forward messages from C++ -> Bridge -> Browser
    tcpClient.on('data', (data) => {
        if (ws.readyState === 1) { // 1 = WebSocket.OPEN
            const lines = data.toString().split('\n');
            for (const line of lines) {
                if (line.trim().length > 0) {
                    ws.send(line.trim());
                }
            }
        }
    });

    // Handle disconnects safely
    ws.on('close', () => tcpClient.destroy());
    tcpClient.on('error', (err) => {
        console.error("C++ Engine connection error:", err.message);
        ws.close();
    });
});
