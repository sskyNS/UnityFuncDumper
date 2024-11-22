// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>

// Include the main libnx system header, for Switch development
#include "Console.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "asmInterpreter.hpp"
#include <switch.h>
#include <sys/stat.h>

// Declare global variable for heap size
uint64_t existingHeapSize = 64 * 1024 * 1024; // Start with 128MB

// Function to decide if memory should be expanded
bool shouldExpandMemory() {
    // Implement logic, for now, always returning true for demonstration
    return true;
}

// Function to check and potentially expand memory
void CheckAndExpandMemory() {
    void* newHeapAddr = nullptr;
    u64 additionalSize = 128 * 1024 * 1024; // Increase by 199MB

    if (shouldExpandMemory()) { // Use the correct condition check here
        Result result = svcSetHeapSize(&newHeapAddr, existingHeapSize + additionalSize);
        if (R_SUCCEEDED(result)) {
            existingHeapSize += additionalSize; // Update existing heap size
            // Display success message with expanded heap size
            Console::Printf("内存拓展成功！当前内存堆大小: %llu MB\n", existingHeapSize / (1024 * 1024));
        } else {
            // Error handling logic
            Console::Printf("无法拓展可用内存堆! Code: 0x%x\n", result);
        }
    }
}


DmntCheatProcessMetadata cheatMetadata = {0};
u64 mappings_count = 0;
MemoryInfo *memoryInfoBuffers = 0;

bool isServiceRunning(const char *serviceName)
{
    Handle handle;
    SmServiceName service_name = smEncodeName(serviceName);
    if (R_FAILED(smRegisterService(&handle, service_name, false, 1)))
        return true;
    else
    {
        svcCloseHandle(handle);
        smUnregisterService(service_name);
        return false;
    }
}

template <typename T>
T searchString(char *buffer, T string, u64 buffer_size, bool null_terminated = false, bool whole = false)
{
    char *buffer_end = &buffer[buffer_size];
    size_t string_len = (std::char_traits<std::remove_pointer_t<std::remove_reference_t<T>>>::length(string) + (null_terminated ? 1 : 0)) *
                        sizeof(std::remove_pointer_t<std::remove_reference_t<T>>);
    T string_end = &string[std::char_traits<std::remove_pointer_t<std::remove_reference_t<T>>>::length(string) + (null_terminated ? 1 : 0)];
    char *result = std::search(buffer, buffer_end, (char *)string, (char *)string_end);
    if (whole)
    {
        while ((uint64_t)result != (uint64_t)&buffer[buffer_size])
        {
            if (!result[-1 * sizeof(std::remove_pointer_t<std::remove_reference_t<T>>)])
                return (T)result;
            result = std::search(result + string_len, buffer_end, (char *)string, (char *)string_end);
        }
    }
    else if ((uint64_t)result != (uint64_t)&buffer[buffer_size])
    {
        return (T)result;
    }
    return nullptr;
}

std::string unity_sdk = "";

size_t checkAvailableHeap()
{
    size_t startSize = 200 * 1024 * 1024;
    void *allocation = malloc(startSize);
    while (allocation)
    {
        free(allocation);
        startSize += 1024 * 1024;
        allocation = malloc(startSize);
    }
    return startSize - (1024 * 1024);
}

bool checkIfUnity()
{
    size_t i = 0;
    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx &&
            memoryInfoBuffers[i].type == MemType_CodeStatic)
        {
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                continue;
            }
            char test_4[] = "SDK MW+UnityTechnologies+Unity";
            char *buffer_c = new char[memoryInfoBuffers[i].size];
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer_c, memoryInfoBuffers[i].size);
            char *result = searchString(buffer_c, (char *)test_4, memoryInfoBuffers[i].size);
            if (result)
            {
                Console::Printf("^%s^\n", result);
                unity_sdk = result;
                delete[] buffer_c;
                return true;
            }
            delete[] buffer_c;
        }
        i++;
    }
    Console::Printf(">这个游戏没有使用Unity引擎!>\n");
    return false;
}

