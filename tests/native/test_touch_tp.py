#!/usr/bin/env python3
"""Compile the real virtual touch panel and check pen-up semantics (#28).

Run from the repository root: python tests/native/test_touch_tp.py
Set CC to a host C compiler if `cc` is not on PATH.
"""

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]

NITRO_TYPES = """
#ifndef STUB_NITRO_TYPES_H
#define STUB_NITRO_TYPES_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif
"""

NITRO_H = """
#ifndef STUB_NITRO_H
#define STUB_NITRO_H
#include <nitro/types.h>
typedef struct OS_SystemWork {
    unsigned char touch_panel[4];
} OS_SystemWork;
OS_SystemWork *OS_GetSystemWork(void);
#endif
"""

NITRO_PORT_H = """
#ifndef STUB_NITRO_PORT_H
#define STUB_NITRO_PORT_H
#include <nitro/types.h>
#define NITRO_PORT_TP_RAW_SCALE 16
#define PXI_FIFO_TAG_TOUCHPANEL 9
void NitroPort_Arm7Reply(int tag, u32 data, BOOL err);
void NitroPort_SetTouch(BOOL down, u16 x, u16 y);
void NitroPort_ResetTouchState(void);
void NitroPort_HandleTp(int tag, u32 data, BOOL err);
#endif
"""

PM_COMMON_H = """
#ifndef STUB_PM_COMMON_H
#define STUB_PM_COMMON_H
#endif
"""


def main():
    with tempfile.TemporaryDirectory() as temp:
        work = Path(temp)
        stub = work / "include"
        (stub / "nitro" / "spi" / "common").mkdir(parents=True)
        (stub / "nitro" / "types.h").write_text(NITRO_TYPES, encoding="utf-8")
        (stub / "nitro.h").write_text(NITRO_H, encoding="utf-8")
        (stub / "nitro_port.h").write_text(NITRO_PORT_H, encoding="utf-8")
        (stub / "nitro" / "spi" / "common" / "pm_common.h").write_text(
            PM_COMMON_H, encoding="utf-8"
        )

        executable = work / ("touch_tp_test.exe" if os.name == "nt" else "touch_tp_test")
        subprocess.run([
            os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-I", str(stub),
            "-I", str(ROOT / "src" / "compat" / "hgss" / "include"),
            str(ROOT / "tests" / "native" / "touch_tp_test.c"),
            str(ROOT / "src" / "compat" / "nitro" / "src" / "port_tp.c"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
