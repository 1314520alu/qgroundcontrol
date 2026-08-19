# UniPod MT11 Media Gallery (Preview & Download)

**Date:** 2026-08-17  
**Status:** Implemented — pending HIL  
**Plan:** `docs/superpowers/plans/2026-08-17-unipod-mt11-media-gallery.md` (Cursor plan: MT11 media gallery)  
**Scope:** Browse, preview, and download onboard photos/videos from UniPod MT11 TF card via Web Server API  
**Primary targets:** Landscape handheld remotes (SIYI / Skydroid) with radio ethernet `192.168.144.x`  
**Depends on:** Phase 1 onboard photo/record (`docs/superpowers/specs/2026-08-15-unipod-mt11-photo-video-control-design.md`)

## Problem

Phase 1 stores captures on the payload TF card. Operators on SIYI/Skydroid remotes cannot browse, preview, or pull those files into the GCS without leaving QGC (e.g. UniGCS). QGC has no media gallery for MT11.

## Goals

1. **List** onboard photos and videos from the MT11 Web Server when UniPod video source + `192.168.144.x` are ready.
2. **Preview** — photo: full image in overlay; video: play inside overlay (`MediaPlayer` / HTTP URL, fallback download-then-play).
3. **Download** one file at a time to `AppSettings` photo/video save paths.
4. **Entry** — button on Fly view `PhotoVideoControl` strip (MT11 only); landscape split overlay.

## Non-goals

- Delete media / directory tree navigation
- Dedicated settings/analyze page
- Thumbnail generation API (use downscaled `Image` for photos; icon placeholder for video)
- Batch download queue
- UDP SDK media commands (Web Server only)

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Entry | Fly overlay from `PhotoVideoControl` |
| Transport | HTTP `http://<mt11-host>/api/v1/...` (host from RTSP URL, default `192.168.144.25`) |
| Approach | Independent `UnipodMt11MediaClient` (not inside UDP client / camera control) |
| List | `path=""` all files of type; page size 24; load-more on scroll |
| GET params | Query string first; HIL may require GET+JSON body later |
| Live vs playback | `VideoManager.stopVideo()` while playing video in gallery; `startVideo()` on close / leave video preview |
| Download | `QGCFileDownload` → `photoSavePath` / `videoSavePath` |

## Architecture

```text
PhotoVideoControl (gallery button)
        │
        ▼
UnipodMt11MediaGallery.qml   left grid | right preview
        │
        ├─► UnipodMt11MediaClient   GET list / count
        │         │
        │         ▼
        │   http://192.168.144.25/api/v1/getmedialist
        │
        ├─► Image / MediaPlayer     preview via item.url
        ├─► QGCFileDownload         save to photo/video paths
        └─► VideoManager            pause/resume live RTSP for video preview
```

### Module responsibilities

| Unit | Responsibility |
| --- | --- |
| `UnipodMt11MediaFile` | `name`, `url`, `isVideo` for QML |
| `UnipodMt11MediaClient` | Ready gate; fetch list (paged); expose `QmlObjectListModel`; download helper |
| `QGCCameraManager` | Owns media client; exposes to QML next to UDP/camera control |
| `UnipodMt11MediaGallery.qml` | Landscape dialog UI |
| `PhotoVideoControl.qml` | Gallery open button when MT11 + ready |

### Lifecycle

1. Video source UniPod MT11 + ethernet ready → media client `ready`.
2. User opens gallery → fetch photo (or video) list page 0.
3. Select item → photo Image or video play (stop live RTSP for video).
4. Download → `QGCFileDownload` to save path; toast on complete/fail.
5. Close gallery → stop player; `VideoManager.startVideo()` if was stopped.

## Web API

Authority: `docs/manuals/unipod-mt11/UniPod-MT11-Web-Server.pdf` / handbook §5.

| Method | Path | Use |
| --- | --- | --- |
| GET | `/api/v1/getdirectories` | Fallback if empty `path` rejected |
| GET | `/api/v1/getmediacount` | Optional total for paging |
| GET | `/api/v1/getmedialist` | Primary list; items `{name,url}` |

`media_type`: 0 photo, 1 video.

## Error handling

| Condition | Behavior |
| --- | --- |
| Not ready | Hide gallery button |
| List error / timeout | Short error + retry in overlay |
| Empty list | Empty state |
| Play fail | Message + offer download |
| Download fail | Progress error; cancelable |

## Testing

| Case | Expect |
| --- | --- |
| Unit | Parse PDF sample JSON; build query URLs; local save path join |
| Unit | Empty / `code=400` responses handled |
| HIL | See new photo after capture; preview; download; play video; RTSP resumes |

## Out of scope follow-ups

- Delete APIs
- Folder browser
- Dedicated media settings page
- GET+JSON body if query-string fails on device
