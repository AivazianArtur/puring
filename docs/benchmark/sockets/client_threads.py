import socket, threading, time
from config import *
from metrics import record, report

def worker():

    s = socket.socket()
    s.connect((HOST, PORT))

    for _ in range(MESSAGES):
        t0 = time.perf_counter()

        s.sendall(PAYLOAD)

        received = 0
        while received < MSG_SIZE:
            received += len(s.recv(MSG_SIZE))

        record(t0)

    s.close()


threads = []

time.sleep(WARMUP)

start = time.perf_counter()

for _ in range(CONNECTIONS):
    t = threading.Thread(target=worker)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

elapsed = time.perf_counter() - start

report(
    "threads",
    CONNECTIONS * MESSAGES,
    elapsed
)
