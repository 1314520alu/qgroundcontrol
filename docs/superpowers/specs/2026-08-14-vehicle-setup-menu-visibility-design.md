# Vehicle Setup Menu Visibility

**Date:** 2026-08-14  
**Status:** Approved — implementing  
**Scope:** Application Settings (General) + Vehicle Setup sidebar (`VehicleConfigView.qml`)  
**Primary targets:** Landscape handheld remotes (Skydroid / SIYI); Joystick is optional

## Problem

Vehicle Setup’s left sidebar lists ~20 items (Summary, Frame, ESC, Joystick, Tuning, Scripting, …). On short-height remotes this is noisy. Joystick is for USB gamepads on PC and is not needed when the remote’s sticks use **Radio**. Users need a way to choose which menu entries appear, with a remote-friendly default.

## Goals

1. **User-selectable visibility** — checkboxes in Application Settings → General for which Vehicle Setup components appear in the sidebar.
2. **Remote-first defaults** — default visible set is essential + safety items only; Joystick and other advanced pages start hidden.
3. **Fixed recommended order** — Summary always first; remaining items use a fixed priority order (no drag-and-drop).
4. **Immediate effect** — changing checkboxes updates the sidebar without reboot.
5. **Stable persistence** — store comma-separated component IDs (not translated display names).

## Non-goals

- Drag-and-drop / user-defined sort order
- Checkbox control of Summary, Parameters, Firmware, or Optical Flow (keep existing special-button rules)
- Per-firmware / per-vehicle-class default profiles
- “Show all unknown components” toggle
- Changing which components the autopilot plugin *registers* — only UI visibility

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Default visible set | Option A — essential + safety |
| Settings location | Application Settings → General |
| Sort | Fixed recommended order; Summary pinned first |
| Storage | Single string Fact (comma-separated IDs) |

## Architecture

```text
AppSettings.vehicleSetupVisibleComponents (string Fact)
        │
        ▼
AppSettings helpers (parse / toggle / reset / isVisible)
        │
        ├─► VehicleSetupMenuVisibilitySettings.qml  (General settings UI)
        └─► VehicleConfigView.qml                   (sidebar filter + sort)
```

- **Fact default** = remote-first ID list.
- **Settings UI** embeds a hand-written QML component via SettingsUI `"component"` (checkbox grids are a poor fit for generated Fact controls alone).
- **Sidebar** keeps existing `setupSource` / search / special-button logic; adds visibility filter and priority sort on top.

## Data model

### Fact

| Field | Value |
| --- | --- |
| Name | `vehicleSetupVisibleComponents` |
| Group | `AppSettings` |
| Type | `string` |
| Default | `frame,sensors,radio,flightModes,power,esc,escTelemetry,motors,flightSafety,failsafes` |

Empty string after user edits is allowed and means “no vehicle components shown” (Summary / Parameters / Firmware still follow their own rules).

### Stable IDs

| ID | Display name | Default visible |
| --- | --- | --- |
| `frame` | Frame | yes |
| `sensors` | Sensors | yes |
| `radio` | Radio | yes |
| `flightModes` | Flight Modes | yes |
| `power` | Power | yes |
| `esc` | ESC | yes |
| `motors` | Motors | yes |
| `flightSafety` | Flight Safety | yes |
| `failsafes` | Failsafes | yes |
| `joystick` | Joystick | no |
| `tuning` | Tuning | no |
| `tuningAdvanced` | Tuning - Advanced | no |
| `servo` | Servo Outputs | no |
| `gimbal` | Gimbal | no |
| `airspeed` | Airspeed | no |
| `logging` | Logging | no |
| `scripting` | Scripting | no |
| `remoteSupport` | Remote Support | no |
| `wifiBridge` | WiFi Bridge | no |
| `heli` | Heli | no |
| `lights` | Lights | no |
| `followMe` | Follow Me | no |

**PX4 ID mapping (explicit):**

| PX4 component | ID |
| --- | --- |
| Airframe | `frame` |
| Sensors | `sensors` |
| Radio | `radio` |
| Flight Modes | `flightModes` |
| Power | `power` |
| Motor **or** Actuator | `motors` |
| Safety | `flightSafety` |
| Tuning | `tuning` |
| Flight Behavior | `tuningAdvanced` |
| Joystick | `joystick` |
| WiFi Bridge (ESP8266) | `wifiBridge` |
| Syslink | *(unmapped → hidden)* |

