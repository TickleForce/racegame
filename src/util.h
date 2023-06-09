#pragma once

#include "misc.h"
#include "buffer.h"

#if _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

template <typename T>
void walkDirectory(const char* dir, T const& callback)
{
#if _WIN32
    WIN32_FIND_DATA fileData;
    HANDLE handle = FindFirstFile(tmpStr("%s\\*", dir), &fileData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        FATAL_ERROR("Failed to read directory: ", dir);
    }
    do
    {
        if (fileData.cFileName[0] != '.')
        {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                callback(dir, fileData.cFileName, true);
                const char* childDirectory = tmpStr("%s/%s", dir, fileData.cFileName);
                walkDirectory(childDirectory, callback);
            }
            else
            {
                callback(dir, fileData.cFileName, false);
            }
        }
    }
    while (FindNextFileA(handle, &fileData) != 0);
    FindClose(handle);
#else
    DIR* dirp = opendir(dir);
    if (!dirp)
    {
        FATAL_ERROR("Failed to read directory: %s", dir);
    }
    dirent* dp;
    while ((dp = readdir(dirp)))
    {
        if (dp->d_name[0] != '.')
        {
            if (dp->d_type == DT_DIR)
            {
                callback(dir, dp->d_name, true);
				const char* childDirectory = tmpStr("%s/%s", dir, dp->d_name);
				walkDirectory(childDirectory, callback);
            }
            else if (dp->d_type == DT_REG)
            {
                callback(dir, dp->d_name, false);
            }
        }
    }
    closedir(dirp);
#endif
}

struct FileItem
{
    const char* path;
    bool isDirectory;
    Array<FileItem> children;
};

Array<FileItem> readDirectory(const char* dir, bool recursive=true)
{
    Array<FileItem> files;

#if _WIN32
    WIN32_FIND_DATA fileData;
    HANDLE handle = FindFirstFile(tmpStr("%s\\*", dir), &fileData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        FATAL_ERROR("Failed to read directory: ", dir);
    }
    do
    {
        if (fileData.cFileName[0] != '.')
        {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                files.push({ tmpStr("%s", fileData.cFileName), true });
                const char* childDirectory = tmpStr("%s/%s", dir, fileData.cFileName);
                if (recursive)
                {
                    files.back().children = readDirectory(childDirectory, recursive);
                }
            }
            else
            {
                files.push({ tmpStr("%s", fileData.cFileName), false });
            }
        }
    }
    while (FindNextFileA(handle, &fileData) != 0);
    FindClose(handle);
#else
    DIR* dirp = opendir(dir);
    if (!dirp)
    {
        FATAL_ERROR("Failed to read directory: %s", dir);
    }
    dirent* dp;
    while ((dp = readdir(dirp)))
    {
        if (dp->d_name[0] != '.')
        {
            const char* name = tmpStr("%s", dp->d_name);
            if (dp->d_type == DT_DIR)
            {
				files.push({ name, true });
				const char* childDirectory = tmpStr("%s/%s", dir, dp->d_name);
				if (recursive)
				{
					files.back().children = readDirectory(childDirectory, recursive);
				}
            }
            else if (dp->d_type == DT_REG)
            {
                files.push({ name, false });
            }
        }
    }
    closedir(dirp);
#endif

    files.sort([](auto& a, auto& b) {
        if (a.isDirectory && !b.isDirectory) return true;
        if (!a.isDirectory && b.isDirectory) return false;
        return strcmp(a.path, b.path) < 0;
    });

    return files;
}

void createDirectory(const char* dir)
{
#if _WIN32
    LPSECURITY_ATTRIBUTES attr;
    attr = nullptr;
    auto result = CreateDirectoryA(dir, attr);
    if (result == 0)
    {
        //error("Failed to create directory: %s", dir);
    }
#else
    auto result = mkdir(dir, 0700);
    if (result != 0)
    {
        //error("Failed to create directory: %s: %s", dir, strerror(errno));
    }
#endif
}

void renameFile(const char* oldFilename, const char* newFilename)
{
#if _WIN32
    auto result = MoveFile(oldFilename, newFilename);
    if (result == 0)
    {
        error("Failed to rename file: %s -> %s", oldFilename, newFilename);
    }
#else
    auto result = rename(oldFilename, newFilename);
    if (result != 0)
    {
        error("Failed to rename file: %s -> %s: %s", oldFilename, newFilename,
                strerror(errno));
    }
#endif
}

void deleteDirectory(const char* path)
{
#if _WIN32
    auto result = RemoveDirectoryA(path);
    if (result == 0)
    {
        error("Failed to delete directory: %s", path);
    }
#else
    auto result = remove(path);
    if (result != 0)
    {
        error("Failed to delete directory: %s: %s", path, strerror(errno));
    }
#endif
}

void deleteFile(const char* path)
{
#if _WIN32
    auto result = DeleteFileA(path);
    if (result == 0)
    {
        error("Failed to delete file: %s", path);
    }
#else
    auto result = remove(path);
    if (result != 0)
    {
        error("Failed to delete file: %s: %s", path, strerror(errno));
    }
#endif
}

