### Level Editor (Python)

A simple Tkinter-based editor for GV3D obstacle-only levels.

Features:
- Place player start and grid-aligned obstacles (base shape + optional modifier)
- Horizontal scroll only; vertically centered grid
- Copy/paste rectangle selection (does not copy start)
- Undo (Ctrl+Z)
- Auto-appends the standard 6-column endcap + portal metadata on export
- Renders endcap + portal as a ghost overlay; endcap region is locked

Run:
```bash
python level_editor/level_editor.py