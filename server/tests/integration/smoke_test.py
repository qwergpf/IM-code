#!/usr/bin/env python3

import argparse
import importlib.util
import socket
import struct
import subprocess
import sys
import tempfile
import uuid
from pathlib import Path


MAX_PAYLOAD_SIZE = 1024 * 1024


def generate_python_protocol(proto_file: Path, output_dir: Path) -> Path:
    subprocess.run(
        [
            "protoc",
            f"--proto_path={proto_file.parent}",
            f"--python_out={output_dir}",
            str(proto_file),
        ],
        check=True,
    )
    return output_dir / "im_protocol_pb2.py"


def load_protocol(module_path: Path):
    spec = importlib.util.spec_from_file_location("im_protocol_pb2", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("Could not load generated Protobuf module")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def encode_frame(envelope) -> bytes:
    payload = envelope.SerializeToString()
    if not payload or len(payload) > MAX_PAYLOAD_SIZE:
        raise ValueError("Invalid Protobuf payload length")
    return struct.pack("!I", len(payload)) + payload


def receive_exact(sock: socket.socket, size: int) -> bytes:
    chunks = []
    remaining = size
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise ConnectionError("Server closed the connection")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def receive_envelope(sock: socket.socket, protocol):
    payload_length = struct.unpack("!I", receive_exact(sock, 4))[0]
    if payload_length == 0 or payload_length > MAX_PAYLOAD_SIZE:
        raise RuntimeError(f"Invalid response length: {payload_length}")
    response = protocol.Envelope()
    response.ParseFromString(receive_exact(sock, payload_length))
    return response


def make_ping(protocol, request_id: str, text: str):
    request = protocol.Envelope(protocol_version=1, request_id=request_id)
    request.ping_request.text = text
    return request


def run_tests(host: str, port: int, protocol, expect_database_healthy: bool) -> None:
    with socket.create_connection((host, port), timeout=5) as sock:
        sock.settimeout(5)

        split_request = make_ping(protocol, f"ping-{uuid.uuid4()}", "split-frame")
        split_frame = encode_frame(split_request)
        sock.sendall(split_frame[:2])
        sock.sendall(split_frame[2:7])
        sock.sendall(split_frame[7:])
        split_response = receive_envelope(sock, protocol)
        assert split_response.request_id == split_request.request_id
        assert split_response.ping_response.text == "split-frame"
        assert split_response.ping_response.server_time_unix_ms > 0

        first = make_ping(protocol, f"ping-{uuid.uuid4()}", "sticky-one")
        second = make_ping(protocol, f"ping-{uuid.uuid4()}", "sticky-two")
        sock.sendall(encode_frame(first) + encode_frame(second))
        first_response = receive_envelope(sock, protocol)
        second_response = receive_envelope(sock, protocol)
        assert first_response.request_id == first.request_id
        assert second_response.request_id == second.request_id
        assert first_response.ping_response.text == "sticky-one"
        assert second_response.ping_response.text == "sticky-two"

        health = protocol.Envelope(
            protocol_version=1,
            request_id=f"health-{uuid.uuid4()}",
        )
        health.database_health_request.SetInParent()
        sock.sendall(encode_frame(health))
        health_response = receive_envelope(sock, protocol)
        assert health_response.request_id == health.request_id
        assert (
            health_response.database_health_response.healthy
            is expect_database_healthy
        )
        assert health_response.database_health_response.database_name
        health_message = health_response.database_health_response.message.lower()
        assert "password" not in health_message
        assert "host=" not in health_message
        assert "user=" not in health_message


def main() -> int:
    parser = argparse.ArgumentParser(description="IM server Protobuf smoke test")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=9000, type=int)
    parser.add_argument(
        "--proto-build-dir",
        default=None,
        help="Reserved for compatibility with the documented invocation",
    )
    parser.add_argument(
        "--expect-db-unhealthy",
        action="store_true",
        help="Expect DatabaseHealthResponse.healthy to be false",
    )
    args = parser.parse_args()

    repository_root = Path(__file__).resolve().parents[3]
    proto_file = repository_root / "server" / "protocol" / "im_protocol.proto"
    with tempfile.TemporaryDirectory(prefix="im-proto-") as temp_directory:
        module_path = generate_python_protocol(proto_file, Path(temp_directory))
        protocol = load_protocol(module_path)
        run_tests(args.host, args.port, protocol, not args.expect_db_unhealthy)

    print("IM server smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
