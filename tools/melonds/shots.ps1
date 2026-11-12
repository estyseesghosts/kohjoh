# shots.ps1: run a ROM in melonDS (the DS oracle) and capture the window at
# fixed times. melonDS's software 3D renderer is the reference for DS 3D.
#   pwsh tools/melonds/shots.ps1 -Rom ..\hgss-private\heartgold.nds -Shots 60,70 -Tag title
param(
  [string]$Rom = "C:\Users\terezi\workdir\hgss-private\heartgold.nds",
  [string]$Exe = "C:\Users\terezi\workdir\melonDS.exe",
  [string]$Shots = "60",
  [string]$Tag = "melon",
  [string]$OutDir = "build/melonds"
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
if (-not ("M.W" -as [type])) { Add-Type -MemberDefinition $sig -Name "W" -Namespace "M" | Out-Null }
[M.W]::SetProcessDPIAware() | Out-Null
$out = Join-Path $OutDir $Tag
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-Process melonDS -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
$p = Start-Process -FilePath $Exe -ArgumentList "`"$Rom`"" -WorkingDirectory (Split-Path $Exe) -PassThru
$start = Get-Date
for ($i = 0; $i -lt 100; $i++) {
  Start-Sleep -Milliseconds 100
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($proc -and $proc.MainWindowHandle -ne 0) { [M.W]::MoveWindow($proc.MainWindowHandle, 0, 0, 560, 860, $true) | Out-Null; break }
}
foreach ($s in $Shots.Split(',')) {
  $wait = [double]$s - ((Get-Date) - $start).TotalSeconds
  if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($null -eq $proc) { Write-Output "melonDS exited"; break }
  $h = $proc.MainWindowHandle
  $r = New-Object M.W+RECT
  [M.W]::GetWindowRect($h, [ref]$r) | Out-Null
  $bmp = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
  $g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
  [M.W]::PrintWindow($h, $dc, 2) | Out-Null
  $g.ReleaseHdc($dc); $g.Dispose()
  $file = Join-Path $out ("t{0:000}.png" -f [int]$s)
  $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  Write-Output "shot $file"
}
Get-Process -Id $p.Id -ErrorAction SilentlyContinue | Stop-Process -Force
