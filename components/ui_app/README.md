# UI App Component

This component contains the LVGL user interface used by the ESP32 kid board.

## What Lives Here

- `ui_app.c` calls SquareLine's `ui_init()`.
- `ui_message_bridge.*` connects UI events to firmware callbacks for MQTT,
  audio, alarm, sketchpad color, and prediction.
- `ui_camera_gallery.*` stores small in-memory thumbnails captured from the
  camera preview.
- `squareline/project/` contains the SquareLine project, generated screens,
  generated image descriptors, and custom `ui_events.*`.
- `squareline/SQUARELINE.md` explains how to edit and export the UI.

Most UI files are generated. Custom behavior should stay in `ui_events.*`,
`ui_message_bridge.*`, `ui_camera_gallery.*`, or hand-written screen files such
as the camera and gallery screens.
