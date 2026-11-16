A 3DSX-to-CIA forwarder is a small native 3DS application installed on the HOME Menu. It doesn't contain or convert the actual homebrew executable. Instead, it tells the homebrew loader which `.3dsx` to execute, then arranges for that executable to start in its place.

On a modern Luma3DS installation, you can implement this without bundling the Homebrew Launcher or writing your own executable loader.

## 1. How the forwarding works

HOME Menu

User selects your installed CIA

Forwarder executable

Sets the target `.3dsx` and requests a relaunch

Luma3DS loader (`hb:ldr`)

Loads the 3DSX as a new process in the selected title's place

Your homebrew runs

For example, `sdmc:/3ds/myapp/myapp.3dsx`

The critical detail is that the forwarder does not load executable code into its own process. Luma3DS handles creating the replacement process, mapping the 3DSX's code and data, and starting execution.

There's an existing implementation demonstrating this exact architecture:

[Emerald 3DS — forwarder/source/main.c](https://github.com/ZallaxDev/emerald-3ds/blob/main/3ds_port/forwarder/source/main.c?utm_source=chatgpt.com)

It uses Luma's `hb:ldr` interface, changes the designated 3DSX title, then chains back into its own title ID. Its companion game restores the previous loader configuration.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+1



## 2. The important code

Luma3DS exposes two relevant commands through its private `hb:ldr` port:

| Command            | Purpose                              |
| ------------------ | ------------------------------------ |
| `SetTarget` (ID 2) | Set the SD-card path of the 3DSX     |
| `SetArgv` (ID 3)   | Supply startup arguments to the 3DSX |

Both use the 3DS's normal IPC mechanism. Luma implements their handlers in `sysmodules/loader/source/hbldr.c`.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



Here is a minimal adaptation of the forwarder's core, using libctru:

```

#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define TARGET "/3ds/myapp/myapp.3dsx"
#define ARG0   "sdmc:" TARGET
#define RESTORE_PREFIX "myapp-forwarder:restore="

typedef struct {
    u64 current;
    u64 selected;
    bool enabled;
} LumaConfig;

#define LUMA_CFG \
    ((volatile LumaConfig *)(OS_SHAREDCFG_VADDR + 0x800))

static char pathBuf[0x400] __attribute__((aligned(4)));
static u32 argvBuf[0x400 / 4];

static Result sendTarget(Handle h) {
    u32 *cmd = getThreadCommandBuffer();

    memset(pathBuf, 0, sizeof(pathBuf));
    strcpy(pathBuf, TARGET);

    cmd[0] = IPC_MakeHeader(2, 0, 2);
    cmd[1] = IPC_Desc_StaticBuffer(strlen(pathBuf) + 1, 0);
    cmd[2] = (u32)pathBuf;

    Result r = svcSendSyncRequest(h);
    return R_FAILED(r) ? r : (Result)cmd[1];
}

static Result sendArgs(Handle h, u64 previousTid) {
    u32 *cmd = getThreadCommandBuffer();
    char *p = (char *)&argvBuf[1];

    memset(argvBuf, 0, sizeof(argvBuf));
    argvBuf[0] = 2; // argc

    p += sprintf(p, "%s", ARG0) + 1;
    sprintf(p, RESTORE_PREFIX "%016llX",
            (unsigned long long)previousTid);

    cmd[0] = IPC_MakeHeader(3, 0, 2);
    cmd[1] = IPC_Desc_StaticBuffer(sizeof(argvBuf), 1);
    cmd[2] = (u32)argvBuf;

    Result r = svcSendSyncRequest(h);
    return R_FAILED(r) ? r : (Result)cmd[1];
}

static Result forward(void) {
    s64 lumaVersion;
    if (R_FAILED(svcGetSystemInfo(&lumaVersion, 0x10000, 0)))
        return -1;

    if (!LUMA_CFG->enabled)
        return -2;

    u64 selfTid;
    Result r = APT_GetProgramID(&selfTid);
    if (R_FAILED(r)) return r;

    u64 previousTid = LUMA_CFG->selected;
    Handle h;

    r = svcConnectToPort(&h, "hb:ldr");
    if (R_FAILED(r)) return r;

    r = sendTarget(h);
    if (R_SUCCEEDED(r))
        r = sendArgs(h, previousTid);

    svcCloseHandle(h);
    if (R_FAILED(r)) return r;

    LUMA_CFG->selected = selfTid;
    aptSetChainloader(selfTid, MEDIATYPE_SD);
    return 0;
}

int main(void) {
    Result r = forward();
    if (R_SUCCEEDED(r)) return 0;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    printf("Forwarder failed: %08lX\n",
           (unsigned long)r);
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}

```

This is an illustrative core, not a tested complete forwarder. It assumes Luma3DS and a valid installed application title. Add a missing-file check before changing loader state.

### Why the shared configuration matters

The forwarder's two most important operations are:

```
LUMA_CFG->selected = selfTid;
aptSetChainloader(selfTid, MEDIATYPE_SD);
```

The first tells Luma that this title should serve as its 3DSX loading title. The second schedules that title to launch again when the forwarder exits.

On that second launch, Luma substitutes the requested `.3dsx` for the installed executable.

Luma keeps separate current and selected title IDs. The process manager applies the pending selection when the current application closes; writing the current field directly would be incorrect.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+1



## 3. Restore the previous loader configuration

There's one complication: you don't want your forwarder permanently changing which title Luma uses for homebrew loading.

That's why the forwarder passes the previous title ID to the launched application.

At the beginning of your 3DSX's `main()`, add something like:

```

#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#define PREFIX "myapp-forwarder:restore="

static void restoreLoader(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], PREFIX, sizeof(PREFIX) - 1))
            continue;

        char *end;
        u64 tid = strtoull(argv[i] + sizeof(PREFIX) - 1,
                           &end, 16);

        s64 version;
        if (*end == '\0' && tid != 0 &&
            R_SUCCEEDED(svcGetSystemInfo(&version, 0x10000, 0))) {
            *(volatile u64 *)(OS_SHAREDCFG_VADDR + 0x808) = tid;
        }
        break;
    }
}

```

Call `restoreLoader(argc, argv)` before the application's usual initialization. This restores the selected title for future launches, without changing the title under which the current process is running.

The Emerald 3DS implementation uses the same approach.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub



## 4. Build and package the CIA

The quickest implementation is to adapt Emerald 3DS's existing files rather than recreate its build system.

| File                                                                                                                        | What to change                                   |
| --------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| [source/main.c](https://github.com/ZallaxDev/emerald-3ds/blob/main/3ds_port/forwarder/source/main.c?utm_source=chatgpt.com) | Target path, argument prefix, error messages     |
| [forwarder.rsf](https://github.com/ZallaxDev/emerald-3ds/blob/main/3ds_port/forwarder/forwarder.rsf?utm_source=chatgpt.com) | Unique title ID, product code, title metadata    |
| [Makefile](https://github.com/ZallaxDev/emerald-3ds/blob/main/3ds_port/forwarder/Makefile?utm_source=chatgpt.com)           | Application name, artwork paths, output filename |

The important RSF options include:

```
Option:
  UseOnSD: true

AccessControlInfo:
  FileSystemAccess:
    - DirectSdmc

  CanWriteSharedPage: true
  PermitMainFunctionArgument: true
```

Use a unique application title ID that doesn't conflict with anything installed. Ensure the code and RSF agree on it.

With devkitARM, libctru, `makerom`, and `bannertool` installed, the existing Makefile performs the ELF compilation and CIA packaging:

```
make
```

Your SD-card installation would look like this:

```
SD:/
├── 3ds/
│   └── myapp/
│       ├── myapp.3dsx
│       └── assets/
└── cias/
    └── myapp-forwarder.cia
```

Install the CIA once using FBI. Subsequently, replacing `myapp.3dsx` updates the application without reinstalling the CIA.

## 5. Important limitations

- Save data: The forwarded 3DSX doesn't automatically gain native CIA save archives. Its SD-based save implementation remains unchanged.
- RomFS: Embedded 3DSX RomFS still relies on its normal loading path and startup arguments.
- Performance: Forwarding doesn't improve execution speed or change the application into a natively packaged CIA.
- Compatibility: This approach depends on Luma3DS internals, particularly its shared configuration layout and `hb:ldr` behavior.
- Error recovery: Handle a missing 3DSX before changing Luma's selected title. If the target fails after selection, the configuration may need manual recovery.

The distinction worth preserving: a forwarder CIA is an installed shortcut, whereas a native CIA is an actual installed application with its own packaged executable, RomFS and potentially native save data.

For a project where the same SD-based homebrew must be independently updatable, I'd use the modern Luma3DS forwarder approach. For a self-contained install with real 3DS save archives, I'd build a separate native CIA target from the same source code.
