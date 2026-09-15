# Vehicle Config Summary Card Header Style

**Date:** 2026-09-15  
**Status:** Approved — implementing  
**Scope:** Vehicle Setup summary cards only (`VehicleSummary.qml`, `SummaryStatusPill.qml`)  
**Primary targets:** Landscape handheld remotes (Skydroid / SIYI), 1920×1200  
**Chosen look:** Layout A (keep left sidebar + card grid) + visual style 9 (colored title bars)

## Problem

Summary cards on 飞行器配置 are uniform gray panels. Ready vs needs-setup is a small pill. On a short landscape remote it is easy to miss a warning. The user chose to keep the current page structure and restyle the cards so status is visible from the title bar color.

## Goals

1. Keep the current layout: left sidebar, search, summary grid (1/2/3 columns from available width).
2. Restyle each summary card to **style 9**: full-width colored header, white/window body with existing chips.
3. Header color encodes status: ready = brand blue (`qgcPal.buttonHighlight`); needs setup / warning = orange (`qgcPal.warningText` / `qgcPal.colorOrange`).
4. Number, title, and status pill sit on the header with light text (`qgcPal.buttonHighlightText`).
5. Card body stays `qgcPal.window` (or a light card fill) so child summary QML chips are unchanged.
6. Touch target of the whole card still opens that component. Glove-sized header (~40–48 dp).
7. Light and dark QGC themes both work via palette tokens (no hardcoded mockup hex).

## Non-goals

- Sidebar redesign (icon rail, top categories, issues-first list)
- Reordering cards so warnings come first (that was style 1; not chosen)
- Changing which components appear or how visibility settings work
- Rewriting inner summary pages (`SensorsComponentSummary.qml`, etc.)
- New C++ APIs or Facts
- Brand PNG / SettingsButton icon colorize work from earlier in this thread

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Layout | A — existing left nav + summary grid |
| Visual | 9 — colored title bars |
| Ready header | `qgcPal.buttonHighlight` |
| Not-ready / warn header | Warning orange from `QGCPalette` |
| Body | Existing window / chip chrome |
| Order | Keep `_summaryOrder` |
| Pill copy | Existing `Ready` / `Needs setup` / `Not connected` / `Mapped` |

## UI rules

```text
┌─────────────────────────────────┐
│ 1  传感器              就绪     │  ← colored header
├─────────────────────────────────┤
│ [罗盘 ×1] [IMU ×3] …            │  ← existing loader content
└─────────────────────────────────┘
```

- `clip: true` + card `radius` keep the header painted inside rounded corners.
- Header is a `RowLayout` in a `Rectangle`, not a thin divider line.
- Incomplete cards must not show a green/ready treatment. `_statusOk` / `_statusWarn` already exist; bind header color to those.
- Dark theme: header still uses highlight / warning; body uses `qgcPal.window` so chips stay readable.

## Architecture

```text
VehicleSummary.qml
  Repeater card
    header Rectangle (status color)
      index · name · SummaryStatusPill
    body Item
      Loader summaryQmlSource   (unchanged)
```

`SummaryStatusPill` needs an inverted (on-accent) color path when placed on the header, or a small `onAccent: true` property. Do not duplicate a second pill component.

## Testing

- QML/UI: summary still lists visible components; click still navigates.
- Visual check on short landscape (~1920×1200): header readable, pills not clipped, 3-column grid unchanged.
- Dark and light theme: header contrast ≥ existing toolbar highlight buttons.
- A card with `setupComplete === false` uses the orange header, not blue.

## Out of this change

Settings / Configure sidebar `SettingsButton` PNG colorize fix stays separate. This spec only covers summary card chrome.
