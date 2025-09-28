#!/usr/bin/env python3
"""Simple WebSocket bombardment tool.

Connects to the Altair console WebSocket (default localhost:8082) and
pushes a stream of pseudo-random text frames. Useful for exercising the
server when you need a lot of inbound traffic quickly.

No third-party dependencies are required – the script performs the
RFC6455 handshake and frame masking manually using only the standard
library.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import os
import random
import socket
import string
import struct
import sys
import time
from typing import Tuple

_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
_TEXT_CHARACTERS = string.ascii_letters + string.digits + " .,;?!-_/\\|"


def _build_handshake(host_header: str, path: str) -> Tuple[bytes, str]:
    """Create the HTTP upgrade request and return (bytes, key)."""
    key_bytes = os.urandom(16)
    key = base64.b64encode(key_bytes).decode("ascii")
    request = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {host_header}\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n"
    )
    return request.encode("ascii"), key


def _perform_handshake(sock: socket.socket, host_header: str, path: str) -> None:
    request, key = _build_handshake(host_header, path)
    sock.sendall(request)

    response = bytearray()
    while b"\r\n\r\n" not in response:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("WebSocket handshake failed (no response)")
        response.extend(chunk)

    header_bytes, _, _ = response.partition(b"\r\n\r\n")
    header_lines = header_bytes.decode("iso-8859-1").split("\r\n")
    status_line = header_lines[0]

    if "101" not in status_line:
        raise ConnectionError(f"Unexpected handshake response: {status_line}")

    headers = {}
    for line in header_lines[1:]:
        if not line or ":" not in line:
            continue
        name, value = line.split(":", 1)
        headers[name.strip().lower()] = value.strip()

    expected_accept = base64.b64encode(hashlib.sha1((key + _GUID).encode("ascii")).digest()).decode("ascii")
    accept = headers.get("sec-websocket-accept")
    if accept != expected_accept:
        raise ConnectionError("Invalid Sec-WebSocket-Accept header")


def _send_ws_frame(sock: socket.socket, payload: bytes) -> None:
    """Send a single masked text websocket frame."""
    payload_len = len(payload)
    header = bytearray()
    header.append(0x81)  # FIN + text opcode

    if payload_len < 126:
        header.append(0x80 | payload_len)
    elif payload_len < (1 << 16):
        header.append(0x80 | 126)
        header.extend(struct.pack("!H", payload_len))
    else:
        header.append(0x80 | 127)
        header.extend(struct.pack("!Q", payload_len))

    mask = os.urandom(4)
    header.extend(mask)

    masked = bytes(payload[i] ^ mask[i % 4] for i in range(payload_len))
    sock.sendall(header + masked)


def _generate_text_payload(rng: random.Random, minimum: int, maximum: int) -> tuple[bytes, int]:
    """Create an ASCII text block terminated by a carriage return."""
    maximum = min(maximum, 40)
    minimum = max(1, min(minimum, maximum))

    length = rng.randint(minimum, maximum)
    text = "".join(rng.choice(_TEXT_CHARACTERS) for _ in range(length))
    payload = f"{text}\r".encode("ascii")
    return payload, length


def flood(host: str, port: int, path: str, messages: int, minimum: int, maximum: int, delay: float) -> None:
    address = (host, port)
    host_header = f"{host}:{port}"
    rng = random.Random()

    with socket.create_connection(address, timeout=5) as sock:
        _perform_handshake(sock, host_header, path)
        print(f"Connected to ws://{host_header}{path}, sending {messages if messages else 'unbounded'} text frames")

        count = 0
        total_chars = 0
        try:
            while messages <= 0 or count < messages:
                payload, text_length = _generate_text_payload(rng, minimum, maximum)
                _send_ws_frame(sock, payload)
                count += 1
                total_chars += text_length

                if count % 100 == 0:
                    print(f"sent {count} frames / {total_chars} chars", end="\r", flush=True)

                if delay:
                    time.sleep(delay)
        except KeyboardInterrupt:
            print("\nInterrupted by user")
        finally:
            print(f"Sent {count} frames ({total_chars} chars)")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Blast random text at a WebSocket server")
    parser.add_argument("--host", default="127.0.0.1", help="Server host (default: %(default)s)")
    parser.add_argument("--port", type=int, default=8082, help="Server port (default: %(default)s)")
    parser.add_argument("--path", default="/", help="WebSocket request path (default: %(default)s)")
    parser.add_argument("--messages", type=int, default=0, help="Number of frames to send (0 = run forever)")
    parser.add_argument(
        "--min-chars",
        "--min-bytes",
        dest="min_bytes",
        type=int,
        default=1,
        help="Minimum characters per frame before carriage return (default: %(default)s)",
    )
    parser.add_argument(
        "--max-chars",
        "--max-bytes",
        dest="max_bytes",
        type=int,
        default=40,
        help="Maximum characters per frame (default: %(default)s, cap: 40)",
    )
    parser.add_argument("--delay", type=float, default=0.0, help="Delay between frames in seconds (default: %(default)s)")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    if args.min_bytes <= 0 or args.max_bytes <= 0:
        print("Character counts must be positive", file=sys.stderr)
        return 1
    if args.max_bytes > 40:
        print("--max-chars/--max-bytes cannot exceed 40", file=sys.stderr)
        return 1
    if args.min_bytes > args.max_bytes:
        print("--min-chars/--min-bytes cannot exceed --max-chars/--max-bytes", file=sys.stderr)
        return 1

    flood(
        host=args.host,
        port=args.port,
        path=args.path,
        messages=args.messages,
        minimum=args.min_bytes,
        maximum=args.max_bytes,
        delay=args.delay,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
