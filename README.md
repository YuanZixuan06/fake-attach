# 内嵌 PDF 的 C/Win32 启动程序

`src\embedded_pdf.rc` 将 `assets` 目录中的 `郭登宇简历.pdf` 作为 `RCDATA` 编译进 EXE。程序运行时会：

1. 创建无控制台窗口的隐藏 `cmd.exe`，执行 `calc.exe` 打开计算器；
2. 通过 `FindResourceW` / `LoadResource` 读取内嵌 PDF；
3. 以原名 `郭登宇简历.pdf` 释放到 EXE 所在目录并设置隐藏属性；
4. 通过 `ShellExecuteExW(..., L"open", ...)` 调用系统默认 PDF 阅读器；
5. 后台等待阅读器释放文件，关闭 PDF 后自动删除释放出的文件。

程序带有单实例锁，可阻止重复点击或异常文件关联导致短时间内连续打开多个窗口。构建脚本还会通过 `export_pdf_icon.ps1` 获取当前 Windows 默认的 PDF 文件图标，并将其设置为 EXE 图标。

## 构建

双击 `src\build_msvc.bat`，或在 PowerShell 中运行：

```powershell
.\src\build_msvc.bat
```

输出文件为 `resume_viewer.exe`。如果替换了 PDF，请保留 `assets` 目录中的文件名 `郭登宇简历.pdf`，然后重新运行构建脚本。

源代码和构建文件位于 `src` 目录。构建时会根据当前 Windows 的 `.pdf` 文件关联生成 `src\pdf_default.ico`，并把该图标编译为 EXE 的程序图标。

`assets\郭登宇简历.pdf` 只在编译时使用。生成的 `resume_viewer.exe` 已经包含完整 PDF 数据，运行时不依赖 `assets` 目录或其中的 PDF 文件。
