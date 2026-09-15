# 米多地面站 software update (notify + download)

**Date:** 2026-09-14  
**Status:** Approved  
**Plan:** `docs/superpowers/plans/2026-09-14-software-auto-update.md`

## Problem

Operators install 米多地面站 on 思翼 / 云卓 Android remotes and on macOS. There is no product update channel. Upstream QGC already fetches `QGC.version.txt` from AWS S3 on launch and shows a text dialog pointing at qgroundcontrol.com. This fork would either miss that check (daily builds) or notify about **upstream QGC**, not 米多. Gitee Release attachments cap at 100 MB, which is too small for QGC/GStreamer installers. A home PC is not an always-on public host.

## Goals

1. On every launch (except unit tests), if a manifest URL is configured, fetch it from Aliyun OSS over the network.
2. If the remote version is newer than the local `QGC_APP_VERSION_STR` **and** the vehicle is not flying, show an update dialog.
3. The dialog can download the current-platform installer (Android APK or macOS DMG) to the device. The user installs it themselves.
4. Flying never interrupts: defer the dialog until `flying` becomes false or the active vehicle disconnects.
5. Check/parse failures are silent. Download errors are visible only after the user starts a download.

## Non-goals

- Silent / automatic install (Sparkle, unattended APK install)
- Windows / Linux / iOS packages
- Gitee as the update source
- Hosting OSS or creating the bucket in this change
- Background download before the user taps Download
- Manual “Check for updates” button in Settings
- Changing version numbering beyond existing `vX.Y.Z`

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Behavior | Notify + one-click download; user installs |
| Host | Aliyun OSS for `latest.json` and installers |
| Platforms | Android APK and macOS DMG only |
| When to check | Every launch, including daily builds; skip unit tests |
| When to prompt | Only if not flying; download starts only on tap |
| Dismiss | “Later” hides the dialog for the rest of this process |
| Existing QGC check | Stop calling `QGCApplication::_checkForNewVersion()` so AWS/qgroundcontrol.com is unused |
| Manifest URL | `AppSettings.updateManifestUrl` string Fact; empty = no check |
| Compare | Same regex as today: `v(\d+)\.(\d+)\.(\d+)`; git suffix ignored |
| Integrity | SHA-256 via existing `QGCFileDownload::setExpectedHash` |
| UI target | Short landscape remotes first (compact dialog, glove-sized buttons) |

## Manifest (`latest.json`)

The setting stores the **full HTTPS URL** of this file (not the bucket root). Example:

```json
{
  "version": "v1.2.0",
  "notes": "修复视频源切换；增加 XF200 顶栏功率显示。",
  "android": {
    "url": "https://<bucket>.oss-cn-hangzhou.aliyuncs.com/releases/v1.2.0/miduo-android.apk",
    "sha256": "<64 hex chars>",
    "size": 180000000
  },
  "macos": {
    "url": "https://<bucket>.oss-cn-hangzhou.aliyuncs.com/releases/v1.2.0/miduo.dmg",
    "sha256": "<64 hex chars>",
    "size": 200000000
  }
}
```

| Field | Rule |
| --- | --- |
| `version` | Required. Must match `vX.Y.Z`. Compared to local app version. |
| `notes` | Required string. Shown in the dialog (may be Chinese). |
| `android` / `macos` | Object or omitted. Client uses only the current OS key (`Q_OS_ANDROID` → `android`, `Q_OS_MACOS` → `macos`). |
| `url` | Required HTTPS URL inside a platform object. Objects are public-read (or otherwise anonymously GET-able). |
| `sha256` | Required 64-char hex. Download fails if mismatch; incomplete file is deleted. |
| `size` | Optional bytes. Progress prefers HTTP `Content-Length`, then `size`; otherwise indeterminate. |

Unknown keys are ignored. Missing `version`/`notes`, unparsable version, or JSON parse failure → treat as no update. If the current OS has no platform object, the dialog may still show (newer version + notes) but **must not** show Download.

## Architecture

```text
QGCApplication (startup, not unit tests)
        │
        ▼
UpdateChecker::checkOnStartup()
        │  empty updateManifestUrl → return
        ▼
GET latest.json (QGCFileDownload)
        │  network/JSON/version fail → log only
        ▼
compare vX.Y.Z  ── older/equal → return
        │
        ▼
flying? ── yes → pending=true; wait flyingChanged / activeVehicleChanged
        │ no (no vehicle, or vehicle.flying == false)
        ▼
MainWindow update dialog
        │
        ├─ Later  → sessionDismissed; hide
        └─ Download → QGCFileDownload + sha256 → open local file
```

Replace the `_checkForNewVersion()` call in `QGCApplication` with `UpdateChecker::checkOnStartup()`. Move version parse/compare into `UpdateChecker`. Remove `_checkForNewVersion`, `_qgcCurrentStableVersionDownloadComplete`, `_parseVersionText`, and the unused `_majorVersion` / `_minorVersion` / `_buildVersion` members from `QGCApplication`. Leave `QGCCorePlugin::stableVersionCheckFileUrl()` / `stableDownloadLocation()` in place for upstream custom-build API compatibility; this fork does not call them.

### UpdateChecker (C++)