APM components without a catalog row (if any appear later) are **hidden** until an ID is added.

### ID resolution

Prefer matching on `summaryQmlSource` / setup QML path / C++ type name via a single lookup table (locale-independent). Do **not** key off translated `comp.name`.

## Sidebar behavior

1. **Summary** — always first, always visible, not in the checkbox list.
2. **Vehicle components** — visible only if:
   - `setupSource` is non-empty (existing rule), and
   - search filter matches (existing rule), and
   - resolved ID is present in `vehicleSetupVisibleComponents`.
3. **Sort order** (after Summary):

   `frame` → `sensors` → `radio` → `flightModes` → `power` → `esc` → `motors` → `flightSafety` → `failsafes` → then any other checked IDs by display-name localeCompare.

4. **Parameters / Firmware / Optical Flow** — unchanged visibility conditions; not part of this Fact.
5. **Search** — hidden components do not appear in search results.
6. **Selection recovery** — if the currently selected component becomes hidden, navigate to Summary so the panel is not blank.
7. **Filtering / sort location** — implement in `VehicleConfigView.qml` (filter + display order). Autopilot plugins keep registering the full component list; their internal alphabetical sort must not dictate sidebar order (sidebar applies the priority table when presenting).

## Settings UI

### General.SettingsUI.json

New group:

```json
{
  "heading": "Vehicle Setup Menu",
  "keywords": ["vehicle setup", "menu", "summary", "esc", "radio", "joystick"],
  "component": "VehicleSetupMenuVisibilitySettings.qml"
}
```

### VehicleSetupMenuVisibilitySettings.qml

- Compact two-column checkbox list (short landscape remotes).
- Shows the **full catalog** of known IDs (not only components on the currently connected vehicle), so users can pre-configure offline.
- Labels = translated display names; persistence = stable IDs.
- “Restore defaults” calls AppSettings reset helper.
- Writes Fact on each toggle; no reboot required.

### AppSettings helpers (Q_INVOKABLE)

- `vehicleSetupVisibleIdList()` → `QStringList`
- `isVehicleSetupComponentVisible(const QString &id) const`
- `setVehicleSetupComponentVisible(const QString &id, bool visible)`
- `resetVehicleSetupVisibleComponents()`

## Testing

1. **Unit (AppSettings helpers)**  
   - Default string parses to the nine expected IDs.  
   - Toggle off `radio` removes it; toggle on `joystick` adds it.  
   - Reset restores the default string.  
   - Unknown ID → `isVehicleSetupComponentVisible` is false.

2. **Manual / optional QML**  
   - Fresh settings: sidebar shows Summary + default nine (when the vehicle exposes them); Joystick absent.  
   - Enable Joystick in General → appears after Failsafes (or in secondary alpha group).  
   - Disable currently open page → panel returns to Summary.

## Out of scope follow-ups

- Per-airframe default presets  
- Including Parameters/Firmware in the checkbox list  
- User drag reorder  

## File map (implementation)

| File | Change |
| --- | --- |
| `src/Settings/App.SettingsGroup.json` | Add `vehicleSetupVisibleComponents` |
| `src/Settings/AppSettings.h` / `.cc` | Fact + helpers |
| `src/AppSettings/pages/General.SettingsUI.json` | New group with `component` |
| `src/AppSettings/VehicleSetupMenuVisibilitySettings.qml` | Checkbox UI (new) |
| `src/Vehicle/VehicleSetup/VehicleConfigView.qml` | Filter, sort, selection recovery |
| `src/AppSettings/CMakeLists.txt` (or QML module lists) | Register new QML if required |
| `test/Settings/` (or existing AppSettings tests) | Helper unit tests |

## Success criteria

- New install / reset defaults: remote-friendly short sidebar with Summary first.
- User can show Joystick (and other optional pages) from General settings.
- No reboot required; hiding the active page lands on Summary.
- Existing Parameters / Firmware / Optical Flow behavior unchanged.
