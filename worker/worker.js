const redis = require('redis');
const mysql = require('mysql2/promise');

async function start() {
    const db = await mysql.createConnection({
        host: 'localhost',
        user: 'hft_user',
        password: 'password',
        database: 'hft'
    });

    const subscriber = redis.createClient();
    await subscriber.connect();

    let tradeBatch = [];

    // Subscribe to the C++ Engine
    await subscriber.subscribe('trades', (message) => {
        const parts = message.split(' ');
        tradeBatch.push([parts[0], parseInt(parts[1]), parseInt(parts[2])]);
        
        if (tradeBatch.length >= 1000) {
            flushBatch();
        }
    });

    // Flush every 1 second in case we don't hit 1,000 trades
    setInterval(() => {
        if (tradeBatch.length > 0) flushBatch();
    }, 1000);

    async function flushBatch() {
        const copy = [...tradeBatch];
        tradeBatch = []; // instantly clear so it can keep filling
        
        const query = "INSERT INTO trades (side, price, quantity) VALUES ?";
        await db.query(query, [copy]);
        console.log(`Flushed ${copy.length} trades to MySQL`);
    }

    console.log("Worker listening to C++ Engine...");
}

start();
