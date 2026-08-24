[CmdletBinding()]
param(
    [string]$Version = '0.6.0'
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$distRoot = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'dist'))
$tag = "v$Version"
$stageDir = [IO.Path]::GetFullPath(
    (Join-Path $distRoot "SnipNexs-$Version-win64"))
$archivePath = [IO.Path]::GetFullPath(
    (Join-Path $distRoot "SnipNexs-$Version-win64.zip"))
$qtSourceName = 'qtbase-everywhere-src-6.11.2.tar.xz'
$qtSourcePath = [IO.Path]::GetFullPath((Join-Path $distRoot $qtSourceName))
$checksumsPath = [IO.Path]::GetFullPath((Join-Path $distRoot 'SHA256SUMS.txt'))
$qtSourceHash = '5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22'
$qtSourceUrl =
    'https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/' + $qtSourceName

Set-Location -LiteralPath $repositoryRoot

$cmakeText = Get-Content -LiteralPath (Join-Path $repositoryRoot 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch "project\(SnipNexs\s+VERSION\s+$([regex]::Escape($Version))") {
    throw "CMake project version does not match $Version."
}

$status = & git status --porcelain
if ($LASTEXITCODE -ne 0 -or $status) {
    throw 'The Git worktree must be clean before packaging a release.'
}

$headTags = @(& git tag --points-at HEAD)
if ($LASTEXITCODE -ne 0 -or $headTags -notcontains $tag) {
    throw "HEAD must have the exact release tag $tag."
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
$vsInstall = & $vswhere -latest -products '*' `
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
        if ($environmentName -ine 'PATH') {
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

$cmake = Get-Command cmake.exe -ErrorAction Stop
$ctest = Join-Path (Split-Path -Parent $cmake.Source) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest)) {
    $ctest = (Get-Command ctest.exe -ErrorAction Stop).Source
}

& $cmake.Source --preset windows-release
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmake.Source --build --preset windows-release
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& $ctest --test-dir build/release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }

$distPrefix = $distRoot.TrimEnd('\') + '\'
if (-not $stageDir.StartsWith($distPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to replace a stage directory outside $distRoot."
}
New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}
if (Test-Path -LiteralPath $checksumsPath) {
    Remove-Item -LiteralPath $checksumsPath -Force
}

& $cmake.Source --install build/release --config Release --prefix $stageDir
if ($LASTEXITCODE -ne 0) { throw 'Release install failed.' }

$previousQpa = $env:QT_QPA_PLATFORM
$env:QT_QPA_PLATFORM = 'offscreen'
try {
    & (Join-Path $stageDir 'bin\SnipNexs.exe') --self-test
    if ($LASTEXITCODE -ne 0) { throw 'The deployed application self-test failed.' }
} finally {
    $env:QT_QPA_PLATFORM = $previousQpa
}

Compress-Archive -Path (Join-Path $stageDir '*') `
    -DestinationPath $archivePath -CompressionLevel Optimal

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
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
$checksumLines = @(
    "$archiveHash  $([IO.Path]::GetFileName($archivePath))"
    "$actualQtHash  $qtSourceName"
)
[IO.File]::WriteAllLines(
    $checksumsPath, $checksumLines, [Text.UTF8Encoding]::new($false))

Write-Host "Release package: $archivePath"
Write-Host "Qt source:      $qtSourcePath"
Write-Host "Checksums:      $checksumsPath"
