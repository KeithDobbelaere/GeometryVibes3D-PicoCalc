#!/usr/bin/env python3
from __future__ import annotations

import json
import tkinter as tk
from tkinter import filedialog, messagebox
from dataclasses import dataclass
from typing import Dict, Optional, Tuple, Any, List, Set

# -----------------------------
# Spec constants
# -----------------------------
GRID_H = 9
DEFAULT_W = 332
CELL = 28
GRID_LINE = "#2b2b2b"

# Base shapes
SHAPE_SQUARE = "square"
SHAPE_RIGHT_TRI = "right_tri"
SHAPE_HALF_SPIKE = "half_spike"
SHAPE_FULL_SPIKE = "full_spike"
SHAPES = [SHAPE_SQUARE, SHAPE_RIGHT_TRI, SHAPE_HALF_SPIKE, SHAPE_FULL_SPIKE]

# Modifiers (optional)
MOD_NONE = "none"
MOD_ROT_L = "rot_left"   # CCW
MOD_ROT_R = "rot_right"  # CW
MOD_INVERT = "invert"    # flip H+V
MODS = [MOD_NONE, MOD_ROT_L, MOD_ROT_R, MOD_INVERT]
MOD_SHAPES = {SHAPE_RIGHT_TRI, SHAPE_HALF_SPIKE, SHAPE_FULL_SPIKE}

# -----------------------------
# Endcap
# Stored as dx relative to last column (width-1).
# Portal is also relative to last column.
# -----------------------------
ENDCAP_W = 6
ENDCAP_OBS = [
    # y=0
    {"dx": -5, "y": 0, "shape": "right_tri", "mod": "rot_left"},
    {"dx": -4, "y": 0, "shape": "square"},
    {"dx": -3, "y": 0, "shape": "square"},
    {"dx": -2, "y": 0, "shape": "square"},
    {"dx": -1, "y": 0, "shape": "square"},
    {"dx":  0, "y": 0, "shape": "square"},
    # y=1
    {"dx": -4, "y": 1, "shape": "right_tri", "mod": "rot_left"},
    {"dx": -3, "y": 1, "shape": "square"},
    {"dx": -2, "y": 1, "shape": "square"},
    {"dx": -1, "y": 1, "shape": "square"},
    {"dx":  0, "y": 1, "shape": "square"},
    # y=2
    {"dx": -3, "y": 2, "shape": "right_tri", "mod": "rot_left"},
    {"dx": -2, "y": 2, "shape": "square"},
    {"dx": -1, "y": 2, "shape": "square"},
    {"dx":  0, "y": 2, "shape": "square"},
    # y=3
    {"dx": -2, "y": 3, "shape": "square"},
    {"dx": -1, "y": 3, "shape": "square"},
    {"dx":  0, "y": 3, "shape": "square"},
    # y=4
    {"dx": -2, "y": 4, "shape": "square"},
    {"dx": -1, "y": 4, "shape": "square"},
    {"dx":  0, "y": 4, "shape": "square"},
    # y=5
    {"dx": -2, "y": 5, "shape": "square"},
    {"dx": -1, "y": 5, "shape": "square"},
    {"dx":  0, "y": 5, "shape": "square"},
    # y=6
    {"dx": -3, "y": 6, "shape": "right_tri"},
    {"dx": -2, "y": 6, "shape": "square"},
    {"dx": -1, "y": 6, "shape": "square"},
    {"dx":  0, "y": 6, "shape": "square"},
    # y=7
    {"dx": -4, "y": 7, "shape": "right_tri"},
    {"dx": -3, "y": 7, "shape": "square"},
    {"dx": -2, "y": 7, "shape": "square"},
    {"dx": -1, "y": 7, "shape": "square"},
    {"dx":  0, "y": 7, "shape": "square"},
    # y=8
    {"dx": -5, "y": 8, "shape": "right_tri"},
    {"dx": -4, "y": 8, "shape": "square"},
    {"dx": -3, "y": 8, "shape": "square"},
    {"dx": -2, "y": 8, "shape": "square"},
    {"dx": -1, "y": 8, "shape": "square"},
    {"dx":  0, "y": 8, "shape": "square"},
]

# Portal: relative to last column (width-1)
PORTAL_REL = {"dx": -3, "y": 4}

# --- Binary export mappings (v1) ---
SHAPE_ID = {
    "empty": 0,
    "square": 1,
    "right_tri": 2,
    "half_spike": 3,
    "full_spike": 4,
}
ID_SHAPE = {v: k for k, v in SHAPE_ID.items()}

MOD_ID = {
    "none": 0,
    "rot_left": 1,
    "rot_right": 2,
    "invert": 3,
}

# -----------------------------
# Data model
# -----------------------------

