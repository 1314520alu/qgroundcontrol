# Aircraft Model Selection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Do not commit unless the user asked.

**Goal:** Add a persist-only Aircraft Model radio group on Settings → General (ZY-XF100, ZY-XF100 Tethered, ZY-XF200 Tethered, Default).

**Architecture:** Store the choice as `appSettings.aircraftModel` (uint32 enum Fact). Teach the settings QML generator a `radiogroup` control that Repeats `QGCRadioButton` over `Fact.enumStrings` bound to `Fact.enumIndex`. Place a new group in `General.SettingsUI.json` immediately above Vehicle Preferences. No Fly-view or payload consumers this round.

**Tech Stack:** Qt6 Fact / SettingsFact, SettingsUI JSON → Python generator (`tools/generators/settings_qml`), `QGCRadioButton`, zh_CN `.ts` files.

## Global Constraints

- Control is vertical `QGCRadioButton` (`control: "radiogroup"`), not a combobox.
- Enum display order: ZY-XF100=1, ZY-XF100 Tethered=2, ZY-XF200 Tethered=3, Default=0. Default selected value is `0`.
- JSON UI files do not list radio options; Fact enum metadata is the only source of truth.
- Do not hand-edit generated `GeneralSettings.qml` (CMake regenerates it).
- Persist only: do not read this fact from Fly view, payload, video, or My Aircraft.
- Landscape remotes first: keep the four radios in a compact vertical column (`QGCRadioButton` / `ScreenTools` sizing).
- Do not commit unless the user asked.

---

### Task 1: Settings QML generator `radiogroup`

**Files:**
- Create: `tools/generators/settings_qml/templates/control_radiogroup.qml.j2`
- Modify: `tools/generators/settings_qml/emit.py`
- Modify: `tools/generators/settings_qml/README.md` (control-types table + objectName row)
- Test: `tools/tests/test_settings_qml_generator.py`

**Interfaces:**
- Consumes: existing `ControlDef.control`, `generate_page_qml()`, `has_enum_strings()`
- Produces: when `control == "radiogroup"`, generated QML contains `QGCRadioButton`, `enumStrings`, and `enumIndex`; facts without `enumStrings` raise `ValueError`

- [ ] **Step 1: Write the failing tests**

Add these methods to `class TestGeneratePageQml` in `tools/tests/test_settings_qml_generator.py` (the `settings_dir` fixture already defines `appSettings.colorScheme` with `enumStrings`):

```python
    def test_radiogroup_generates_radio_buttons(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(setting="appSettings.colorScheme", control="radiogroup"),
                    ]
                ),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "QGCRadioButton {" in qml
        assert "enumStrings" in qml
        assert "enumIndex" in qml
        assert "autoExclusive: false" in qml
        assert "LabelledFactComboBox" not in qml
        assert 'objectName: "settingsRadioGroup_colorScheme"' in qml

    def test_radiogroup_requires_enum(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(
                    controls=[
                        ControlDef(setting="appSettings.maxAlt", control="radiogroup"),
                    ]
                ),
            ]
        )
        with pytest.raises(ValueError, match="radiogroup"):
            generate_page_qml(page, settings_dir)

    def test_enum_without_radiogroup_still_combobox(self, settings_dir: Path):
        page = PageDef(
            groups=[
                GroupDef(controls=[ControlDef(setting="appSettings.colorScheme")]),
            ]
        )
        qml = generate_page_qml(page, settings_dir)
        assert "LabelledFactComboBox {" in qml
        assert "QGCRadioButton {" not in qml
```

`pytest` is already imported at the top of that file.

- [ ] **Step 2: Run tests to verify they fail**

```bash
uv run --project tools --extra test pytest -q \
  tools/tests/test_settings_qml_generator.py::TestGeneratePageQml::test_radiogroup_generates_radio_buttons \
  tools/tests/test_settings_qml_generator.py::TestGeneratePageQml::test_radiogroup_requires_enum \
  tools/tests/test_settings_qml_generator.py::TestGeneratePageQml::test_enum_without_radiogroup_still_combobox
```

Expected: first two FAIL (`QGCRadioButton` missing / no `ValueError`). Third already PASSES (combobox auto-detect).

- [ ] **Step 3: Add the Jinja template**

Create `tools/generators/settings_qml/templates/control_radiogroup.qml.j2`:

