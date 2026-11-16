# Menus: settings, presets, notes and debug pages

Scope: every <Game> build. Use this layout as the template for a new project's menus. Source of the numbers: the Halo Dreamcast port (dc-halo). Paths are relative to the dc-halo repo root unless they name another repo. Checked against dc-halo on 2026-11-15.

Labels: **MEASURED**, **CODE**, **ESTIMATE**, **GUESS**.

Generic version: the page names below are dc-halo's. Keep the shape (page list, row types, presets, notes, debug pages), and rename the pages to your game's.

## 1. The rule

Start every new project from this layout: the same page list order, the same row types, the same preset-to-setting mapping, the same note rules, and the same debug gate. Only the names and the values change.

## 2. Page map (dc-halo)

**Main menu** (page ids `MAIN_*` in `src/menu.h`): CAMPAIGN, MULTIPLAYER, FIREFIGHT, GALLERY (the model viewer, named ONI DATABASE in the game), DISCOGRAPHY, SETTINGS, CREDITS, RECORDINGS (THEATRE), EXTRAS.

**EXTRAS** (`EXTRAS_*` in `src/menu.h`): ONI DATABASE, DISCOGRAPHY, THEATRE, ACHIEVEMENTS, SKULLS. Achievement pages are in [ACHIEVEMENTS.md](ACHIEVEMENTS.md).

**SETTINGS** (`SROW_*`, labels in `src/menu_draw.c`, line 3557): LANGUAGE, SUBTITLES, CONTROLS, VIDEO, AUDIO, CONSOLE, THEME, MASTER CHIEF, RESTORE ALL DEFAULTS, DEBUG (DEBUG shows only on dev images, section 7).

**VIDEO tabs** (`VIDEO_TAB_*` in `src/menu.h`): DISPLAY, CAMERA, DISTANCE, OBJECTS, EFFECTS. The L and R triggers move between tabs (`INPUT_TAB_PREV`, `INPUT_TAB_NEXT`). The tab table is `video_tab_rows` (`src/menu.c`).

**Other pages with a fixed job**:

| Page | Job | Source |
|---|---|---|
| Video confirm | a resolution change waits until the player picks it | `PAGE_VIDEO_CONFIRM`, `video_pending` |
| Defaults confirm | KEEP (the default) or RESTORE | `PAGE_DEFAULTS_CONFIRM`, `defaults_choice` |
| Wide test | a live test screen; it reverts after 10 s unless the player accepts (`WIDE_TEST_MS` 10000) | `src/menu.h` |
| Experience (first start) | picks a preset: PERFORMANCE or BALANCED, two rows | `PAGE_EXPERIENCE`, `src/menu_draw.c` around line 3903 |
| Theme, scheme and filter asks | first-start questions | `PAGE_THEME_ASK`, `PAGE_SCHEME_ASK`, `PAGE_FILTER_ASK` |

## 3. Page layout (list and card)

Source: `docs/archive/ui/MENU_LAYOUT.md` (dc-halo, layout dated 2026-10-22; not re-measured for this doc). Menu pixels, 640 x 480.

| Role | Font and size | Colour |
|---|---|---|
| Header | INTERSTATE at 150 %, accent rule under it | 0xafe4fc |
| Item in the list | LARGE at 90 %, row pitch 28 px | 0xa5c3d6 |
| Selected item | LARGE at 90 %, accent bar and glow | 0xf0faff |
| Value in the list | SMALL, right aligned, only if it fits. Longer values go to the card | 0x8cdcff selected, 0x708ea4 otherwise |
| Card name | LARGE, up to two lines | 0xafe4fc |
| Card value | INTERSTATE at 100 % (LARGE if too wide). Accent chevrons when LEFT or RIGHT change it | white |
| Card options | SMALL chips, the current one lit | |
| Note | SMALL, wrapped to the card | 0xbed6e6 |
| Help strip | SMALL with button chips, full width at y 408 | |
| Footer | SMALL, faint | 0x50697d |

