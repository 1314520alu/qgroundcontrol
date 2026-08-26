# Payload capabilities catalog

Cross-product UniGCS Fly View overlay capability table for UniPod MT11, SIYI A8 Mini, and Topotek TQ10N.

| Artifact | Path |
| --- | --- |
| Source JSON (canonical) | [capabilities.json](capabilities.json) |
| SQLite (handbook query) | [db/capabilities.db](db/capabilities.db) |
| Schema | [db/schema.sql](db/schema.sql) |
| Rebuild DB | `python3 db/build_db.py` |
| QGC runtime | `:/json/PayloadCapabilities.json` (copy under `src/Camera/`) |

- `features` — product truth (what the camera has).
- `overlay_phase1` — buttons shown in the current release (subset of features).

QGC loads the JSON at runtime via `PayloadCapabilityCatalog`; it does **not** open the SQLite file on device.
