CREATE TABLE meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE cameras (
  id TEXT PRIMARY KEY,
  vendor TEXT NOT NULL,
  model_name TEXT NOT NULL,
  video_source TEXT NOT NULL UNIQUE,
  rtsp TEXT,
  control_host TEXT,
  control_port INTEGER,
  hw_id TEXT
);

CREATE TABLE features (
  camera_id TEXT PRIMARY KEY,
  gimbal INTEGER NOT NULL DEFAULT 0,
  lens INTEGER NOT NULL DEFAULT 0,
  laser INTEGER NOT NULL DEFAULT 0,
  ai TEXT NOT NULL DEFAULT '0',
  follow TEXT NOT NULL DEFAULT '0',
  exposure_auto INTEGER NOT NULL DEFAULT 0,
  photo INTEGER NOT NULL DEFAULT 0,
  video INTEGER NOT NULL DEFAULT 0,
  zoom INTEGER NOT NULL DEFAULT 0,
  focus INTEGER NOT NULL DEFAULT 0,
  media_library INTEGER NOT NULL DEFAULT 0,
  FOREIGN KEY(camera_id) REFERENCES cameras(id)
);

CREATE TABLE overlay_phase1 (
  id INTEGER PRIMARY KEY,
  camera_id TEXT NOT NULL,
  feature TEXT NOT NULL,
  UNIQUE(camera_id, feature),
  FOREIGN KEY(camera_id) REFERENCES cameras(id)
);