char *findStringInBuffer(char *buffer_c, size_t buffer_size, const char *description)
{
    char *result = 0;
    result = (char *)searchString(buffer_c, (char *)description, buffer_size);
    return result;
}

std::vector<std::string> UnityNames;
std::vector<uint32_t> UnityOffsets;

void searchFunctionsUnity2()
{
    size_t i = 0;
    const char first_entry[] = "UnityEngineInternal.GIDebugVisualisation::ResetRuntimeInputTextures";
    const char second_entry[] = "UnityEngineInternal.GIDebugVisualisation::PlayCycleMode";
    const char *first_result = 0;
    const char *second_result = 0;
    Console::Printf("基地址: 0x%lx\n", cheatMetadata.main_nso_extents.base);
    Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);

    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx &&
            (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly))
        {
            Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                i++;
                continue;
            }
            char *buffer_c = new char[memoryInfoBuffers[i].size];
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer_c, memoryInfoBuffers[i].size);
            char *result = 0;
            result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, first_entry);
            if (result)
            {
                first_result = (char *)(memoryInfoBuffers[i].addr + (result - buffer_c));
                Console::Printf("在地址 0x%lx 发现第一个引用字符串\n", (uint64_t)first_result);
                result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, second_entry);
                if (result)
                {
                    second_result = (char *)(memoryInfoBuffers[i].addr + (result - buffer_c));
                    Console::Printf("在地址 0x%lx 发现第二个引用字符串\n", (uint64_t)second_result);
                    delete[] buffer_c;
                    break;
                }
            }
            delete[] buffer_c;
        }
        i++;
    }
    if (!first_result || !second_result)
    {
        Console::Printf("未找到引用字符串! 正在中止...\n");
        return;
    }
    Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx &&
            (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly))
        {
            Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                i++;
                continue;
            }
            int32_t *buffer = new int32_t[memoryInfoBuffers[i].size / sizeof(int32_t)];
            Console::Printf("缓冲区: 0x%lx, 大小: 0x%lx\n", (uint64_t)buffer, memoryInfoBuffers[i].size);
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer, memoryInfoBuffers[i].size);
            int32_t *result = 0;
            for (size_t x = 0; x + 1 < memoryInfoBuffers[i].size / sizeof(uint32_t); x++)
            {
                int32_t diff1 = (int64_t)first_result - (memoryInfoBuffers[i].addr + (x * sizeof(uint32_t)));
                int32_t diff2 = (int64_t)second_result - (memoryInfoBuffers[i].addr + (x * sizeof(uint32_t)));
                if (buffer[x] == diff1 && buffer[x + 1] == diff2)
                {
                    result = &buffer[x];
                    break;
                }
            }
            if (!result)
            {
                delete[] buffer;
                i++;
                continue;
            }
            Console::Printf("在缓冲区地址 0x%lx 发现字符串数组\n", (uint64_t)result);
            size_t x = 0;
            while (true)
            {
                int32_t offset = result[x];
                char *address = (char *)((uint64_t)result + offset);
                if (((uint64_t)address > (uint64_t)buffer + memoryInfoBuffers[i].size) || ((uint64_t)address < (uint64_t)buffer))
                {
                    break;
                }
                if (!strncmp(address, "Unity", 5))
                {
                    std::string name = address;
                    UnityNames.push_back(name);
                    x++;
                    Console::Printf("#%ld: %s\n", x, name.c_str());
                }
                else
                    break;
            }
            delete[] buffer;
            break;
        }
        i++;
    }
    Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
    MemoryInfo main = {0};
    dmntchtQueryCheatProcessMemory(&main, cheatMetadata.main_nso_extents.base);
    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw &&
            (memoryInfoBuffers[i].type == MemType_CodeMutable || memoryInfoBuffers[i].type == MemType_CodeWritable))
        {
            Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                i++;
                continue;
            }
            uint64_t *buffer = new uint64_t[memoryInfoBuffers[i].size / sizeof(uint64_t)];
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer, memoryInfoBuffers[i].size);
            uint16_t count = 0;
            size_t start_index = 0;
            for (size_t x = 0; x < (memoryInfoBuffers[i].size / sizeof(uint64_t)); x++)
            {
                if (buffer[x] == 0 || (buffer[x] < main.addr) || (buffer[x] > (main.addr + main.size)))
                {
                    if (count == UnityNames.size())
                    {
                        start_index = x - count;
                        break;
                    }
                    count = 0;
                    continue;
                }
                count++;
            }
            if (count != UnityNames.size())
            {
                delete[] buffer;
                i++;
                continue;
            }
            for (size_t x = 0; x < count; x++)
            {
                UnityOffsets.push_back(buffer[start_index + x] - cheatMetadata.main_nso_extents.base);
            }
            delete[] buffer;
            Console::Printf("发现 %ld 个 Unity 函数.\n", UnityNames.size());
            return;
        }
        i++;
    }
    return;
}

