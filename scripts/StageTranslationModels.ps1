[CmdletBinding()]
param([Parameter(Mandatory)][string]$Source, [Parameter(Mandatory)][string]$Stage)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ReleaseFiles.ps1')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$Stage = [IO.Path]::GetFullPath($Stage)
$Source = [IO.Path]::GetFullPath($Source)
$null = @(Get-DeploymentFiles $Stage (Join-Path $repository 'dist'))
$catalog = Get-Content -LiteralPath (Join-Path $repository 'src/translate/TranslationModels.cpp') -Raw -Encoding UTF8
foreach ($factory in @('createHyMt2Model')) {
    $body = [regex]::Match($catalog, "(?s)TranslationModelSpec $factory\(\)\s*\{(.*?)return spec;").Groups[1].Value
    $fields = @{}
    foreach ($field in @('id', 'sourceLanguage', 'targetLanguage', 'licenseNote')) {
        $fields[$field] = [regex]::Match($body, "spec\.$field = QStringLiteral\(\s*`"([^`"]+)`"\)").Groups[1].Value
        if (-not $fields[$field]) { throw "Model catalog field missing: $factory $field" }
    }
    $files = [regex]::Matches($body, '\{\s*QStringLiteral\("([^"]+)"\),\s*QStringLiteral\("([^"]+)"\),\s*QByteArrayLiteral\("([0-9a-f]{64})"\),\s*([0-9]+)\s*\}')
    if (-not $files.Count) { throw "Empty model catalog: $factory" }
    $destination = Join-Path $Stage ('bin/models/translation/' + $fields.id)
    Assert-SafeChildPath $destination $Stage
    $manifestFiles = @()
    foreach ($file in $files) {
        $name = $file.Groups[1].Value
        $digest = $file.Groups[3].Value
        $size = [long]$file.Groups[4].Value
        $inputPath = Join-Path $Source ($fields.id + '/' + $name)
        if ($factory -eq 'createHyMt2Model' -and -not (Test-Path -LiteralPath $inputPath)) {
            $inputPath = Join-Path $Source $name
        }
        if ((Get-Item -LiteralPath $inputPath).Length -ne $size -or
            (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $digest) {
            throw "Model source failed size/SHA-256 validation: $inputPath"
        }
        New-Item -ItemType Directory -Force -Path $destination | Out-Null
        $outputPath = Join-Path $destination $name
        if (Test-Path -LiteralPath $outputPath) { throw "Refusing to overwrite an existing model: $outputPath" }
        Copy-Item -LiteralPath $inputPath -Destination $outputPath
        if ((Get-FileHash -LiteralPath $outputPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $digest) {
            throw "Staged model hash mismatch: $outputPath"
        }
        $manifestFiles += @{name=$name; sha256=$digest}
    }
    @{format=1; id=$fields.id; sourceLanguage=$fields.sourceLanguage; targetLanguage=$fields.targetLanguage;
        license=$fields.licenseNote; files=$manifestFiles} | ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath (Join-Path $destination 'manifest.json') -Encoding UTF8
}
Write-Host 'Staged the Hy-MT2 translation package with catalog SHA-256 verification.'
