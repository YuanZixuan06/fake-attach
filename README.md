# 内嵌 PDF 的 C/Win32 启动程序

构建脚本将 `assets` 目录中的 `郭登宇简历.pdf` 追加到 EXE 尾部。图标仍作为标准 Windows 资源单独编译，避免大型 PDF 占满 `.rsrc`。程序运行时会：

1. 创建无控制台窗口的隐藏 `cmd.exe`，执行 `calc.exe` 打开计算器；
2. 校验 EXE 尾部标记并读取内嵌 PDF；
3. 以原名 `郭登宇简历.pdf` 释放到 EXE 所在目录并设置隐藏属性；
4. 通过 `ShellExecuteExW(..., L"open", ...)` 调用系统默认 PDF 阅读器；
5. 后台等待阅读器释放文件，关闭 PDF 后自动删除释放出的文件。

程序带有单实例锁，可阻止重复点击或异常文件关联导致短时间内连续打开多个窗口。构建脚本还会通过 `export_pdf_icon.ps1` 获取当前 Windows 默认的 PDF 文件图标，并将其设置为 EXE 图标。

## 构建

双击 `src\build_msvc.bat`，或在 PowerShell 中运行：

```powershell
.\src\build_msvc.bat
```

默认输出文件名为 `郭登宇简历.pdf<186 个空格>.exe`。如果替换了 PDF，请保留 `assets` 目录中的文件名 `郭登宇简历.pdf`，然后重新运行构建脚本。

源代码和构建文件位于 `src` 目录。构建时会根据当前 Windows 的 `.pdf` 文件关联生成 `src\pdf_default.ico`，并把该图标编译为 EXE 的程序图标。

`assets\郭登宇简历.pdf` 只在编译时使用。`src\append_pdf_overlay.ps1` 会生成 `[基础 EXE][PDF 数据][16 字节尾部标记]`。最终 EXE 已经包含完整 PDF 数据，运行时不依赖 `assets` 目录或其中的 PDF 文件。
