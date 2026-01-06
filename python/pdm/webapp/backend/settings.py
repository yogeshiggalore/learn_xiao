from dataclasses import dataclass
from pathlib import Path

@dataclass(frozen=True)
class Settings:
    default_baud: int = 921600
    default_sample_rate_hz: int = 16000   # used for display scaling & recording metadata
    wave_seconds: float = 2.0             # browser window
    recordings_dir: Path = Path(__file__).resolve().parents[1] / "recordings"

SETTINGS = Settings()