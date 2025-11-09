# ==== EDIT THESE 4 LINES IF NEEDED ====
$oldModule      = 'ReboundRapture'      # old C++ module/folder name, e.g., Source\ReboundRapture
$newModule      = 'BottomlessPit'       # new module/folder name, e.g., Source\BottomlessPit
$oldTargetRoot  = 'ReboundRapture'      # base name for *.Target.cs classes/files
$newTargetRoot  = 'BottomlessPit'       # new base name for *.Target.cs classes/files
# ======================================

$oldApi = ($oldModule.ToUpper()) + '_API'
$newApi = ($newModule.ToUpper()) + '_API'

Write-Host "Old Module: $oldModule  -> New Module: $newModule"
Write-Host "Old API:    $oldApi      -> New API:    $newApi"
Write-Host "Old Target: $oldTargetRoot -> New Target: $newTargetRoot"

# 1) Clean build junk (safe)
Remove-Item -Recurse -Force .vs,Binaries,DerivedDataCache,Intermediate,Saved -ErrorAction SilentlyContinue
Get-ChildItem -Recurse -Directory -Include Binaries,Intermediate,Saved `
  | Where-Object { $_.FullName -match '\\Plugins\\' } `
  | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Force *.sln -ErrorAction SilentlyContinue

# 2) Rename module source folder + Build.cs file
if (Test-Path "Source\$oldModule") { Rename-Item "Source\$oldModule" $newModule }
if (Test-Path "Source\$oldModule.Build.cs") { Rename-Item "Source\$oldModule.Build.cs" "$newModule.Build.cs" }

# 3) Rename Target .cs files (Game & Editor) — use Move-Item and full paths
$oldGame = Join-Path "Source" "$oldTargetRoot.Target.cs"
$newGame = Join-Path "Source" "$newTargetRoot.Target.cs"
if (Test-Path $oldGame) {
  if (-not (Test-Path $newGame)) { Move-Item -LiteralPath $oldGame -Destination $newGame }
  else { Write-Host "Already renamed: $newGame" }
}

$oldEd = Join-Path "Source" "${oldTargetRoot}Editor.Target.cs"
$newEd = Join-Path "Source" "${newTargetRoot}Editor.Target.cs"
if (Test-Path $oldEd) {
  if (-not (Test-Path $newEd)) { Move-Item -LiteralPath $oldEd -Destination $newEd }
  else { Write-Host "Already renamed: $newEd" }
}
# 4) Text replacements (ONLY text files; no assets touched)
$ex = '*.h','*.hpp','*.cpp','*.cxx','*.inl','*.cs','*.uproject','*.uplugin','*.ini'
Get-ChildItem -Recurse -Include $ex | ForEach-Object {
  $p = $_.FullName
  $t = Get-Content $p -Raw

  # Replace API macro and /Script path first
  $t = $t -replace [regex]::Escape($oldApi), $newApi
  $t = $t -replace "/Script/$oldModule", "/Script/$newModule"

  # Replace plain module name tokens
  $t = $t -replace [regex]::Escape($oldModule), $newModule

  # Targets class names
  $t = $t -replace "class\s+${oldTargetRoot}Target", "class ${newTargetRoot}Target"
  $t = $t -replace "class\s+${oldTargetRoot}EditorTarget", "class ${newTargetRoot}EditorTarget"

  # IMPLEMENT_PRIMARY_GAME_MODULE(..., OldModule, "...")
  $t = $t -replace "(IMPLEMENT_PRIMARY_GAME_MODULE\([^\)]*,\s*)$oldModule(\s*,)", "`$1$newModule`$2"

  Set-Content -Encoding UTF8 $p $t
}

# 5) Ensure Build.cs class name matches file/module
$buildCs = "Source\$newModule.Build.cs"
if (Test-Path $buildCs) {
  $t = Get-Content $buildCs -Raw
  $t = $t -replace "class\s+$([regex]::Escape($oldModule))\s*:\s*ModuleRules", "class $newModule : ModuleRules"
  Set-Content -Encoding UTF8 $buildCs $t
}

# 6) Update .uproject "Modules" entry
$uproject = Get-ChildItem -Filter *.uproject | Select-Object -First 1
if ($uproject) {
  $t = Get-Content $uproject.FullName -Raw
  $t = $t -replace '"Name"\s*:\s*"' + [regex]::Escape($oldModule) + '"','"Name": "' + $newModule + '"'
  Set-Content -Encoding UTF8 $uproject.FullName $t
}

Write-Host "=== Script finished ==="
Write-Host "Next steps:"
Write-Host "1) Add redirects to Config/DefaultEngine.ini"
Write-Host "2) Right-click the .uproject -> Generate VS project files"
Write-Host "3) Build Development Editor and run"