void searchFunctionsUnity()
{
    size_t i = 0;
    const char first_entry[] = "UnityEngineInternal.GIDebugVisualisation::ResetRuntimeInputTextures";
    Console::Printf("基地址: 0x%lx\n", cheatMetadata.main_nso_extents.base);
    Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
    char *found_string = 0;
    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_R) == Perm_R && (memoryInfoBuffers[i].perm & Perm_Rx) != Perm_Rx &&
            (memoryInfoBuffers[i].type == MemType_CodeStatic || memoryInfoBuffers[i].type == MemType_CodeReadOnly))
        {
            Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                i++;
                continue;
            }
            char *buffer_c = new char[memoryInfoBuffers[i].size];
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer_c, memoryInfoBuffers[i].size);
            char *result = 0;
            result = findStringInBuffer(buffer_c, memoryInfoBuffers[i].size, first_entry);
            if (result)
            {
                found_string = (char *)(memoryInfoBuffers[i].addr + (result - buffer_c));
                Console::Printf("在偏移量 0x%lx 发现参考字符串\n", (uint64_t)found_string);
                delete[] buffer_c;
                break;
            }
            delete[] buffer_c;
        }
        i++;
    }
    if (!found_string)
    {
        Console::Printf("未找到参考字符串。\n");
        return;
    }
    i = 0;
    uint64_t *array_start = 0;
    Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
    while (i < mappings_count)
    {
        if ((memoryInfoBuffers[i].perm & Perm_Rw) == Perm_Rw &&
            (memoryInfoBuffers[i].type == MemType_CodeMutable || memoryInfoBuffers[i].type == MemType_CodeWritable))
        {
            Console::Printf("映射 %ld / %ld\r", i + 1, mappings_count);
            if (memoryInfoBuffers[i].size > 200'000'000)
            {
                i++;
                continue;
            }
            uint64_t *buffer = new uint64_t[memoryInfoBuffers[i].size / sizeof(uint64_t)];
            dmntchtReadCheatProcessMemory(memoryInfoBuffers[i].addr, (void *)buffer, memoryInfoBuffers[i].size);
            uint64_t *result = 0;
            for (size_t x = 0; x < memoryInfoBuffers[i].size / sizeof(uint64_t); x++)
            {
                if (buffer[x] == (uint64_t)found_string)
                {
                    result = (uint64_t *)(memoryInfoBuffers[i].addr + (sizeof(uint64_t) * x));
                    break;
                }
            }
            delete[] buffer;
            if (result)
            {
                array_start = result;
                Console::Printf("在偏移量 0x%lx 发现字符串数组\n", (uint64_t)array_start);
                break;
            }
        }
        i++;
    }
    if (!array_start)
    {
        Console::Printf("未找到数组字符串。启动第二种方法...\n");
        return searchFunctionsUnity2();
    }
    i = 0;
    while (true)
    {
        uint64_t address = (u64)array_start + (i * 8);
        uint64_t string_address = 0;
        dmntchtReadCheatProcessMemory(address, (void *)&string_address, sizeof(uint64_t));
        i++;
        char UnityCheck[6] = "";
        dmntchtReadCheatProcessMemory(string_address, (void *)UnityCheck, 5);
        if (!strncmp(UnityCheck, "Unity", 5))
        {
            char buffer[1024] = "";
            dmntchtReadCheatProcessMemory(string_address, (void *)buffer, 1024);
            std::string string = buffer;
            if (string.length() > 0)
            {
                UnityNames.push_back(string);
                Console::Printf("*#*^%ld^: %s\n", UnityNames.size(), UnityNames.back().c_str());
            }
        }
        else
            break;
    }
    Console::Printf("发现 *%ld* 个 Unity 函数。\n", UnityNames.size());
    uint64_t second_array = (u64)array_start + ((i - 1) * 8);
    for (size_t x = 0; x < UnityNames.size(); x++)
    {
        uint64_t address = second_array + (x * 8);
        uint64_t function_address = 0;
        if (dmntchtReadCheatProcessMemory(address, (void *)&function_address, sizeof(uint64_t)))
        {
            Console::Printf("出现问题了。\n");
            break;
        }
        UnityOffsets.push_back(function_address - cheatMetadata.main_nso_extents.base);
    }
    return;
}

