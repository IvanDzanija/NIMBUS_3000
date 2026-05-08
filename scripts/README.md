# Scripts

Project maintenance helpers.

- `apply_patch.sh` and `apply_patch.ps1` apply the LVGL ESP32 driver patch and
  prepare `sdkconfig` from `sdkconfig.defaults`.
- `regenerate_ui_images.py` rebuilds compact LVGL C image descriptors from the
  SquareLine PNG assets to reduce firmware flash/RAM pressure.

Run scripts from the repository root unless the script says otherwise.
