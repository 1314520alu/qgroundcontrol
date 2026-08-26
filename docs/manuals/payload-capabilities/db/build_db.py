#!/usr/bin/env python3
"""Build capabilities.db from ../capabilities.json."""

from __future__ import annotations

import json
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_PATH = ROOT.parent / "capabilities.json"
SCHEMA_PATH = ROOT / "schema.sql"
DB_PATH = ROOT / "capabilities.db"

FEATURE_BOOL_KEYS = (
    "gimbal",
    "lens",
    "laser",
    "exposure_auto",
    "photo",
    "video",
    "zoom",
    "focus",
    "media_library",
)
FEATURE_TEXT_KEYS = ("ai", "follow")


def _as_int(value: object) -> int:
    if isinstance(value, bool):
        return int(value)
    if value in (1, "1", "true", "True"):
        return 1
    return 0


def _as_feature_text(value: object) -> str:
    if value is True or value == 1:
        return "1"
    if value is False or value == 0:
        return "0"
    return str(value)


def main() -> None:
    data = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    if DB_PATH.exists():
        DB_PATH.unlink()

    conn = sqlite3.connect(DB_PATH)
    try:
        conn.executescript(SCHEMA_PATH.read_text(encoding="utf-8"))
        meta = data.get("meta", {})
        for key, value in meta.items():
            conn.execute(
                "INSERT INTO meta(key, value) VALUES (?, ?)",
                (key, json.dumps(value) if not isinstance(value, str) else value),
            )

        for cam in data["cameras"]:
            conn.execute(
                """
                INSERT INTO cameras(
                    id, vendor, model_name, video_source, rtsp,
                    control_host, control_port, hw_id
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    cam["id"],
                    cam["vendor"],
                    cam["model_name"],
                    cam["video_source"],
                    cam.get("rtsp"),
                    cam.get("control_host"),
                    cam.get("control_port"),
                    cam.get("hw_id"),
                ),
            )
            features = cam["features"]
            cols = FEATURE_BOOL_KEYS + FEATURE_TEXT_KEYS
            placeholders = ", ".join("?" for _ in cols)
            values: list[object] = [cam["id"]]
            for key in FEATURE_BOOL_KEYS:
                values.append(_as_int(features.get(key, 0)))
            for key in FEATURE_TEXT_KEYS:
                values.append(_as_feature_text(features.get(key, 0)))
            conn.execute(
                f"""
                INSERT INTO features(
                    camera_id, {", ".join(FEATURE_BOOL_KEYS + FEATURE_TEXT_KEYS)}
                ) VALUES (?, {placeholders})
                """,
                values,
            )
            for feature in cam.get("overlay_phase1", []):
                conn.execute(
                    "INSERT INTO overlay_phase1(camera_id, feature) VALUES (?, ?)",
                    (cam["id"], feature),
                )
        conn.commit()
    finally:
        conn.close()

    print(f"Wrote {DB_PATH}")


if __name__ == "__main__":
    main()
