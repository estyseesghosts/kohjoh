<#
  Runs `lotus-desktop --mcp <host.json>` so its window is visible when the MCP
  client spawns it with a hidden-window start-up flag (Claude Code does).
  stdio passes straight through to lotus-desktop, so this is a drop-in MCP command:

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File <this> [-Lotus <dir>] [-HostJson <file>]

  Cause: the child inherits STARTF_USESHOWWINDOW/SW_HIDE, which Windows applies to
  the first ShowWindow(SW_SHOWDEFAULT). A watcher shows the window once it exists.
#>
param(
  [string]$Lotus = 'C:\Users\terezi\workdir\LOTUS-v0.0.2-windows-x64',
  [string]$HostJson = 'C:\Users\terezi\workdir\LOTUS-v0.0.2-windows-x64\data\citnlo\kohjoh-host.json'
)
$exe = Join-Path $Lotus 'bin\lotus-desktop.exe'

# Watcher: wait for lotus-desktop's top-level "LOTUS" window, then show it.
$null = Start-Job -ScriptBlock {
  param($exe)
  Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class LW{[DllImport("user32.dll")]public static extern bool ShowWindow(IntPtr h,int c);
[DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h);}
'@
  $deadline = (Get-Date).AddSeconds(60)
  while ((Get-Date) -lt $deadline) {
    $p = Get-Process lotus-desktop -ErrorAction SilentlyContinue |
         Where-Object { $_.MainWindowHandle -ne 0 -and $_.Path -eq $exe -and $_.StartTime -gt (Get-Date).AddSeconds(-70) }
    foreach ($q in $p) { [LW]::ShowWindow($q.MainWindowHandle, 9) | Out-Null; [LW]::SetForegroundWindow($q.MainWindowHandle) | Out-Null }
    if ($p) { return }
    Start-Sleep -Milliseconds 300
  }
} -ArgumentList $exe

& $exe --mcp $HostJson
exit $LASTEXITCODE