Geometry: list x 40 to 272; card x 288 to 614; card top 140; card bottom 400; help strip from 408; the action safe line is 456.

Rules from the same doc: chips never wrap (one row, spacing and size shrink in steps); a list row whose value does not fit beside its label puts the value on a second line, so the row does not change height while the value changes.

## 4. Row types

| Row type | Example in dc-halo | How it behaves | Source |
|---|---|---|---|
| Option step | OBJECT DISTANCE | LEFT and RIGHT step through the values and stop at the ends. A (accept) steps forward and wraps from the last value to the first. The row marks each end (`MENU_END_FIRST`, `MENU_END_LAST`) | `step_option`, `ends_at` in `src/menu.c` |
| Chips | the card's choice chips | every value in one row; the gap and text shrink in steps | `docs/archive/ui/MENU_LAYOUT.md` |
| Toggle | FAR PROPS (Off, On) | two values, the same step rule | `src/menu.c` |
| Preset row | QUALITY | picks a preset and sets many rows at once. Changing any one of those rows after that reads CUSTOM | `video_preset_match`, section 5 |
| Action | RESTORE DEFAULTS | leads to the confirm page | `PAGE_DEFAULTS_CONFIRM` |
| Confirm | video resolution | the change is held until the player chooses | `video_pending` |
| Live test | WIDE test (768 x 480) | reverts after 10 s unless accepted | `WIDE_TEST_MS` |
| Session-only slider | FAR ENEMY CAP, ENEMY FADE START (debug) | not saved; its note says `Not saved.`; default equals the release rule | `src/menu.h` lines 198 to 213 |
| Info row | a live read-out (CARVE START shows the light level where the player stands) | read-only; the note is the live text | `src/menu_draw.c` around line 1240 |

Every row's note is computed in `src/menu_draw.c` (one `switch` on the row id, setting `label`, `note` and `value`).

## 5. Presets: how a preset maps to the options

dc-halo has four preset rows in the table and two active presets:

- `VIDEO_PRESETS` = 4 rows in `video_presets` (`src/menu.c`). `VIDEO_PRESETS_ACTIVE` = 2: PERFORMANCE and BALANCED. The owner's spec of 2026-11-12 removed FIDELITY and XTREME from the menu. The two old rows stay so that an old save reads as BALANCED (`src/menu.h` lines 575 to 581).
- Each preset is a full `video_preset_t` struct: resolution, draw distance, object detail, particles, flash objects, detail distance, bump distance, the reflection rows, object lighting, shadows, anti-aliasing, object distance, armour detail, vehicle particles, cutscene detail, far props, water, mip bias, eye adapt, and (when the FX subset is built) bloom, rays and lens rows.
- `video_preset_match_n` compares every field of the live setting with each active preset, in order. A match returns the preset number. No match returns CUSTOM. Resolution is ignored at 768 x 480 (the preset keeps its name). Some fields accept an "any" value (`VIDEO_*_ANY`) while an old save is migrated.
- `video_apply_options` (`src/level.c`) reads the same values at level start and on every resume.
- Default preset: BALANCED (`VIDEO_PRESET_DEFAULT` 1).

Example, PERFORMANCE and BALANCED on the rows that matter for the look (`src/menu.c` `video_presets`, CODE):

| Row | PERFORMANCE | BALANCED |
|---|---|---|
| Draw distance | DYNAMIC (STRONG), 30 to 90 % | DYNAMIC, 50 to 90 % |
| Object detail | DYNAMIC | DYNAMIC |
| Detail distance | 9 wu = 27 m (index 3) | 16 wu = 49 m (index 5) |
| Object distance | 16 wu (index 1) | 16 wu (index 1) |
| Particles | 32 | 96 |
| Vehicle particles | 50 % | 50 % |
| Anti-aliasing | off | 2X |
| Far props | on | on |

Rule (`AGENTS.md` in dc-halo): when a preset value or a side effect changes, update the menu note, `docs/GAME_GUIDE.md` (the "Video settings" preset table) and `docs/CONTROLS.md` in the same change. `menu.c video_presets` and `video_apply_options` must agree.

