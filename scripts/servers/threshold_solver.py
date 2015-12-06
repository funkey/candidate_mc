#!/usr/bin/python

from pycmc import *
import json
import zmq
import sys

def read_crag(project_file):

    global crag
    global edge_features
    global seg_to_node
    global node_to_seg
    global id_service_socket

    seg_to_node = {}
    node_to_seg = {}

    crag = Crag()
    crag_store = Hdf5CragStore(project_file)
    crag_store.retrieveCrag(crag)

    edge_features = EdgeFeatures(crag)
    crag_store.retrieveEdgeFeatures(crag, edge_features)

    # initially, segment ids are node ids
    max_id = 0
    for n in crag.nodes():
        id = crag.id(n)
        seg_to_node[id] = id
        node_to_seg[id] = id
        max_id = max(id, max_id)
    print "Read CRAG with max id " +  str(max_id)

    id_service_socket.send(json.dumps({
        "type": "reset"
    }))
    id_service_socket.recv()
    id_service_socket.send(json.dumps({
        "type": "set_used_range",
        "data": { "begin": 0, "end": max_id }
    }))
    id_service_socket.recv()

def read_edge_rf(project_file):

    global edge_rf

    edge_rf = RandomForest()
    edge_rf.read(project_file, "classifiers/edge_rf");

def process(msg):

    print "Received message of type " + msg["type"]

    if msg["type"] == "handshake":
        handshake()
    elif msg["type"] == "merge":
        merge(msg["data"]["fragments"])
    elif msg["type"] == "separate":
        separate(msg["data"]["fragments"])
    elif msg["type"] == "request":
        segment()
    else:
        client_socket.send(json.dumps({
            "type": "unknown-request",
            "data": {
                "message": msg
            }
        }))

def handshake():

    segment()

def merge(fragment_ids):

    global id_service_socket
    global crag

    # update CRAG
    id_service_socket.send(json.dumps({
        "type": "create_ids",
        "data": {
            "number" : 1
        }
    }))
    new_id = json.loads(id_service_socket.recv())["data"]["ids"][0]

    print "Creating merge node with id " + str(new_id)
    n = crag.addNode()
    node_to_seg[crag.id(n)] = new_id
    seg_to_node[new_id] = crag.id(n)

    for fragment_id in fragment_ids:
        child = crag.nodeFromId(seg_to_node[fragment_id])
        crag.addSubsetArc(child, n)

    client_socket.send(json.dumps({
        "type": "ack"
    }))

def separate(fragment_ids):

    print "Adding separation edge"

    if len(fragment_ids) != 2:
        client_socket.send(json.dumps({
            "type": "error",
            "data": { "message" : "Separation edges can only be added between two fragments" }
        }))
        return

    u = crag.nodeFromId(seg_to_node[fragment_ids[0]])
    v = crag.nodeFromId(seg_to_node[fragment_ids[1]])
    crag.addAdjacencyEdge(u, v, Crag.SeparationEdge)

    client_socket.send(json.dumps({
        "type": "ack"
    }))

def train():
    # TODO
    # 1. harvest training data:
    #   take all leaf edges that have to be linked/switched of
    # 2. train RF on these examples
    return

def segment():
    # 1. predict edge scores
    # 2. threshold
    # 3. find connected components

    print "Creating segmentation"

    global edge_features
    global id_service_socket

    solution = CragSolution(crag)

    # all leaf nodes are part of the solution
    for n in crag.nodes():
        if crag.isLeafNode(n):
            solution.setSelected(n, True)

    # threshold leaf node edges
    for e in crag.edges():
        if crag.isLeafEdge(e):
            prob = edge_rf.getProbabilities(edge_features[e])[1]
            if prob > 0.5:
                solution.setSelected(e, True)
            else:
                solution.setSelected(e, False)

    # get connected component ids
    connected_components = set([])
    for n in crag.nodes():
        if crag.isLeafNode(n):
            connected_components.add(solution.label(n))

    # map connected component ids to unique ids
    id_service_socket.send(json.dumps({
        "type": "create_ids",
        "data": {
            "number" : len(connected_components)
        }
    }))
    new_ids = json.loads(id_service_socket.recv())["data"]["ids"]
    cc_to_seg = {}
    for (cc_id, seg_id) in zip(connected_components, new_ids):
        cc_to_seg[cc_id] = seg_id

    # create final LUT from fragment ids to segment ids
    fragments = []
    segments  = []
    for n in crag.nodes():
        if crag.isLeafNode(n):
            fragments.append(node_to_seg[crag.id(n)])
            segments.append(cc_to_seg[solution.label(n)])

    client_socket.send(json.dumps({
        "type": "fragment-segment-lut",
        "data": {
            "fragments" : fragments,
            "segments"  : segments
        }
    }))

def store_crag():
    # TODO
    return

def main():

    global client_socket
    global id_service_socket

    port = 8128
    project_file = sys.argv[1]
    id_service_url = sys.argv[2]

    print "Starting server at port " + str(port)
    context = zmq.Context.instance()
    client_socket = context.socket(zmq.REP)
    client_socket.bind("tcp://*:" + str(port))

    id_service_socket = context.socket(zmq.REQ)
    id_service_socket.connect(id_service_url)

    read_crag(project_file)
    read_edge_rf(project_file)

    while True:
        print "Waiting for messages..."
        process(json.loads(client_socket.recv()))

if __name__ == "__main__":
    main()
