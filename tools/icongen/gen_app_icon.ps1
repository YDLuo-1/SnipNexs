# Generates resources/app.ico from the same geometry as src/app/AppIcon.cpp.
# Windows-only (System.Drawing); run from any directory:
#   pwsh tools/icongen/gen_app_icon.ps1
param(
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\..\resources\app.ico')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$sizes = 16, 24, 32, 48, 64, 128, 256
$supersample = 4
$pngs = @()

function New-RoundedPath {
    param([float]$X, [float]$Y, [float]$Width, [float]$Height, [float]$Radius)
    $d = [Math]::Min($Radius * 2.0, [Math]::Min($Width, $Height))
    $path = New-Object Drawing.Drawing2D.GraphicsPath
    $path.AddArc($X, $Y, $d, $d, 180, 90)
    $path.AddArc($X + $Width - $d, $Y, $d, $d, 270, 90)
    $path.AddArc($X + $Width - $d, $Y + $Height - $d, $d, $d, 0, 90)
    $path.AddArc($X, $Y + $Height - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    return $path
}

foreach ($size in $sizes) {
    $canvasSize = $size * $supersample
    $canvas = New-Object Drawing.Bitmap $canvasSize, $canvasSize, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($canvas)
    $graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([Drawing.Color]::Transparent)

    $margin = 1.0 * $supersample
    $background = New-RoundedPath -X $margin -Y $margin `
        -Width ($canvasSize - 2 * $margin) -Height ($canvasSize - 2 * $margin) `
        -Radius ($size * 0.22 * $supersample)
    $backgroundBrush = New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(255, 13, 25, 33))
    $graphics.FillPath($backgroundBrush, $background)

    $inset = $size * 0.20 * $supersample
    $arm = $size * 0.16 * $supersample
    $far = $canvasSize - $inset
    $cropPen = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(255, 55, 214, 192)), ([Math]::Max(1.0, $size * 0.055) * $supersample)
    $cropPen.StartCap = [Drawing.Drawing2D.LineCap]::Round
    $cropPen.EndCap = [Drawing.Drawing2D.LineCap]::Round
    $cropPen.LineJoin = [Drawing.Drawing2D.LineJoin]::Round
    $graphics.DrawLine($cropPen, $inset, $inset + $arm, $inset, $inset)
    $graphics.DrawLine($cropPen, $inset, $inset, $inset + $arm, $inset)
    $graphics.DrawLine($cropPen, $far - $arm, $inset, $far, $inset)
    $graphics.DrawLine($cropPen, $far, $inset, $far, $inset + $arm)
    $graphics.DrawLine($cropPen, $inset, $far - $arm, $inset, $far)
    $graphics.DrawLine($cropPen, $inset, $far, $inset + $arm, $far)
    $graphics.DrawLine($cropPen, $far - $arm, $far, $far, $far)
    $graphics.DrawLine($cropPen, $far, $far, $far, $far - $arm)

    $letterPen = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(255, 245, 249, 252)), ([Math]::Max(1.4, $size * 0.075) * $supersample)
    $letterPen.StartCap = [Drawing.Drawing2D.LineCap]::Round
    $letterPen.EndCap = [Drawing.Drawing2D.LineCap]::Round
    $letterPen.LineJoin = [Drawing.Drawing2D.LineJoin]::Round
    $letter = [Drawing.PointF[]]@(
        (New-Object Drawing.PointF ($canvasSize * 0.36), ($canvasSize * 0.67)),
        (New-Object Drawing.PointF ($canvasSize * 0.36), ($canvasSize * 0.34)),
        (New-Object Drawing.PointF ($canvasSize * 0.64), ($canvasSize * 0.67)),
        (New-Object Drawing.PointF ($canvasSize * 0.64), ($canvasSize * 0.34))
    )
    $graphics.DrawLines($letterPen, $letter)

    $graphics.Dispose()
    $backgroundBrush.Dispose()
    $background.Dispose()
    $cropPen.Dispose()
    $letterPen.Dispose()

    $final = New-Object Drawing.Bitmap $size, $size, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $finalGraphics = [Drawing.Graphics]::FromImage($final)
    $finalGraphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $finalGraphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $finalGraphics.DrawImage($canvas, 0, 0, $size, $size)
    $finalGraphics.Dispose()
    $canvas.Dispose()

    $stream = New-Object IO.MemoryStream
    $final.Save($stream, [Drawing.Imaging.ImageFormat]::Png)
    $pngs += ,$stream.ToArray()
    $stream.Dispose()
    $final.Dispose()
}

$headerSize = 6 + 16 * $pngs.Count
$totalSize = $headerSize
foreach ($png in $pngs) {
    $totalSize += $png.Length
}
$bytes = New-Object byte[] $totalSize
[BitConverter]::GetBytes([UInt16]0).CopyTo($bytes, 0)
[BitConverter]::GetBytes([UInt16]1).CopyTo($bytes, 2)
[BitConverter]::GetBytes([UInt16]$pngs.Count).CopyTo($bytes, 4)

$offset = $headerSize
for ($index = 0; $index -lt $pngs.Count; $index++) {
    $size = $sizes[$index]
    $entry = 6 + 16 * $index
    $bytes[$entry] = if ($size -ge 256) { 0 } else { $size }
    $bytes[$entry + 1] = if ($size -ge 256) { 0 } else { $size }
    $bytes[$entry + 2] = 0
    $bytes[$entry + 3] = 0
    [BitConverter]::GetBytes([UInt16]1).CopyTo($bytes, $entry + 4)
    [BitConverter]::GetBytes([UInt16]32).CopyTo($bytes, $entry + 6)
    [BitConverter]::GetBytes([UInt32]$pngs[$index].Length).CopyTo($bytes, $entry + 8)
    [BitConverter]::GetBytes([UInt32]$offset).CopyTo($bytes, $entry + 12)
    $pngs[$index].CopyTo($bytes, $offset)
    $offset += $pngs[$index].Length
}

$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
$outputDir = Split-Path -Parent $outputFullPath
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
[IO.File]::WriteAllBytes($outputFullPath, $bytes)
Write-Host "Generated $outputFullPath ($($bytes.Length) bytes)"
