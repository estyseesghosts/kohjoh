# NATIVE_TEXT — surface ledger and migration basis

**State:** Task 1 survey for the native Galmuri14 renderer
([plan](../../text-renderer.txt), [contract](FONT_RENDERER.md)). Source facts
are cited from the pinned decompilation (`8cbb4a1`, checked out at
`build/upstream`). Baseline pictures are in `build/text-baseline/` and were
captured through LOTUS from the Goldenrod bench save (`96727c6d…`, HeartGold).

This ledger is deliberately literal: every row was read out of the source or
observed in a capture, not inferred from a screen's name. Where a fact is not
yet established it says so.

## 1. The five rendering pathways

| Pathway | Representative source | What it means for native text |
| --- | --- | --- |
| **P1 Ordinary BG window text** | `src/render_text.c` → `CopyGlyphToWindow()` | The window owns a DS pixel buffer; text is baked into its tiles and published by `CopyWindowToVram`. Native text needs the clean backing surface and a publish generation. |
| **P2 Mixed text and graphics** | `src/application/bag/bag_app_display.c` → `BagApp_PrintItemListEntry()` | Text, quantities and icons share one window, in order. Erasing "the text colour" would erase artwork; the replacement must preserve the operation order. |
| **P3 Window converted into OBJ sprites** | `src/font_sprite.c` → `FontOAM_Create()` | The source window is often destroyed immediately. Native text must be transferred to a retained text-OBJ record that follows the sprite cells. |
| **P4 Specialised numeric bitmap printing** | `src/message_printer.c` → `PrintUIntOnWindow()` | Digits are 8×8 bitmaps blitted directly, blank cells are filled. This is not `FontID_TryLoadGlyph`; it needs its own hook and a fixed-width numeric variant. |
| **P5 Direct writes into HP-box sprite graphics** | `src/battle/battle_hp_bar.c` → `BattleHpBar_DrawLevel()` etc. | Nicknames are printed into a temporary window and copied into VRAM regions; levels/HP are copied as raw tile buffers. The box sprite also carries frame, gauge, gender and status art. |

A sixth, out-of-scope pathway exists: lettering baked into artwork (title
"TOUCH TO START", some battle banners). Task 12F audits it; it never passes
through a printer.

## 2. Surface ledger

Legend for **source survives**: does the originating `Window` still exist
after publication, or must its presentation be retained separately?

### 2.1 Overworld dialogue, signs and scripted messages

| Item | Value |
| --- | --- |
| Producer | `DialogBox_PrintMessage()` / `DialogBox_PrintMessageEx()` (`src/dialog_box.c`) → `AddTextPrinterParameterized()`; script side `scrcmd_message.c` |
| Font ID | 1 for ordinary dialogue (`DialogBox_PrintMessage`); caller-selected for `PrintMessageEx` |
| Receiver | A field-owned `Window`, added to the main BG layer 3 (`DialogBox_AddWindowToLayer3`), frame drawn by `DialogBox_LoadFrame` → `DrawFrameAndWindow2` |
| Publication | `RenderText` → `CopyGlyphToWindow`; printer/field commits with `CopyWindowToVram` (and `ScheduleWindowCopyToVram` variants) |
| Engine / layer | Engine A, main BG 3 (upper screen) |
| Position / palette / priority | Window placed by the field map layout; fill palette index 15 (`DialogBox_Clear`); BG priority from the layer; clipped by the DS hardware window and the BG's visible area |
| Other artwork | Yes: the frame and wayfinding graphic are drawn into the same BG window/tilemap (`DialogBox_DrawFrameWithWayfindingGraphic`) |
| Source survives | Yes (field owns it), but its tiles are the only text record |
| Migration | P1 with a clean backing surface: replay fill/frame ops, omit glyph writes, draw native runs. Preflight whole page where possible. |

Baseline: `start-menu-top.png` (field), and the dialogue path is also exercised
by the battle box (`battle-command-top.png` at frame 5872).

### 2.2 Battle dialogue

| Item | Value |
| --- | --- |
| Producer | `BattleSystem_PrintMessage()` family (`src/battle/battle_system.c:1390–1459`) → `AddTextPrinterParameterized(window, 1, …)` |
| Font ID | 1 |
| Receiver | Battle message window (upper screen BG) |
| Publication | `CopyWindowToVram` through the printer; battle task also re-copies on state changes |
| Engine / layer | Engine A, main BG (the box occupies the lower part of the 256×192 canvas) |
| Other artwork | Frame (same as dialogue); no sprites inside the text area |
| Source survives | Yes for the duration of the battle |
| Migration | Same P1 adapter as 2.1 |
| Baseline | `battle-command-top.png` ("What will ENGELS do?"), captured at frame 5872 |

