from __future__ import annotations
import csv
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

@dataclass
class RecordingState:
    enabled: bool = False
    path: Optional[Path] = None
    sample_index: int = 0
    sample_rate_hz: int = 16000

class CSVRecorder:
    """
    Writes one row per sample:
      timestamp_ms, sample_index, sample_i16
    """
    def __init__(self, recordings_dir: Path):
        self.recordings_dir = recordings_dir
        self.recordings_dir.mkdir(parents=True, exist_ok=True)
        self.state = RecordingState()
        self._fp = None
        self._writer = None

    def start(self, sample_rate_hz: int) -> Path:
        if self.state.enabled:
            return self.state.path  # already recording

        ts = time.strftime("%Y%m%d_%H%M%S")
        path = self.recordings_dir / f"audio_{ts}_{sample_rate_hz}hz.csv"
        fp = open(path, "w", newline="")
        writer = csv.writer(fp)
        writer.writerow(["timestamp_ms", "sample_index", "sample_i16"])

        self.state = RecordingState(
            enabled=True,
            path=path,
            sample_index=0,
            sample_rate_hz=sample_rate_hz
        )
        self._fp = fp
        self._writer = writer
        return path

    def stop(self) -> None:
        if not self.state.enabled:
            return
        try:
            if self._fp:
                self._fp.flush()
                self._fp.close()
        finally:
            self._fp = None
            self._writer = None
            self.state.enabled = False

    def write_frame(self, timestamp_ms: int | None, samples_i16: list[int]) -> None:
        if not self.state.enabled or self._writer is None:
            return

        # If device doesn't send timestamp, you can store blank or server time.
        ts = timestamp_ms if timestamp_ms is not None else ""

        for s in samples_i16:
            self._writer.writerow([ts, self.state.sample_index, int(s)])
            self.state.sample_index += 1
