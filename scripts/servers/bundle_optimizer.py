#!/usr/bin/python

import zmq
import json
import pycmc
from pycmc import BundleOptimizer, BundleOptimizerParameters, BundleOptimizerResult, BundleOptimizerEpsStrategy
from pycmc import PyOracle, PyOracleWeights

# {
#
#   "dims" : <int>, (number of dimensions)
#
#   "parameters" : (optional)
#   {
#       "lambda"  : <double>, (regularizer weight)
#       "steps"   : <int>,    (number of iterations of bundle method)
#       "min_eps" : <double>, (convergence threshold, meaning depends on
#                              eps_strategy)
#       "eps_strategy" : enum { EpsFromGap, EpsFromChange }
#   }
# }
INITIAL_REQ      = 0

CONTINUATION_REQ = 1

EVALUATE_P_RES   = 2

EVALUATE_R_RES   = 4

FINAL_RES        = 3

class BundleMethodServer:

    def __init__(self):

        # init oracle
        self.oracle = PyOracle()
        self.oracle.setValueGradientPCallback(self.valueGradientP)
        self.oracle.setValueGradientRCallback(self.valueGradientR)
        self.running = True

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

    # callback for bundle method, concave part of objective
    def valueGradientR(self, w, value, gradient):

        print([ x for x in w ])

        self.send_message(EVALUATE_R_RES, { "x" : [ x for x in w] })
        (message_type, reply) = self.receive_message()

        if message_type != CONTINUATION_REQ:
            raise RuntimeError("Error: expected client to send CONTINUATION_REQ (= " + str(CONTINUATION_REQ) + "), sent " + str(message_type) + " instead")

        value.v = reply["value"]
        for i in range(self.dims):
            gradient[i] = reply["gradient"][i]

    # callback for bundle method, convex part of objective
    def valueGradientP(self, w, value, gradient):

        print([ x for x in w ])

        self.send_message(EVALUATE_P_RES, { "x" : [ x for x in w], "eps" : self.bundle_method.getEps() })
        (message_type, reply) = self.receive_message()

        if message_type != CONTINUATION_REQ:
            raise RuntimeError("Error: expected client to send CONTINUATION_REQ (= " + str(CONTINUATION_REQ) + "), sent " + str(message_type) + " instead")

        value.v = reply["value"]
        for i in range(self.dims):
            gradient[i] = reply["gradient"][i]

    def run(self):

        print("Setting up zmq socket")

        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind("tcp://*:4711")

        print("Waiting for client...")

        (message_type, request) = self.receive_message()

        if message_type != INITIAL_REQ:
            raise RuntimeError("Error: expected client to send INITIAL_REQ (= " + str(INITIAL_REQ) + "), sent " + str(message_type) + " instead")

        self.dims = request["dims"]

        print("Got initial request for optimization with " + str(self.dims) + " variables")

        parameters = BundleOptimizerParameters()

        if 'parameters' in request:

            if 'lambda' in request["parameters"]:
                parameters.lambada = request["parameters"]["lambda"]
            if 'steps' in request["parameters"]:
                parameters.steps = request["parameters"]["steps"]
            if 'min_eps' in request["parameters"]:
                parameters.min_eps = request["parameters"]["min_eps"]
            if 'eps_strategy' in request["parameters"]:
                if request["parameters"]["eps_strategy"] == "eps_from_gap":
                    parameters.eps_strategy = EpsStrategy.EpsFromGap
                elif request["parameters"]["eps_strategy"] == "eps_from_change":
                    parameters.eps_strategy = EpsStrategy.EpsFromChange
                else:
                    raise RuntimeError("Unknown eps strategy: " + str(request["parameters"]["eps_strategy"]))

        self.bundle_method = BundleOptimizer(parameters)

        # start bundle method (which will start sending a response, finishes 
        # receiving a request)
        w = PyOracleWeights(self.dims)

        if 'initial_x' in request:
            for i in range(len(w)):
                w[i] = request["initial_x"][i]

        result = self.bundle_method.optimize(self.oracle, w)

        if result == BundleOptimizerResult.ReachedMinGap:
            print("Optimal solution found at " + str([x for x in w]))
            result = "reached_min_eps"
        elif result == BundleOptimizerResult.ReachedMaxSteps:
            print("Maximal number of iterations reached")
            result = "reached_max_steps"
        else:
            print("Optimal solution NOT found")
            result = "error"

        self.send_message(FINAL_RES, {
            "x" : [x for x in w],
            "value" : self.bundle_method.getMinValue(),
            "eps" : self.bundle_method.getEps(),
            "status" : result
        })

if __name__ == "__main__":

    pycmc.setLogLevel(pycmc.LogLevel.Debug)

    bms = BundleMethodServer()
    bms.run()
