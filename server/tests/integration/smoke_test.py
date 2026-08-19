#!/usr/bin/env python3
import argparse
import socket
import struct
import sys
import time
import uuid

MAX_BODY_SIZE = 1024 * 1024
Envelope = None
PING_REQUEST = 1
DATABASE_HEALTH_REQUEST = 3


def frame(message_type, envelope):
    body = envelope.SerializeToString()
    return struct.pack("!II", len(body), message_type) + body


def read_frame(sock):
    header = read_exact(sock, 8)
    body_length, message_type = struct.unpack("!II", header)
    if body_length == 0 or body_length > MAX_BODY_SIZE:
        raise AssertionError("invalid response length")
    body = read_exact(sock, body_length)
    envelope = Envelope()
    envelope.ParseFromString(body)
    return message_type, envelope


def read_exact(sock, size):
    result = bytearray()
    while len(result) < size:
        chunk = sock.recv(size - len(result))
        if not chunk:
            raise AssertionError("server closed connection before complete frame")
        result.extend(chunk)
    return bytes(result)


def ping(request_id, text):
    request = Envelope(protocol_version=1, request_id=request_id)
    request.ping_request.text = text
    return request


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8888)
    parser.add_argument("--proto-build-dir", required=True)
    args = parser.parse_args()

    sys.path.insert(0, args.proto_build_dir)
    global Envelope
    try:
        from im_protocol_pb2 import Envelope as GeneratedEnvelope
    except ImportError as exc:
        raise SystemExit("Run with --proto-build-dir pointing to a built server directory") from exc
    Envelope = GeneratedEnvelope

    with socket.create_connection((args.host, args.port), timeout=5) as sock:
        split_id = "split-" + str(uuid.uuid4())
        split = frame(PING_REQUEST, ping(split_id, "split"))
        for byte in split:
            sock.send(bytes([byte]))
        message_type, response = read_frame(sock)
        assert message_type == 2
        assert response.request_id == split_id
        assert response.ping_response.text == "split"
        assert response.ping_response.server_time_unix_ms > 0

        first = frame(PING_REQUEST, ping("first", "one"))
        second = frame(PING_REQUEST, ping("second", "two"))
        sock.sendall(first + second)
        _, first_response = read_frame(sock)
        _, second_response = read_frame(sock)
        assert first_response.request_id == "first"
        assert second_response.request_id == "second"

        health = Envelope(protocol_version=1, request_id="health")
        health.database_health_request.SetInParent()
        sock.sendall(frame(DATABASE_HEALTH_REQUEST, health))
        _, health_response = read_frame(sock)
        assert health_response.database_health_response.database_name
        print("smoke test passed")


if __name__ == "__main__":
    main()
