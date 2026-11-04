param([Parameter(Mandatory)][string]$WorkRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../scripts/ReleaseFiles.ps1')
$work = [IO.Path]::GetFullPath((Join-Path $WorkRoot 'release-files-fixture'))
Assert-SafeChildPath $work $WorkRoot
if (Test-Path -LiteralPath $work) { throw "Fixture already exists: $work" }
New-Item -ItemType Directory -Path $work | Out-Null
function Write-Fixture([string]$Path, [string]$Content) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    [IO.File]::WriteAllText($Path, $Content)
}
function Require([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Expect-Throw([scriptblock]$Action) {
    $failed = $false
    try { & $Action | Out-Null } catch { $failed = $true }
    Require $failed 'Expected a safe rejection.'
}
try {
    $dist = Join-Path $work 'dist'
    $old = Join-Path $dist 'SnipNexs-1.0.0-win64'
    $new = Join-Path $dist 'SnipNexs-1.0.1-win64'
    $saved = Join-Path $work 'user-data'
    Write-Fixture "$old/bin/SnipNexs.exe" 'old executable'
    Write-Fixture "$old/bin/known.dll" 'original library'
    Write-Fixture "$old/bin/models/translation/bundled/model.bin" 'bundled model'
    Compress-Archive -Path "$old/*" -DestinationPath "$old.zip"
    Write-Fixture "$old/bin/models/translation/opus-mt-en-zh-int8/model.bin" 'retired fast model'
    Write-Fixture "$old/bin/history/private.png" 'private screenshot'
    Write-Fixture "$old/bin/history/index.json" 'private index'
    Write-Fixture "$old/bin/models/translation/custom/model.bin" 'user model'
    Write-Fixture "$old/bin/known.dll" 'modified library'
    Write-Fixture "$old/bin/settings.ini" 'private settings'
    Write-Fixture "$new/bin/SnipNexs.exe" 'new executable'
    Compress-Archive -Path "$new/*" -DestinationPath "$new.zip"
    $null = Test-ReleaseArchive "$new.zip" $new $dist
    Write-Fixture "$new/bin/ctranslate2.dll" 'retired library'
    Expect-Throw { Assert-CleanReleaseStage $new $dist }
    Remove-Item -LiteralPath "$new/bin/ctranslate2.dll"
    Write-Fixture "$new/bin/models/translation/opus-mt-en-zh-int8/model.bin" 'retired model'
    Expect-Throw { Assert-CleanReleaseStage $new $dist }
    Remove-Item -LiteralPath "$new/bin/models/translation/opus-mt-en-zh-int8/model.bin"
    Remove-Item -LiteralPath "$new/bin/models/translation/opus-mt-en-zh-int8"
    $record = Remove-OldDeployment $old "$old.zip" $dist $saved
    Require (-not (Test-Path -LiteralPath $old)) 'Old deployment was not retired.'
    Require ($record.preservedFiles.Count -eq 7) 'Unknown or modified files were lost.'
    foreach ($file in $record.preservedFiles) {
        Require ((Get-FileHash -LiteralPath $file.target).Hash.ToLowerInvariant() -eq $file.sha256) 'Preserved bytes changed.'
    }
    Restore-PreservedRuntimeData $record.preservedDirectory $new $dist $saved
    Require ([IO.File]::ReadAllText("$new/bin/history/private.png") -eq 'private screenshot') 'History did not migrate.'
    Require ([IO.File]::ReadAllText("$new/bin/models/translation/custom/model.bin") -eq 'user model') 'Custom model did not migrate.'
    Require (-not (Test-Path "$new/bin/models/translation/opus-mt-en-zh-int8")) 'Retired fast model migrated to the new app.'
    Require ([IO.File]::ReadAllText("$($record.preservedDirectory)/bin/models/translation/opus-mt-en-zh-int8/model.bin") -eq 'retired fast model') 'Retired model was not preserved.'
    Require (-not (Test-Path "$new/bin/known.dll")) 'Modified old runtime replaced the new runtime.'
    $clean = Get-ZipInventory "$new.zip"
    Require ($clean.Count -eq 1 -and -not $clean.ContainsKey('bin/history/private.png')) 'Private history entered the ZIP.'
    Expect-Throw { Assert-CleanReleaseStage $new $dist }
    Expect-Throw { Remove-OldDeployment $work "$old.zip" $dist $saved }
    Expect-Throw { Remove-OldDeployment $new "$work/missing.zip" $dist $saved }
    Require (Test-Path "$new/bin/history/private.png") 'Failed cleanup damaged the deployment.'
    # Never-used deployments have zero runtime files; retirement must still finish.
    $unused = Join-Path $dist 'SnipNexs-3.0.0-win64'
    Write-Fixture "$unused/bin/SnipNexs.exe" 'clean binary'
    Compress-Archive -Path "$unused/*" -DestinationPath "$unused.zip"
    $unusedRecord = Remove-OldDeployment $unused "$unused.zip" $dist $saved
    Require ($unusedRecord.preservedFiles.Count -eq 0 -and $unusedRecord.removedBytes -eq 12) 'Clean deployment retirement failed.'
    Require (-not (Test-Path -LiteralPath $unused)) 'Clean deployment remains.'
    # A collision must be detected before the old deployment is changed.
    $collision = Join-Path $dist 'SnipNexs-2.0.0-win64'
    Write-Fixture "$collision/bin/SnipNexs.exe" 'exe'
    Compress-Archive -Path "$collision/*" -DestinationPath "$collision.zip"
    Write-Fixture "$collision/bin/history/keep.png" 'original'
    Write-Fixture "$saved/SnipNexs-2.0.0-win64/bin/history/keep.png" 'existing backup'
    Expect-Throw { Remove-OldDeployment $collision "$collision.zip" $dist $saved }
    Require ([IO.File]::ReadAllText("$collision/bin/history/keep.png") -eq 'original') 'Collision destroyed original data.'
    # A junction inside dist must never be followed by retirement/packaging.
    $link = Join-Path $new 'linked-data'
    New-Item -ItemType Junction -Path $link -Target $record.preservedDirectory | Out-Null
    try { Expect-Throw { Get-DeploymentFiles $new $dist } }
    finally {
        Assert-SafeChildPath (Split-Path -Parent $link) $dist
        Require ([bool]((Get-Item -LiteralPath $link -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) 'Fixture link changed.'
        [IO.Directory]::Delete($link) # Nonrecursive removal of this fixture junction only.
    }
    Require (Test-Path "$($record.preservedDirectory)/bin/settings.ini") 'Junction target was removed.'
    $installer = Get-Content (Join-Path $PSScriptRoot '../scripts/installer.iss') -Raw
    Require (-not $installer.Contains('Source: "{#StageDir}\*"')) 'Installer still includes the entire used deployment.'
    Require (-not ($installer -match '(?m)^Source:.*(history|models|\.ini)')) 'Installer includes runtime data.'
    Write-Output 'Release filesystem regression: passed (preservation, migration, clean ZIP, collisions, links, installer allowlist).'
} finally {
    $null = @(Get-DeploymentFiles $work $WorkRoot)
    Remove-Item -LiteralPath $work -Recurse -Force
}