@dataclass(frozen=True)
class Obstacle:
    shape: str
    mod: str = MOD_NONE

    def to_obj(self) -> Dict[str, Any]:
        o = {"shape": self.shape}
        if self.mod != MOD_NONE:
            o["mod"] = self.mod
        return o

    @staticmethod
    def from_obj(obj: Dict[str, Any]) -> "Obstacle":
        shape = str(obj.get("shape", SHAPE_SQUARE))
        mod = str(obj.get("mod", MOD_NONE))
        if shape not in SHAPES:
            shape = SHAPE_SQUARE
        if mod not in MODS:
            mod = MOD_NONE
        if shape not in MOD_SHAPES:
            mod = MOD_NONE
        return Obstacle(shape=shape, mod=mod)

@dataclass
class Level:
    width: int
    height: int = GRID_H
    start: Tuple[int, int] = (0, 4)
    obstacles: Dict[str, Obstacle] = None

    def __post_init__(self) -> None:
        if self.height != GRID_H:
            raise ValueError("Height must be 9.")
        if self.obstacles is None:
            self.obstacles = {}

    def key(self, x: int, y: int) -> str:
        return f"{x},{y}"

    def in_bounds(self, x: int, y: int) -> bool:
        return 0 <= x < self.width and 0 <= y < self.height

    def endcap_x0(self) -> int:
        return max(0, self.width - ENDCAP_W)

    def in_endcap(self, x: int) -> bool:
        return x >= self.endcap_x0()

    def clamp_start(self) -> None:
        sx, sy = self.start
        sx = min(max(0, sx), max(0, self.width - 1))
        sy = min(max(0, sy), self.height - 1)
        # don't allow start inside locked endcap
        if self.in_endcap(sx):
            sx = max(0, self.endcap_x0() - 1)
        self.start = (sx, sy)

    def resize_width(self, new_w: int) -> None:
        """Allow grow or shrink. Shrink clips obstacles beyond new width and clamps start."""
        new_w = int(new_w)
        if new_w < 1:
            new_w = 1
        self.width = new_w

        # clip obstacles
        kill: List[str] = []
        for k in self.obstacles.keys():
            xs, ys = k.split(",")
            x = int(xs)
            if x >= new_w:
                kill.append(k)
        for k in kill:
            self.obstacles.pop(k, None)

        self.clamp_start()

    def set_start(self, x: int, y: int) -> None:
        if self.in_bounds(x, y) and not self.in_endcap(x):
            self.start = (x, y)

    def set_obstacle(self, x: int, y: int, obs: Optional[Obstacle]) -> None:
        if not self.in_bounds(x, y):
            return
        if self.in_endcap(x):
            return  # locked endcap region
        k = self.key(x, y)
        if obs is None:
            self.obstacles.pop(k, None)
        else:
            self.obstacles[k] = obs

    def get_obstacle(self, x: int, y: int) -> Optional[Obstacle]:
        return self.obstacles.get(self.key(x, y))
    
    def effective_obstacle_at(self, x: int, y: int):
        """Return authored obstacle if present; else if in endcap, return endcap obstacle; else None."""
        obs = self.get_obstacle(x, y)
        if obs:
            return obs

        # endcap overlay
        last = self.width - 1
        if x >= self.endcap_x0():
            dx = x - last
            for e in ENDCAP_OBS:
                if int(e["dx"]) == dx and int(e["y"]) == y:
                    return Obstacle.from_obj(e)
        return None

    def clear_cell(self, x: int, y: int) -> None:
        if not self.in_bounds(x, y):
            return
        if self.in_endcap(x):
            return  # locked
        self.obstacles.pop(self.key(x, y), None)

    def strip_endcap_from_authored(self) -> None:
        """Remove any obstacles that are in the auto endcap region (keeps authored data clean)."""
        x0 = self.endcap_x0()
        kill = []
        for k in self.obstacles.keys():
            xs, _ = k.split(",")
            if int(xs) >= x0:
                kill.append(k)
        for k in kill:
            self.obstacles.pop(k, None)
        self.clamp_start()

    def apply_endcap_to_export_list(self, out_list: List[Dict[str, Any]]) -> None:
        """Append the fixed endcap obstacles as absolute x,y entries."""
        last = self.width - 1
        for e in ENDCAP_OBS:
            x = last + int(e["dx"])
            y = int(e["y"])
            if 0 <= x < self.width and 0 <= y < self.height:
                o = {"x": x, "y": y, "shape": e["shape"]}
                if "mod" in e:
                    o["mod"] = e["mod"]
                out_list.append(o)

    def portal_abs(self) -> Tuple[int, int]:
        last = self.width - 1
        return (last + int(PORTAL_REL["dx"]), int(PORTAL_REL["y"]))
    
    def write_bin(self, path: str) -> None:
        # Ensure authored data is clean; export will still include endcap via effective_obstacle_at
        self.strip_endcap_from_authored()

        magic = b"GVL1"
        version = 1
        width = int(self.width)
        height = int(self.height)  # should be 9
        sx, sy = self.start

        portal_dx = int(PORTAL_REL["dx"]) & 0xFF  # store as int8 in file
        portal_y = int(PORTAL_REL["y"]) & 0xFF
        endcap_w = int(ENDCAP_W) & 0xFF

        header = bytearray()
        header += magic
        header += bytes([version])
        header += width.to_bytes(2, "little", signed=False)
        header += bytes([height & 0xFF])
        header += bytes([sx & 0xFF, sy & 0xFF])
        header += int.to_bytes((portal_dx if portal_dx < 128 else portal_dx - 256), 1, "little", signed=True)
        header += bytes([portal_y])
        header += bytes([endcap_w])
        header += b"\x00\x00\x00"  # reserved to 16 bytes

        with open(path, "wb") as f:
            f.write(header)

            # Columns: width * 7 bytes
            for x in range(width):
                col = 0
                for y in range(9):
                    obs = self.effective_obstacle_at(x, y)
                    if obs is None:
                        shape_id = 0
                        mod_id = 0
                    else:
                        shape_id = SHAPE_ID.get(obs.shape, 0) & 0xF
                        mod_id = MOD_ID.get(obs.mod, 0) & 0x3
                        # modifiers only meaningful for some shapes; enforce like editor
                        if obs.shape not in MOD_SHAPES:
                            mod_id = 0

                    cell6 = (shape_id & 0xF) | ((mod_id & 0x3) << 4)
                    col |= (cell6 & 0x3F) << (y * 6)

                # top 2 bits reserved (0)
                f.write(int(col).to_bytes(7, "little", signed=False))

    def to_json_obj(self) -> Dict[str, Any]:
        # authored obstacles (without endcap)
        authored = []
        for k, obs in self.obstacles.items():
            xs, ys = k.split(",")
            authored.append({"x": int(xs), "y": int(ys), **obs.to_obj()})
        authored.sort(key=lambda o: (o["y"], o["x"], o["shape"]))

        # export list = authored + endcap
        out_obs = list(authored)
        self.apply_endcap_to_export_list(out_obs)
        out_obs.sort(key=lambda o: (o["y"], o["x"], o["shape"]))

        # portal
        px, py = self.portal_abs()
        return {
            "width": self.width,
            "height": self.height,
            "start": {"x": self.start[0], "y": self.start[1]},
            "obstacles": out_obs,
            "portal": {"dx": int(PORTAL_REL["dx"]), "y": int(PORTAL_REL["y"]), "x": px},
            "endcap": {"width": ENDCAP_W},  # metadata
        }

    @staticmethod
    def from_json_obj(obj: Dict[str, Any]) -> "Level":
        w = int(obj.get("width", DEFAULT_W))
        h = int(obj.get("height", GRID_H))
        if h != GRID_H:
            raise ValueError("Expected height=9.")
        lvl = Level(width=w, height=h)

        s = obj.get("start", {})
        lvl.start = (int(s.get("x", 0)), int(s.get("y", 4)))
        lvl.clamp_start()

        lvl.obstacles.clear()
        for o in obj.get("obstacles", []):
            x = int(o.get("x", 0))
            y = int(o.get("y", 0))
            if not lvl.in_bounds(x, y):
                continue
            obs = Obstacle.from_obj(o)
            # load everything, then we'll strip endcap after
            k = lvl.key(x, y)
            lvl.obstacles[k] = obs

        # IMPORTANT: keep authored data clean by stripping endcap
        lvl.strip_endcap_from_authored()
        return lvl

