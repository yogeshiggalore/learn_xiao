from __future__ import annotations
import struct
import threading
import time
from dataclasses import dataclass
from typing import Optional, Iterable
from queue import Queue, Empty

import serial
from serial.tools import list_ports

from .base import AudioSource, AudioFrame
from typing import Optional, Callable

TLV_PCM = 0x01  # V = int16 LE PCM bytes
TLV_TS  = 0x02  # V = uint32 LE timestamp ms

FRAME_HDR = 0xAA55AA55
FRAME_FTR = 0xA5A5A5A5
MAX_L = 4096

@dataclass
class SerialConfig:
    baud: int
    sample_rate_hz: int  # used for metadata only (device doesn't need it)

class SerialTLVSource(AudioSource):
    def __init__(
        self,
        on_rx_tlv: Optional[Callable[[int, int], None]] = None,
        log_cb: Optional[Callable[[str, str], None]] = None,
    ) -> None:
        self._ser: Optional[serial.Serial] = None
        self._cfg: Optional[SerialConfig] = None
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._q: "Queue[AudioFrame]" = Queue(maxsize=64)
        self._last_ts: Optional[int] = None

        self._on_rx_tlv = on_rx_tlv
        self._log_cb = log_cb

    def list_endpoints(self) -> list[dict]:
        out = []
        for p in list_ports.comports():
            out.append({
                "id": p.device,
                "label": f"{p.device} — {p.description or ''}".strip(),
                "device": p.device,
                "description": p.description or "",
                "manufacturer": p.manufacturer or "",
                "hwid": p.hwid or "",
                "kind": "serial",
            })
        return out

    def connect(self, endpoint: str, **kwargs) -> None:
        baud = int(kwargs.get("baud", 921600))
        sr = int(kwargs.get("sample_rate_hz", 16000))
        self.disconnect()

        self._cfg = SerialConfig(baud=baud, sample_rate_hz=sr)
        self._ser = serial.Serial(endpoint, baudrate=baud, timeout=0.2)
        time.sleep(0.2)
        try:
            self._ser.reset_input_buffer()
        except Exception:
            pass

        self._stop.clear()
        self._thread = threading.Thread(target=self._reader_loop, daemon=True)
        self._thread.start()

    def disconnect(self) -> None:
        self._stop.set()
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None
        self._cfg = None
        self._last_ts = None
        # drain queue
        while True:
            try:
                self._q.get_nowait()
            except Exception:
                break

    def is_connected(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def frames(self) -> Iterable[AudioFrame]:
        """
        Yields frames as they arrive. This blocks until frames are available.
        """
        while self.is_connected():
            try:
                yield self._q.get(timeout=0.5)
            except Empty:
                continue

    def _read_exact(self, n: int) -> bytes:
        assert self._ser is not None
        buf = bytearray()
        while len(buf) < n and not self._stop.is_set():
            chunk = self._ser.read(n - len(buf))
            if chunk:
                buf.extend(chunk)
        return bytes(buf)

    def _reader_loop(self) -> None:
        assert self._ser is not None

        ALLOWED_TYPES = {TLV_TS, TLV_PCM, 0x7F}  # TS, PCM, SYNC (optional)

        # sliding 4-byte window to find header
        win = bytearray()

        def read_u32_le(b: bytes) -> int:
            return int.from_bytes(b, "little")

        while not self._stop.is_set():
            try:
                # ---- 1) Find header 0xAA55AA55 ----
                while not self._stop.is_set():
                    b = self._ser.read(1)
                    if not b:
                        continue
                    win += b
                    if len(win) > 4:
                        del win[0]
                    if len(win) == 4 and read_u32_le(win) == FRAME_HDR:
                        break

                if self._stop.is_set():
                    break

                # ---- 2) Read TLV header: T(1) + L(2 LE) ----
                t_b = self._read_exact(1)
                if len(t_b) != 1:
                    continue
                t = t_b[0]

                l_bytes = self._read_exact(2)
                if len(l_bytes) != 2:
                    continue
                (L,) = struct.unpack("<H", l_bytes)

                # Reject false header hits early
                if t not in ALLOWED_TYPES:
                    if self._log_cb:
                        self._log_cb(f"Unknown TLV type 0x{t:02X} after header; resync", "warn")
                    win.clear()
                    continue

                if L > MAX_L:
                    if self._log_cb:
                        self._log_cb(f"Bad TLV length {L}, resyncing...", "warn")
                    win.clear()
                    continue

                # ---- 3) Read value ----
                v = self._read_exact(L) if L else b""
                if len(v) != L:
                    win.clear()
                    continue

                # ---- 4) Read footer EXACTLY 4 bytes ----
                ftr = self._read_exact(4)
                if len(ftr) != 4 or read_u32_le(ftr) != FRAME_FTR:
                    if self._log_cb:
                        got = read_u32_le(ftr) if len(ftr) == 4 else None
                        self._log_cb(f"Footer mismatch (got={got}), resyncing...", "warn")
                    win.clear()
                    continue

                # ---- 5) Process TLV ----
                if t == TLV_TS and L == 4:
                    (self._last_ts,) = struct.unpack("<I", v)

                elif t == TLV_PCM:
                    if L % 2 != 0:
                        continue
                    samples = struct.unpack("<" + "h" * (L // 2), v)
                    frame = AudioFrame(timestamp_ms=self._last_ts, samples_i16=list(samples))
                    try:
                        self._q.put_nowait(frame)
                        if self._log_cb:
                            self._log_cb(f"Queued PCM frame: {len(samples)} samples ts={self._last_ts}", "ok")
                    except Exception:
                        pass

                # ---- 6) Log (optional) ----
                if self._log_cb:
                    hdr_hex = " ".join(f"{b:02X}" for b in (bytes([t]) + l_bytes))
                    self._log_cb(f"RX FRAMED TLV: T=0x{t:02X} L={L} hdr=[{hdr_hex}]", "dim")

                win.clear()  # clean slate after a good frame

            except (serial.SerialException, OSError):
                break
            except Exception:
                win.clear()
                continue


