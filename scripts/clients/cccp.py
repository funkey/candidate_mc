#!/usr/bin/python

import zmq
import json

INITIAL_REQ      = 0

CONTINUATION_REQ = 1

EVALUATE_P_RES   = 2

EVALUATE_R_RES   = 4

FINAL_RES        = 3

class Client:

    def __init__(self):

        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect("tcp://127.0.0.1:4711")
        print("connected to server")

    def send_message(self, message_type, message_payload):

        message = json.dumps({
                'type': message_type,
                'payload': message_payload
        })
        print("sending " + message)
        self.socket.send(message.encode('ascii'))

    def receive_message(self):

        message = json.loads(self.socket.recv().decode())
        return (message['type'], message['payload'])

    def run(self):

        print("sending initial request")
        self.send_message(INITIAL_REQ, { "dims" : 1, "initial_x" : [ 00.1 ], "parameters" : { "lambda" : 0.0001 } })

        while True:

            print("waiting for reply")
            (message_type, data) = self.receive_message()

            if message_type == EVALUATE_P_RES:
                self.evaluate_P([x for x in data["x"]])
            if message_type == EVALUATE_R_RES:
                self.evaluate_R([x for x in data["x"]])
            if message_type == FINAL_RES:
                break

    def evaluate_P(self, w):

        # P(x) = max(max(max(x,-x),2*x-2),-2*x-2)

        x = w[0]
        if abs(x) >= 2:
            value = 2*abs(x) - 2
            gradient = [ 2 ]
        else:
            value = abs(x)
            gradient = [ 1 ]
        if x < 0:
            gradient[0] = -gradient[0]

        self.send_message(CONTINUATION_REQ, { "value" : value, "gradient" : [ x for x in gradient] })

    def evaluate_R(self, w):

        # R(x) = min(-1.5*x,1.5*x)

        x = w[0]
        value = -1.5*abs(x)
        gradient = [ -1.5 ]
        if x < 0:
            gradient[0] = -gradient[0]

        self.send_message(CONTINUATION_REQ, { "value" : value, "gradient" : [ x for x in gradient] })

if __name__ == "__main__":
    client = Client()
    client.run()