char path[128] = "";

void dumpAsLog()
{
    if (UnityNames.size() != UnityOffsets.size())
    {
        Console::Printf("无法生成日志，名称和偏移量的数量不匹配。\n");
        return;
    }
    uint64_t BID = 0;
    memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
    mkdir("sdmc:/switch/UnityFuncDumper/", 777);
    snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/", cheatMetadata.title_id);
    mkdir(path, 777);
    snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.log", cheatMetadata.title_id, __builtin_bswap64(BID));
    FILE *text_file = fopen(path, "w");
    if (!text_file)
    {
        Console::Printf("无法创建日志文件!\n");
        return;
    }
    fwrite(unity_sdk.c_str(), unity_sdk.size(), 1, text_file);
    fwrite("\n\n", 2, 1, text_file);
    for (size_t i = 0; i < UnityNames.size(); i++)
    {
        fwrite(UnityNames[i].c_str(), UnityNames[i].length(), 1, text_file);
        fwrite(": ", 2, 1, text_file);
        char temp[128] = "";
        snprintf(temp, sizeof(temp), "0x%X\n", UnityOffsets[i]);
        fwrite(temp, strlen(temp), 1, text_file);
    }
    fclose(text_file);
    Console::Printf("将日志文件转储到: *%s*\n", path);
}

