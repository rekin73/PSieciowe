#!/usr/bin/env python3

# Ten przykład należy czytać po wcześniejszym przeanalizowaniu wersji w C.

import selectors, socket, sys

rot13_table = bytes.maketrans(
        b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
        b"nopqrstuvwxyzabcdefghijklmNOPQRSTUVWXYZABCDEFGHIJKLM")

class Encryptor:

    def __init__(self, slctor, sock):
        slctor.register(sock, selectors.EVENT_READ, data=self)
        self.selector = slctor
        self.socket = sock

    def read(self):
        data = self.socket.recv(1024)
        if len(data) == 0:
            self.close()
        else:
            self.socket.send(data.translate(rot13_table))

    def close(self):
        self.selector.unregister(self.socket)
        self.socket.close()

class Listener:

    def __init__(self, slctor, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind( ("0.0.0.0", port) )
        sock.listen(10)
        slctor.register(sock, selectors.EVENT_READ, data=self)
        self.selector = slctor
        self.socket = sock

    def read(self):
        conn, addr = self.socket.accept()
        Encryptor(self.selector, conn)

    def close(self):
        self.selector.unregister(self.socket)
        self.socket.close()

# ---- kod poniżej byłby w main(), gdyby ten program był napisany w C

if len(sys.argv) <= 1:
    sys.stderr.write('Usage: %s port...\n' % sys.argv[0])
    sys.exit(1)

slctor = selectors.DefaultSelector()

for arg in sys.argv[1:]:
    Listener(slctor, int(arg))

while True:
    ready_list = slctor.select(timeout=180)
    if len(ready_list) == 0:
        break
    for key, event in ready_list:
        if event | selectors.EVENT_READ > 0:
            obj = key.data
            obj.read()

keys = [ x for x in slctor.get_map().values() ]
for key in keys:
    key.data.close()
slctor.close()
