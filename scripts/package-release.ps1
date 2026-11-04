[CmdletBinding()]
param(
    [string]$Version = '0.8.5',
    [switch]$IncludeQtSource,
    [switch]$LocalCandidate,
    [string]$ModelSource
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ReleaseFiles.ps1')
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Version must use major.minor.patch format.' }

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$distRoot = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'dist'))
$tag = "v$Version"
$stageDir = [IO.Path]::GetFullPath(
    (Join-Path $distRoot "SnipNexs-$Version-win64"))
$archivePath = [IO.Path]::GetFullPath(
    (Join-Path $distRoot "SnipNexs-$Version-win64.zip"))
$qtSourceName = 'qtbase-everywhere-src-6.11.2.tar.xz'
$qtSourcePath = [IO.Path]::GetFullPath((Join-Path $distRoot $qtSourceName))
$qtSourceHash = '5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22'
$qtSourceUrl =
    'https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/' + $qtSourceName

Set-Location -LiteralPath $repositoryRoot

$cmakeText = Get-Content -LiteralPath (Join-Path $repositoryRoot 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch "project\(SnipNexs\s+VERSION\s+$([regex]::Escape($Version))") {
    throw "CMake project version does not match $Version."
}

# Candidates are explicitly local: the tagged-release gate remains the default.
if (-not $LocalCandidate) {
$status = & git status --porcelain
if ($LASTEXITCODE -ne 0 -or $status) {
    throw 'The Git worktree must be clean before packaging a release.'
}

$headTags = @(& git tag --points-at HEAD)
if ($LASTEXITCODE -ne 0 -or $headTags -notcontains $tag) {
    throw "HEAD must have the exact release tag $tag."
}

}

