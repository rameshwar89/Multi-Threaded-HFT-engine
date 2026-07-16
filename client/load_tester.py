import socket
import threading
import random
import time

def fire_orders(count):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('127.0.0.1', 8080))
        
        for _ in range(count):
            side = random.choice(['B', 'S'])
            qty = random.randint(10, 1000)
            price = random.randint(1000, 90000)
            
            payload = f"{side} {qty} {price}".ljust(64).encode('utf-8')
            s.sendall(payload)
            
        s.close()
    except Exception as e:
        print(f"Connection failed: {e}")

threads = []
start_time = time.time()

for _ in range(10):
    t = threading.Thread(target=fire_orders, args=(100000,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print(f"Sent 1,000,000 orders in {time.time() - start_time} seconds")
