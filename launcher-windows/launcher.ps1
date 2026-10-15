# WWE 2K14 Recomp launcher (Windows). Started by "WWE2K14 Launcher.exe" (or run with
#   powershell -NoProfile -ExecutionPolicy Bypass -File launcher.ps1).
# Reads and writes settings.txt next to it and starts wwe2k14.exe exactly like play-wwe2k14.bat.
# Unofficial fan project by mikeflare007 and Claude. Not affiliated with WWE, 2K or Yuke's.

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -Namespace Native -Name Dpi -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();'
[void][Native.Dpi]::SetProcessDPIAware()
[System.Windows.Forms.Application]::EnableVisualStyles()

# Any unexpected error: write it next to the logs and show it, instead of failing silently.
trap {
  $msg = "$($_.Exception.Message)`r`n$($_.InvocationInfo.PositionMessage)"
  try {
    $d = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) 'logs'
    New-Item -ItemType Directory -Force -Path $d | Out-Null
    Set-Content -Path (Join-Path $d 'launcher-error.txt') -Value $msg
  } catch {}
  [void][System.Windows.Forms.MessageBox]::Show("The launcher hit an error:`r`n`r`n$msg", 'WWE 2K14 Recomp', 'OK', 'Error')
  exit 1
}

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Here
$SettingsPath = Join-Path $Here 'settings.txt'
$IssuesUrl = 'https://github.com/mikeflare007/wwe2k14-recomp/issues'
$Version = ''
if (Test-Path (Join-Path $Here 'VERSION')) { $Version = (Get-Content (Join-Path $Here 'VERSION') -TotalCount 1).Trim() }

# ---------------------------------------------------------------- settings.txt
function Read-Settings {
  $s = @{ resolution = '4k-fsr'; fps = 'original'; fullscreen = 'yes'; texture_fix = 'on'; game_folder = '' }
  if (Test-Path $SettingsPath) {
    foreach ($line in Get-Content $SettingsPath) {
      $t = $line.Trim()
      if ($t -eq '' -or $t.StartsWith('#')) { continue }
      $i = $t.IndexOf('=')
      if ($i -lt 1) { continue }
      $s[$t.Substring(0, $i).Trim().ToLower()] = $t.Substring($i + 1).Trim()
    }
  }
  return $s
}

function Save-Settings($s) {
  # Keep comments and order; replace known keys in place, append any that are missing.
  $lines = @()
  if (Test-Path $SettingsPath) { $lines = @(Get-Content $SettingsPath) }
  $done = @{}
  $out = foreach ($line in $lines) {
    $t = $line.Trim(); $i = $t.IndexOf('=')
    if (-not $t.StartsWith('#') -and $i -gt 0) {
      $k = $t.Substring(0, $i).Trim().ToLower()
      if ($s.ContainsKey($k)) { $done[$k] = $true; "$k=$($s[$k])"; continue }
    }
    $line
  }
  foreach ($k in @('game_folder', 'resolution', 'fps', 'fullscreen', 'texture_fix')) {
    if (-not $done.ContainsKey($k)) { $out += "$k=$($s[$k])" }
  }
  [System.IO.File]::WriteAllLines($SettingsPath, [string[]]$out)
}

function Get-GameFolder($s) {
  if ($s.game_folder) { return $s.game_folder }
  return (Join-Path $Here 'game')
}

# ---------------------------------------------------------------- look
$dpi = 96
$g = [System.Drawing.Graphics]::FromHwnd([IntPtr]::Zero); $dpi = $g.DpiX; $g.Dispose()
$k = $dpi / 96.0
function S([double]$v) { return [int][Math]::Round($v * $k) }

$cBg      = [System.Drawing.Color]::FromArgb(18, 18, 20)
$cPanel   = [System.Drawing.Color]::FromArgb(30, 30, 34)
$cLine    = [System.Drawing.Color]::FromArgb(58, 58, 64)
$cText    = [System.Drawing.Color]::FromArgb(236, 236, 238)
$cMuted   = [System.Drawing.Color]::FromArgb(150, 150, 158)
$cRed     = [System.Drawing.Color]::FromArgb(206, 17, 38)
$cRedHot  = [System.Drawing.Color]::FromArgb(232, 36, 58)
$cGood    = [System.Drawing.Color]::FromArgb(110, 200, 140)
$cWarn    = [System.Drawing.Color]::FromArgb(240, 180, 80)

