from __future__ import annotations
import asyncio
import json
from pathlib import Path
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse, FileResponse
from fastapi.staticfiles import StaticFiles

from .settings import SETTINGS
from .sources.serial_tlv import SerialTLVSource
from .recorder import CSVRecorder
from .streaming import StreamHub

app = FastAPI(title="TLV Audio Scope")

# --- Static web ---
WEB_DIR = Path(__file__).resolve().parents[1] / "web"
app.mount("/static", StaticFiles(directory=str(WEB_DIR)), name="static")

# --- Core services ---
recorder = CSVRecorder(SETTINGS.recordings_dir)
hub = StreamHub(wave_seconds=SETTINGS.wave_seconds, default_sr=SETTINGS.default_sample_rate_hz, recorder=recorder)

serial_source = SerialTLVSource(log_cb=hub.add_log)
hub.set_source(serial_source)

@app.get("/")
def index():
    return FileResponse(str(WEB_DIR / "index.html"))

@app.get("/api/ports")
def ports():
    return serial_source.list_endpoints()

@app.get("/api/status")
def status():
    st = hub.status()
    rec = recorder.state
    return {
        "connected": st.connected,
        "endpoint": st.endpoint,
        "baud": st.baud,
        "sample_rate_hz": st.sample_rate_hz,
        "last_timestamp_ms": st.last_timestamp_ms,
        "recording": rec.enabled,          # ✅ only boolean
    }

@app.post("/api/connect")
def connect(cfg: dict):
    endpoint = str(cfg.get("endpoint", "")).strip()
    baud = int(cfg.get("baud", SETTINGS.default_baud))
    sr = int(cfg.get("sample_rate_hz", SETTINGS.default_sample_rate_hz))
    if not endpoint:
        return {"ok": False, "error": "endpoint required"}, 400

    hub.connect(endpoint, baud=baud, sample_rate_hz=sr)

    # ✅ start recording automatically (don’t return path)
    hub.start_recording()

    return {"ok": True}

@app.post("/api/disconnect")
def disconnect():
    # hub.disconnect() already calls recorder.stop() in your design
    hub.disconnect()
    return {"ok": True}


@app.websocket("/ws")
async def ws_stream(ws: WebSocket):
    await ws.accept()

    while True:
        await asyncio.sleep(1/30)

        # send pending logs first
        logs = hub.pop_logs(30)
        for item in logs:
            await ws.send_text(json.dumps({"type": "log", "data": item}))

        st = hub.status()
        if not st.connected:
            continue

        chunk = hub.ring_snapshot(max_samples=int(st.sample_rate_hz * 0.02))
        await ws.send_text(json.dumps({
            "type": "frame",
            "data": {
                "samples": chunk,
                "timestamp_ms": st.last_timestamp_ms,
                "sample_rate_hz": st.sample_rate_hz
            }
        }))