C++ singleton, `QML_ELEMENT` + `QML_UNCREATABLE`, exposed as `QGroundControl.updateChecker` (same pattern as other app services on `QGroundControlQmlGlobal`). Null-check `MultiVehicleManager::instance()->activeVehicle()` before `flying()`.

Properties (QML): `updateAvailable`, `remoteVersion`, `notes`, `downloadAvailable`, `downloadProgress`, `downloading`, `errorString`, `localInstallerPath`, `dialogVisible`.

Methods: `checkOnStartup()`, `showDialogIfIdle()`, `download()`, `cancelDownload()`, `dismiss()`.

`isFlying()`: active vehicle exists **and** `vehicle->flying()`. No vehicle → not flying → prompt allowed.

Session flags: `_sessionDismissed`. Later, back, or any close that is not “keep the dialog open while downloading” sets it. Pending-from-flight still shows once after landing if `_sessionDismissed` is false. Same process, same version: at most one automatic prompt.

Do not force-reopen the dialog if a download started and the user closed it. Download continues; progress is only on an already-open dialog.

### Dialog (QML)

Host on `MainWindow.qml` as a dedicated dialog (not `QGCSimpleMessageDialog` — that has no progress). Short landscape: title, version, scrollable notes, primary **Download** / **Retry** and **Later**. While downloading: progress + **Cancel**. After success: tell the user to install, then `QDesktopServices::openUrl` on the local file (Android system installer / macOS opens the DMG). If open fails: show the saved path.

Use `ScreenTools` sizes and `QGCPalette` / QGC controls. Chinese strings in `translations/qgc_source_zh_CN.ts`.

| Source (qsTr) | zh_CN |
| --- | --- |
| New Version Available | 发现新版本 |
| Later | 稍后 |
| Download | 下载 |
| Retry | 重试 |
| Cancel | 取消 |
| Update Manifest URL | 更新清单地址 |

### Settings

`AppSettings.updateManifestUrl`:

- `type`: string
- `default`: `""`
- `label`: Update Manifest URL
- Visible on Settings → General in a small **Software Update** group (text field). Empty means updates are off until the OSS URL is pasted. No restart required; next launch uses the new URL. In-session re-check is not required this round.

### Download

Reuse `QGCFileDownload` (progress, cancel, SHA-256 already exist). Save under `QStandardPaths::DownloadLocation`; if that is empty, `AppLocalDataLocation`. Filename is the URL path basename (e.g. `miduo-android.apk`). Overwrite a completed file of the same name. On cancel or hash failure, delete the incomplete file.

Android: opening the APK must use the system package installer. Add `REQUEST_INSTALL_PACKAGES` if the current manifest does not already allow it. If the OS still blocks install, the dialog keeps the file path so the user can install from Files.

## Error handling

| Situation | Behavior |
| --- | --- |
| Empty manifest URL | No request |
| Offline, timeout, HTTP not 200 | Log; no dialog |
| Bad JSON / bad version | Log; no dialog |
| Flying | No dialog; prompt after landing or disconnect |
| No platform package | Dialog without Download |
| Download fail / disk full | Dialog error; Retry; delete incomplete file |
| SHA-256 mismatch | Treat as corrupt; delete; Retry |
| Open installer fails | Show saved path |
| Unit tests | `checkOnStartup()` is a no-op |

## Testing

No live OSS. Unit tests with fixture JSON / fake files:

1. Parse valid `latest.json`; reject missing fields and invalid JSON.
2. Version compare: older, equal, newer; `v1.2.3-10-gabcdef` local equals `v1.2.3`.
3. `isFlying()` true → dialog deferred; `flying` false or vehicle null → allowed.
4. Android fixture selects `android`; macOS fixture selects `macos`; missing key → `downloadAvailable == false`.
5. SHA-256 match vs mismatch.
6. Empty `updateManifestUrl` → no HTTP.

Manual (after an OSS URL exists): short landscape remote, launch on ground, tap Download; launch while flying, confirm no dialog until land.

## Files (expected)

| File | Change |
| --- | --- |
| `src/Utilities/UpdateChecker.h/.cc` | Check, compare, flight gate, download orchestration |
| `src/Utilities/CMakeLists.txt` | Add sources |
| `src/QGCApplication.cc/.h` | Call `UpdateChecker`; remove old version-check helpers |
| `src/Settings/App.SettingsGroup.json` | `updateManifestUrl` |
| `src/Settings/AppSettings.h` | `DEFINE_SETTINGFACT(updateManifestUrl)` |
| `src/AppSettings/pages/General.SettingsUI.json` | Software Update group |
| `src/MainWindow/MainWindow.qml` | Compact update dialog |
| `src/QmlControls/QGroundControlQmlGlobal.h/.cc` | `updateChecker` property |
| `src/QmlControls/UpdateAvailableDialog.qml` | Dialog UI |
| `src/QmlControls/CMakeLists.txt` | Register dialog QML |
| Android manifest / cmake | `REQUEST_INSTALL_PACKAGES` if missing |
| `test/Utilities/UpdateCheckerTest.*` | Unit tests |
| `translations/qgc_source_zh_CN.ts` | Chinese copy |
