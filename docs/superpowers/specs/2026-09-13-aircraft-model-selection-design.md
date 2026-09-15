# Aircraft Model Selection (General Settings)

**Date:** 2026-09-13  
**Status:** Approved  
**Plan:** `docs/superpowers/plans/2026-09-13-aircraft-model-selection.md`

## Problem

Operators on this GCS fork fly a small set of Zhongyun airframes. There is no place in Settings to pick the product model. Existing nearby UI is unrelated: General → Vehicle Preferences is PX4/ArduPilot vehicle *class*, and My Aircraft is Weitong satcom.

## Goals

1. Add an **Aircraft Model** group on **Settings → General**, above Vehicle Preferences.
2. Four exclusive radio buttons, glove-friendly vertical `QGCRadioButton` list, order:

   1. ZY-XF100
   2. ZY-XF100 Tethered (zh: ZY-XF100系留)
   3. ZY-XF200 Tethered (zh: ZY-XF200系留)
   4. Default (zh: 默认)

3. Persist the choice in `AppSettings` via the Fact system. Default selected value is **Default (0)**. No restart required.
4. This round is persist-only: no Fly view, payload, video, or My Aircraft behavior reads the setting yet.

## Non-goals

- Changing GCS behavior based on the selected model
- Linking to My Aircraft / Weitong satcom
- Combo box or other control types
- Auto-detecting model from the connected vehicle
- QML UI automation tests for the General page

## Decisions (locked)

| Topic | Choice |
| --- | --- |
| Placement | Settings → General, new group **Aircraft Model**, immediately above Vehicle Preferences |
| Control | Vertical `QGCRadioButton` group (`control: "radiogroup"`) |
| Storage | `appSettings.aircraftModel` (`uint32` enum Fact) |
| Default | `0` = Default, last radio in the list |
| Consumers | None this round |
| Reboot | Not required |
| Invalid saved value | Existing `SettingsFact` load path (`convertAndValidateRaw`); UI only offers the four enum options |

## Data

Fact metadata in `src/Settings/App.SettingsGroup.json`:

| `enumStrings` (source) | `enumValues` |
| --- | --- |
| ZY-XF100 | 1 |
| ZY-XF100 Tethered | 2 |
| ZY-XF200 Tethered | 3 |
| Default | 0 |

- `type`: `uint32`
- `default`: `0`
- `label`: `Aircraft Model`
- Keywords include aircraft model, ZY-XF100, tethered, 机型

`AppSettings.h` adds `DEFINE_SETTINGFACT(aircraftModel)`. Persistence is the existing `SettingsFact` → `QSettings` path.

Chinese copy lives in `translations/qgc_source_zh_CN.ts` (group heading 机型选择; Default → 默认; Tethered suffixes → 系留). Product codes stay `ZY-XF100` / `ZY-XF200`.

## Architecture

```text
App.SettingsGroup.json          aircraftModel enum Fact (source of truth)
AppSettings.h                   DEFINE_SETTINGFACT(aircraftModel)

General.SettingsUI.json         new "Aircraft Model" group, control radiogroup
settings_qml generator          emit QGCRadioButton Repeater from Fact.enumStrings
                                bound to Fact.enumIndex (no duplicated option list)

QSettings                       persist rawValue under the App settings group
```

Generated control shape (conceptual):

- Outer `ColumnLayout` already used by settings pages (visibility + shortDescription).
- No extra control label when the group heading is “Aircraft Model”.
- `Repeater { model: fact.enumStrings }` → `QGCRadioButton { text: modelData; checked: fact.enumIndex === index; onClicked: fact.enumIndex = index }`.

Radio labels therefore follow Fact enum translations at runtime. Adding a later model is metadata + translation only, plus regenerating the settings page.

The settings QML generator currently auto-maps enums to comboboxes and has no `radiogroup`. Add a `radiogroup` branch in `tools/generators/settings_qml/emit.py` that emits the Repeater above. JSON UI files do not list options; `control: "radiogroup"` plus Fact enum metadata is enough.

## Testing

1. **`AppSettingsTest`**: default `rawValue` is `0`; `enumStrings` / `enumValues` match the table; setting `1`, `2`, `3` round-trips.
2. **`tools/tests/test_settings_qml_generator.py`**: `control: "radiogroup"` emits `QGCRadioButton` and `enumIndex`.
3. **Manual**: on a short landscape remote (or simulated viewport), open General, select each radio, restart app, confirm the last choice is restored.

No Fly-view, payload, or satcom tests this round.

## Files (expected)

| File | Change |
| --- | --- |
| `src/Settings/App.SettingsGroup.json` | New `aircraftModel` fact |
| `src/Settings/AppSettings.h` | `DEFINE_SETTINGFACT(aircraftModel)` |
| `src/AppSettings/pages/General.SettingsUI.json` | New group + radiogroup control |
| `tools/generators/settings_qml/emit.py` (+ README control-type table) | `radiogroup` support |
| `tools/tests/test_settings_qml_generator.py` | Generator coverage |
| `test/Settings/AppSettingsTest.*` | Fact default / enum / round-trip |
| `translations/qgc_source_zh_CN.ts` | zh strings |

`GeneralSettings.qml` is produced at build time from `General.SettingsUI.json`; do not hand-edit it.