```jinja
{{ indent }}ColumnLayout {
{{ indent }}    objectName: "settingsRadioGroup_{{ fact_name }}"
{{ indent }}    Layout.fillWidth: true
{{ indent }}    spacing: 0
{% if enable_when %}
{{ indent }}    enabled: {{ enable_when }}
{% endif %}
{% if label_expr %}
{{ indent }}    QGCLabel {
{{ indent }}        text: {{ label_expr }}
{{ indent }}    }
{% endif %}
{{ indent }}    Repeater {
{{ indent }}        model: {{ fact_ref }}.enumStrings
{{ indent }}        QGCRadioButton {
{{ indent }}            autoExclusive: false
{{ indent }}            text: modelData
{{ indent }}            checked: {{ fact_ref }}.enumIndex === index
{{ indent }}            onClicked: {{ fact_ref }}.enumIndex = index
{{ indent }}        }
{{ indent }}    }
{{ indent }}}
```

`autoExclusive: false` is required so Qt radio grouping does not fight the `enumIndex` binding.

- [ ] **Step 4: Emit `radiogroup` before enum auto-detect**

In `tools/generators/settings_qml/emit.py`:

1. Import stays as-is; templates are loaded via `_env`.
2. Inside `_qml_control`, **before** `elif ctrl.control == "checkbox":`, add:

```python
    elif ctrl.control == "radiogroup":
        if not has_enum_strings(ctrl.setting, settings_dirs):
            raise ValueError(
                f"control 'radiogroup' requires enumStrings metadata, got setting {ctrl.setting!r}"
            )
        label_expr = qml_tr(ctrl.label, json_context) if ctrl.label else ""
        control_qml = _env.get_template("control_radiogroup.qml.j2").render(
            indent=indent,
            fact_ref=fact_ref,
            fact_name=ctrl.fact_name,
            enable_when=ctrl.enableWhen,
            label_expr=label_expr,
        )
        return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)
```

If `radiogroup` falls through to the `else` auto-detect branch, enum facts become comboboxes and Task 3 will look wrong.

- [ ] **Step 5: Document the control**

In `tools/generators/settings_qml/README.md`:

- Add a row to **Explicit `control` values**: `| `radiogroup` | Vertical `QGCRadioButton` Repeater over `fact.enumStrings` / `fact.enumIndex` |`
- Add a subsection:

```markdown
#### `radiogroup`

Requires Fact `enumStrings` / `enumValues`. Do not duplicate options in the UI JSON.
Optional `label` emits a `QGCLabel` above the radios; omit it when the group heading is enough.
```

- Add objectName row: `| Radio group | `settingsRadioGroup_<factName>` |`

- [ ] **Step 6: Re-run generator tests**

Same `pytest` command as Step 2. Expected: all three PASS.

---

### Task 2: `aircraftModel` Fact

**Files:**
- Modify: `src/Settings/App.SettingsGroup.json` (insert after `preferredVehicleClass`)
- Modify: `src/Settings/AppSettings.h` (after `DEFINE_SETTINGFACT(preferredVehicleClass)`)
- Modify: `src/Settings/AppSettings.cc` (after `DECLARE_SETTINGSFACT(AppSettings, preferredVehicleClass)`)
- Modify: `test/Settings/AppSettingsTest.h`
- Modify: `test/Settings/AppSettingsTest.cc`

**Interfaces:**
- Consumes: `DEFINE_SETTINGFACT` / `DECLARE_SETTINGSFACT` / SettingsFact QSettings path
- Produces: `AppSettings::aircraftModel()` → `Fact*` (`uint32`, default `0`, enum values `1,2,3,0`)

- [ ] **Step 1: Write the failing unit test**

`test/Settings/AppSettingsTest.h` — add slot:

```cpp
    void _aircraftModelDefaultAndRoundTrip();
```

`test/Settings/AppSettingsTest.cc` — add include and test. Keep existing tests. Insert:

```cpp
#include <QtCore/QScopeGuard>
```

(if not already present) and:

```cpp
void AppSettingsTest::_aircraftModelDefaultAndRoundTrip()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    QVERIFY(appSettings);

    Fact* const fact = appSettings->aircraftModel();
    QVERIFY(fact);

    const QVariant saved = fact->rawValue();
    const auto guard = qScopeGuard([fact, saved] { fact->setRawValue(saved); });

    QCOMPARE(fact->rawValue().toUInt(), 0u);

    const QStringList expectedStrings{
        QStringLiteral("ZY-XF100"),
        QStringLiteral("ZY-XF100 Tethered"),
        QStringLiteral("ZY-XF200 Tethered"),
        QStringLiteral("Default"),
    };
    QCOMPARE(fact->enumStrings(), expectedStrings);

    QCOMPARE(fact->enumValues().size(), 4);
    QCOMPARE(fact->enumValues().at(0).toUInt(), 1u);
    QCOMPARE(fact->enumValues().at(1).toUInt(), 2u);
    QCOMPARE(fact->enumValues().at(2).toUInt(), 3u);
    QCOMPARE(fact->enumValues().at(3).toUInt(), 0u);

    fact->setRawValue(1);
    QCOMPARE(fact->rawValue().toUInt(), 1u);
    QCOMPARE(fact->enumIndex(), 0);

    fact->setRawValue(2);
    QCOMPARE(fact->rawValue().toUInt(), 2u);
    QCOMPARE(fact->enumIndex(), 1);

    fact->setRawValue(3);
    QCOMPARE(fact->rawValue().toUInt(), 3u);
    QCOMPARE(fact->enumIndex(), 2);

    fact->setRawValue(0);
    QCOMPARE(fact->rawValue().toUInt(), 0u);
    QCOMPARE(fact->enumIndex(), 3);
}
```

