from __future__ import annotations

import shutil
import time
from pathlib import Path


def recreate_build_dir(build_dir: Path) -> None:
    for attempt in range(3):
        try:
            shutil.rmtree(build_dir)
            break
        except FileNotFoundError:
            break
        except OSError:
            if attempt == 2:
                raise
            time.sleep(0.2)
    build_dir.mkdir(parents=True, exist_ok=True)