# -----------------------------
# Undo system
# -----------------------------

@dataclass
class UndoAction:
    # store prior and new states for touched cells + start
    before_cells: Dict[str, Optional[Obstacle]]
    after_cells: Dict[str, Optional[Obstacle]]
    before_start: Tuple[int, int]
    after_start: Tuple[int, int]
    before_width: int
    after_width: int

# -----------------------------
# Editor
# -----------------------------

class EditorApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("GV3D Obstacle Level Editor (9xN)")

        self.level = Level(DEFAULT_W)

        # UI state
        self.width_var = tk.IntVar(value=self.level.width)
        self.mode_var = tk.StringVar(value="paint")  # paint / start
        self.shape_var = tk.StringVar(value=SHAPE_SQUARE)
        self.mod_var = tk.StringVar(value=MOD_NONE)

        # input state
        self._painting = False
        self._erasing = False

        # selection
        self._selecting = False
        self._sel_a: Optional[Tuple[int, int]] = None
        self._sel_b: Optional[Tuple[int, int]] = None
        self._sel_rect_id: Optional[int] = None

        # clipboard
        self._clipboard: Optional[Dict[str, Any]] = None

        # undo
        self._undo: List[UndoAction] = []
        self._txn_active = False
        self._txn_before_cells: Dict[str, Optional[Obstacle]] = {}
        self._txn_after_cells: Dict[str, Optional[Obstacle]] = {}
        self._txn_before_start = self.level.start
        self._txn_before_width = self.level.width

        # view state
        self._last_mouse_cell: Tuple[int, int] = (0, 0)
        self._y_offset_px = 0

        self._build_ui()
        self._rebuild_canvas_region()
        self.redraw_all()
        self._update_modifier_enable()

    # -------- UI --------

    def _build_ui(self) -> None:
        outer = tk.Frame(self.root)
        outer.pack(fill="both", expand=True)

        left = tk.Frame(outer, padx=8, pady=8)
        left.pack(side="left", fill="y")

        right = tk.Frame(outer)
        right.pack(side="right", fill="both", expand=True)

        tk.Label(left, text="File", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        tk.Button(left, text="New (clear)…", command=self.new_level).pack(fill="x", pady=(4, 2))
        tk.Button(left, text="Import JSON…", command=self.import_json).pack(fill="x", pady=2)
        tk.Button(left, text="Export JSON…", command=self.export_json).pack(fill="x", pady=2)
        tk.Button(left, text="Export BIN…", command=self.export_bin).pack(fill="x", pady=2)

        tk.Label(left, text="").pack(pady=6)

        tk.Label(left, text="Grid", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        wrow = tk.Frame(left)
        wrow.pack(fill="x", pady=(4, 8))
        tk.Label(wrow, text="Width:").pack(side="left")
        tk.Spinbox(wrow, from_=1, to=9999, width=6, textvariable=self.width_var,
                   command=self.on_width_change).pack(side="left", padx=(6, 0))
        tk.Button(wrow, text="Apply", command=self.on_width_change).pack(side="left", padx=6)

        tk.Label(left, text="Mode", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(6, 0))
        tk.Radiobutton(left, text="Paint obstacles", variable=self.mode_var, value="paint").pack(anchor="w")
        tk.Radiobutton(left, text="Place start", variable=self.mode_var, value="start").pack(anchor="w")

        tk.Label(left, text="").pack(pady=6)

        tk.Label(left, text="Obstacle Shape", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        shape_menu = tk.OptionMenu(left, self.shape_var, *SHAPES, command=lambda _=None: self._update_modifier_enable())
        shape_menu.pack(fill="x", pady=(4, 4))

        tk.Label(left, text="Modifier", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(10, 0))
        self.mod_buttons = []
        for m in MODS:
            rb = tk.Radiobutton(left, text=m, variable=self.mod_var, value=m)
            rb.pack(anchor="w")
            self.mod_buttons.append(rb)

        tk.Label(left, text="").pack(pady=6)
        tk.Label(left, text="Shortcuts", font=("Segoe UI", 10, "bold")).pack(anchor="w")
        tk.Label(left,
                 text="• Shift+Drag: select rect\n"
                      "• Ctrl+C: copy\n"
                      "• Ctrl+X: cut\n"
                      "• Ctrl+V: paste at mouse\n"
                      "• Ctrl+Z: undo\n"
                      "• Right-drag: erase\n"
                      "• Mouse wheel: scroll horizontally\n"
                      "• Endcap zone (last 6 cols) is locked",
                 justify="left", fg="#444").pack(anchor="w")

        # Canvas with horizontal scrollbar
        self.canvas = tk.Canvas(right, bg="#101010", highlightthickness=0)
        self.hbar = tk.Scrollbar(right, orient="horizontal", command=self.canvas.xview)
        self.canvas.configure(xscrollcommand=self.hbar.set)

        self.canvas.pack(side="top", fill="both", expand=True)
        self.hbar.pack(side="bottom", fill="x")

        # Status bar
        self.status = tk.Label(right, text="x=?, y=?", anchor="w", fg="#ddd", bg="#202020")
        self.status.pack(side="bottom", fill="x")

        # Bindings
        self.canvas.bind("<Configure>", self.on_canvas_resize)
        self.canvas.bind("<Motion>", self.on_motion)

        self.canvas.bind("<ButtonPress-1>", self.on_left_down)
        self.canvas.bind("<B1-Motion>", self.on_left_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_left_up)

        self.canvas.bind("<ButtonPress-3>", self.on_right_down)
        self.canvas.bind("<B3-Motion>", self.on_right_drag)
        self.canvas.bind("<ButtonRelease-3>", self.on_right_up)

        # Horizontal scroll wheel
        self.canvas.bind("<MouseWheel>", self.on_wheel)
        self.canvas.bind("<Button-4>", lambda e: self.canvas.xview_scroll(-3, "units"))  # Linux
        self.canvas.bind("<Button-5>", lambda e: self.canvas.xview_scroll(3, "units"))

        # Hotkeys
        self.root.bind_all("<Control-c>", lambda e: self.copy_selection())
        self.root.bind_all("<Control-x>", lambda e: self.cut_selection())
        self.root.bind_all("<Control-v>", lambda e: self.paste_clipboard())
        self.root.bind_all("<Control-z>", lambda e: self.undo())

    def _update_modifier_enable(self) -> None:
        shape = self.shape_var.get()
        enabled = shape in MOD_SHAPES
        for rb in self.mod_buttons:
            val = rb["value"]
            if val == MOD_NONE:
                rb.configure(state="normal")
            else:
                rb.configure(state=("normal" if enabled else "disabled"))
        if not enabled:
            self.mod_var.set(MOD_NONE)

    # -------- View / layout --------

    def on_canvas_resize(self, e: tk.Event) -> None:
        # Center vertically: compute y offset for drawing grid
        grid_h_px = GRID_H * CELL
        self._y_offset_px = max(0, (e.height - grid_h_px) // 2)
        self.redraw_all()

    def _rebuild_canvas_region(self) -> None:
        # Scroll only horizontally; keep a reasonable scrollregion height (no y scrolling)
        wpx = self.level.width * CELL
        # make scrollregion tall enough for the centered grid
        hpx = max(GRID_H * CELL + 2 * self._y_offset_px, GRID_H * CELL)
        self.canvas.configure(scrollregion=(0, 0, wpx, hpx))

    # -------- Coords --------

    def event_to_cell(self, e: tk.Event) -> Optional[Tuple[int, int]]:
        cx = self.canvas.canvasx(e.x)
        cy = self.canvas.canvasy(e.y) - self._y_offset_px
        x = int(cx // CELL)
        y = int(cy // CELL)
        if 0 <= x < self.level.width and 0 <= y < self.level.height:
            return x, y
        return None

    def on_motion(self, e: tk.Event) -> None:
        c = self.event_to_cell(e)
        if c:
            self._last_mouse_cell = c
            self.status.configure(text=f"x={c[0]}, y={c[1]}")

    # -------- Width --------

    def on_width_change(self) -> None:
        try:
            w = int(self.width_var.get())
        except Exception:
            return

        self.begin_txn()
        self.record_before_width_start()

        old_w = self.level.width
        self.level.resize_width(w)
        self.level.strip_endcap_from_authored()

        self.record_after_width_start()
        self.commit_txn()

        if self.level.width != old_w:
            self.width_var.set(self.level.width)
            self._rebuild_canvas_region()
            self.redraw_all()

    # -------- Undo plumbing --------

    def begin_txn(self) -> None:
        if self._txn_active:
            return
        self._txn_active = True
        self._txn_before_cells.clear()
        self._txn_after_cells.clear()
        self._txn_before_start = self.level.start
        self._txn_before_width = self.level.width

    def record_before_width_start(self) -> None:
        # already captured in begin_txn fields
        pass

    def record_after_width_start(self) -> None:
        # captured at commit time
        pass

    def touch_cell_before(self, x: int, y: int) -> None:
        k = self.level.key(x, y)
        if k in self._txn_before_cells:
            return
        self._txn_before_cells[k] = self.level.get_obstacle(x, y)

    def touch_cell_after(self, x: int, y: int) -> None:
        k = self.level.key(x, y)
        self._txn_after_cells[k] = self.level.get_obstacle(x, y)

    def commit_txn(self) -> None:
        if not self._txn_active:
            return

        # include any cells that were changed but not recorded after (safety)
        for k in list(self._txn_before_cells.keys()):
            if k not in self._txn_after_cells:
                xs, ys = k.split(",")
                self._txn_after_cells[k] = self.level.get_obstacle(int(xs), int(ys))

        act = UndoAction(
            before_cells=dict(self._txn_before_cells),
            after_cells=dict(self._txn_after_cells),
            before_start=self._txn_before_start,
            after_start=self.level.start,
            before_width=self._txn_before_width,
            after_width=self.level.width,
        )

        # skip no-op
        if (act.before_cells == act.after_cells and
            act.before_start == act.after_start and
            act.before_width == act.after_width):
            self._txn_active = False
            return

        self._undo.append(act)
        self._txn_active = False

    def undo(self) -> None:
        if not self._undo:
            return
        act = self._undo.pop()

        # revert width first
        self.level.resize_width(act.before_width)

        # revert obstacles
        for k, prev in act.before_cells.items():
            xs, ys = k.split(",")
            x, y = int(xs), int(ys)
            if not self.level.in_bounds(x, y):
                continue
            if prev is None:
                self.level.obstacles.pop(k, None)
            else:
                self.level.obstacles[k] = prev

        # revert start
        self.level.start = act.before_start
        self.level.strip_endcap_from_authored()

        self.width_var.set(self.level.width)
        self._rebuild_canvas_region()
        self.redraw_all()

    # -------- Drawing --------

    def redraw_all(self) -> None:
        self.canvas.delete("all")
        self._rebuild_canvas_region()

        # draw authored cells
        for y in range(self.level.height):
            for x in range(self.level.width):
                self._draw_cell(x, y, tag=f"cell_{x}_{y}")

        self._draw_grid()
        self._draw_start()
        self._draw_endcap_ghost()   # (5)
        self._draw_portal_icon()    # (4)(5)
        self._draw_selection_overlay()

    def redraw_cell(self, x: int, y: int) -> None:
        tag = f"cell_{x}_{y}"
        self.canvas.delete(tag)
        self._draw_cell(x, y, tag=tag)
        self._draw_start()
        self._draw_endcap_ghost()
        self._draw_portal_icon()
        self._draw_selection_overlay()

    def _cell_rect(self, x: int, y: int) -> Tuple[int, int, int, int]:
        x0 = x * CELL
        y0 = y * CELL + self._y_offset_px
        return x0, y0, x0 + CELL, y0 + CELL

    def _draw_grid(self) -> None:
        wpx = self.level.width * CELL
        hpx = GRID_H * CELL
        y0 = self._y_offset_px
        y1 = y0 + hpx

        for x in range(self.level.width + 1):
            xx = x * CELL
            self.canvas.create_line(xx, y0, xx, y1, fill=GRID_LINE)
        for y in range(self.level.height + 1):
            yy = y0 + y * CELL
            self.canvas.create_line(0, yy, wpx, yy, fill=GRID_LINE)

        # highlight locked endcap region boundary
        x0 = self.level.endcap_x0() * CELL
        self.canvas.create_line(x0, y0, x0, y1, fill="#7a2cff", width=3)

    def _draw_cell(self, x: int, y: int, tag: str) -> None:
        x0, y0, x1, y1 = self._cell_rect(x, y)

        obs = self.level.get_obstacle(x, y)
        if obs:
            self._draw_obstacle(x0, y0, x1, y1, obs, tag)

        self.canvas.create_rectangle(x0, y0, x1, y1, outline=GRID_LINE, width=1, tags=tag)

    def _draw_start(self) -> None:
        self.canvas.delete("start")
        x, y = self.level.start
        x0, y0, x1, y1 = self._cell_rect(x, y)
        pad = 5
        midy = (y0 + y1) / 2
        pts = [x0 + pad, y0 + pad, x1 - pad, midy, x0 + pad, y1 - pad]
        self.canvas.create_polygon(pts, fill="#22ff55", outline="#0b6b1f", width=2, tags="start")

    def _draw_obstacle(self, x0, y0, x1, y1, obs: Obstacle, tag: str, ghost: bool = False) -> None:
        shape = obs.shape
        mod = obs.mod if shape in MOD_SHAPES else MOD_NONE

        poly = self._base_polygon(shape)
        poly = [self._apply_mod(p, mod) for p in poly]

        pts = []
        for u, v in poly:
            pts.extend([x0 + u * (x1 - x0), y0 + v * (y1 - y0)])

        fill = "" if ghost else "#0b0b0b"
        outline = "#ff3bd4" if ghost else "#d0d0d0"
        width = 2 if ghost else 2

        stipple = "gray25" if ghost else ""
        self.canvas.create_polygon(pts, fill=fill, outline=outline, width=width, stipple=stipple, tags=tag)

    def _base_polygon(self, shape: str):
        if shape == SHAPE_SQUARE:
            return [(0.12, 0.12), (0.88, 0.12), (0.88, 0.88), (0.12, 0.88)]
        if shape == SHAPE_RIGHT_TRI:
            return [(0.08, 0.92), (0.92, 0.92), (0.92, 0.08)]
        if shape == SHAPE_HALF_SPIKE:
            return [(0.18, 0.88), (0.82, 0.88), (0.50, 0.45)]
        if shape == SHAPE_FULL_SPIKE:
            return [(0.12, 0.90), (0.88, 0.90), (0.50, 0.10)]
        return [(0.12, 0.12), (0.88, 0.12), (0.88, 0.88), (0.12, 0.88)]

    def _apply_mod(self, p, mod: str):
        u, v = p
        if mod == MOD_NONE:
            return (u, v)
        if mod == MOD_ROT_L:
            return (v, 1.0 - u)
        if mod == MOD_ROT_R:
            return (1.0 - v, u)
        if mod == MOD_INVERT:
            return (1.0 - u, 1.0 - v)
        return (u, v)

    def _draw_endcap_ghost(self) -> None:
        self.canvas.delete("endcap")
        last = self.level.width - 1
        for e in ENDCAP_OBS:
            x = last + int(e["dx"])
            y = int(e["y"])
            if not self.level.in_bounds(x, y):
                continue
            x0, y0, x1, y1 = self._cell_rect(x, y)
            obs = Obstacle.from_obj(e)
            self._draw_obstacle(x0, y0, x1, y1, obs, tag="endcap", ghost=True)

    def _draw_portal_icon(self) -> None:
        self.canvas.delete("portal")
        px, py = self.level.portal_abs()
        if not self.level.in_bounds(px, py):
            return

        # occupy 3 cells vertically: y-1 .. y+1
        y_top = max(0, py - 1)
        y_bot = min(self.level.height - 1, py + 1)

        x0, y0, x1, y1 = self._cell_rect(px, y_top)
        _, _, _, y1b = self._cell_rect(px, y_bot)
        # a slightly inset oval
        pad = 3
        self.canvas.create_oval(
            x0 + pad, y0 + pad, x1 - pad, y1b - pad,
            outline="#ff3bd4", width=3, stipple="gray25", tags="portal"
        )
        self.canvas.create_oval(
            x0 + pad + 5, y0 + pad + 5, x1 - pad - 5, y1b - pad - 5,
            outline="#ff8fe9", width=2, stipple="gray25", tags="portal"
        )

    # -------- Selection rectangle --------

    def _sel_bounds(self) -> Optional[Tuple[int, int, int, int]]:
        if not self._sel_a or not self._sel_b:
            return None
        ax, ay = self._sel_a
        bx, by = self._sel_b
        x0, x1 = (ax, bx) if ax <= bx else (bx, ax)
        y0, y1 = (ay, by) if ay <= by else (by, ay)
        return x0, y0, x1, y1

    def _draw_selection_overlay(self) -> None:
        if self._sel_rect_id is not None:
            self.canvas.delete(self._sel_rect_id)
            self._sel_rect_id = None

        b = self._sel_bounds()
        if not b:
            return
        x0, y0, x1, y1 = b
        # clamp selection out of endcap to avoid confusion
        x1 = min(x1, self.level.endcap_x0() - 1) if self.level.endcap_x0() > 0 else x1
        if x1 < x0:
            return

        px0 = x0 * CELL
        py0 = y0 * CELL + self._y_offset_px
        px1 = (x1 + 1) * CELL
        py1 = (y1 + 1) * CELL + self._y_offset_px
        self._sel_rect_id = self.canvas.create_rectangle(
            px0, py0, px1, py1,
            outline="#ffd54a",
            width=3,
            dash=(6, 4)
        )

    # -------- Copy/Cut/Paste --------

    def copy_selection(self) -> None:
        b = self._sel_bounds()
        if not b:
            messagebox.showinfo("Copy", "No selection. Hold Shift and drag to select.")
            return
        x0, y0, x1, y1 = b
        x1 = min(x1, self.level.endcap_x0() - 1) if self.level.endcap_x0() > 0 else x1
        if x1 < x0:
            return

        cells = []
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                obs = self.level.get_obstacle(x, y)
                if obs:
                    cells.append({"dx": x - x0, "dy": y - y0, **obs.to_obj()})

        self._clipboard = {"w": (x1 - x0 + 1), "h": (y1 - y0 + 1), "obstacles": cells}

    def cut_selection(self) -> None:
        self.copy_selection()
        if not self._clipboard:
            return
        b = self._sel_bounds()
        if not b:
            return
        x0, y0, x1, y1 = b
        x1 = min(x1, self.level.endcap_x0() - 1) if self.level.endcap_x0() > 0 else x1
        if x1 < x0:
            return

        self.begin_txn()
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if self.level.in_endcap(x):
                    continue
                self.touch_cell_before(x, y)
                self.level.clear_cell(x, y)
                self.touch_cell_after(x, y)
        self.commit_txn()
        self.redraw_all()

    def paste_clipboard(self) -> None:
        if not self._clipboard:
            messagebox.showinfo("Paste", "Clipboard is empty. Use Shift+drag then Ctrl+C.")
            return

        tx, ty = self._last_mouse_cell
        w = int(self._clipboard["w"])
        h = int(self._clipboard["h"])

        # expand width if needed, but keep endcap locked at the end
        need_w = tx + w + ENDCAP_W
        if need_w > self.level.width:
            self.begin_txn()
            self._txn_before_width = self.level.width
            self.level.resize_width(need_w)
            self.level.strip_endcap_from_authored()
            self.commit_txn()

            self.width_var.set(self.level.width)
            self._rebuild_canvas_region()

        self.begin_txn()
        for c in self._clipboard["obstacles"]:
            x = tx + int(c["dx"])
            y = ty + int(c["dy"])
            if not self.level.in_bounds(x, y):
                continue
            if self.level.in_endcap(x):
                continue
            self.touch_cell_before(x, y)
            obs = Obstacle.from_obj(c)
            self.level.set_obstacle(x, y, obs)
            self.touch_cell_after(x, y)
        self.commit_txn()
        self.redraw_all()

    # -------- Painting / selection --------

    def on_left_down(self, e: tk.Event) -> None:
        cell = self.event_to_cell(e)
        if not cell:
            return
        x, y = cell

        if (e.state & 0x0001) != 0:  # Shift => selection
            self._selecting = True
            self._sel_a = cell
            self._sel_b = cell
            self._draw_selection_overlay()
            return

        self._painting = True
        self._erasing = False

        self.begin_txn()
        self._txn_before_start = self.level.start
        self._txn_before_width = self.level.width

        self._apply_left_action(x, y)

    def on_left_drag(self, e: tk.Event) -> None:
        cell = self.event_to_cell(e)
        if not cell:
            return
        x, y = cell

        if self._selecting:
            self._sel_b = cell
            self._draw_selection_overlay()
            return

        if not self._painting or self._erasing:
            return
        self._apply_left_action(x, y)

    def on_left_up(self, e: tk.Event) -> None:
        self._painting = False
        if self._selecting:
            self._selecting = False
            self._draw_selection_overlay()
            return
        self.commit_txn()
        self.redraw_all()

    def on_right_down(self, e: tk.Event) -> None:
        cell = self.event_to_cell(e)
        if not cell:
            return
        x, y = cell
        if self.level.in_endcap(x):
            return

        self._painting = True
        self._erasing = True

        self.begin_txn()
        self._txn_before_start = self.level.start
        self._txn_before_width = self.level.width

        self.touch_cell_before(x, y)
        self.level.clear_cell(x, y)
        self.touch_cell_after(x, y)

        self.redraw_cell(x, y)

    def on_right_drag(self, e: tk.Event) -> None:
        cell = self.event_to_cell(e)
        if not cell or not self._painting or not self._erasing:
            return
        x, y = cell
        if self.level.in_endcap(x):
            return

        self.touch_cell_before(x, y)
        self.level.clear_cell(x, y)
        self.touch_cell_after(x, y)
        self.redraw_cell(x, y)

    def on_right_up(self, e: tk.Event) -> None:
        self._painting = False
        self._erasing = False
        self.commit_txn()
        self.redraw_all()

    def _apply_left_action(self, x: int, y: int) -> None:
        if self.level.in_endcap(x):
            return

        if self.mode_var.get() == "start":
            old = self.level.start
            self.level.set_start(x, y)
            self.redraw_cell(*old)
            self.redraw_cell(x, y)
            return

        shape = self.shape_var.get()
        mod = self.mod_var.get()
        if shape not in MOD_SHAPES:
            mod = MOD_NONE

        self.touch_cell_before(x, y)
        self.level.set_obstacle(x, y, Obstacle(shape=shape, mod=mod))
        self.touch_cell_after(x, y)
        self.redraw_cell(x, y)

    # -------- Scrolling --------

    def on_wheel(self, e: tk.Event) -> None:
        delta = -1 if e.delta > 0 else 1
        self.canvas.xview_scroll(delta * 3, "units")

    # -------- File ops --------
    def export_bin(self) -> None:
        path = filedialog.asksaveasfilename(
            title="Export Level Binary",
            defaultextension=".bin",
            filetypes=[("Binary files", "*.bin"), ("All files", "*.*")]
        )
        if not path:
            return
        try:
            self.level.write_bin(path)
            messagebox.showinfo("Export", f"Saved:\n{path}")
        except Exception as ex:
            messagebox.showerror("Export failed", str(ex))

    def export_json(self) -> None:
        path = filedialog.asksaveasfilename(
            title="Export Level JSON",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not path:
            return

        # ensure clean authored + auto endcap on export
        self.level.strip_endcap_from_authored()
        data = self.level.to_json_obj()

        try:
            with open(path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
            messagebox.showinfo("Export", f"Saved:\n{path}")
        except Exception as ex:
            messagebox.showerror("Export failed", str(ex))

    def import_json(self) -> None:
        path = filedialog.askopenfilename(
            title="Import Level JSON",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not path:
            return
        try:
            with open(path, "r", encoding="utf-8") as f:
                obj = json.load(f)
            self.level = Level.from_json_obj(obj)
            self.width_var.set(self.level.width)
            self._rebuild_canvas_region()
            self.redraw_all()
        except Exception as ex:
            messagebox.showerror("Import failed", str(ex))

    def new_level(self) -> None:
        if not messagebox.askyesno("New", "Clear the level?"):
            return
        self.level = Level(width=DEFAULT_W)
        self.level.clamp_start()
        self.width_var.set(self.level.width)
        self._rebuild_canvas_region()
        self.redraw_all()

# -----------------------------
# Main
# -----------------------------

def main() -> None:
    root = tk.Tk()
    EditorApp(root)
    root.minsize(980, 360)
    root.mainloop()

if __name__ == "__main__":
    main()