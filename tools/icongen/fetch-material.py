"""Fetches the Material Symbols Rounded SVG sources used by the icon set.

The glyphs are fetched from Google's short-term release endpoint and vendored
into material-sources/ so regeneration never depends on network availability
or upstream visual drift. License: Apache-2.0 (see
licenses/Material-Symbols-Apache-2.0.txt).
"""
import os
import subprocess

BASE = ("https://fonts.gstatic.com/s/i/short-term/release/"
        "materialsymbolsrounded/{name}/{variant}/48px.svg")

# name -> variants needed (default = outlined, fill1 = solid)
GLYPHS = {
    "edit": ["fill1"],
    "crop_square": ["default"],
    "arrow_outward": ["default"],
    "text_fields": ["default"],
    "title": ["fill1"],
    "colorize": ["fill1"],
    "undo": ["default"],
    "redo": ["default"],
    "text_snippet": ["fill1"],
    "push_pin": ["fill1"],
    "videocam": ["fill1"],
    "content_copy": ["default"],
    "save": ["fill1"],
    "close": ["default"],
    "check": ["default"],
    "text_select_start": ["default"],
}

DEST = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                    "material-sources")


def main():
    os.makedirs(DEST, exist_ok=True)
    for name, variants in sorted(GLYPHS.items()):
        for variant in variants:
            out = os.path.join(DEST, f"mat-{name}-{variant}.svg")
            url = BASE.format(name=name, variant=variant)
            result = subprocess.run(["curl", "-sL", "--max-time", "30",
                                     "-o", out, url], check=False)
            ok = result.returncode == 0 and "<path" in open(
                out, encoding="utf-8", errors="ignore").read()
            print(("ok  " if ok else "FAIL") + f" {name} {variant}")
            if not ok:
                os.remove(out)


if __name__ == "__main__":
    main()
