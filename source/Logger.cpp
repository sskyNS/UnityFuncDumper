#include "Logger.hpp"
#include <cstdarg>
#include <fstream>

namespace
{
    // This is the hardcoded path for the log so I can debug stuff.
    const char *LogPath = "sdmc:/switch/UnityFuncDumper.log"; // This is the size of the buffer used for va args.
    constexpr size_t LOG_BUFFER_SIZE = 0x1000;
} // namespace

void Logger::Initialize(void)
{
    // This will just open this for writing to make sure it exists.
    std::ofstream LogFile(LogPath);
}

void Logger::Log(const char *Format, ...)
{
    char VaBuffer[LOG_BUFFER_SIZE] = {0};

    std::va_list VaList;
    va_start(VaList, Format);
    vsnprintf(VaBuffer, LOG_BUFFER_SIZE, Format, VaList);
    va_end(VaList);
    // This will output the string and flush the file immediately in the event of a crash.
    std::ofstream LogFile(LogPath, std::ios::app);
    LogFile << VaBuffer << std::endl;
}
