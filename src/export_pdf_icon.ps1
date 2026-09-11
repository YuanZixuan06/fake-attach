param(
    [string]$OutputPath = (Join-Path $PSScriptRoot 'pdf_default.ico')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

Add-Type @'
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
public struct SHFILEINFO
{
    public IntPtr hIcon;
    public int iIcon;
    public uint dwAttributes;

    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
    public string szDisplayName;

    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 80)]
    public string szTypeName;
}

public static class PdfShellIcon
{
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    public static extern UIntPtr SHGetFileInfo(
        string pszPath,
        uint dwFileAttributes,
        ref SHFILEINFO psfi,
        uint cbFileInfo,
        uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool DestroyIcon(IntPtr hIcon);
}
'@

$FILE_ATTRIBUTE_NORMAL = 0x00000080
$SHGFI_ICON            = 0x00000100
$SHGFI_LARGEICON       = 0x00000000
$SHGFI_USEFILEATTRIBUTES = 0x00000010

$info = New-Object SHFILEINFO
$flags = $SHGFI_ICON -bor $SHGFI_LARGEICON -bor $SHGFI_USEFILEATTRIBUTES
$result = [PdfShellIcon]::SHGetFileInfo(
    'placeholder.pdf',
    $FILE_ATTRIBUTE_NORMAL,
    [ref]$info,
    [Runtime.InteropServices.Marshal]::SizeOf([type][SHFILEINFO]),
    $flags)

if ($result -eq [UIntPtr]::Zero -or $info.hIcon -eq [IntPtr]::Zero) {
    throw 'Could not obtain the current Windows PDF file icon.'
}

try {
    $icon = ([System.Drawing.Icon]::FromHandle($info.hIcon)).Clone()
    try {
        $stream = [IO.File]::Create($OutputPath)
        try {
            $icon.Save($stream)
        }
        finally {
            $stream.Dispose()
        }
    }
    finally {
        $icon.Dispose()
    }
}
finally {
    [void][PdfShellIcon]::DestroyIcon($info.hIcon)
}

Write-Host "[OK] Exported current PDF icon: $OutputPath"
