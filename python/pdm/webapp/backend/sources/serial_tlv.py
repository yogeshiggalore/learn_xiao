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

@dataclass
class SerialConfig:
    baud: int
    sample_rate_hz: int  # used for metadata only (device doesn't need it)

class SerialTLVSource(AudioSource):
    def __init__(self) -> None:
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

        while not self._stop.is_set():
            try:
                t = self._ser.read(1)
                if not t:
                    continue
                t = t[0]

                l_bytes = self._read_exact(2)
                if len(l_bytes) != 2:
                    continue
                (L,) = struct.unpack("<H", l_bytes)
                # Log header + byte count (T + Llo + Lhi)
                if self._on_rx_tlv:
                    hdr = bytes([t]) + l_bytes
                    hdr_hex = " ".join(f"{b:02X}" for b in hdr)
                    total = 3 + L
                    self._on_rx_tlv(t, L)  # optional raw callback
                    # also log a nice string (recommended)
                    try:
                        self._on_rx_tlv.__self__.add_log(  # works if callback is hub.add_log bound method
                            f"RX TLV hdr=[{hdr_hex}]  T=0x{t:02X}  L={L}  total={total} bytes",
                            "dim"
                        )
                    except Exception:
                        pass

                v = self._read_exact(L) if L else b""
                if len(v) != L:
                    continue

                if t == TLV_TS and L == 4:
                    (self._last_ts,) = struct.unpack("<I", v)
                    continue

                if t == TLV_PCM:
                    if L % 2 != 0:
                        continue
                    samples = struct.unpack("<" + "h" * (L // 2), v)
                    frame = AudioFrame(timestamp_ms=self._last_ts, samples_i16=list(samples))
                    try:
                        self._q.put_nowait(frame)
                    except Exception:
                        # drop if UI/recording can't keep up
                        pass
                if self._log_cb:
                    hdr = bytes([t]) + l_bytes
                    hdr_hex = " ".join(f"{b:02X}" for b in hdr)
                    total = 3 + L
                    self._log_cb(
                        f"RX TLV hdr=[{hdr_hex}]  T=0x{t:02X}  L={L}  total={total} bytes",
                        "dim"
                    )

            except (serial.SerialException, OSError):
                break
            except Exception:
                continue
