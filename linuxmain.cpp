#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>      
#include <sys/mman.h>    
#include <sys/wait.h>    
#include <semaphore.h>   
#include <unistd.h>      

const char* NUME_MEMORIE = "/lab_so_memorie";
const char* NUME_SEMAFOR = "/lab_so_semafor";
const int   LIMITA       = 1000;

struct DateComune {
    int numar;          
    int scrise[2];      
};

void proces_copil(int id, DateComune* mem, sem_t* sem)
{
    pid_t pid = getpid();
    srand(time(NULL) ^ (pid << 8));

    while (true) {
        sem_wait(sem);

        int n = mem->numar;
        if (n >= LIMITA) {
            sem_post(sem);
            break;
        }

        while (n < LIMITA && (rand() % 2) + 1 == 2) {
            n++;
            mem->numar = n;
            mem->scrise[id]++;
            printf("Procesul %d (PID %d) a scris %d\n", id + 1, pid, n);
            fflush(stdout);
        }

        sem_post(sem);

        sched_yield();  
    }

    printf("Procesul %d (PID %d) a terminat. A scris %d numere.\n", id + 1, pid, mem->scrise[id]);
    fflush(stdout);
}

int main()
{
    int fd = shm_open(NUME_MEMORIE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(fd, sizeof(DateComune)) == -1) { perror("ftruncate"); return 1; }

    DateComune* mem = (DateComune*)mmap(NULL, sizeof(DateComune), PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) { perror("mmap"); return 1; }
    mem->numar = 0;
    mem->scrise[0] = mem->scrise[1] = 0;

    sem_unlink(NUME_SEMAFOR);   
    sem_t* sem = sem_open(NUME_SEMAFOR, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); return 1; }

    printf("Parinte (PID %d): pornesc 2 procese...\n", getpid());
    fflush(stdout);   

    pid_t copii[2];
    for (int i = 0; i < 2; i++) {
        copii[i] = fork();
        if (copii[i] == -1) { perror("fork"); return 1; }
        if (copii[i] == 0) {          
            proces_copil(i, mem, sem);
            _exit(0);
        }
    }

    for (int i = 0; i < 2; i++) waitpid(copii[i], NULL, 0);

    printf("\nRezultat final in memorie: %d\n", mem->numar);
    printf("Procesul 1 a scris %d numere, procesul 2 a scris %d numere (total %d).\n",
           mem->scrise[0], mem->scrise[1], mem->scrise[0] + mem->scrise[1]);

    munmap(mem, sizeof(DateComune));
    close(fd);
    shm_unlink(NUME_MEMORIE);
    sem_close(sem);
    sem_unlink(NUME_SEMAFOR);
    return 0;
}
