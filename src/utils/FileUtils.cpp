#include "utils/FileUtils.hpp"

#include "utils/Constants.hpp"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
    bool statPath(const std::string& path, struct stat& info)
    {
        return stat(path.c_str(), &info) == 0;
    }
}

namespace FileUtils
{
    bool exists(const std::string& path)
    {
        struct stat info;
        return statPath(path, info);
    }

    bool isDirectory(const std::string& path)
    {
        struct stat info;
        return statPath(path, info) && S_ISDIR(info.st_mode);
    }

    bool isRegularFile(const std::string& path)
    {
        struct stat info;
        return statPath(path, info) && S_ISREG(info.st_mode);
    }

    bool isReadable(const std::string& path)
    {
        return access(path.c_str(), R_OK) == 0;
    }

    bool isWritable(const std::string& path)
    {
        return access(path.c_str(), W_OK) == 0;
    }

    bool isExecutable(const std::string& path)
    {
        return access(path.c_str(), X_OK) == 0;
    }

    bool fileSize(const std::string& path, size_t& out)
    {
        struct stat info;
        if (!statPath(path, info) || info.st_size < 0)
            return false;
        out = static_cast<size_t>(info.st_size);
        return true;
    }

    bool readFile(const std::string& path, std::string& out, size_t maxBytes)
    {
        size_t size = 0;
        if (!isRegularFile(path) || !fileSize(path, size) || size > maxBytes)
            return false;
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (!file)
            return false;
        std::string content(size, '\0');
        if (size > 0 && !file.read(&content[0], static_cast<std::streamsize>(size)))
            return false;
        out = content;
        return true;
    }

    bool writeFile(const std::string& path, const std::string& data)
    {
        std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file)
            return false;
        file.write(data.c_str(), static_cast<std::streamsize>(data.size()));
        file.close();
        return !file.fail();
    }

    bool copyFile(const std::string& from, const std::string& to)
    {
        std::ifstream input(from.c_str(), std::ios::in | std::ios::binary);
        if (!input)
            return false;
        std::ofstream output(to.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!output)
            return false;
        std::vector<char> buffer(kIoChunkSize);
        while (input)
        {
            input.read(&buffer[0], static_cast<std::streamsize>(buffer.size()));
            std::streamsize got = input.gcount();
            if (got > 0 && !output.write(&buffer[0], got))
                return false;
        }
        if (!input.eof())
            return false;
        output.close();
        return !output.fail();
    }

    bool listDirectory(const std::string& path, std::vector<std::string>& names)
    {
        DIR* directory = opendir(path.c_str());
        if (directory == NULL)
            return false;
        std::vector<std::string> found;
        struct dirent* entry;
        while ((entry = readdir(directory)) != NULL)
        {
            std::string name(entry->d_name);
            if (name != "." && name != "..")
                found.push_back(name);
        }
        closedir(directory);
        std::sort(found.begin(), found.end());
        names = found;
        return true;
    }
}
