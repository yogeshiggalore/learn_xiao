from __future__ import annotations
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Optional, Iterable

@dataclass
class AudioFrame:
    """
    A chunk of PCM audio.
    samples: list[int] or bytes; we standardize to list[int] in the app.
    """
    timestamp_ms: Optional[int]
    samples_i16: list[int]  # int16 values

class AudioSource(ABC):
    @abstractmethod
    def list_endpoints(self) -> list[dict]:
        """Return selectable endpoints (ports, BLE devices, etc.)."""
        raise NotImplementedError

    @abstractmethod
    def connect(self, endpoint: str, **kwargs) -> None:
        raise NotImplementedError

    @abstractmethod
    def disconnect(self) -> None:
        raise NotImplementedError

    @abstractmethod
    def is_connected(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def frames(self) -> Iterable[AudioFrame]:
        """
        Blocking generator/iterator producing AudioFrame objects until disconnected.
        Implementation can run in a background thread and push into a queue.
        """
        raise NotImplementedError
