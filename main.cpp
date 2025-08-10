#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

const int ping = 60;

const int timeToMount = 1000;
const int timeToHearthstone = 6000;
const int timeForBonus = 550;

const int fullMagazineValue = 8192 * 10;
const int emptyMagazineValue = 0;

static void PressKey(WORD key) {
    INPUT ip = {0};
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = key;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));
    Sleep(15);
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}

static void PressD() {
    PressKey(0x44);
}

static void WaitForBonusAndPressD() {
    Sleep(timeForBonus + ping);
    PressD();
}

static void ScanProcessMemory(HANDLE hProcess, int targetValue, vector<DWORDLONG>& foundAddresses) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MEMORY_BASIC_INFORMATION memInfo;
    DWORDLONG address = (DWORDLONG)sysInfo.lpMinimumApplicationAddress;
    DWORDLONG maxAddress = (DWORDLONG)sysInfo.lpMaximumApplicationAddress;
    while (address < maxAddress) {
        if (VirtualQueryEx(hProcess, (LPCVOID)address, &memInfo, sizeof(memInfo))) {
            if (memInfo.State == MEM_COMMIT && (memInfo.Protect == PAGE_READWRITE)) {
                BYTE* buffer = new BYTE[memInfo.RegionSize];
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer, memInfo.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i < bytesRead; i += sizeof(int)) {
                        int value = *(int*)(buffer + i);
                        if (value == targetValue) {
                            foundAddresses.push_back(address + i);
                        }
                    }
                }
                delete[] buffer;
            }
        }
        address += memInfo.RegionSize;
    }
}

static bool IsValidMultipleOf8192(int value) {
    const int factor = 8192;
    int quotient = value / factor;
    return (value % factor == 0) && (quotient >= 1 && quotient <= 9);
}

int main() {
    LPCSTR className = "Heroes of the Storm";
    HWND hWnd = FindWindowA(className, nullptr);
    DWORD pID = 0;
    GetWindowThreadProcessId(hWnd, &pID);
    if (pID == 0) {
        cerr << "Couldn't find Heroes of the Storm process." << endl;
        system("pause");
        return 0;
    }
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
    vector<DWORDLONG> fullMagazineAddresses;
    cout << "Tool has to be launched when Tracer has full magazine." << endl;
    ScanProcessMemory(pHandle, fullMagazineValue, fullMagazineAddresses);
    cout << "Start shooting to complete tool initialization." << endl;
    DWORDLONG magazineAddress = 0;
    while (true) {
        int value = 0;
        for (const auto& addr : fullMagazineAddresses) {
            ReadProcessMemory(pHandle, (LPCVOID)addr, &value, sizeof(value), nullptr);
            if (value != 8192 * 9)
                continue;
            const int minIndex = 7;
            for (int curIndex = 9; curIndex >= minIndex; ) {
                ReadProcessMemory(pHandle, (LPCVOID)addr, &value, sizeof(value), nullptr);
                if (value == 8192 * curIndex)
                    continue;
                else if (value == 8192 * (curIndex - 1))
                    curIndex--;
                else
                    break;
                if (curIndex == minIndex)
                    magazineAddress = addr;
            }
            if (magazineAddress != 0)
                break;
        }
        if (magazineAddress != 0)
            break;
        Sleep(100);
    }
    DWORDLONG reloadAddress = magazineAddress + 0x6c;
    cout << "Tool initialized." << endl;
    int reloadValue = 0;
    int magazineValue = 0, secondMagazineValue = 0;
    while (true)
    {
        if (GetAsyncKeyState(0x5A) & 0x8000) { // Z
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &magazineValue, sizeof(magazineValue), nullptr);
            Sleep(timeToMount + timeForBonus + ping);
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &secondMagazineValue, sizeof(secondMagazineValue), nullptr);
            if (magazineValue == secondMagazineValue)
                PressD();
        }
        else if (GetAsyncKeyState(0x42) & 0x8000) { // B
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &magazineValue, sizeof(magazineValue), nullptr);
            Sleep(timeToHearthstone + timeForBonus + ping);
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &secondMagazineValue, sizeof(secondMagazineValue), nullptr);
            if (magazineValue == secondMagazineValue)
                PressD();
        }
        else if (GetAsyncKeyState(0x44) & 0x8000) { // D
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &magazineValue, sizeof(magazineValue), nullptr);
            if (magazineValue != fullMagazineValue) 
                WaitForBonusAndPressD();
        }
        else if (GetAsyncKeyState(0x45) & 0x8000) { // E
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &magazineValue, sizeof(magazineValue), nullptr);
            if (magazineValue != fullMagazineValue)
                WaitForBonusAndPressD();
        }
        else {
            ReadProcessMemory(pHandle, (LPCVOID)reloadAddress, &reloadValue, sizeof(reloadValue), nullptr);
            ReadProcessMemory(pHandle, (LPCVOID)magazineAddress, &magazineValue, sizeof(magazineValue), nullptr);
            if (magazineValue == 0 && reloadValue >= 1536 && reloadValue <= 3072) {
                Sleep(ping);
                PressD();
                Sleep(350);
            }
        }
        Sleep(15);
    }
    CloseHandle(pHandle);
    system("pause");
    return 0;
}
