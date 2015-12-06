#!/usr/bin/python

import json
import zmq

def process(msg):

    global next_id

    print "Received message of type " + msg["type"]

    if msg["type"] == "create_ids":

        n = msg["data"]["number"]
        ids = range(next_id, next_id + n)
        next_id += n

        socket.send(json.dumps({
            "type": "ids",
            "data": {
                "ids" : ids
            }
        }))

    elif msg["type"] == "set_used":

        next_id = max(next_id, max(msg["data"]["ids"]) + 1)
        socket.send(json.dumps({
            "type": "ack"
        }))

    elif msg["type"] == "set_used_range":

        next_id = max(next_id, msg["data"]["end"])
        socket.send(json.dumps({
            "type": "ack"
        }))

    elif msg["type"] == "reset":

        next_id = 0
        socket.send(json.dumps({
            "type": "ack"
        }))

    else:
        socket.send(json.dumps({
            "type": "unknown-request",
            "data": {
                "message": msg
            }
        }))

def main():

    global socket
    global next_id

    port = 8129
    next_id = 0

    print "Starting ID server at port " + str(port)
    context = zmq.Context.instance()
    socket = context.socket(zmq.REP)
    socket.bind("tcp://*:" + str(port))

    while True:
        print "Waiting for messages..."
        process(json.loads(socket.recv()))

if __name__ == "__main__":
    main()