### 2.3 Battle command and move menus

| Item | Value |
| --- | --- |
| Producer | `BattleInput_CreateTextObject()` (`src/battle/battle_input.c:2722`) → `AddTextWindowTopLeftCorner` + `AddTextPrinterParameterizedWithColorAndSpacing(..., font 4, …)` → `FontOAM_Create` |
| Font ID | 4 for labels; 0 for HP/PP values in cached move display objects |
| Receiver | Temporary `Window` → converted to `TextOBJ` sprite cells; the window is removed when `displayObj == NULL` (`RemoveWindow` at line 2790) |
| Publication | `FontOAM_Create` copies window chars into reserved OBJ character space; sprites are then drawn by the sprite manager |
| Engine / layer | Engine B (`NNS_G2D_VRAM_TYPE_2DSUB`), OBJ; `bgPriority 0`, `drawPriority 100`, palette `20023` + per-label offset |
| Other artwork | No; the button backgrounds are separate sprites. Move menus also have type-icon sprites |
| Source survives | **No** for one-shot labels; **yes** for cached move windows (`BattleInput_SetMoveDisplayObjectTextFont`, `battle_input.c:2947–2960`) |
| Migration | P3: transfer glyph runs into a retained text-OBJ record keyed by `TextOBJ` + generation; follow `FontOAM_Delete`, position, draw flag, priority and palette setters |
| Baseline | `battle-command-bottom.png` (FIGHT/BAG/RUN/POKéMON), `battle-move-bottom.png` (Quick Attack/Flame Wheel/SmokeScreen/Ember, PP 30/30 23/25 20/20 25/25) |

### 2.4 Battle HP boxes

| Item | Value |
| --- | --- |
| Producer | `BattleHpBar_DrawNickname()` (`battle_hp_bar.c:908`) → `AddTextWindowTopLeftCorner(8,2)` + font 0 + `MAKE_TEXT_COLOR(14,2,15)`, then four `MI_CpuCopy16` copies into `sNicknameVramRegions[type][0..3]`; `DrawLevel`/`DrawCurrentHp`/`DrawMaxHp` → `BattleNumberPrinter_PrintUInt` into tile buffers copied to `s*VramRegions`; `DrawGenderIcon` copies component graphics 60–76 |
| Font ID | 0 (nickname); specialised numeric printer for level/HP |
| Receiver | HP-box sprite (`hpBar->boxObj->sprite`) OBJ character data; the temporary window is removed after the copy |
| Publication | Direct `MI_CpuCopy16` into `G2_GetOBJCharPtr()` + `imgProxy->vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DMAIN]` |
| Engine / layer | Engine A, OBJ (upper screen). Kohjoh's wide-layout tagging shifts the emitted OAM cells, not these VRAM regions |
| Position / palette / priority | Sprite position/priority; palette from the HP-box sprite; regions are tile ranges, not screen rectangles |
| Other artwork | Yes: frame, HP/EXP gauge, caught icon, status icons, gender icon |
| Source survives | **No** (window removed); the sprite remains |
| Migration | P5: capture displayed values from `BattleHpBar_Draw*` (never poll battle state), retain per-box generation, replace only the text regions, follow the effective sprite position including the wide offset |
| Baseline | `battle-command-top.png`: `PIDGEY ♂ Lv.9` enemy, `ENGELS ♂ Lv.22`, `23/63` |

### 2.5 Bottom-screen field menu

