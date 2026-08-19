CREATE TABLE ardupilot_params (
  id INTEGER PRIMARY KEY,
  protocol TEXT NOT NULL,
  param TEXT NOT NULL,
  value TEXT NOT NULL,
  meaning TEXT,
  notes TEXT
);
CREATE TABLE chapters (
  id INTEGER PRIMARY KEY,
  chapter_no TEXT NOT NULL,
  title TEXT NOT NULL,
  parent TEXT,
  summary TEXT
);
CREATE TABLE documents (
  id INTEGER PRIMARY KEY,
  doc_key TEXT UNIQUE NOT NULL,
  title TEXT NOT NULL,
  version TEXT,
  lang TEXT,
  date TEXT,
  filename TEXT NOT NULL,
  source_url TEXT,
  notes TEXT
);
CREATE TABLE endpoints (
  id INTEGER PRIMARY KEY,
  kind TEXT NOT NULL,
  uri TEXT NOT NULL,
  notes TEXT
);
CREATE TABLE faq (
  id INTEGER PRIMARY KEY,
  q_no INTEGER NOT NULL,
  question TEXT NOT NULL,
  answer TEXT NOT NULL
);
CREATE TABLE firmware_releases (
  id INTEGER PRIMARY KEY,
  release_date TEXT NOT NULL,
  package_version TEXT,
  camera_version TEXT,
  gimbal_version TEXT,
  unigcs_version TEXT,
  changes TEXT NOT NULL
);
CREATE TABLE interfaces (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  role TEXT NOT NULL,
  details TEXT
);
CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
CREATE TABLE safety_notes (
  id INTEGER PRIMARY KEY,
  level TEXT NOT NULL,
  text TEXT NOT NULL
);
CREATE TABLE specs (
  id INTEGER PRIMARY KEY,
  category TEXT NOT NULL,
  name TEXT NOT NULL,
  value TEXT NOT NULL,
  unit TEXT,
  notes TEXT
);