$fTitle = New-Object System.Drawing.Font('Segoe UI', 26, [System.Drawing.FontStyle]::Bold)
$fSub   = New-Object System.Drawing.Font('Segoe UI', 9.5)
$fHead  = New-Object System.Drawing.Font('Segoe UI Semibold', 9.5)
$fBody  = New-Object System.Drawing.Font('Segoe UI', 10)
$fPlay  = New-Object System.Drawing.Font('Segoe UI', 18, [System.Drawing.FontStyle]::Bold)

$form = New-Object System.Windows.Forms.Form
$form.Text = 'WWE 2K14 Recomp'
$form.FormBorderStyle = 'FixedSingle'
$form.MaximizeBox = $false
$form.StartPosition = 'CenterScreen'
$form.BackColor = $cBg
$form.ForeColor = $cText
$form.Font = $fBody
$form.ClientSize = New-Object System.Drawing.Size((S 560), (S 640))
$exeIcon = Join-Path $Here 'wwe2k14.exe'
try { if (Test-Path $exeIcon) { $form.Icon = [System.Drawing.Icon]::ExtractAssociatedIcon($exeIcon) } } catch {}

function New-Label($text, $x, $y, $w, $h, $font, $color) {
  $l = New-Object System.Windows.Forms.Label
  $l.Text = $text; $l.Font = $font; $l.ForeColor = $color; $l.BackColor = [System.Drawing.Color]::Transparent
  $l.Location = New-Object System.Drawing.Point((S $x), (S $y)); $l.Size = New-Object System.Drawing.Size((S $w), (S $h))
  return $l
}

function New-Button($text, $x, $y, $w, $h) {
  $b = New-Object System.Windows.Forms.Button
  $b.Text = $text; $b.FlatStyle = 'Flat'; $b.BackColor = $cPanel; $b.ForeColor = $cText
  $b.FlatAppearance.BorderColor = $cLine; $b.FlatAppearance.MouseOverBackColor = [System.Drawing.Color]::FromArgb(44, 44, 50)
  $b.Cursor = [System.Windows.Forms.Cursors]::Hand
  $b.Location = New-Object System.Drawing.Point((S $x), (S $y)); $b.Size = New-Object System.Drawing.Size((S $w), (S $h))
  return $b
}

function New-Combo($x, $y, $w, $items) {
  $c = New-Object System.Windows.Forms.ComboBox
  $c.DropDownStyle = 'DropDownList'; $c.FlatStyle = 'Flat'; $c.BackColor = $cPanel; $c.ForeColor = $cText
  $c.Location = New-Object System.Drawing.Point((S $x), (S $y)); $c.Width = (S $w)
  foreach ($i in $items) { [void]$c.Items.Add($i) }
  return $c
}

function New-Check($text, $x, $y, $w) {
  $c = New-Object System.Windows.Forms.CheckBox
  $c.Text = $text; $c.ForeColor = $cText; $c.FlatStyle = 'Flat'; $c.Cursor = [System.Windows.Forms.Cursors]::Hand
  $c.Location = New-Object System.Drawing.Point((S $x), (S $y)); $c.Size = New-Object System.Drawing.Size((S $w), (S 26))
  return $c
}

# Header: red rule + title
$rule = New-Object System.Windows.Forms.Panel
$rule.BackColor = $cRed; $rule.Location = New-Object System.Drawing.Point(0, 0); $rule.Size = New-Object System.Drawing.Size((S 560), (S 6))
$form.Controls.Add($rule)
$titleW = [System.Windows.Forms.TextRenderer]::MeasureText('WWE 2K14', $fTitle).Width
$t1 = New-Label 'WWE 2K14' 26 22 260 50 $fTitle $cText
$t1.AutoSize = $true
$form.Controls.Add($t1)
$recomp = New-Label 'RECOMP' 0 22 200 50 $fTitle $cRed
$recomp.AutoSize = $true
$recomp.Left = $t1.Left + $titleW - (S 8)
$form.Controls.Add($recomp)
$sub = 'Native PC port  |  mikeflare007 and Claude'
if ($Version) { $sub += "  |  v$Version" }
$form.Controls.Add((New-Label $sub 30 72 500 22 $fSub $cMuted))