$qtDir = if ($env:SNIPNEXS_QT_DIR) {
    [IO.Path]::GetFullPath($env:SNIPNEXS_QT_DIR)
} else {
    [IO.Path]::GetFullPath(
        (Join-Path $repositoryRoot '.qt\6.11.2\msvc2022_64'))
}
if (-not (Test-Path -LiteralPath (Join-Path $qtDir 'bin\Qt6Core.dll'))) {
    throw "Qt 6.11.2 MSVC x64 was not found at $qtDir."
}
$env:SNIPNEXS_QT_DIR = $qtDir

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio Installer helper was not found: $vswhere"
}
$vsInstall = & $vswhere -latest -version '[17.0,18.0)' -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
$vsDevCmd = Join-Path $vsInstall 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path -LiteralPath $vsDevCmd)) {
    throw "Visual Studio 2022 developer environment was not found: $vsDevCmd"
}
$environmentLines = & cmd.exe /d /s /c "`"$vsDevCmd`" -arch=x64 >nul && set"
foreach ($environmentLine in $environmentLines) {
    if ($environmentLine -match '^([^=]+)=(.*)$') {
        $environmentName = $matches[1]
        $environmentValue = $matches[2]
        if ($environmentName -notin @('PATH', 'HOME', 'CODEX_HOME')) {
            [Environment]::SetEnvironmentVariable(
                $environmentName, $environmentValue, 'Process')
        }
    }
}
$developerPathLine = $environmentLines | Where-Object {
    $_ -match '^PATH=.*\\VC\\Tools\\MSVC\\.*\\bin\\HostX64\\x64'
} | Select-Object -First 1
if (-not $developerPathLine) {
    throw 'Visual Studio developer PATH was not produced by VsDevCmd.bat.'
}
$env:Path = $developerPathLine.Substring($developerPathLine.IndexOf('=') + 1)

$env:VSLANG = '1033'
$cmakePath = Join-Path $vsInstall 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
if (-not (Test-Path -LiteralPath $cmakePath)) {
    $cmakePath = (Get-Command cmake.exe -ErrorAction Stop).Source
}
$ctest = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest)) {
    $ctest = (Get-Command ctest.exe -ErrorAction Stop).Source
}

# Reuse one controlled staging path; never overwrite a used deployment.
$pendingStage = Join-Path $distRoot "SnipNexs-$Version-pending"
$pendingArchive = Join-Path $distRoot "SnipNexs-$Version-pending.zip"
foreach ($path in @($stageDir, $archivePath, $pendingStage, $pendingArchive)) {
    Assert-SafeChildPath $path $distRoot
    if (Test-Path -LiteralPath $path) { throw "Output already exists; inspect it before retrying: $path" }
}
$drive = Get-PSDrive ([IO.Path]::GetPathRoot($distRoot).Substring(0,1))
# 8 GiB rebuildable output budget plus a 20 GiB free-space reserve.
if ($drive.Free -lt 28GB) { throw 'Packaging requires 28 GiB free (8 GiB output budget + 20 GiB reserve).' }
New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
$taskTemp = Join-Path $repositoryRoot 'build/package-temp'
New-Item -ItemType Directory -Force -Path $taskTemp | Out-Null
$env:TEMP = $taskTemp
$env:TMP = $taskTemp
& $cmakePath --fresh --preset windows-release -DSNIPNEXS_DESKTOP_TESTS=OFF
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmakePath --build --preset windows-release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& $ctest --test-dir build/release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }

$finalStage = $stageDir
$stageDir = $pendingStage
& $cmakePath --install build/release --config Release --prefix $stageDir
if ($LASTEXITCODE -ne 0) { throw 'Release install failed; previous deployment is untouched.' }

if ($ModelSource) {
    & (Join-Path $PSScriptRoot 'StageTranslationModels.ps1') -Source $ModelSource -Stage $stageDir
}

$deployedExe = Join-Path $stageDir 'bin\SnipNexs.exe'
$deployedVersion = (Get-Item -LiteralPath $deployedExe).VersionInfo.FileVersion
if ($deployedVersion -ne $Version) {
    throw "Deployed file version '$deployedVersion' does not match $Version."
}

# Run deployed checks through CTest with an isolated PATH. Only the offscreen
# platform plugin used by the GUI self-test comes from the developer Qt kit.
$deployedTests = Join-Path $taskTemp 'deployed-validation'
New-Item -ItemType Directory -Force -Path $deployedTests | Out-Null
$exeCmake = $deployedExe.Replace('\', '/')
$pluginsCmake = (Join-Path $stageDir 'plugins').Replace('\', '/')
$offscreenCmake = (Join-Path $qtDir 'plugins/platforms').Replace('\', '/')
$windowsCmake = (Join-Path $env:SystemRoot 'System32').Replace('\', '/')
@"
add_test(DeployedSelfTest [=[$exeCmake]=] --self-test)
set_tests_properties(DeployedSelfTest PROPERTIES TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=$offscreenCmake;QT_PLUGIN_PATH=$pluginsCmake;PATH=$windowsCmake"
    PASS_REGULAR_EXPRESSION "self-test: ok")
add_test(DeployedRuntime [=[$exeCmake]=] --self-test-runtime)
set_tests_properties(DeployedRuntime PROPERTIES TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=$pluginsCmake;PATH=$windowsCmake"
    PASS_REGULAR_EXPRESSION "runtime-self-test: ok")
"@ | Set-Content -LiteralPath (Join-Path $deployedTests 'CTestTestfile.cmake') -Encoding UTF8
& $ctest --test-dir $deployedTests --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Deployed CTest checks failed; previous deployment is untouched.' }

Assert-CleanReleaseStage $stageDir $distRoot
[IO.Compression.ZipFile]::CreateFromDirectory(
    $stageDir, $pendingArchive, [IO.Compression.CompressionLevel]::Optimal, $false)
$inventory = Test-ReleaseArchive $pendingArchive $stageDir $distRoot
# Promote only after both runtime checks and a full ZIP content/hash comparison.
Move-Item -LiteralPath $pendingStage -Destination $finalStage
Move-Item -LiteralPath $pendingArchive -Destination $archivePath
$stageDir = $finalStage

if ($IncludeQtSource) {
    if (-not (Test-Path -LiteralPath $qtSourcePath)) {
        $downloadPath = "$qtSourcePath.download"
        Invoke-WebRequest -Uri $qtSourceUrl -OutFile $downloadPath
        $downloadHash = (Get-FileHash -LiteralPath $downloadPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($downloadHash -ne $qtSourceHash) {
            Remove-Item -LiteralPath $downloadPath -Force
            throw "Qt source SHA-256 mismatch: $downloadHash"
        }
        Move-Item -LiteralPath $downloadPath -Destination $qtSourcePath
    }

    $actualQtHash = (Get-FileHash -LiteralPath $qtSourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualQtHash -ne $qtSourceHash) {
        throw "Existing Qt source SHA-256 mismatch: $actualQtHash"
    }
}
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()

$preserveRoot = Join-Path $repositoryRoot 'user-data'
$cleanup = @()
$staleDirectories = @(Get-ChildItem -LiteralPath $distRoot -Directory -Force | Where-Object {
    $_.Name -match '^SnipNexs-\d+\.\d+\.\d+-win64$' -and $_.FullName -ine $stageDir
} | Sort-Object LastWriteTime -Descending)
foreach ($old in $staleDirectories) {
    try {
        $record = Remove-OldDeployment $old.FullName ($old.FullName + '.zip') $distRoot $preserveRoot
        $cleanup += $record
        Restore-PreservedRuntimeData $record.preservedDirectory $stageDir $distRoot $preserveRoot
    } catch {
        # Packaging succeeds even when a busy/unknown old deployment must be kept.
        Write-Warning "Old deployment retained or data preserved: $($_.Exception.Message)"
    }
}
# Keep rollback ZIPs until a separate, successful GitHub Release publication.
$evidenceDir = Join-Path $repositoryRoot 'artifacts'
New-Item -ItemType Directory -Force -Path $evidenceDir | Out-Null
@{version=$Version; localCandidate=[bool]$LocalCandidate; commit=(& git rev-parse HEAD);
    archive=$archivePath; sha256=$archiveHash; files=$inventory; cleanup=$cleanup} |
    ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $evidenceDir "package-v$Version.json") -Encoding UTF8

Write-Host "Release package: $archivePath"
Write-Host "Package SHA-256: $archiveHash"
if ($IncludeQtSource) {
    Write-Host "Qt source:      $qtSourcePath"
    Write-Host "Qt SHA-256:     $actualQtHash"
}
