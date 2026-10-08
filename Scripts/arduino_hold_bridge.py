"""Local TCP-to-serial bridge for the CaveGame hold interaction.

Run this script before starting the game. Send one newline-terminated command
per connection: HOLD starts the Arduino's active output; RELEASE stops it.
The bridge also stops the Arduino output whenever the game disconnects.

Install pyserial with: py -m pip install pyserial
Run with: py Scripts/arduino_hold_bridge.py --port COM3

The Arduino sketch must read lines from Serial and interpret the configured
active/inactive commands (defaults: 1 and 0).
"""

from __future__ import annotations

import argparse
import socket
import sys
import threading


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Arduino serial port, e.g. COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--tcp-port", type=int, default=8765)
    parser.add_argument("--active-command", default="1")
    parser.add_argument("--inactive-command", default="0")
    args = parser.parse_args()

    try:
        import serial  # type: ignore[import-not-found]
    except ImportError:
        print("pyserial is required: py -m pip install pyserial", file=sys.stderr)
        return 2

    try:
        device = serial.Serial(args.port, args.baud, timeout=0.1, write_timeout=1)
    except (OSError, serial.SerialException) as exc:
        print(f"Could not open Arduino on {args.port}: {exc}", file=sys.stderr)
        return 2

    active = False
    lock = threading.Lock()

    def set_active(value: bool) -> None:
        nonlocal active
        with lock:
            if active == value:
                return
            command = args.active_command if value else args.inactive_command
            device.write((command + "\n").encode("utf-8"))
            device.flush()
            active = value
            print("Arduino active" if value else "Arduino stopped", flush=True)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind((args.host, args.tcp_port))
        server.listen(1)
        print(f"Listening on {args.host}:{args.tcp_port}; Arduino {args.port} @ {args.baud}", flush=True)
        while True:
            connection, address = server.accept()
            print(f"Game connected: {address[0]}", flush=True)
            try:
                with connection:
                    connection.settimeout(0.5)
                    buffer = b""
                    while True:
                        try:
                            chunk = connection.recv(256)
                            if not chunk:
                                break
                            buffer += chunk
                            while b"\n" in buffer:
                                raw_line, buffer = buffer.split(b"\n", 1)
                                command = raw_line.decode("ascii", errors="ignore").strip().upper()
                                if command == "HOLD":
                                    set_active(True)
                                elif command == "RELEASE":
                                    set_active(False)
                                elif command:
                                    print(f"Ignoring unknown command: {command}", flush=True)
                        except socket.timeout:
                            continue
            except (OSError, serial.SerialException) as exc:
                print(f"Connection/serial error: {exc}", file=sys.stderr, flush=True)
            finally:
                # Fail safe: game closing or losing the link always deactivates Arduino.
                try:
                    set_active(False)
                except (OSError, serial.SerialException) as exc:
                    print(f"Could not send stop command: {exc}", file=sys.stderr, flush=True)
    except KeyboardInterrupt:
        print("Stopping bridge", flush=True)
    except OSError as exc:
        print(f"Could not listen on {args.host}:{args.tcp_port}: {exc}", file=sys.stderr)
        return 2
    finally:
        try:
            set_active(False)
        except (OSError, serial.SerialException):
            pass
        server.close()
        device.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