`AppSettings.h` is already included via `"AppSettings.h"`.

- [ ] **Step 2: Build the test to confirm it does not compile**

```bash
just build
```

Expected: compile error `no member named 'aircraftModel'` (or equivalent) in `AppSettings`.

- [ ] **Step 3: Add Fact metadata + accessors**

Insert this fact in `src/Settings/App.SettingsGroup.json` immediately after the `preferredVehicleClass` object:

```json
        {
            "name": "aircraftModel",
            "shortDesc": "Product airframe used by this ground station.",
            "longDesc": "Selects a Zhongyun airframe profile. This value is stored only; other features may read it later.",
            "type": "uint32",
            "enumStrings": "ZY-XF100,ZY-XF100 Tethered,ZY-XF200 Tethered,Default",
            "enumValues": "1,2,3,0",
            "default": 0,
            "label": "Aircraft Model",
            "keywords": "aircraft model,airframe,ZY-XF100,ZY-XF200,tethered,机型"
        },
```

`AppSettings.h` after `DEFINE_SETTINGFACT(preferredVehicleClass)`:

```cpp
    DEFINE_SETTINGFACT(aircraftModel)
```

`AppSettings.cc` after `DECLARE_SETTINGSFACT(AppSettings, preferredVehicleClass)`:

```cpp
DECLARE_SETTINGSFACT(AppSettings, aircraftModel)
```

Do not set `qgcRebootRequired`.

- [ ] **Step 4: Rebuild and run the unit test**

```bash
just build
ctest --test-dir build -R AppSettingsTest --output-on-failure
```

Expected: `AppSettingsTest` PASS, including `_aircraftModelDefaultAndRoundTrip`.

---

### Task 3: General page group

**Files:**
- Modify: `src/AppSettings/pages/General.SettingsUI.json`

**Interfaces:**
- Consumes: Task 1 `radiogroup` emitter; Task 2 `appSettings.aircraftModel`
- Produces: Settings → General group heading `Aircraft Model` with four radios; group `objectName` `settingsGroup_AircraftModel`

- [ ] **Step 1: Insert the group above Vehicle Preferences**

In `src/AppSettings/pages/General.SettingsUI.json`, add this group **immediately before** the existing `"heading": "Vehicle Preferences"` group:

```json
        {
            "heading": "Aircraft Model",
            "keywords": ["aircraft model", "airframe", "ZY-XF100", "ZY-XF200", "tethered", "机型", "系留"],
            "controls": [
                {
                    "setting": "appSettings.aircraftModel",
                    "control": "radiogroup"
                }
            ]
        },
```

No `label` on the control (group heading is the menu name).

- [ ] **Step 2: Rebuild so CMake regenerates `GeneralSettings.qml`**

```bash
just build
```

Expected: build succeeds. Confirm generated QML (do not edit it) contains the radios:

```bash
rg -n "aircraftModel|QGCRadioButton|Aircraft Model" build/**/GeneralSettings.qml
```

Expected snippets: `settingsGroup_AircraftModel`, `settingsRadioGroup_aircraftModel`, `QGCRadioButton`, `appSettings.aircraftModel.enumIndex`.

If `rg` finds nothing, the generator change from Task 1 is not being picked up — check `src/AppSettings/CMakeLists.txt` globs include `tools/generators/settings_qml/templates/*.j2`.

---

### Task 4: zh_CN translations

**Files:**
- Modify: `translations/qgc_json_zh_CN.ts` (Fact metadata + SettingsUI heading)
- Modify: `translations/qgc_source_zh_CN.ts` (qsTranslate group heading used by generated QML)

**Interfaces:**
- Consumes: source strings `Aircraft Model`, `ZY-XF100,ZY-XF100 Tethered,ZY-XF200 Tethered,Default`
- Produces: zh UI 机型选择 / ZY-XF100 / ZY-XF100系留 / ZY-XF200系留 / 默认

- [ ] **Step 1: Fact metadata in `qgc_json_zh_CN.ts`**

