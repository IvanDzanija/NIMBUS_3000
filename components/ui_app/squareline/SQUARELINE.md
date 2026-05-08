# SquareLine UI

The SquareLine project for the ESP32 kid board lives in `project/`.

Use SquareLine Studio to edit screens and export C files back into this folder.
The project is configured for LVGL 8.x, 16-bit swapped color, and a 320x240
rotated display layout.

## Export Settings

In SquareLine Studio, open `project/esp32_gui.spj`, then check:

- Project export root: `components/ui_app/squareline/project/`
- UI files export path: `components/ui_app/squareline/project/`
- Call functions export file: `.c`

After export, rebuild the firmware with `idf.py build`.

## Custom Code

SquareLine can regenerate most files in `project/`. Keep custom logic in:

- `project/ui_events.c`
- `project/ui_events.h`
- project-owned support files in `components/ui_app/`

The camera and gallery screens include extra hand-written logic, so review those
files before re-exporting from SquareLine.