// Main program entrypoint
int main(int argc, char *argv[]) {
    Logger::Initialize();

    if (!SDL::Initialize("UnityFuncDumper", 1280, 720))
    {
        Logger::Log("Error initializing SDL: %s", SDL::GetErrorString());
    }

    if (!SDL::Text::Initialize())
    {
        Logger::Log("Error initializing SDL::Text: %s", SDL::GetErrorString());
    }

    SDL::Text::AddColorCharacter(L'^', {0x00FFFFFF});
    SDL::Text::AddColorCharacter(L'*', {0x00FF00FF});
    SDL::Text::AddColorCharacter(L'>', {0xFF0000FF});

    Console::SetMaxLineCount(27);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    // Check and expand memory
    CheckAndExpandMemory();
    bool error = false;
    if (!isServiceRunning("dmnt:cht"))
    {
        Console::Printf(">未检测到 DMNT:CHT 服务!>\n");
        error = true;
    }
    pmdmntInitialize();
    uint64_t PID = 0;
    if (R_FAILED(pmdmntGetApplicationProcessId(&PID)))
    {
        Console::Printf(">游戏未初始化.>\n");
        error = true;
    }
    pmdmntExit();

    // Check and expand memory if needed
    CheckAndExpandMemory(); // Call the function here to attempt to expand the memory

    if (error)
    {
        Console::Printf("按 + 键退出。\n");
        while (appletMainLoop())
        {
            padUpdate(&pad);

            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus)
                break;
        }
    }
    else
    {
        pmdmntExit();
        size_t availableHeap = checkAvailableHeap();
        Console::Printf("可用内存堆：*%ld* MB\n", (availableHeap / (1024 * 1024)));
        // Initialize and check for Unity engine and handle mappings
        dmntchtInitialize();
        bool hasCheatProcess = false;
        dmntchtHasCheatProcess(&hasCheatProcess);
        if (!hasCheatProcess)
        {
            dmntchtForceOpenCheatProcess();
        }

        Result res = dmntchtGetCheatProcessMetadata(&cheatMetadata);
        if (res)
            Console::Printf(">dmntchtGetCheatProcessMetadata 返回: 0x%x>\n", res);

        res = dmntchtGetCheatProcessMappingCount(&mappings_count);
        if (res)
            Console::Printf(">dmntchtGetCheatProcessMappingCount 返回: 0x%x>\n", res);
        else
            Console::Printf("映射数量: *%ld*\n", mappings_count);

        memoryInfoBuffers = new MemoryInfo[mappings_count];

        res = dmntchtGetCheatProcessMappings(memoryInfoBuffers, mappings_count, 0, &mappings_count);
        if (res)
            Console::Printf(">DmntchtGetCheatProcessMappings返回: 0x%x>\n", res);

        if (checkIfUnity())
        {
            uint64_t BID = 0;
            memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
            mkdir("sdmc:/switch/UnityFuncDumper/", 777);
            snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/", cheatMetadata.title_id);
            mkdir(path, 777);
            snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.log", cheatMetadata.title_id, __builtin_bswap64(BID));
            bool file_exists = false;
            FILE *text_file = fopen(path, "r");
            if (text_file)
            {
                file_exists = true;
                fclose(text_file);
            }
            if (file_exists)
            {
                Console::Printf("\n函数偏移已被转储。\n按 \uE0E0 覆盖它们。\n按 \uE0E2 转储数据。\n按 \uE0EF 退出\n\n");
            }
            else
                Console::Printf("\n^----------^\n按 \uE0E0 开始\n按 \uE0EF 退出\n\n");
            bool overwrite = true;
            while (appletMainLoop())
            {
                padUpdate(&pad);

                u64 kDown = padGetButtonsDown(&pad);

                if (kDown & HidNpadButton_A)
                    break;

                if (kDown & HidNpadButton_Plus)
                {
                    dmntchtExit();
                    SDL::Text::Exit();
                    SDL::Exit();
                    return 0;
                }

                if (file_exists && (kDown & HidNpadButton_X))
                {
                    Console::Printf("将偏移量恢复到程序...\n");
                    text_file = fopen(path, "r");
                    if (!text_file)
                    {
                        Console::Printf("打开失败?!\n");
                    }
                    else
                    {
                        char line[256] = "";
                        while (fgets(line, sizeof(line), text_file))
                        {
                            if (strncmp("Unity", line, 5))
                                continue;
                            char *ptr = strchr(line, ' ');
                            char temp[256] = "";
                            memcpy(temp, line, ptr - (line + 1));
                            UnityOffsets.push_back(std::stoi(ptr, nullptr, 16));
                            UnityNames.push_back(temp);
                        }
                        fclose(text_file);
                    }
                    overwrite = false;
                    break;
                }
            }
            if (overwrite)
            {
                Console::Printf("正在搜索 RAM...\n\n");
                appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
                searchFunctionsUnity();
                Console::Printf("\n^---------------------------------------------^\n\n");
                Console::Reset();
                Console::Printf("搜索完成!\n");
                dumpAsLog();
                appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
                delete[] memoryInfoBuffers;
            }
        }
        dmntchtExit();
        Console::Printf("按 \uE0EF 键退出。\n");
        while (appletMainLoop())
        {
            padUpdate(&pad);

            u64 kDown = padGetButtonsDown(&pad);

            if (kDown & HidNpadButton_Plus)
                break;
        }
    }

    SDL::Text::Exit();
    SDL::Exit();
    return 0;
}
