#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifndef EDITION
#define EDITION "gold"
#endif
#define TARGET "/3ds/kohjoh-" EDITION ".3dsx"
#define ARG "kohjoh-forwarder:restore="
typedef struct { u64 current, selected; bool enabled; } Config;
#define CFG ((volatile Config *)(OS_SHAREDCFG_VADDR + 0x800))
static char path[0x400] __attribute__((aligned(4)));
static u32 args[0x100];
static Result forward(void)
{
    struct stat st;
    s64 version;
    u64 self;
    Handle port;
    Result r;
    if (stat("sdmc:" TARGET, &st) || R_FAILED(svcGetSystemInfo(&version, 0x10000, 0))) return -1;
    if (!CFG->enabled || R_FAILED(APT_GetProgramID(&self))) return -1;
    r = svcConnectToPort(&port, "hb:ldr");
    if (R_FAILED(r)) return r;
    strcpy(path, TARGET);
    u32 *cmd = getThreadCommandBuffer();
    cmd[0] = IPC_MakeHeader(2, 0, 2);
    cmd[1] = IPC_Desc_StaticBuffer(strlen(path) + 1, 0);
    cmd[2] = (u32)path;
    r = svcSendSyncRequest(port);
    if (R_SUCCEEDED(r)) r = (Result)cmd[1];
    if (R_SUCCEEDED(r)) {
        args[0] = 2;
        char *p = (char *)&args[1];
        p += sprintf(p, "sdmc:" TARGET) + 1;
        sprintf(p, ARG "%016llX", (unsigned long long)CFG->selected);
        cmd[0] = IPC_MakeHeader(3, 0, 2);
        cmd[1] = IPC_Desc_StaticBuffer(sizeof(args), 1);
        cmd[2] = (u32)args;
        r = svcSendSyncRequest(port);
        if (R_SUCCEEDED(r)) r = (Result)cmd[1];
    }
    svcCloseHandle(port);
    if (R_SUCCEEDED(r)) { CFG->selected = self; aptSetChainloader(self, MEDIATYPE_SD); }
    return r;
}
int main(void)
{
    Result r = forward();
    if (R_SUCCEEDED(r)) return 0;
    gfxInitDefault(); consoleInit(GFX_TOP, NULL);
    printf("Could not launch sdmc:" TARGET "\nLuma3DS and the 3DSX are required.\nError: %08lX\nPress START.\n", (unsigned long)r);
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gspWaitForVBlank(); }
    gfxExit(); return 0;
}
