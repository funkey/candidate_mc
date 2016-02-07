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

    # callback for bundle method, concave part of objective
    def valueGradientR(self, w, value, gradient):

        print [ x for x in w ]

        self.socket.send(chr(EVALUATE_R_RES) + json.dumps({ "x" : [ x for x in w] }))
        reply = self.socket.recv()

        t = ord(reply[0])
        if t != CONTINUATION_REQ:
            raise "Error: expected client to send CONTINUATION_REQ (" + str(CONTINUATION_REQ) + ", sent " + str(t) + " instead"

        data = json.loads(reply[1:])
        value.v = data["value"]
        for i in range(self.dims):
            gradient[i] = data["gradient"][i]

    # callback for bundle method, convex part of objective
    def valueGradientP(self, w, value, gradient):

        print [ x for x in w ]

        self.socket.send(chr(EVALUATE_P_RES) + json.dumps({ "x" : [ x for x in w], "eps" : self.bundle_method.getEps() }))
        reply = self.socket.recv()

        t = ord(reply[0])
        if t != CONTINUATION_REQ:
            raise "Error: expected client to send CONTINUATION_REQ (" + str(CONTINUATION_REQ) + ", sent " + str(t) + " instead"

        data = json.loads(reply[1:])
        value.v = data["value"]
        for i in range(self.dims):
            gradient[i] = data["gradient"][i]

    def run(self):

        print "Setting up zmq socket"

        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind("tcp://*:4711")

        print "Waiting for client..."

        request = self.socket.recv()

        t = ord(request[0])

        if t != INITIAL_REQ:
            raise "Error: expected client to send INITIAL_REQ (" + str(INITIAL_REQ) + ", sent " + str(t) + " instead"

        data = json.loads(request[1:])
        self.dims = data["dims"]

        print "Got initial request for optimization with " + str(self.dims) + " variables"

        parameters = BundleOptimizerParameters()

        if data.has_key("parameters"):

            if data["parameters"].has_key("lambda"):
                parameters.lambada = data["parameters"]["lambda"]
            if data["parameters"].has_key("steps"):
                parameters.steps = data["parameters"]["steps"]
            if data["parameters"].has_key("min_eps"):
                parameters.min_eps = data["parameters"]["min_eps"]
            if data["parameters"].has_key("eps_strategy"):
                if data["parameters"]["eps_strategy"] == "eps_from_gap":
                    parameters.eps_strategy = EpsStrategy.EpsFromGap
                elif data["parameters"]["eps_strategy"] == "eps_from_change":
                    parameters.eps_strategy = EpsStrategy.EpsFromChange
                else:
                    raise "Unknown eps strategy: " + str(data["parameters"]["eps_strategy"])

        self.bundle_method = BundleOptimizer(parameters)

        # start bundle method (which will start sending a response, finishes 
        # receiving a request)
        w = PyOracleWeights(self.dims)

        if data.has_key("initial_x"):
            for i in range(len(w)):
                w[i] = data["initial_x"][i]

        result = self.bundle_method.optimize(self.oracle, w)

        if result == BundleOptimizerResult.ReachedMinGap:
            print "Optimal solution found at " + str([x for x in w])
            result = "reached_min_eps"
        elif result == BundleOptimizerResult.ReachedMaxSteps:
            print "Maximal number of iterations reached"
            result = "reached_max_steps"
        else:
            print "Optimal solution NOT found"
            result = "error"

        self.socket.send(chr(FINAL_RES) + json.dumps({
            "x" : [x for x in w],
            "value" : self.bundle_method.getMinValue(),
            "eps" : self.bundle_method.getEps(),
            "status" : result
        }))

if __name__ == "__main__":

    pycmc.setLogLevel(pycmc.LogLevel.Debug)

    bms = BundleMethodServer()
    bms.run()