Inside context `App.SettingsGroup.json` (after the `preferredVehicleClass` messages is fine), add:

```xml
    <message>
      <extracomment>.QGC.MetaData.Facts[aircraftModel].shortDesc</extracomment>
      <location filename="/home/runner/work/qgroundcontrol/qgroundcontrol/src/Settings/App.SettingsGroup.json"/>
      <source>Product airframe used by this ground station.</source>
      <translation>本地面站使用的产品机型。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[aircraftModel].longDesc</extracomment>
      <location filename="/home/runner/work/qgroundcontrol/qgroundcontrol/src/Settings/App.SettingsGroup.json"/>
      <source>Selects a Zhongyun airframe profile. This value is stored only; other features may read it later.</source>
      <translation>选择中云机型配置。当前仅保存选择，其他功能稍后可读取。</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[aircraftModel].enumStrings</extracomment>
      <translatorcomment>Only use english comma ',' to separate strings</translatorcomment>
      <location filename="/home/runner/work/qgroundcontrol/qgroundcontrol/src/Settings/App.SettingsGroup.json"/>
      <source>ZY-XF100,ZY-XF100 Tethered,ZY-XF200 Tethered,Default</source>
      <translation>ZY-XF100,ZY-XF100系留,ZY-XF200系留,默认</translation>
    </message>
    <message>
      <extracomment>.QGC.MetaData.Facts[aircraftModel].label</extracomment>
      <location filename="/home/runner/work/qgroundcontrol/qgroundcontrol/src/Settings/App.SettingsGroup.json"/>
      <source>Aircraft Model</source>
      <translation>机型选择</translation>
    </message>
```

Keep English commas in the translated enum list (same rule as `preferredVehicleClass`).

- [ ] **Step 2: SettingsUI heading in `qgc_json_zh_CN.ts`**

Inside context `General.SettingsUI.json`, **immediately before** the `Vehicle Preferences` heading message, add:

```xml
    <message>
      <extracomment>.groups[Aircraft Model].heading</extracomment>
      <location filename="/home/runner/work/qgroundcontrol/qgroundcontrol/src/AppSettings/pages/General.SettingsUI.json"/>
      <source>Aircraft Model</source>
      <translation>机型选择</translation>
    </message>
```

- [ ] **Step 3: Generated-QML heading in `qgc_source_zh_CN.ts`**

Inside existing context `General.SettingsUI.json`, add a non-vanished entry (keep any vanished siblings as-is):

```xml
    <message>
        <source>Aircraft Model</source>
        <translation>机型选择</translation>
    </message>
```

Radio labels come from Fact metadata (`qgc_json`), not this file.

- [ ] **Step 4: Rebuild translations into the app**

```bash
just build
```

Expected: success. No extra `lupdate` run required if the zh strings were added by hand as above.

---

### Task 5: Verify

**Files:** none (run only)

- [ ] **Step 1: Generator tests**

```bash
uv run --project tools --extra test pytest -q tools/tests/test_settings_qml_generator.py
```

Expected: PASS (full file, not only the new tests).

- [ ] **Step 2: AppSettings unit test**

```bash
ctest --test-dir build -R AppSettingsTest --output-on-failure
```

Expected: PASS.

- [ ] **Step 3: Lint the touched Python**

```bash
just lint
```

If `just lint` is too broad / slow, at least:

```bash
uv run --project tools ruff check tools/generators/settings_qml/emit.py tools/tests/test_settings_qml_generator.py
```

Expected: no new findings on these files.

- [ ] **Step 4: Manual (short landscape)**

Open Settings → General on a short landscape viewport (Skydroid G20 / SIYI MK32 class, or a ~960×600 dp window). Confirm:

1. Group **机型选择** / Aircraft Model sits above Vehicle Preferences.
2. Four radios, order ZY-XF100 → ZY-XF100系留 → ZY-XF200系留 → 默认.
3. Fresh install / cleared settings: 默认 selected.
4. Select ZY-XF100, quit, relaunch: still ZY-XF100.
5. Fly view / payload / My Aircraft unchanged.

---

## Self-review

| Spec item | Task |
| --- | --- |
| General group above Vehicle Preferences | Task 3 |
| Four radios, locked order + Default=0 | Task 2 metadata + Task 3 UI |
| Persist via AppSettings Fact | Task 2 |
| `radiogroup` Repeater / enumIndex, no option list in UI JSON | Task 1 |
| zh 机型选择 / 系留 / 默认 | Task 4 |
| AppSettingsTest + generator tests | Tasks 1, 2, 5 |
| No Fly/payload consumers | no task (non-goal) |
| Do not edit generated QML | Task 3 |
