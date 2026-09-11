$ErrorActionPreference = 'Stop'

$stubPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\resume_viewer.stub.exe'))
$outputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\resume_viewer.exe'))
$assetsPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\assets'))
$pdfFiles = @(Get-ChildItem -LiteralPath $assetsPath -Filter '*.pdf' -File)

if ($pdfFiles.Count -ne 1) {
    throw "Expected exactly one PDF in assets, found $($pdfFiles.Count)."
}

$pdfPath = $pdfFiles[0].FullName
$magic = [Text.Encoding]::ASCII.GetBytes('FPDFv001')
$bufferSize = 1024 * 1024

$stubStream = $null
$pdfStream = $null
$outputStream = $null

try {
    $stubStream = [IO.File]::Open($stubPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    $pdfStream = [IO.File]::Open($pdfPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    $outputStream = [IO.File]::Open($outputPath, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::None)

    $stubStream.CopyTo($outputStream, $bufferSize)
    $pdfStream.CopyTo($outputStream, $bufferSize)
    $outputStream.Write($magic, 0, $magic.Length)

    $sizeBytes = [BitConverter]::GetBytes([UInt64]$pdfStream.Length)
    $outputStream.Write($sizeBytes, 0, $sizeBytes.Length)
    $outputStream.Flush($true)
}
finally {
    if ($outputStream -ne $null) { $outputStream.Dispose() }
    if ($pdfStream -ne $null) { $pdfStream.Dispose() }
    if ($stubStream -ne $null) { $stubStream.Dispose() }
}

Write-Host "[OK] Appended PDF overlay: $outputPath"
