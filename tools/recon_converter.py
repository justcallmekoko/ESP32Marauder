#!/usr/bin/env python3
"""Small desktop front end for the Marauder Recon report converter."""

from __future__ import annotations

import sys
import webbrowser
from pathlib import Path

from tools.recon_report import ReconReportError, convert


def run_conversion(mission: Path) -> Path:
    return convert(mission, make_zip=True) / "index.html"


def gui() -> int:
    import tkinter as tk
    from tkinter import filedialog, messagebox

    root = tk.Tk()
    root.title("Marauder Recon Converter")
    root.geometry("520x235")
    root.configure(bg="#080c13")
    selected = tk.StringVar(value="Select a /recon/m#### mission folder")
    tk.Label(root, text="MARAUDER RECON", fg="#28e7ff", bg="#080c13",
             font=("TkDefaultFont", 18, "bold")).pack(pady=(24, 5))
    tk.Label(root, textvariable=selected, fg="#a9b8c8", bg="#080c13",
             wraplength=470).pack(pady=10)

    def choose() -> None:
        folder = filedialog.askdirectory(title="Select Recon mission folder")
        if folder:
            selected.set(folder)

    def build() -> None:
        mission = Path(selected.get())
        try:
            report = run_conversion(mission)
        except (OSError, ReconReportError) as error:
            messagebox.showerror("Conversion failed", str(error))
            return
        webbrowser.open(report.as_uri())
        messagebox.showinfo("Report ready", f"Created report and ZIP in:\n{mission}")

    buttons = tk.Frame(root, bg="#080c13")
    buttons.pack(pady=15)
    tk.Button(buttons, text="SELECT MISSION", command=choose, width=18).pack(side="left", padx=7)
    tk.Button(buttons, text="BUILD REPORT", command=build, width=18).pack(side="left", padx=7)
    root.mainloop()
    return 0


def main() -> int:
    if len(sys.argv) == 2:
        try:
            report = run_conversion(Path(sys.argv[1]))
        except (OSError, ReconReportError) as error:
            print(f"Conversion failed: {error}", file=sys.stderr)
            return 2
        webbrowser.open(report.as_uri())
        print(report)
        return 0
    return gui()


if __name__ == "__main__":
    raise SystemExit(main())
