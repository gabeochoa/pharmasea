
/*


   To detect if OBS (Open Broadcaster Software) is running on the computer
alongside your C++ program, you can use platform-specific methods or techniques.
Here are a few approaches for different operating systems:

Windows:

Check if the OBS executable process is running using the
CreateToolhelp32Snapshot and Process32First functions from the Windows API. You
can iterate through the running processes and check if the process name matches
"obs.exe". Example code snippet: cpp Copy code #include <Windows.h> #include
<tlhelp32.h>

bool isOBSRunning()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &entry))
        {
            do
            {
                if (wcscmp(entry.szExeFile, L"obs.exe") == 0)
                {
                    CloseHandle(hSnapshot);
                    return true;
                }
            } while (Process32Next(hSnapshot, &entry));
        }
    }

    CloseHandle(hSnapshot);
    return false;
}
macOS:

Use the sysctl function to query the running processes on macOS and check if OBS
is among them. Example code snippet: cpp Copy code #include <sys/sysctl.h>

#include <cstring>

bool isOBSRunning()
{
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_ALL;
    mib[3] = 0;

    struct kinfo_proc* processList = nullptr;
    size_t length = 0;

    if (sysctl(mib, 4, nullptr, &length, nullptr, 0) < 0)
        return false;

    processList = (struct kinfo_proc*)malloc(length);
    if (processList == nullptr)
        return false;

    if (sysctl(mib, 4, processList, &length, nullptr, 0) < 0)
    {
        free(processList);
        return false;
    }

    bool isRunning = false;
    const size_t numProcesses = length / sizeof(struct kinfo_proc);
    for (size_t i = 0; i < numProcesses; ++i)
    {
        const char* processName = processList[i].kp_proc.p_comm;
        if (strcmp(processName, "obs") == 0)
        {
            isRunning = true;
            break;
        }
    }

    free(processList);
    return isRunning;
}
Linux:

Use the proc file system to check the running processes on Linux and look for
OBS among them. Example code snippet: cpp Copy code #include <fstream> #include
<string>

bool isOBSRunning()
{
    std::ifstream procFile("/proc/self/comm");
    if (procFile)
    {
        std::string processName;
        std::getline(procFile, processName);
        if (processName == "obs")
            return true;
    }

    return false;
}
Keep in mind that these methods rely on the process name "obs.exe" on Windows,
"obs" on macOS and Linux. If the OBS executable has a different name or the
process is renamed, these methods may not work.

*/
