// Header file pre proj2

// Knižnice
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<semaphore.h>
#include<unistd.h>
#include<time.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/wait.h>

// Makrá
#define MIN(a, b) (a > b) ? b : a
#define RANDSLEEP(max) if (max > 0) usleep(1000 * (rand() % (max + 1)))
#define MMAP(ptr) (ptr) = mmap(NULL, sizeof(*(ptr)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#define MUNMAP(ptr) munmap((ptr), sizeof((ptr)))

// Zdieľané premenné a semafory
FILE *file; //!< Výstupný súbor proj2.out
sem_t *mutex = NULL; //!< Mutex na koordináciu celého programu
sem_t *barrier = NULL; //!< Bariéra používaná pri bondingu
sem_t *oxy_barrier = NULL; //!< Bariéra atómov kyslíka
sem_t *oxy_queue = NULL; //!< Fronta čakajúcich atómov kyslíka
sem_t *hydro_barrier = NULL; //!< Bariéra atómov vodíka
sem_t *hydro_queue = NULL; //!< Fronta čakajúcich atómov vodíka
sem_t *moleculelock = NULL; //!< Semafór čakajúci na 3*"creating molecule" pred povolením "molecule created"
sem_t *bondlock = NULL; //!< Semafór pre správnu tvorbu bonds
sem_t *outlock = NULL; //!< Semafór pre synchronizáciu zápisu do výstupného súboru
int *oxy_count = 0; //!< Zdielaný počet atómov kyslíka
int *hydro_count = 0; //!< Zdielaný počet atómov vodíka
int *water_count = 0; //!< Zdielaný počet už vytvorených molekúl vody
int *bonded = 0; //!< Zdielaný počet bonded atómov
int *bondready = 0; //!< Zdielaný počet atómov pripravené na bonding
int *action = 0; //!< Zdielaný počet akcií (riadok v output súbore)
int *watermax = NULL; //!< Zdielaný maximálny počet molekúl

// Funkcie
void oxygen(int id_o, int atom_delay, int bond_delay); //!< Funkcia procesu atómu kyslíka
void hydrogen(int id_h, int atom_delay, int bond_delay); //!< Funkcia procesu atómu vodíka
int bond(int id, char type, int bond_delay); //!< Funkcia na simulovanú tvorbu väzby molekúl (podľa Little Book of Semaphores), vracia ID molekuly
void args_validate(int NO, int NH, int TI, int TB); //!< Kontrola správnosti argumentov, ukončí program pokiaľ sú argumenty neplatné
bool startup(); //!< Spustenie semafórov, mapovanie zdieľaných zdrojov a tvorba výstupného súboru
void clean(); //!< Ukončenie a vyčistenie semafórov a zdrojov
