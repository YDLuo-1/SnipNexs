# Shared by release packaging and filesystem regression tests.
Set-StrictMode -Version Latest
# CTest may inherit PowerShell 7's module path when invoking Windows PowerShell.
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility') -Force
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Archive') -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Assert-SafeChildPath {
    param([string]$Directory, [string]$AllowedRoot)
    $root = [IO.Path]::GetFullPath($AllowedRoot).TrimEnd('\')
    $path = [IO.Path]::GetFullPath($Directory).TrimEnd('\')
    if (-not $path.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the permitted root: $path"
    }
    # Check ancestors too: an apparently in-tree path can point through a junction.
    for ($parent = $path; $parent.Length -ge $root.Length; $parent = Split-Path -Parent $parent) {
        if ((Test-Path -LiteralPath $parent) -and
            ((Get-Item -LiteralPath $parent -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing a reparse point: $parent"
        }
        if ($parent -ieq $root) { break }
    }
}

function Get-DeploymentFiles {
    param([string]$Directory, [string]$AllowedRoot)
    Assert-SafeChildPath $Directory $AllowedRoot
    $path = [IO.Path]::GetFullPath($Directory).TrimEnd('\')
    $pending = New-Object 'System.Collections.Generic.Stack[string]'
    $pending.Push($path)
    while ($pending.Count) {
        foreach ($item in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Refusing a reparse point: $($item.FullName)"
            }
            if ($item.PSIsContainer) { $pending.Push($item.FullName) }
            else { $item }
        }
    }
}

function Get-ZipInventory {
    param([string]$Archive)
    $result = @{}
    $zip = [IO.Compression.ZipFile]::OpenRead($Archive)
    try {
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.Replace('\', '/')
            if ($name.EndsWith('/')) { continue }
            if ($name -match '(^/|:|(^|/)\.\.(/|$))' -or $result.ContainsKey($name)) {
                throw "Unsafe or duplicate archive entry: $name"
            }
            $stream = $entry.Open()
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $hash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
            finally { $sha.Dispose(); $stream.Dispose() }
            $result[$name] = @{ bytes = $entry.Length; sha256 = $hash }
        }
    } finally { $zip.Dispose() }
    return $result
}

function Assert-CleanReleaseStage {
    param([string]$Stage, [string]$DistRoot)
    $files = @(Get-DeploymentFiles $Stage $DistRoot)
    foreach ($file in $files) {
        $name = $file.FullName.Substring($Stage.TrimEnd('\').Length + 1).Replace('\', '/')
        # User history, arbitrary settings, recordings and caches are never payload.
        $allowed = $name -match '^bin/(SnipNexs\.exe|[^/]+\.dll|qt\.conf)$' -or
            $name -match '^plugins/[^/]+/[^/]+\.dll$' -or
            $name -match '^licenses/[^/]+\.(txt|md|spdx)$' -or
            $name -match '^(LICENSE|THIRD_PARTY_NOTICES\.md|release-files\.json)$' -or
            $name -eq 'docs/local-translation-decision.md' -or
            $name -match '^bin/models/translation/hy-mt2-1\.8b-q4/(Hy-MT2-1\.8B-Q4_K_M\.gguf|manifest\.json)$'
        if ($name -match '^bin/(ctranslate2\.dll|sentencepiece[^/]*\.dll|spm_[^/]*\.exe)$') { $allowed = $false }
        if (-not $allowed) { throw "Runtime or unknown file must not enter the package: $name" }
    }
}

function Test-ReleaseArchive {
    param([string]$Archive, [string]$Stage, [string]$DistRoot)
    Assert-CleanReleaseStage $Stage $DistRoot
    $inventory = Get-ZipInventory $Archive
    $files = @(Get-DeploymentFiles $Stage $DistRoot)
    if ($inventory.Count -ne $files.Count) { throw 'Archive file count differs from stage.' }
    foreach ($file in $files) {
        $name = $file.FullName.Substring($Stage.TrimEnd('\').Length + 1).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if (-not $inventory.ContainsKey($name) -or $inventory[$name].sha256 -ne $hash) {
            throw "Archive differs from stage: $name"
        }
    }
    return $inventory
}

function Remove-OldDeployment {
    param([string]$Stage, [string]$Archive, [string]$DistRoot, [string]$PreserveRoot)
    $stagePath = [IO.Path]::GetFullPath($Stage).TrimEnd('\')
    $files = @(Get-DeploymentFiles $stagePath $DistRoot)
    if (-not (Test-Path -LiteralPath $Archive -PathType Leaf)) {
        throw "Cannot safely retire a deployment without its original ZIP: $stagePath"
    }
    foreach ($process in @(Get-Process -Name SnipNexs -ErrorAction SilentlyContinue)) {
        if (-not $process.Path -or $process.Path.StartsWith($stagePath + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Close the running SnipNexs instance before retiring $stagePath."
        }
    }
    $inventory = Get-ZipInventory $Archive
    $preserved = @()
    $removedBytes = 0L
    $destination = Join-Path $PreserveRoot (Split-Path $stagePath -Leaf)
    Assert-SafeChildPath $destination $PreserveRoot
    # Validate every destination before moving any file or deleting the deployment.
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($stagePath.Length + 1)
        $name = $relative.Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($name -notmatch '^bin/(history|models)/' -and
            $inventory.ContainsKey($name) -and $inventory[$name].sha256 -eq $hash) {
            $removedBytes += $file.Length
            continue
        }
        $target = Join-Path $destination $relative
        if (Test-Path -LiteralPath $target) { throw "Preserved data already exists; refusing to overwrite: $target" }
        $preserved += [pscustomobject]@{ source=$file.FullName; target=$target; relative=$relative; bytes=$file.Length; sha256=$hash }
    }
    if ($preserved.Count) {
        New-Item -ItemType Directory -Force -Path $destination | Out-Null
        $null = @(Get-DeploymentFiles $destination $PreserveRoot)
        foreach ($item in $preserved) {
            New-Item -ItemType Directory -Force -Path (Split-Path $item.target -Parent) | Out-Null
            Move-Item -LiteralPath $item.source -Destination $item.target
        }
    }
    # Recheck remaining content: only unchanged archive files may be deleted.
    foreach ($file in @(Get-DeploymentFiles $stagePath $DistRoot)) {
        $name = $file.FullName.Substring($stagePath.Length + 1).Replace('\', '/')
        if (-not $inventory.ContainsKey($name) -or
            $inventory[$name].sha256 -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()) {
            throw "Deployment changed during retirement; preserved data is at $destination."
        }
    }
    Remove-Item -LiteralPath $stagePath -Recurse -Force
    return [pscustomobject]@{ deployment=$stagePath; preservedDirectory=$destination; preservedFiles=$preserved; removedBytes=$removedBytes }
}

function Restore-PreservedRuntimeData {
    param([string]$PreservedDirectory, [string]$Stage, [string]$DistRoot, [string]$PreserveRoot)
    if (-not (Test-Path -LiteralPath $PreservedDirectory)) { return }
    $null = @(Get-DeploymentFiles $Stage $DistRoot)
    $files = @(Get-DeploymentFiles $PreservedDirectory $PreserveRoot)
    $oldHistory = Join-Path $PreservedDirectory 'bin/history'
    $newHistory = Join-Path $Stage 'bin/history'
    # Never merge two history indexes. Keep any other old history intact in user-data.
    if ((Test-Path -LiteralPath $oldHistory) -and -not (Test-Path -LiteralPath $newHistory)) {
        Move-Item -LiteralPath $oldHistory -Destination $newHistory
    }
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($PreservedDirectory.TrimEnd('\').Length + 1)
        if ($relative -notmatch '^bin\\models\\') { continue }
        # Retired engine data remains recoverable in user-data, never in the new app.
        if ($relative -match '^bin\\models\\translation\\opus-mt-(en-zh|zh-en)-int8\\') { continue }
        $target = Join-Path $Stage $relative
        $originalHash = (Get-FileHash -LiteralPath $file.FullName).Hash
        if (Test-Path -LiteralPath $target) {
            # Deduplicate only when the new deployment has exactly the same bytes.
            if ((Get-FileHash -LiteralPath $target).Hash -eq $originalHash) {
                Remove-Item -LiteralPath $file.FullName -Force
            }
            continue
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Move-Item -LiteralPath $file.FullName -Destination $target
        if ($originalHash -ne (Get-FileHash -LiteralPath $target).Hash) {
            throw "Runtime data migration verification failed: $relative"
        }
    }
}