const char* chooseFile(bool open, const char* fileType,
        SmallArray<const char*> extensions, const char* defaultDir)
{
#if _WIN32
    char szFile[260];

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    // TODO: Implement file filter
    ofn.lpstrFilter = "All\0*.*\0Tracks\0*.dat\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    // TODO: set this
    ofn.lpstrInitialDir = NULL;
    if (open)
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    }

    if (open)
    {
        if (GetOpenFileName(&ofn) == TRUE)
        {
            return tmpStr("%s", szFile);
        }
        else
        {
            return {};
        }
    }
    else
    {
        if (GetSaveFileName(&ofn) == TRUE)
        {
            return tmpStr("%s", szFile);
        }
        else
        {
            return {};
        }
    }
#else
    StrBuf buf;
    buf.writef("zenity --file-filter \"%s |", fileType);
    for (auto& ext : extensions)
    {
        buf.write(" ");
        buf.write(ext);
    }
    buf.write("\"");
    if (open)
    {
        buf.write(" --title 'Open File' --file-selection");
    }
    else
    {
        buf.write(" --title 'Save File' --file-selection --save --confirm-overwrite");
    }
    if (defaultDir)
    {
        buf.write(" --filename ");
        buf.write(defaultDir);
    }
    println("%s", buf.data());
    FILE *f = popen(buf.data(), "r");
    char* filename = g_tmpMem.bump<char>(1024);
    memset(filename, 0, 1024);
    if (!f || !fgets(filename, 1024 - 1, f))
    {
        error("Unable to create file dialog");
        return {};
    }
    pclose(f);
    if (filename[0] != 0)
    {
        // strip newline
        auto len = strlen(filename)-1;
        if (filename[len] == '\n')
        {
            filename[len] = '\0';
        }
        println("File chosen: %s", filename);
        return filename;
    }
    return nullptr;
#endif
}

struct CommandResult
{
    i32 exitCode;
    const char* output;
};

CommandResult runShellCommand(const char* command)
{
    println("%s", command);

#if _WIN32
    SECURITY_ATTRIBUTES attr;
    attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    attr.bInheritHandle = TRUE;
    attr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdoutRead, hChildStdoutWrite;
    if (!CreatePipe(&hChildStdoutRead, &hChildStdoutWrite, &attr, 0))
    {
        return { -1000 };
    }

    HANDLE hChildStdinRead, hChildStdinWrite;
    if (!CreatePipe(&hChildStdinRead, &hChildStdinWrite, &attr, 0))
    {
        return { -1002 };
    }

    PROCESS_INFORMATION procInfo = {};

    STARTUPINFO startInfo = {};
    startInfo.cb = sizeof(startInfo);
    startInfo.hStdError = hChildStdoutWrite;
    startInfo.hStdOutput = hChildStdoutWrite;
    startInfo.hStdInput = hChildStdinRead;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, (LPSTR)command, NULL, NULL, TRUE, 0,
                NULL, NULL, &startInfo, &procInfo))
    {
        return { -1004 };
    }

    char buf[4096];
    DWORD read = 0;
    ReadFile(hChildStdoutRead, buf, sizeof(buf), &read, NULL);

    DWORD exitCode;
    GetExitCodeProcess(procInfo.hProcess, &exitCode);
    CommandResult result;
    result.exitCode = exitCode;
    result.output = buf;

    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);
    CloseHandle(hChildStdoutRead);
    CloseHandle(hChildStdoutWrite);
    CloseHandle(hChildStdinRead);
    CloseHandle(hChildStdinWrite);

    return result;
#else
    FILE* stream = popen(tmpStr("%s 2>&1", command), "r");
    if (!stream)
    {
        return { -1, "" };
    }

    char* commandOutput = g_tmpMem.get<char*>();
    char* writePos = commandOutput;
    u32 totalSize = 0;
    while (fgets(writePos, 1024, stream))
    {
        totalSize += 1024;
        writePos = commandOutput + totalSize;
    }
    i32 code = pclose(stream);

    return { code, commandOutput };
#endif
}

Buffer readFileBytes(const char* filename)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "r+b");
    if (!file)
    {
        FATAL_ERROR("File %s does not exist.", filename);
    }
    size_t size = SDL_RWsize(file);
    Buffer buffer(size);
    SDL_RWread(file, buffer.data.get(), size, 1);
    SDL_RWclose(file);
    return buffer;
}

StrBuf readFileString(const char* filename)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "r+b");
    if (!file)
    {
        FATAL_ERROR("File %s does not exist.", filename);
    }
    u32 size = (u32)SDL_RWsize(file);
    StrBuf buffer(size);
    SDL_RWread(file, buffer.data(), size, 1);
    SDL_RWclose(file);
    return buffer;
}

bool fileExists(const char* filename)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "r+b");
    if (!file)
    {
        return false;
    }
    SDL_RWclose(file);
    return true;
}

void writeFile(const char* filename, void* data, size_t len)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "w+b");
    if (!file)
    {
        FATAL_ERROR("Failed to open file for writing: %s", filename);
    }
    if (SDL_RWwrite(file, data, 1, len) != len)
    {
        FATAL_ERROR("Failed to complete file write: %s", filename);
    }
    SDL_RWclose(file);
}
