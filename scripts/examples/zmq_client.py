#!/usr/bin/python

import json
import zmq
import sys

port=8128
if len(sys.argv) > 1:
    port = sys.argv[1]

name="alice"
if len(sys.argv) > 2:
    name = sys.argv[2]

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:" + str(port))

while True:
    msg = { "type": "copy_me", "content": name }
    socket.send(json.dumps(msg))
    rpl = json.loads(socket.recv())
    print "Received reply " + rpl["content"]