# Play
$play = New-Object System.Windows.Forms.Button
$play.Text = 'PLAY'; $play.Font = $fPlay; $play.FlatStyle = 'Flat'; $play.BackColor = $cRed; $play.ForeColor = [System.Drawing.Color]::White
$play.FlatAppearance.BorderSize = 0; $play.FlatAppearance.MouseOverBackColor = $cRedHot
$play.Cursor = [System.Windows.Forms.Cursors]::Hand
$play.Location = New-Object System.Drawing.Point((S 30), (S 108)); $play.Size = New-Object System.Drawing.Size((S 500), (S 64))
$form.Controls.Add($play)

# Display
$form.Controls.Add((New-Label 'DISPLAY' 30 192 200 20 $fHead $cMuted))
$form.Controls.Add((New-Label 'Resolution' 30 220 150 24 $fBody $cText))
$resItems = @('720p (native)', '1440p', '4K', '4K-FSR (looks like 4K, runs like 1440p)')
$resKeys  = @('720p', '1440p', '4k', '4k-fsr')
$cbRes = New-Combo 190 217 340 $resItems
$form.Controls.Add($cbRes)
$form.Controls.Add((New-Label 'Frame rate' 30 256 150 24 $fBody $cText))
$fpsItems = @('Original (60 in matches, 30 in entrances)', '60 everywhere (entrances run fast)')
$fpsKeys  = @('original', '60')
$cbFps = New-Combo 190 253 340 $fpsItems
$form.Controls.Add($cbFps)
$chkFull = New-Check 'Fullscreen  (F11 switches in game)' 28 290 400
$form.Controls.Add($chkFull)

# Fixes
$form.Controls.Add((New-Label 'FIXES' 30 334 200 20 $fHead $cMuted))
$chkTex = New-Check 'Fix managers and alternate / created attires' 28 358 480
$form.Controls.Add($chkTex)
$form.Controls.Add((New-Label 'Stops black or rainbow-striped models (Miss Elizabeth, alt outfits). Leave on.' 48 384 480 20 $fSub $cMuted))

# Game folder
$form.Controls.Add((New-Label 'GAME FILES' 30 422 200 20 $fHead $cMuted))
$txtGame = New-Object System.Windows.Forms.TextBox
$txtGame.ReadOnly = $true; $txtGame.BorderStyle = 'FixedSingle'; $txtGame.BackColor = $cPanel; $txtGame.ForeColor = $cText
$txtGame.Location = New-Object System.Drawing.Point((S 30), (S 448)); $txtGame.Width = (S 390)
$form.Controls.Add($txtGame)
$btnBrowse = New-Button 'Browse...' 430 446 100 28
$form.Controls.Add($btnBrowse)
$lblGame = New-Label '' 30 478 500 20 $fSub $cMuted
$form.Controls.Add($lblGame)

# Tools row
$form.Controls.Add((New-Label 'TOOLS' 30 512 200 20 $fHead $cMuted))
$btnBackup = New-Button 'Back up saves' 30 538 118 32
$btnDlc    = New-Button 'Install DLC...' 157 538 118 32
$btnLogs   = New-Button 'Open logs' 284 538 118 32
$btnBug    = New-Button 'Report a bug' 411 538 119 32
$form.Controls.AddRange(@($btnBackup, $btnDlc, $btnLogs, $btnBug))

# Status
$sep = New-Object System.Windows.Forms.Panel
$sep.BackColor = $cLine; $sep.Location = New-Object System.Drawing.Point((S 30), (S 588)); $sep.Size = New-Object System.Drawing.Size((S 500), 1)
$form.Controls.Add($sep)
$lblStatus = New-Label 'Ready.' 30 598 500 30 $fSub $cMuted
$form.Controls.Add($lblStatus)