Stale text found on 2026-11-15: `docs/GAME_GUIDE.md` lines 30 and 42 still say the game has four presets (FIDELITY and XTREME included). The code has two. Fix the guide before anyone copies it.

Generic: two presets to start: a Performance preset and a Balanced default. Add a third only with an owner decision and a cost line (RAM, VRAM, ms).

## 6. Menu notes (what the player reads)

Rules, from dc-halo `AGENTS.md` and `docs/archive/ui/MENU_WORDING.md`:

1. **WHAT, never WHERE.** The note says what the setting does.
2. **One short sentence.** About 60 characters; 100 at most. Cut, never add.
3. **No model, map, tag, pass or engine name. No mechanism.** The player does not need the technique.
4. **Do not repeat the option name** in its note.
5. **Numbers the player can match on screen only**: seconds, %, metres, on or off. No triangle or vertex counts. No world units (wu). Metres = wu x 3.048 (`metres()` in `src/menu_draw.c`).
6. **Player words only.** No map codes (A10, D20_START). Level and enemy names as the game prints them. No spoilers.
7. **No ranking words in a new note** ("slowest", "worst"). State the cost as a fact: "Uses more CPU". The owner's 2026-11-10 ruling: "Recommended" and "Best at" are recommendations, not judgements, and existing notes stay unless the owner names one.

Examples from dc-halo, with their length in characters (CODE):

| Setting | Note | Length |
|---|---|---|
| OBJECT DISTANCE | Props, vehicles and allies fade past this. | 42 |
| OBJECT DETAIL (LOW) | Models use their lightest version. | 34 |
| OBJECT DETAIL (DYNAMIC) | The enemy you aim at is shown in full detail. | 45 |
| CUTSCENE DETAIL | The heroes in cutscenes. MEDIUM and up: Cortana glows. | 54 |
| FAR PROPS | Trees and rocks stay to 152 m. OFF: they fade with objects. Next load. | 70 (over the target: it is a two-sentence note, kept as is; do not copy the length) |
| FAR ENEMY CAP (debug) | Most idle enemies drawn, furthest go first. Not saved. | 54 |

Two more rules for notes that carry numbers:

- Put the number the player sees in the note: "Trees and rocks stay to 152 m." (the same value as the row's far clip).
- Say when a change takes effect: "Next load." or "Quit to the title to change it." Keep the tense plain.

Generic: use your game's units. Keep the rule that a note never says where a setting lives in the code and never names an engine.

## 7. Debug pages

**The gate.** Debug pages exist only in dev images (`menu_dev_image`). The title code (UP UP DOWN DOWN UP UP UP UP on PRESS START) opens SETTINGS > DEBUG for one session. A normal boot hides it (`docs/GAME_GUIDE.md`, "Debug (dev images only)").

**The tabs**: INFO, RENDER, COMPARE, TOOLS, SKULLS (`VIDEO_TAB_INFO`.. `VIDEO_TAB_SKULLS`).

| Tab | What is on it (examples) |
|---|---|
| INFO | FAR ENEMY CAP and ENEMY FADE START (session-only sliders); DEBUG INFO; the live read-outs |
| RENDER | the flashlight beam and lamp tuning rows (session tuning) |
| COMPARE | switches that turn one look off to compare: enemy hide, far trees, far rocks, balanced near set, sphere shadows |
| TOOLS | teleport, next map, god, freeze, give shield or camouflage, the theatre pick, direct render |
| SKULLS | a developer override for one skull |

**Row kinds on the debug pages:**

1. **Info row**: read-only. Its note is a live read-out (`CARVE START`).
2. **Tuner**: a step row that changes how the game draws or plays, for testing only.
3. **Session-only slider**: not saved. Its note says `Not saved.` Its default is the release rule, so an untouched slider changes nothing (`src/menu.h` lines 198 to 213: FAR ENEMY CAP defaults to OFF, ENEMY FADE START to 80 wu).
4. **Serial log row**: switches printing for the session (below).

**Rules**:

- A debug tool must not silently change gameplay (`AGENTS.md` in dc-halo, "OpenCE is bible"). A row that changes gameplay must say so in its note, and it stops achievements for the session (`debug_cheating` in `src/profile_save.h`).

**Serial log rows (dev images):**

| Row | What it does (`docs/GAME_GUIDE.md`, debug table) |
|---|---|
| SERIAL LOG | switches between the full log and the quiet log for one session. A PARITYTEST image starts with the full log; a release image starts quiet (`docs/BUILD.md`) |
| MEMORY LOG | a screen of what each loading step left free: main memory, video memory, the sound blocks that did not load |
| UPFAILS | lists every texture, model, buffer or file that failed to load, with its size and the space left, on screen (pages turn every 3 s) and on the serial log |
| LOG ON SCREEN | the last 100 serial log lines on screen, a page every 4 s |

Serial budget (dc-halo `docs/BUILD.md`): the quiet log is about 4 KB of the 40 KB that a map load prints (MEASURED on the a30 and b25 logs). At 115200 baud, 1 KB costs about 94 ms, so per-frame prints are banned.

## 8. Translations

- **English stays at the call site.** The code's English text is the code it ran. `lang_tr(en)` returns the English text when no language is set (`src/lang.c`, `src/lang.h`).
- **Context key.** `lang_trc(ctx, en)` gives one English text two meanings, with a context key (the `ctx` field in `src/lang_en.json`).
- **Formats.** `lang_snprintf` translates the format and each `%s` piece, so a composed line stays in one language.
- **The table.** `src/lang_en.json` lists every menu string: `text`, the source `files`, an optional `ctx` and an optional `blob`. About 4,800 entries (count of `"text"` keys, 2026-11-15). `src/lang_pt.json` and `src/lang_ru.json` hold the other languages (about 5,000 keys each).
- **The key.** `tools/lang_strings.py` bakes the table. The key is FNV-1a 32 of `ctx`, then 0x1F, then the text (the text alone when there is no ctx). The build fails on a collision.
- **The language code** is stored in the settings block (HALOCFG byte 364, `SETTINGS_SIZE_LANG`). An unknown code reads as English.
- **Rule for new text:** add it to `src/lang_en.json` with its files in the same change. Never change an English string without a re-bake of the other languages.

Departure (dc-halo, tracker F-LANG-TABLE): OpenCE has no string table. A language is therefore our own table, read from the disc. The English text still comes from the game's own code.

Generic: keep one English-keyed table for menu text and one for dialogue or story text. Each key is the English text plus an optional context.

## 9. Checklist for a new project

1. Copy the page map (section 2). Rename the pages; keep the order.
2. Set up the row types in section 4 once, then reuse them.
3. Write the presets as full rows (section 5). Two presets to start.
4. Write every note to section 6. Count the characters.
5. Add the debug gate (section 7), the session-only sliders and the serial rows.
6. Put every menu string in the translation table at the call site (section 8).
7. Boot test: each row changes the game, each note matches its number on screen, and the preset label reads CUSTOM after one change.

## Sources

- `src/menu.h` (page ids, video rows, preset struct, session-only slider limits) (dc-halo)
- `src/menu.c` (`video_presets`, `video_preset_match_n`, `step_option`, `video_object_distance`) (dc-halo)
- `src/menu_draw.c` (row notes, the settings labels at line 3557, the experience page at line 3903) (dc-halo)
- `src/lang.c`, `src/lang.h`, `src/lang_en.json`, `tools/lang_strings.py` (dc-halo)
- `docs/archive/ui/MENU_LAYOUT.md`, `docs/archive/ui/MENU_WORDING.md`, `docs/GAME_GUIDE.md`, `docs/CONTROLS.md` (dc-halo)
- `AGENTS.md` (menu note rule, debug rule, preset update rule) (dc-halo)
