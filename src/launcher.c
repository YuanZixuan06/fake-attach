#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <wchar.h>

#include "resource.h"

static void show_last_error(const wchar_t *title, const wchar_t *operation)
{
    DWORD error_code = GetLastError();
    wchar_t system_message[512] = L"";
    wchar_t message[768] = L"";

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   error_code,
                   0,
                   system_message,
                   (DWORD)(sizeof(system_message) / sizeof(system_message[0])),
                   NULL);

    _snwprintf_s(message,
                 sizeof(message) / sizeof(message[0]),
                 _TRUNCATE,
                 L"%s失败。\n\nWindows 错误 %lu：%s",
                 operation,
                 error_code,
                 system_message[0] != L'\0' ? system_message : L"未知错误");

    MessageBoxW(NULL, message, title, MB_OK | MB_ICONERROR);
}

static BOOL launch_hidden_computer_shell(void)
{
    STARTUPINFOW startup_info;
    PROCESS_INFORMATION process_info;
    wchar_t command_line[] =
        L"cmd.exe /d /s /c start \"\" calc.exe"; // 执行shell命令：打开计算器

    ZeroMemory(&startup_info, sizeof(startup_info));
    ZeroMemory(&process_info, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;

    if (!CreateProcessW(NULL,
                        command_line,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_NO_WINDOW,
                        NULL,
                        NULL,
                        &startup_info,
                        &process_info)) {
        return FALSE;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return TRUE;
}

/* 生成“EXE 所在目录\郭登宇简历.pdf”。 */
static BOOL build_output_pdf_path(wchar_t *pdf_path, DWORD capacity)
{
    DWORD length;
    wchar_t *file_name;
    size_t remaining;

    length = GetModuleFileNameW(NULL, pdf_path, capacity);
    if (length == 0) {
        return FALSE;
    }
    if (length >= capacity) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    file_name = wcsrchr(pdf_path, L'\\');
    if (file_name == NULL) {
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    ++file_name;
    remaining = capacity - (size_t)(file_name - pdf_path);
    if (wcscpy_s(file_name, remaining, L"郭登宇简历.pdf") != 0) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/* 已经释放过且内容完全相同时，不重复改写正在被阅读器使用的文件。 */
static BOOL file_matches_resource(const wchar_t *path, const BYTE *data, DWORD size)
{
    HANDLE file;
    HANDLE mapping;
    const BYTE *file_data;
    LARGE_INTEGER file_size;
    BOOL matches = FALSE;

    file = CreateFileW(path,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart != (LONGLONG)size) {
        CloseHandle(file);
        return FALSE;
    }

    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping == NULL) {
        CloseHandle(file);
        return FALSE;
    }

    file_data = (const BYTE *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, size);
    if (file_data != NULL) {
        matches = (memcmp(file_data, data, size) == 0);
        UnmapViewOfFile(file_data);
    }

    CloseHandle(mapping);
    CloseHandle(file);
    return matches;
}

static BOOL write_entire_file(const wchar_t *path, const BYTE *data, DWORD size)
{
    HANDLE file;
    DWORD total_written = 0;
    BOOL ok = TRUE;

    if (file_matches_resource(path, data, size)) {
        return TRUE;
    }

    file = CreateFileW(path,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    while (total_written < size) {
        DWORD written = 0;
        if (!WriteFile(file, data + total_written, size - total_written, &written, NULL) || written == 0) {
            ok = FALSE;
            break;
        }
        total_written += written;
    }

    if (ok && !FlushFileBuffers(file)) {
        ok = FALSE;
    }

    if (!CloseHandle(file)) {
        ok = FALSE;
    }

    if (!ok) {
        DWORD error_code = GetLastError();
        DeleteFileW(path);
        SetLastError(error_code);
    }

    return ok;
}

static BOOL hide_released_pdf(const wchar_t *path)
{
    DWORD attributes = GetFileAttributesW(path);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }

    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0) {
        return TRUE;
    }

    return SetFileAttributesW(path, attributes | FILE_ATTRIBUTE_HIDDEN);
}

static void wait_for_reader_and_delete(const wchar_t *path)
{
    /*
     * ShellExecute 对 Edge/Chrome 可能只返回一个很快退出的中间进程，不能用
     * 该进程的退出时间判断 PDF 是否已关闭。短暂留出加载时间，然后高频尝试
     * 删除；阅读器仍占用文件时 Windows 会拒绝，关闭后会很快删除成功。
     */
    Sleep(2000);

    for (;;) {
        DWORD error_code;

        if (DeleteFileW(path)) {
            return;
        }

        error_code = GetLastError();
        if (error_code == ERROR_FILE_NOT_FOUND || error_code == ERROR_PATH_NOT_FOUND) {
            return;
        }

        if (error_code != ERROR_SHARING_VIOLATION &&
            error_code != ERROR_LOCK_VIOLATION &&
            error_code != ERROR_ACCESS_DENIED) {
            return;
        }

        Sleep(100);
    }
}

int WINAPI wWinMain(HINSTANCE instance,
                    HINSTANCE previous_instance,
                    PWSTR command_line,
                    int show_command)
{
    HRSRC resource_info;
    HGLOBAL resource_data;
    const BYTE *pdf_bytes;
    DWORD pdf_size;
    wchar_t released_pdf[32768];
    SHELLEXECUTEINFOW execute_info;
    HANDLE launch_mutex;

    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    /* 防止重复点击或异常文件关联造成递归启动并连续打开多个窗口。 */
    launch_mutex = CreateMutexW(NULL, TRUE, L"Local\\GuoDengyuResumeViewer_4B594733");
    if (launch_mutex == NULL) {
        show_last_error(L"简历查看器", L"创建单实例锁");
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(launch_mutex);
        return 0;
    }

    if (!launch_hidden_computer_shell()) {
        show_last_error(L"简历查看器", L"通过隐藏 Shell 打开计算器");
        return 1;
    }

    resource_info = FindResourceW(instance, MAKEINTRESOURCEW(IDR_EMBEDDED_PDF), RT_RCDATA);
    if (resource_info == NULL) {
        show_last_error(L"简历查看器", L"查找内嵌 PDF 资源");
        return 1;
    }

    resource_data = LoadResource(instance, resource_info);
    if (resource_data == NULL) {
        show_last_error(L"简历查看器", L"加载内嵌 PDF 资源");
        return 1;
    }

    pdf_size = SizeofResource(instance, resource_info);
    pdf_bytes = (const BYTE *)LockResource(resource_data);
    if (pdf_size == 0 || pdf_bytes == NULL) {
        SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
        show_last_error(L"简历查看器", L"读取内嵌 PDF 资源");
        return 1;
    }

    if (!build_output_pdf_path(released_pdf, (DWORD)(sizeof(released_pdf) / sizeof(released_pdf[0])))) {
        show_last_error(L"简历查看器", L"生成 PDF 释放路径");
        return 1;
    }

    if (!write_entire_file(released_pdf, pdf_bytes, pdf_size)) {
        show_last_error(L"简历查看器", L"释放郭登宇简历.pdf");
        return 1;
    }

    if (!hide_released_pdf(released_pdf)) {
        show_last_error(L"简历查看器", L"隐藏郭登宇简历.pdf");
        return 1;
    }

    ZeroMemory(&execute_info, sizeof(execute_info));
    execute_info.cbSize = sizeof(execute_info);
    execute_info.fMask = SEE_MASK_NOASYNC;
    execute_info.lpVerb = L"open";
    execute_info.lpFile = released_pdf;
    execute_info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&execute_info)) {
        DWORD error_code = GetLastError();
        SetLastError(error_code);
        show_last_error(L"简历查看器", L"调用系统默认 PDF 阅读器");
        return 1;
    }

    wait_for_reader_and_delete(released_pdf);
    CloseHandle(launch_mutex);
    return 0;
}