function Set-Status($text, $color) { $lblStatus.Text = $text; $lblStatus.ForeColor = $color }

# ---------------------------------------------------------------- state
$Settings = Read-Settings
$i = [Array]::IndexOf($resKeys, $Settings.resolution.ToLower()); if ($Settings.resolution -ieq '2160p') { $i = 2 }; if ($i -lt 0) { $i = 3 }
$cbRes.SelectedIndex = $i
$i = [Array]::IndexOf($fpsKeys, $Settings.fps.ToLower()); if ($Settings.fps -eq '30') { $i = 0 }; if ($i -lt 0) { $i = 0 }
$cbFps.SelectedIndex = $i
$chkFull.Checked = ($Settings.fullscreen -ine 'no')
$chkTex.Checked = ($Settings.texture_fix -ine 'off')

function Update-GameLabel {
  $folder = Get-GameFolder $Settings
  $txtGame.Text = $folder
  if (Test-Path (Join-Path $folder 'default.xex')) {
    $lblGame.Text = 'Game found.'; $lblGame.ForeColor = $cGood
  } else {
    $lblGame.Text = 'default.xex not found here. Pick your extracted WWE 2K14 folder.'; $lblGame.ForeColor = $cWarn
  }
}
Update-GameLabel

function Store-Ui {
  $Settings.resolution = $resKeys[$cbRes.SelectedIndex]
  $Settings.fps = $fpsKeys[$cbFps.SelectedIndex]
  $Settings.fullscreen = $(if ($chkFull.Checked) { 'yes' } else { 'no' })
  $Settings.texture_fix = $(if ($chkTex.Checked) { 'on' } else { 'off' })
  try { Save-Settings $Settings } catch { Set-Status "Couldn't save settings.txt: $($_.Exception.Message)" $cWarn }
}
$cbRes.Add_SelectedIndexChanged({ Store-Ui })
$cbFps.Add_SelectedIndexChanged({ Store-Ui })
$chkFull.Add_CheckedChanged({ Store-Ui })
$chkTex.Add_CheckedChanged({ Store-Ui })

function Show-Error($text) {
  [void][System.Windows.Forms.MessageBox]::Show($form, $text, 'WWE 2K14 Recomp', 'OK', 'Warning')
}

function Find-Python {
  foreach ($c in @(@('py', '-3'), @('python'))) {
    $cmd = Get-Command $c[0] -ErrorAction SilentlyContinue
    if ($cmd) { return $c }
  }
  return $null
}

# ---------------------------------------------------------------- buttons
$btnBrowse.Add_Click({
  $d = New-Object System.Windows.Forms.FolderBrowserDialog
  $d.Description = 'Pick your extracted WWE 2K14 folder (the one that contains default.xex).'
  $cur = Get-GameFolder $Settings
  if (Test-Path $cur) { $d.SelectedPath = $cur }
  if ($d.ShowDialog($form) -eq 'OK') {
    $Settings.game_folder = $d.SelectedPath
    Store-Ui
    Update-GameLabel
  }
})

$btnLogs.Add_Click({
  $p = Join-Path $Here 'logs'; New-Item -ItemType Directory -Force -Path $p | Out-Null
  Start-Process explorer.exe $p
})

$btnBug.Add_Click({ Start-Process $IssuesUrl })

