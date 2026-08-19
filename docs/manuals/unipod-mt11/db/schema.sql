CREATE TABLE command_fields (
  id INTEGER PRIMARY KEY,
  cmd_id INTEGER NOT NULL,
  direction TEXT NOT NULL, -- send | ack
  seq INTEGER NOT NULL,
  name TEXT NOT NULL,
  ctype TEXT NOT NULL,
  description TEXT,
  FOREIGN KEY(cmd_id) REFERENCES commands(cmd_id)
)
CREATE TABLE commands (
  cmd_id INTEGER PRIMARY KEY,
  cmd_hex TEXT NOT NULL,
  name TEXT NOT NULL,
  name_en TEXT,
  category TEXT NOT NULL,
  direction TEXT NOT NULL, -- request | push | both
  need_ack INTEGER NOT NULL DEFAULT 1,
  mt11_notes TEXT,
  description TEXT
)
CREATE TABLE enums (
  id INTEGER PRIMARY KEY,
  enum_name TEXT NOT NULL,
  value INTEGER NOT NULL,
  label TEXT NOT NULL,
  notes TEXT,
  UNIQUE(enum_name, value)
)
CREATE TABLE examples (
  id INTEGER PRIMARY KEY,
  cmd_id INTEGER,
  title TEXT NOT NULL,
  hex_packet TEXT NOT NULL,
  notes TEXT,
  FOREIGN KEY(cmd_id) REFERENCES commands(cmd_id)
)
CREATE TABLE frame_fields (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  offset INTEGER NOT NULL,
  size_bytes INTEGER NOT NULL,
  endian TEXT,
  description TEXT
)
CREATE TABLE media_endpoints (
  id INTEGER PRIMARY KEY,
  kind TEXT NOT NULL,
  uri TEXT NOT NULL,
  notes TEXT
)
CREATE TABLE meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
)
CREATE TABLE transports (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  media TEXT NOT NULL,
  host TEXT,
  port INTEGER,
  baud INTEGER,
  notes TEXT
)
CREATE TABLE web_apis (
  id INTEGER PRIMARY KEY,
  path TEXT NOT NULL,
  method TEXT NOT NULL,
  summary TEXT NOT NULL,
  request_params TEXT,
  response_notes TEXT
);
