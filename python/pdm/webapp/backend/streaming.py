from __future__ import annotations
import threading
from dataclasses import dataclass
from typing import Optional
from collections import deque

from .sources.base import AudioSource, AudioFrame
from .recorder import CSVRecorder
from collections import deque


@dataclass
class StreamStatus:
    connected: bool = False
    endpoint: Optional[str] = None
    baud: Optional[int] = None
    sample_rate_hz: int = 16000
    last_timestamp_ms: Optional[int] = None
    dropped_frames: int = 0

class StreamHub:
    """
    One source -> ring buffer for UI + optional recorder.
    Future BLE can plug in via AudioSource.
    """
    def __init__(self, wave_seconds: float, default_sr: int, recorder: CSVRecorder):
        self._lock = threading.Lock()
        self._status = StreamStatus(sample_rate_hz=default_sr)
        self._source: Optional[AudioSource] = None
        self._thread: Optional[threading.Thread] = None
        self._stop = threading.Event()

        self._wave_seconds = wave_seconds
        self._ring = deque(maxlen=int(default_sr * wave_seconds))  # int16 samples for UI
        self._recorder = recorder
        self._logs = deque(maxlen=300)

    def status(self) -> StreamStatus:
        with self._lock:
            return StreamStatus(**self._status.__dict__)

    def ring_snapshot(self, max_samples: int) -> list[int]:
        with self._lock:
            if max_samples <= 0:
                return []
            data = list(self._ring)
        return data[-max_samples:]

    def set_source(self, source: AudioSource) -> None:
        with self._lock:
            self._source = source

    def connect(self, endpoint: str, *, baud: int, sample_rate_hz: int) -> None:
        if self._source is None:
            raise RuntimeError("No source configured")

        self.disconnect()

        self._stop.clear()
        self._source.connect(endpoint, baud=baud, sample_rate_hz=sample_rate_hz)

        with self._lock:
            self._status.connected = True
            self._status.endpoint = endpoint
            self._status.baud = baud
            self._status.sample_rate_hz = sample_rate_hz
            self._status.last_timestamp_ms = None
            self._status.dropped_frames = 0
            self._ring = deque(maxlen=int(sample_rate_hz * self._wave_seconds))

        self._thread = threading.Thread(target=self._pump_loop, daemon=True)
        self._thread.start()

    def disconnect(self) -> None:
        self._stop.set()
        if self._source is not None:
            self._source.disconnect()
        self._recorder.stop()
        with self._lock:
            self._status.connected = False
            self._status.endpoint = None
            self._status.baud = None
            self._status.last_timestamp_ms = None

    def start_recording(self) -> str:
        st = self.status()
        path = self._recorder.start(sample_rate_hz=st.sample_rate_hz)
        return str(path)

    def stop_recording(self) -> None:
        self._recorder.stop()

    def _pump_loop(self) -> None:
        assert self._source is not None
        for frame in self._source.frames():
            if self._stop.is_set():
                break
            self._handle_frame(frame)

    def _handle_frame(self, frame: AudioFrame) -> None:
        with self._lock:
            self._status.last_timestamp_ms = frame.timestamp_ms
            self._ring.extend(frame.samples_i16)

        # CSV write outside lock
        self._recorder.write_frame(frame.timestamp_ms, frame.samples_i16)

    def add_log(self, message: str, level: str = "dim") -> None:
        with self._lock:
            self._logs.append({"message": message, "level": level})

    def pop_logs(self, max_items: int = 50) -> list[dict]:
        out = []
        with self._lock:
            for _ in range(min(max_items, len(self._logs))):
                out.append(self._logs.popleft())
        return out