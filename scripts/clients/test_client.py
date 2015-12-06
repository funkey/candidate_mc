#!/usr/bin/python

import json
import zmq
import sys

def test(socket, msg):

    socket.send(json.dumps(msg))
    rpl = json.loads(socket.recv())
    print "Received reply " + str(rpl)

port=8128

context = zmq.Context()
solver_socket = context.socket(zmq.REQ)
solver_socket.connect("tcp://localhost:" + str(port))
id_service_socket = context.socket(zmq.REQ)
id_service_socket.connect("tcp://localhost:" + str(port + 1))

test(solver_socket,
    {
        "type": "handshake"
    })

test(solver_socket,
    {
        "type": "merge",
        "data": {
            "fragments": [0, 1]
        }
    })
