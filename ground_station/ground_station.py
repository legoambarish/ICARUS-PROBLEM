import socket
import struct
import sys
from pathlib import Path


LOG_PATH = Path("ground_station.log")


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def main() -> int:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 9001))
    sock.settimeout(0.5)
    count = 0

    with LOG_PATH.open("w", encoding="utf-8") as log:
        while True:
            try:
                data, _addr = sock.recvfrom(1024)
            except socket.timeout:
                if count > 0:
                    break
                continue
            payload = struct.unpack("___", data)
            tick, temperature_raw, battery_voltage, crc = payload
            expected = crc16_ccitt(data[:-2])
            status = "CRC_OK" if expected == crc else "CRC_BAD"
            log.write(
                f"TICK:{tick} TEMP:{temperature_raw} VBAT:{battery_voltage:.3f} {status}\n"
            )
            log.flush()
            count += 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