$btnBackup.Add_Click({
  $src = Join-Path $Here 'userdata'
  if (-not (Test-Path $src)) { Show-Error 'No saves yet: play the game first.'; return }
  $dest = Join-Path $Here ('save-backup\' + (Get-Date -Format 'yyyy-MM-dd HHmm'))
  New-Item -ItemType Directory -Force -Path $dest | Out-Null
  $n = 0
  foreach ($item in Get-ChildItem $src -Force) {
    if ($item.Name -in @('cache', 'cache_drive')) { continue }   # shader and game caches, not saves
    Copy-Item $item.FullName -Destination $dest -Recurse -Force; $n++
  }
  Set-Status "Saves backed up to save-backup\$(Split-Path $dest -Leaf)" $cGood
})

$btnDlc.Add_Click({
  $py = Find-Python
  if (-not $py) { Show-Error "Installing DLC needs Python 3 (one time). Get it from https://www.python.org and tick 'Add python.exe to PATH'."; return }
  $d = New-Object System.Windows.Forms.OpenFileDialog
  $d.Title = 'Pick your WWE 2K14 DLC package files'; $d.Multiselect = $true; $d.Filter = 'All files (*.*)|*.*'
  if ($d.ShowDialog($form) -ne 'OK') { return }
  $script = Join-Path $Here 'install-dlc.py'
  $a = @()
  if ($py.Count -gt 1) { $a += $py[1] }
  $a += @($script) + $d.FileNames + @('--userdata', (Join-Path $Here 'userdata'))
  $quoted = ($a | ForEach-Object { '"' + $_ + '"' }) -join ' '
  $p = Start-Process -FilePath $py[0] -ArgumentList $quoted -Wait -PassThru -WindowStyle Hidden
  if ($p.ExitCode -eq 0) { Set-Status 'DLC installed. It shows up in game like on the console.' $cGood }
  else { Set-Status "DLC install failed (exit $($p.ExitCode)). Check the files are WWE 2K14 DLC packages." $cWarn }
})

# ---------------------------------------------------------------- launch
function Prepare-Game {
  # 1) A new build delivered as zip parts: join, unpack, remove.
  $p0 = Join-Path $Here 'wwe2k14-build.zip.000'
  if (Test-Path $p0) {
    Set-Status 'Unpacking the new game build...' $cMuted; $form.Refresh()
    $zip = Join-Path $Here 'wwe2k14-build.zip'
    $parts = Get-ChildItem $Here -Filter 'wwe2k14-build.zip.*' | Where-Object { $_.Name -match '\.\d{3}$' } | Sort-Object Name
    $fs = [System.IO.File]::Create($zip)
    try { foreach ($part in $parts) { $b = [System.IO.File]::ReadAllBytes($part.FullName); $fs.Write($b, 0, $b.Length) } } finally { $fs.Close() }
    & tar.exe -xf $zip -C $Here
    if ($LASTEXITCODE -ne 0) { throw 'Unpacking the new build failed.' }
    Remove-Item $zip; $parts | Remove-Item
  }

  $game = Get-GameFolder $Settings
  if (-not (Test-Path (Join-Path $game 'default.xex'))) {
    throw "Can't find your game. Use Browse... to pick your extracted WWE 2K14 folder (the one with default.xex)."
  }

  # 2) Title update 2.0.1.0, unpacked once from the user's own TU file.
  $update = Join-Path $Here 'update'
  if (-not (Test-Path (Join-Path $update 'default.xexp'))) {
    $tuDir = Join-Path $Here 'title-update'
    New-Item -ItemType Directory -Force -Path $tuDir | Out-Null
    $tu = Get-ChildItem $tuDir -File | Where-Object { $_.Extension -ne '.txt' } | Select-Object -First 1
    if (-not $tu) {
      Start-Process explorer.exe $tuDir
      throw "Title update 2.0.1.0 is needed. Put your own title update file (named like tu00000002_00000000) in the title-update folder that just opened, then press Play again."
    }
    $py = Find-Python
    if (-not $py) { throw "Unpacking the title update needs Python 3 (one time only). Get it from https://www.python.org and tick 'Add python.exe to PATH'." }
    Set-Status 'Unpacking your title update (one time only)...' $cMuted; $form.Refresh()
    $script = Join-Path $Here 'tools\extract-title-update.py'
    $a = @(); if ($py.Count -gt 1) { $a += $py[1] }
    $a += @($script, $tu.FullName, $update)
    $quoted = ($a | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $p = Start-Process -FilePath $py[0] -ArgumentList $quoted -Wait -PassThru -WindowStyle Hidden
    if ($p.ExitCode -ne 0 -or -not (Test-Path (Join-Path $update 'default.xexp'))) {
      if (Test-Path $update) { Remove-Item $update -Recurse -Force }
      throw "That file doesn't look like the WWE 2K14 title update 2.0.1.0."
    }
  }
  if (-not (Test-Path (Join-Path $game 'default.xexp'))) {
    try { Copy-Item (Join-Path $update 'default.xexp') $game } catch {}
  }
  return $game
}

$script:GameProc = $null
$watch = New-Object System.Windows.Forms.Timer
$watch.Interval = 1000
$watch.Add_Tick({
  if ($script:GameProc -and $script:GameProc.HasExited) {
    $watch.Stop()
    $code = $script:GameProc.ExitCode
    $script:GameProc = $null
    $play.Enabled = $true; $play.Text = 'PLAY'
    $form.Show(); $form.WindowState = 'Normal'; $form.Activate()
    if ($code -eq 0) { Set-Status 'The game closed normally.' $cMuted }
    else { Set-Status "The game stopped (code $code). If it crashed, Report a bug and attach the newest file from Open logs." $cWarn }
  }
})

$play.Add_Click({
  Store-Ui
  try {
    $play.Enabled = $false
    $game = Prepare-Game
  } catch {
    $play.Enabled = $true
    Set-Status 'Not started.' $cWarn
    Show-Error $_.Exception.Message
    return
  }

  $exe = Join-Path $Here 'wwe2k14.exe'
  if (-not (Test-Path $exe)) { $play.Enabled = $true; Show-Error 'wwe2k14.exe is missing next to the launcher.'; return }

  # Same switches as play-wwe2k14.bat.
  $scale = '3'; $fx = $null
  switch ($Settings.resolution.ToLower()) {
    '720p'   { $scale = '1' }
    '1440p'  { $scale = '2' }
    '4k-fsr' { $scale = '2'; $fx = '--present_effect=fsr' }
  }
  $env:WWE2K14_RES_SCALE = $scale
  $env:WWE2K14_KEEP_30 = $(if ($Settings.fps -ieq 'original' -or $Settings.fps -eq '30') { '1' } else { '' })
  $env:WWE2K14_UNTILE_COHERENCY = $(if ($Settings.texture_fix -ieq 'off') { '0' } else { '' })
  $env:REX_DEBUG_UI = 'false'
  $env:REX_UPDATE_DATA_ROOT = (Join-Path $Here 'update')
  $env:REX_MOUNT_CACHE = 'true'
  Remove-Item Env:WWE13_READBACK_RANGE -ErrorAction SilentlyContinue
  Remove-Item Env:WWE13_RESOLVE_LOG -ErrorAction SilentlyContinue

  New-Item -ItemType Directory -Force -Path (Join-Path $Here 'logs'), (Join-Path $Here 'userdata') | Out-Null
  $log = Join-Path $Here ('logs\wwe2k14-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
  $argv = @("`"$game`"", $(if ($Settings.fullscreen -ieq 'no') { '--fullscreen=false' } else { '--fullscreen=true' }))
  if ($fx) { $argv += $fx }
  $argv += @('--vulkan_allow_present_mode_immediate=false', '--ignore_offset_for_ranged_allocations=true',
             '--log_verbose=false', '--mnk_mode=true', "`"--log_file=$log`"",
             "`"--user_data_root=$(Join-Path $Here 'userdata')`"", '--debug_ui=false')

  try {
    $script:GameProc = Start-Process -FilePath $exe -ArgumentList ($argv -join ' ') -WorkingDirectory $Here -PassThru
  } catch {
    $play.Enabled = $true
    Show-Error "Couldn't start the game: $($_.Exception.Message)"
    return
  }
  $play.Text = 'RUNNING...'
  Set-Status 'Playing. The launcher comes back when the game closes.' $cGood
  $watch.Start()
  $form.Hide()
})

$form.Add_Shown({ $form.Activate() })
# Application.Run (not ShowDialog) so the launcher can hide while the game runs and come back after.
[System.Windows.Forms.Application]::Run($form)