| Item | Value |
| --- | --- |
| Producer | `TouchscreenMenu_PrintIconLabels()` (`touchscreen_menu.c:1252`) → font 0, color `MAKE_TEXT_COLOR(14,2,0)`, centred by `FontID_String_GetWidth`; `TouchscreenMenu_PrintActionLabel()`; `TouchscreenMenu_PrintFramedText()`; `TouchscreenMenu_PrintMessageAt()` (`:1347`) |
| Font ID | 0 and 4 (the brief's "0 and 4") |
| Receiver | `data->iconWindows[]` and `data->menuWindow` for persistent labels; a stack `Window` for temporary messages; `PhoneRing` caller window |
| Publication | `CopyWindowToVram` after printing (`TouchscreenMenu_CopyIconLabelsToVram`), `CopyWindowToVram(&window)` inside `PrintMessageAt` immediately before `RemoveWindow(&window)` |
| Engine / layer | Engine B, sub BG 1 (`GF_BG_LYR_SUB_1`) |
| Position / palette / priority | Fixed window coordinates over the lower screen; icons are separate OBJ sprites; labels clipped by their windows |
| Other artwork | The window frame (drawn by `TouchscreenMenu_DrawWindowFrame`) and the icon sprites |
| Source survives | Persistent labels yes; **temporary messages no** — this is the canonical "published then destroyed" case |
| Migration | P1 + published-presentation record keyed by BG character range/tilemap, not by the temporary window. Labels must follow BG scroll/animations |
| Baseline | `start-menu-bottom.png` (POKéDEX/POKéMON/BAG/POKéGEAR/AAA/SAVE/OPTIONS) |

### 2.6 Save interface

| Item | Value |
| --- | --- |
| Producer | `SaveStatsPrinter_PrintStats()` (`save_stats_printer.c:146`) for the prompt, `SaveStatsPrinter_PrintStatRows()` (`:230`) for the touch save card; both `AddTextPrinterParameterized(window, 0/1, …)` |
| Font ID | 0, and 1 for some lines; right alignment is `0x68 - FontID_String_GetWidth(0, str, letterSpacing)` and `SaveStatsPrinter_GetAlignedX()` |
| Receiver | window from `AddWindowParameterized(..., 0xD, 0x189)` (prompt) or `(..., 4, 0x10B)` (touch card); frame by `DrawFrameAndWindow1` |
| Publication | Prompt: frame + window drawn by the field task; touch card: `CopyWindowToVram` after `FillWindowPixelBuffer(0)` |
| Engine / layer | Engine B, save BG layer (lower screen) |
| Other artwork | Frame graphics; the prompt's stats are text-only inside the frame |
| Source survives | Yes during the prompt; the printer allocates/removes around it |
| Migration | P1. Preserve `FontID_String_GetWidth` alignment maths untouched; right-aligned values must not shift when 9→10 badges or 99→100 hours |
| Baseline | The save prompt was captured in this pass (Goldenrod City / PLAYER AAA / BADGES 2 / POKéDEX 10 / TIME 5:24 / YES-NO, then "There is already a saved file. Is it OK to overwrite it?", then `Sa…` while saving). Frames were not retained because the capture helper overwrote its slot; re-capture is in the Task 14 matrix. |

### 2.7 Item bag

| Item | Value |
| --- | --- |
| Producer | `BagApp_PrintItemListEntry()` (`bag_app_display.c:404`), `BagApp_PrintItemQuantity()` (`:429`), `BagApp_PrintTMHMNumber()`, `BagApp_PrintPageNumber()` (`:436`), `BagApp_PrintItemMenuLabel()` (`:453`), `BagApp_PrintMoveItemPrompt()`, `BagApp_PrintMoney()`, `BagApp_PrintCancelButton()` |
| Font ID | 0 for everything in the ordinary bag |
| Receiver | Item-list windows, description window, page-number window, dynamic action windows |
| Publication | `AddTextPrinterParameterizedWithColor(..., TEXT_SPEED_NOTRANSFER, …)` then `ScheduleWindowCopyToVram()` (or the caller's later copy) |
| Engine / layer | Engine B, bag BG layers (lower screen) |
| Other artwork | **Yes and interleaved**: item icons, registered-item icons and TM/HM type icons are drawn into the same windows in the same order as text |
| Source survives | Yes, but list windows are reused and scrolled |
| Migration | P2: ordered mutation log per window (text vs blit/fill), clean backing tiles, incremental runs; never detect text from pixels |
| Baseline | Not captured this pass (menu navigation in LOTUS did not reach the bag). The party screen `field-party-bottom.png` shows the same kind of dense window text and is an interim visual reference. |

### 2.8 Battle bag and battle party selection

| Item | Value |
| --- | --- |
| Producer | `BattleBagMenu_Print*` (`src/battle/battle_bag_menu.c`, e.g. `:865`, `:884`, `:905`, `:1044`) and `BattlePartyMenu_Print*` (`src/battle/battle_party_menu.c`, e.g. `:1884`, `:1965`, `:2023`, `:2035`) |
| Font ID | 0, 1 and 4 depending on the label |
| Receiver | Many `pageWindows[]` and per-row windows; battle bag also has a message window |
| Publication | `AddTextPrinterParameterizedWithColor(..., TEXT_SPEED_NOTRANSFER, …)`; committed by the application's window copy |
| Engine / layer | Engine B, sub BG layers (OBJ for icons) |
| Other artwork | Icons, cursors, pocket graphics |
| Source survives | Yes for the menu; strings/windows are rebuilt on page changes |
| Migration | P1/P2 with the shared window adapter; the battle bag is a separate application and needs its own registration, not a copy of the field bag |
| Baseline | Party selection captured (`field-party-actions.png`: SUMMARY/SWITCH/ITEM/QUIT, six names/levels/HP) from the field party, not the battle party. Battle-party capture remains for Task 14. |

### 2.9 Specialised numeric printers

| Item | Value |
| --- | --- |
| Producer | `MessagePrinter_PrintGlyph()` (`message_printer.c:69`) and `PrintUIntOnWindow()` (`:73`) |
| Font ID | none — 8×8 bitmap glyphs from `messagePrinter->ppCharData` |
| Receiver | Caller's `Window` (`BlitBitmapRectToWindow` / `FillWindowPixelRect`) |
| Publication | caller's window copy |
| Engine / layer | whatever window the caller passes (bag, money, counters) |
| Other artwork | blank digit cells are filled with `backgroundColor` |
| Source survives | caller-dependent |
| Migration | P4: narrow semantic hooks that record value, digit count, mode and cell size; draw with the fixed-width numeric variant. Not every `BlitBitmapRectToWindow` is a number |

## 3. Baked lettering audit

| Asset / region | Contains lettering? | Decision |
| --- | --- | --- |
| Battle menu button backgrounds | No — FIGHT/BAG/RUN/POKéMON are text-OBJ (2.3) | no artwork replacement |
| Move-menu type icons | Type symbols only | review at Task 12; no readable letters observed |
| HP-box frame, gauge, caught/status icons | No letters; gender is a symbol, handled as a glyph | native gender glyph via P5 |
| Title / first-run info | "TOUCH TO START" and info labels are image art | out of targeted scope; legacy |
| WideArt banners (Route/city names) | Yes, baked into wide replacement art | outside the seven targeted surfaces; documented fallback |
| Pokégear, Pokédex, PC, Trainer Card | Text, but not in the seven targeted surfaces | legacy, explicitly out of scope |

## 4. Baseline capture set

All captures are native 400×240 (top) / 320×240 (bottom), nearest-neighbour
useful for review. LOTUS v0.1.4, CITnLO backend, Goldenrod bench save
`96727c6d34f8aed9…`, HeartGold `.3dsx` built from this tree.

| File | Frame | Shows |
| --- | --- | --- |
| `build/text-baseline/start-menu-top.png` | ~4300 | field, upper screen |
| `build/text-baseline/start-menu-bottom.png` | ~4300 | start/field menu labels (font 0) |
| `build/text-baseline/field-party-top.png` | ~4900 | field + party overlay |
| `build/text-baseline/field-party-actions.png` | ~5000 | party names, levels, HP, SUMMARY/SWITCH/ITEM/QUIT |
| `build/text-baseline/battle-command-top.png` | 5872 | battle dialogue mid-print, enemy and player HP boxes |
| `build/text-baseline/battle-command-bottom.png` | 5872 | FIGHT/BAG/RUN/POKéMON |
| `build/text-baseline/battle-move-bottom.png` | 6040 | move names, PP, CANCEL |

Verified during the run but not retained (capture slots were reused):
the save prompt, overwrite confirmation and the `Sa…` printing frame.

**Not captured this pass:** SoulSilver surfaces (no SoulSilver save in the
approved card set; the pack and `.3dsx` exist). The SoulSilver boot path can
be captured without a save, but the seven surfaces need a user save. This is a
data blocker, not a renderer blocker, and is listed in the Task 14 matrix.

## 5. Migration strategy per pathway

1. **P1 BG text.** Capture resolved glyph events at
   `render_text.c:287–298` (font, char, x/y, colours, glyph table) and
   window mutations in `bg_window.c`. Publish by window generation. Replace
   the BG's text tiles with a clean backing only for registered windows;
   draw native runs at the mapped position in the BG's priority slot.
2. **P2 mixed windows.** Same capture, plus an ordered mutation log
   (`Fill`, `Blit`, `Scroll`). The clean backing is the window's own buffer
   with glyph writes omitted. Any non-text write that overlaps text
   invalidates or clips the affected native records.
3. **P3 OBJ text.** Transfer the captured runs into a retained text-OBJ
   record at `FontOAM_Create`; follow each `TextOBJ_*` setter. Suppress only
   the text cells, never the whole sprite.
4. **P4 numeric.** Hook `PrintUIntOnWindow`/`MessagePrinter_PrintGlyph`
   semantically; use the numeric variant and preserve blank-cell behaviour.
5. **P5 HP box.** Capture values in `BattleHpBar_Draw*` and copy only the
   text regions; use the existing emitted-OAM tagging for position, never a
   screen-coordinate guess.

Every migration is opt-in by audited caller, with the legacy path retained as
the authoritative fallback (Task 6).
