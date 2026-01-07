import serial
import struct
import time

PORT = "/dev/cu.usbmodem9384AD1C3"   # CHANGE THIS
BAUD = 921600

FRAME_HDR = 0xAA55AA55
FRAME_FTR = 0xA5A5A5A5

TLV_PCM = 0x01
TLV_TS  = 0x02

MAX_L = 4096


def read_exact(ser, n):
    buf = bytearray()
    while len(buf) < n:
        b = ser.read(n - len(buf))
        if not b:
            continue
        buf.extend(b)
    return bytes(buf)


def find_header(ser):
    win = bytearray()
    while True:
        b = ser.read(1)
        if not b:
            continue
        win += b
        if len(win) > 4:
            win = win[-4:]
        if len(win) == 4 and int.from_bytes(win, "little") == FRAME_HDR:
            return


def main():
    print(f"Opening {PORT} @ {BAUD}")
    ser = serial.Serial(PORT, BAUD, timeout=0.2)
    time.sleep(0.2)
    ser.reset_input_buffer()

    last_ts = None

    print("Waiting for framed TLV...\n")

    while True:
        # ---- find header ----
        find_header(ser)

        # ---- read TLV header ----
        t = read_exact(ser, 1)[0]
        L = struct.unpack("<H", read_exact(ser, 2))[0]

        if L > MAX_L:
            print(f"Bad length {L}, resyncing")
            continue

        v = read_exact(ser, L)

        # ---- read footer ----
        ftr = struct.unpack("<I", read_exact(ser, 4))[0]
        if ftr != FRAME_FTR:
            print(f"Footer mismatch: {hex(ftr)}")
            continue

        # ---- process ----
        if t == TLV_TS and L == 4:
            last_ts = struct.unpack("<I", v)[0]
            print(f"[TS ] {last_ts} ms")

        elif t == TLV_PCM:
            samples = struct.unpack("<" + "h" * (L // 2), v)
            mn = min(samples)
            mx = max(samples)
            avg = sum(samples) / len(samples)
            print(
                f"[PCM] {len(samples)} samples  "
                f"min={mn:6d} max={mx:6d} avg={avg:7.1f}  "
                f"ts={last_ts}"
            )

        else:
            print(f"[TLV] type=0x{t:02X} len={L}")


if __name__ == "__main__":
    main()
