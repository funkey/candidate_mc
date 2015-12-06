#!/usr/bin/python

import pycmc
import json
import zmq
import time
import sys
import threading

global socket

def process(socket, msg):
    print "Received message of type " + msg["type"]
    #time.sleep(1)
    if msg["type"] == "copy_me":
        # send back the content
        socket.send(json.dumps({"type": "copy", "content": msg["content"]}))

def worker_routine(id, worker_url):

    context = zmq.Context.instance()

    socket = context.socket(zmq.REP)
    socket.connect(worker_url)

    while True:
        print str(id) + " Waiting for messages..."
        process(socket, json.loads(socket.recv()))

def main():

    port=8128
    if len(sys.argv) > 1:
        port = sys.argv[1]

    print "Starting server at port " + str(port)
    context = zmq.Context.instance()

    clients = context.socket(zmq.ROUTER)
    clients.bind("tcp://*:" + str(port))

    workers = context.socket(zmq.DEALER)
    workers.bind("inproc://workers")

    for i in range(10):
        thread = threading.Thread(target=worker_routine, args=(i, "inproc://workers"))
        thread.start()

    zmq.device(zmq.QUEUE, clients, workers)

if __name__ == "__main__":
    main()
