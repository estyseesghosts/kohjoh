# run.ps1: stage kohjoh into Azahar's virtual SD card, launch it, take window
# screenshots at fixed times and collect the program's log.
#
# Azahar is an integration debugger, not a timing authority (kohjoh.md 1):
# every result written here is AZAHAR-only evidence.
#
#   pwsh tools/azahar/run.ps1 -Shots 8,15,25 -Tag boot
#   pwsh tools/azahar/run.ps1 -Save path\to\heartgold.sav -Script tests\inputs\x.txt
param(
  [string]$Edition = "heartgold",
  [string]$Artifact = "build/kj/pokeheartgold.3dsx",
  [string]$Pack = "",
  [string]$Manifest = "",
  [string]$Save = "",
  [switch]$FreshSave,
  [string]$Script = "",
  [string]$Shots = "",
  [string]$Holds = "",
  [int]$Timeout = 300,
  [string]$Tag = "run",
  [string]$OutDir = "build/azahar",
  [int]$Extra = 2
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path))
Set-Location $repo
Add-Type -AssemblyName System.Drawing
$sig = @'
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
[DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int w, int h, bool repaint);
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
public struct RECT { public int Left, Top, Right, Bottom; }
'@
if (-not ("N.W" -as [type])) { Add-Type -MemberDefinition $sig -Name "W" -Namespace "N" | Out-Null }
[N.W]::SetProcessDPIAware() | Out-Null

$sd = Join-Path $env:APPDATA "Azahar\sdmc\3ds"
$root = Join-Path $sd "kohjoh"
foreach ($d in @($root, "$root\data", "$root\saves", "$root\logs")) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
if ($Pack -ne "") { Copy-Item $Pack (Join-Path $root "data\$Edition.pak") -Force }
if ($Manifest -ne "") { Copy-Item $Manifest (Join-Path $root "data\$Edition.manifest.json") -Force }
if ($Save -ne "") { Copy-Item $Save (Join-Path $root "saves\$Edition.sav") -Force }
if ($FreshSave) { Remove-Item (Join-Path $root "saves\$Edition.sav") -ErrorAction SilentlyContinue }
$scriptDst = Join-Path $root "script.txt"
if ($Script -ne "") { Copy-Item $Script $scriptDst -Force } else { Remove-Item $scriptDst -ErrorAction SilentlyContinue }
$log = Join-Path $root "logs\$Edition.log"

$out = Join-Path $OutDir $Tag
New-Item -ItemType Directory -Force -Path $out | Out-Null
$dsx = Join-Path $sd "kohjoh.3dsx"
Copy-Item $Artifact $dsx -Force

Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 800
$logOffset = if (Test-Path $log) { (Get-Item $log).Length } else { 0 }
function Read-NewLog {
  if (-not (Test-Path $log)) { return "" }
  $stream = [System.IO.File]::Open($log, [System.IO.FileMode]::Open,
    [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
  try {
    [void]$stream.Seek([Math]::Min($logOffset, $stream.Length), [System.IO.SeekOrigin]::Begin)
    $reader = [System.IO.StreamReader]::new($stream)
    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
  } finally { $stream.Dispose() }
}
$p = Start-Process -FilePath "C:\Program Files\Azahar\azahar.exe" -ArgumentList "`"$dsx`"" -PassThru
$start = Get-Date
# A fixed, large window so the two screens are readable in the shots.
for ($i = 0; $i -lt 50; $i++) {
  Start-Sleep -Milliseconds 100
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($proc -and $proc.MainWindowHandle -ne 0) { [N.W]::MoveWindow($proc.MainWindowHandle, 0, 0, 840, 1100, $true) | Out-Null; break }
}

function Shot([string]$name) {
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($null -eq $proc) { Write-Output "azahar exited before $name"; return }
  $h = $proc.MainWindowHandle
  $r = New-Object N.W+RECT
  [N.W]::GetWindowRect($h, [ref]$r) | Out-Null
  $w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
  if ($w -le 0 -or $hh -le 0) { Write-Output "no window for $name"; return }
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  [N.W]::PrintWindow($h, $dc, 2) | Out-Null
  $g.ReleaseHdc($dc); $g.Dispose()
  $file = Join-Path $out "$name.png"
  $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  Write-Output "shot $file"
}

# Log-synchronised shots: the script freezes game frame N; shoot while held.
if ($Holds -ne "") {
  foreach ($hf in $Holds.Split(',')) {
    $pat = "holding game frame $hf "
    $deadline = (Get-Date).AddSeconds($Timeout)
    $found = $false
    while ((Get-Date) -lt $deadline) {
      if ((Read-NewLog).Contains($pat)) { $found = $true; break }
      Start-Sleep -Milliseconds 250
    }
    if (-not $found) { Write-Output "frame $hf never held"; break }
    Start-Sleep -Milliseconds 900
    Shot ("f{0:00000}" -f [int]$hf)
  }
}
if ($Shots -ne "") { foreach ($s in $Shots.Split(',')) {
  $t = [double]$s
  $wait = $t - ((Get-Date) - $start).TotalSeconds
  if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
  Shot ("t{0:000}" -f [int]$t)
} }
Start-Sleep -Seconds $Extra
Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
if (Test-Path $log) { [System.IO.File]::WriteAllText((Join-Path (Resolve-Path $out) "$Edition.log"), (Read-NewLog)); Write-Output "log copied" } else { Write-Output "no program log" }
$azlog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"
if (Test-Path $azlog) { Copy-Item $azlog (Join-Path $out "azahar_log.txt") -Force }
