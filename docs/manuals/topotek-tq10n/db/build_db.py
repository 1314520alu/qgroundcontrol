#!/usr/bin/env python3
"""Populate sdk.db and manual.db from JSON sources (Topotek TQ10N)."""
from __future__ import annotations

import json
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def load_json(name: str) -> dict:
    with open(ROOT / name, encoding="utf-8") as f:
        return json.load(f)


def meta_rows(meta: dict) -> list[tuple[str, str]]:
    return [(k, str(v)) for k, v in meta.items()]


def build_sdk(db_path: Path, data: dict) -> None:
    schema = (ROOT / "schema.sql").read_text(encoding="utf-8").replace(")\nCREATE", ");\nCREATE")
    if db_path.exists():
        db_path.unlink()
    conn = sqlite3.connect(db_path)
    conn.executescript(schema if schema.rstrip().endswith(";") else schema.rstrip() + ";")
    conn.executemany("INSERT INTO meta(key,value) VALUES(?,?)", meta_rows(data["meta"]))
    for row in data.get("transports", []):
        conn.execute(
            "INSERT INTO transports(name,media,host,port,baud,notes) VALUES(?,?,?,?,?,?)",
            (
                row["name"],
                row["media"],
                row.get("host"),
                row.get("port"),
                row.get("baud"),
                row.get("notes"),
            ),
        )
    for row in data.get("frame_fields", []):
        conn.execute(
            "INSERT INTO frame_fields(name,offset,size_bytes,endian,description) VALUES(?,?,?,?,?)",
            (row["name"], row["offset"], row["size_bytes"], row.get("endian"), row.get("description")),
        )
    for row in data.get("commands", []):
        conn.execute(
            "INSERT INTO commands(cmd_id,cmd_hex,name,name_en,category,direction,need_ack,mt11_notes,description) "
            "VALUES(?,?,?,?,?,?,?,?,?)",
            (
                row["cmd_id"],
                row["cmd_hex"],
                row["name"],
                row.get("name_en"),
                row["category"],
                row["direction"],
                row.get("need_ack", 1),
                row.get("notes"),
                row.get("description"),
            ),
        )
    for row in data.get("command_fields", []):
        conn.execute(
            "INSERT INTO command_fields(cmd_id,direction,seq,name,ctype,description) VALUES(?,?,?,?,?,?)",
            (row["cmd_id"], row["direction"], row["seq"], row["name"], row["ctype"], row.get("description")),
        )
    for row in data.get("enums", []):
        conn.execute(
            "INSERT INTO enums(enum_name,value,label,notes) VALUES(?,?,?,?)",
            (row["enum_name"], row["value"], row["label"], row.get("notes")),
        )
    for row in data.get("examples", []):
        conn.execute(
            "INSERT INTO examples(cmd_id,title,hex_packet,notes) VALUES(?,?,?,?)",
            (row.get("cmd_id"), row["title"], row["hex_packet"], row.get("notes")),
        )
    for row in data.get("media_endpoints", []):
        conn.execute(
            "INSERT INTO media_endpoints(kind,uri,notes) VALUES(?,?,?)",
            (row["kind"], row["uri"], row.get("notes")),
        )
    conn.commit()
    conn.close()


def build_manual(db_path: Path, data: dict) -> None:
    schema = (ROOT / "manual_schema.sql").read_text(encoding="utf-8").replace(")\nCREATE", ");\nCREATE")
    if db_path.exists():
        db_path.unlink()
    conn = sqlite3.connect(db_path)
    conn.executescript(schema if schema.rstrip().endswith(";") else schema.rstrip() + ";")
    conn.executemany("INSERT INTO meta(key,value) VALUES(?,?)", meta_rows(data["meta"]))
    for row in data.get("documents", []):
        conn.execute(
            "INSERT INTO documents(doc_key,title,version,lang,date,filename,source_url,notes) "
            "VALUES(?,?,?,?,?,?,?,?)",
            (
                row["doc_key"],
                row["title"],
                row.get("version"),
                row.get("lang"),
                row.get("date"),
                row["filename"],
                row.get("source_url"),
                row.get("notes"),
            ),
        )
    for row in data.get("endpoints", []):
        conn.execute(
            "INSERT INTO endpoints(kind,uri,notes) VALUES(?,?,?)",
            (row["kind"], row["uri"], row.get("notes")),
        )
    for row in data.get("specs", []):
        conn.execute(
            "INSERT INTO specs(category,name,value,notes) VALUES(?,?,?,?)",
            (row["category"], row["name"], row["value"], row.get("notes")),
        )
    for row in data.get("faq", []):
        conn.execute(
            "INSERT INTO faq(q_no,question,answer) VALUES(?,?,?)",
            (row["q_no"], row["question"], row["answer"]),
        )
    conn.commit()
    conn.close()


def main() -> None:
    build_sdk(ROOT / "sdk.db", load_json("sdk.json"))
    build_manual(ROOT / "manual.db", load_json("manual.json"))
    print("Wrote", ROOT / "sdk.db", "and", ROOT / "manual.db")


if __name__ == "__main__":
    main()
