# ESC Telemetry UI (Adaptive)

**Date:** 2026-08-14  
**Status:** Locked — implementing  
**Scope:** New Vehicle Setup page for ArduPilot (`APMESCTelemetryComponent`)  
**Visual locks:**

- [`assets/esc-telemetry-ui-preview-4esc-cards-unirc10-pro.png`](assets/esc-telemetry-ui-preview-4esc-cards-unirc10-pro.png) — 1–4 cards  
- [`assets/esc-telemetry-ui-preview-8esc-table-unirc10-pro.png`](assets/esc-telemetry-ui-preview-8esc-table-unirc10-pro.png) — 5–8 compact table  
- [`assets/esc-telemetry-ui-preview-16esc-table-unirc10-pro.png`](assets/esc-telemetry-ui-preview-16esc-table-unirc10-pro.png) — 9–16 dense table  

**Temporary verification:** SIYI UniRC 10 Pro (1920×1200); also G20-class short landscape.

## Problem

ESC setup page is configuration/calibration only. Live MAVLink ESC telemetry (`ESC_INFO` / `ESC_STATUS`) exists on `vehicle.escs` and in the toolbar indicator, but Vehicle Setup lacks a dedicated read-only monitor sized for landscape remotes and up to 16 ESCs.

## Goals

1. Sidebar page **ESC Telemetry** (after ESC), read-only.
2. Adaptive layout by ESC count: cards (≤4) / compact table (5–8) / dense table (9–16).
3. Bind existing Facts only — no new parameter storage.
4. One content viewport on short landscape (inner scroll only if unavoidable for empty/error copy).

## Non-goals

- Changing ESC config/calibration page  
- Parsing legacy `ESC_TELEMETRY_1_TO_4`  
- Motor test / writing MOT_* / SERVO_*  
- Replacing toolbar `EscIndicator`

## Data (MAVLink → Facts)

| UI | Fact / notes |
|----|----------------|
| Index | `id` |
| Online | `info` bitmask bit `(id % 4)` |
| Protocol (header) | `connectionType` |
| RPM | `rpm` |
| Voltage | `voltage` |
| Current | `current` |
| Temperature | `temperature` — raw centi-°C; display ÷100; 32767 → — |
| Errors | `errorCount` |
| Failures | `failureFlags` bitmask → chips |

Cap display at **16**. Empty `escs` → empty-state banner.

## Layout

Shared header: title, subtitle, `SummaryStatusPill`, chips (healthy / protocol / errors / online).

| `n` | Mode |
|-----|------|
| 1–4 | 2×2 (or fewer) white cards, green/red edge |
| 5–8 | Same columns as dense table; taller equal rows |
| 9–16 | Servo-style dense equal-height rows, fill remaining height |
