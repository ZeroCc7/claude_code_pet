from __future__ import annotations

import runpy
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "asset_pipeline/qingyun/convert_qingyun_assets.py"


if __name__ == "__main__":
    runpy.run_path(str(TARGET), run_name="__main__")
