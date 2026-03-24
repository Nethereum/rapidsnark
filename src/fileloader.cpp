#ifdef _WIN32
#include <windows.h>
#include <stdexcept>
#include <system_error>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <stdexcept>
#endif

#include "fileloader.hpp"

namespace BinFileUtils {

#ifdef _WIN32

FileLoader::FileLoader()
    : addr(nullptr), size(0), hFile(INVALID_HANDLE_VALUE), hMapping(nullptr)
{
}

FileLoader::FileLoader(const std::string& fileName)
    : addr(nullptr), size(0), hFile(INVALID_HANDLE_VALUE), hMapping(nullptr)
{
    load(fileName);
}

void FileLoader::load(const std::string& fileName)
{
    if (hFile != INVALID_HANDLE_VALUE) {
        throw std::invalid_argument("file already loaded");
    }

    hFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ,
                        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(), std::system_category(), "CreateFile");
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        throw std::system_error(GetLastError(), std::system_category(), "GetFileSizeEx");
    }

    size = static_cast<size_t>(fileSize.QuadPart);

    hMapping = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMapping == nullptr) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        throw std::system_error(GetLastError(), std::system_category(), "CreateFileMapping");
    }

    addr = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (addr == nullptr) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        hMapping = nullptr;
        hFile = INVALID_HANDLE_VALUE;
        throw std::system_error(GetLastError(), std::system_category(), "MapViewOfFile");
    }
}

FileLoader::~FileLoader()
{
    if (addr != nullptr) {
        UnmapViewOfFile(addr);
    }
    if (hMapping != nullptr) {
        CloseHandle(hMapping);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

#else

FileLoader::FileLoader()
    : fd(-1)
{
}

FileLoader::FileLoader(const std::string& fileName)
    : fd(-1)
{
    load(fileName);
}

void FileLoader::load(const std::string& fileName)
{
    if (fd != -1) {
        throw std::invalid_argument("file already loaded");
    }

    struct stat sb;

    fd = open(fileName.c_str(), O_RDONLY);
    if (fd == -1)
        throw std::system_error(errno, std::generic_category(), "open");


    if (fstat(fd, &sb) == -1) {          /* To obtain file size */
        close(fd);
        throw std::system_error(errno, std::generic_category(), "fstat");
    }

    size = sb.st_size;

    addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (addr == MAP_FAILED) {
        close(fd);
        throw std::system_error(errno, std::generic_category(), "mmap failed");
    }

    madvise(addr, size, MADV_SEQUENTIAL);
}

FileLoader::~FileLoader()
{
    if (fd != -1) {
        munmap(addr, size);
        close(fd);
    }
}

#endif

} // Namespace
