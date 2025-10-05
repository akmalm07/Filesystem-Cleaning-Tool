#include "headers.h"
#include "file_metadata.h"


namespace clean
{
    FileMetaData get_file_metadata(const std::string& filePath)
    {
        namespace fs = std::filesystem;

        fs::path pathObj(filePath);
        std::string fileName = pathObj.filename().string();
        std::string fileExtension = pathObj.extension().string();

        uintmax_t fileSize = 0;
        std::string creationTime = "N/A";
        std::string lastAccessTime = "N/A";
        std::string lastWriteTime = "N/A";
        try
        {
            if (fs::exists(pathObj) && fs::is_regular_file(pathObj))
            {
                fileSize = fs::file_size(pathObj);
                fs::file_time_type ftime = fs::last_write_time(pathObj);
                
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );

                std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                lastWriteTime = std::asctime(std::localtime(&cftime));
                lastWriteTime.pop_back(); // Remove trailing newline
#ifdef _WIN32
                // -----------------------
                // Windows implementation
                // -----------------------

                HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    FILETIME ftCreate, ftAccess, ftWrite;
                    if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
                    {
                        SYSTEMTIME stUTC, stLocal;
                        FileTimeToSystemTime(&ftCreate, &stUTC);
                        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
                        char buffer[100];
                        sprintf_s(buffer, "%02d/%02d/%d %02d:%02d:%02d", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
                        creationTime = buffer;
                        FileTimeToSystemTime(&ftAccess, &stUTC);
                        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
                        sprintf_s(buffer, "%02d/%02d/%d %02d:%02d:%02d", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
                        lastAccessTime = buffer;
                    }
                    CloseHandle(hFile);
                }
#else
                // -----------------------
                // MacOS / POSIX implementation
                // -----------------------
                struct stat attr;
                if (stat(filePath.c_str(), &attr) == 0)
                {
                    // Convert time_t -> readable strings
                    char buffer[100];

                    // Creation time (macOS/BSD only; may not exist on Linux)
#if defined(__APPLE__)
                    std::time_t ctime = attr.st_birthtime;
#elif defined(__linux__)
                    // Linux often lacks birthtime; fallback to mtime
                    std::time_t ctime = attr.st_mtime;
#endif
                    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M:%S", std::localtime(&ctime));
                    creationTime = buffer;

                    // Last access time
                    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M:%S", std::localtime(&attr.st_atime));
                    lastAccessTime = buffer;
                }
#endif
            }
        }
        catch (const fs::filesystem_error& e)
        {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "General error: " << e.what() << std::endl;
        }
        return FileMetaData(filePath, fileName, fileExtension, fileSize, creationTime, lastAccessTime, lastWriteTime);
    }
    void print_data(const FileMetaData& meta)
    {
        std::cout << "File Path: " << meta.filePath << std::endl;
        std::cout << "File Name: " << meta.fileName << std::endl;
        std::cout << "File Extension: " << meta.fileExtension << std::endl;
        std::cout << "File Size: " << meta.fileSize << " bytes" << std::endl;
        std::cout << "Creation Time: " << meta.creationTime << std::endl;
        std::cout << "Last Access Time: " << meta.lastAccessTime << std::endl;
		std::cout << "Last Write Time: " << meta.lastWriteTime << std::endl;
    }
}