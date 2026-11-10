#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

const char* NUME_MEMORIE = "Local\\Lab_SO_Memorie";
const char* NUME_SEMAFOR = "Local\\Lab_SO_Semafor";
const int   LIMITA       = 1000;

struct DateComune {
    int numar;        
    int scrise[2];      
};

int proces_copil(int id)
{
    DWORD pid = GetCurrentProcessId();
    srand((unsigned)time(NULL) ^ (pid << 8));

    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, NUME_MEMORIE);
    if (hMap == NULL) { std::cerr << "OpenFileMapping a esuat: " << GetLastError() << "\n"; return 1; }

    DateComune* mem = (DateComune*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DateComune));
    if (mem == NULL) { std::cerr << "MapViewOfFile a esuat: " << GetLastError() << "\n"; return 1; }

    HANDLE hSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, NUME_SEMAFOR);
    if (hSem == NULL) { std::cerr << "OpenSemaphore a esuat: " << GetLastError() << "\n"; return 1; }

    while (true) {
        WaitForSingleObject(hSem, INFINITE);

        // 2. citim memoria
        int n = mem->numar;
        if (n >= LIMITA) {
            ReleaseSemaphore(hSem, 1, NULL);
            break;
        }

        while (n < LIMITA && (rand() % 2) + 1 == 2) {
            n++;
            mem->numar = n;
            mem->scrise[id]++;
            std::cout << "Procesul " << id + 1 << " (PID " << pid << ") a scris " << n << std::endl;
        }

        ReleaseSemaphore(hSem, 1, NULL);

        Sleep(0);   
    }

    std::cout << "Procesul " << id + 1 << " (PID " << pid << ") a terminat. A scris "
              << mem->scrise[id] << " numere.\n";

    UnmapViewOfFile(mem);
    CloseHandle(hMap);
    CloseHandle(hSem);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc == 3 && std::string(argv[1]) == "copil") {
        return proces_copil(atoi(argv[2]));
    }

    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                     0, sizeof(DateComune), NUME_MEMORIE);
    if (hMap == NULL) { std::cerr << "CreateFileMapping a esuat: " << GetLastError() << "\n"; return 1; }

    DateComune* mem = (DateComune*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DateComune));
    mem->numar = 0;
    mem->scrise[0] = mem->scrise[1] = 0;

    HANDLE hSem = CreateSemaphoreA(NULL, 1, 1, NUME_SEMAFOR);
    if (hSem == NULL) { std::cerr << "CreateSemaphore a esuat: " << GetLastError() << "\n"; return 1; }

    std::cout << "Parinte (PID " << GetCurrentProcessId() << "): pornesc 2 procese..." << std::endl;

    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);

    PROCESS_INFORMATION pi[2];
    for (int i = 0; i < 2; i++) {
        std::string cmd = "\"" + std::string(exe) + "\" copil " + std::to_string(i);
        STARTUPINFOA si = { sizeof(si) };
        if (!CreateProcessA(NULL, &cmd[0], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi[i])) {
            std::cerr << "CreateProcess a esuat: " << GetLastError() << "\n";
            return 1;
        }
    }
    ResumeThread(pi[0].hThread);
    ResumeThread(pi[1].hThread);

    HANDLE procese[2] = { pi[0].hProcess, pi[1].hProcess };
    WaitForMultipleObjects(2, procese, TRUE, INFINITE);

    std::cout << "\nRezultat final in memorie: " << mem->numar << "\n";
    std::cout << "Procesul 1 a scris " << mem->scrise[0] << " numere, procesul 2 a scris "
              << mem->scrise[1] << " numere (total " << mem->scrise[0] + mem->scrise[1] << ").\n";

    for (int i = 0; i < 2; i++) { CloseHandle(pi[i].hProcess); CloseHandle(pi[i].hThread); }
    UnmapViewOfFile(mem);
    CloseHandle(hMap);
    CloseHandle(hSem);
    return 0;
}