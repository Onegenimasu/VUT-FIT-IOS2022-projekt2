// Libraries
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Macros
#define MIN(a, b) (a > b) ? b : a
#define RANDSLEEP(max) \
    if (max > 0) usleep(1000 * (rand() % (max + 1)))
#define MMAP(ptr) (ptr) = mmap(NULL, sizeof(*(ptr)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#define MUNMAP(ptr) munmap((ptr), sizeof((ptr)))

// Semaphores and shared resources
FILE *file;                   //!< Output file proj2.out
sem_t *mutex = NULL;          //!< Mutex for program coordination
sem_t *barrier = NULL;        //!< Bonding barrier
sem_t *oxy_barrier = NULL;    //!< Oxygen barrier
sem_t *oxy_queue = NULL;      //!< Queue of waiting oxygen atoms
sem_t *hydro_barrier = NULL;  //!< Hydrogen barrier
sem_t *hydro_queue = NULL;    //!< Queue of waiting hydrogen atoms
sem_t *moleculelock = NULL;   //!< Semaphore awaiting three atoms to bond
sem_t *bondlock = NULL;       //!< Bonding semaphore
sem_t *outlock = NULL;        //!< Semaphore for output synchronisation
int *oxy_count = 0;           //!< Oxygen atom counter
int *hydro_count = 0;         //!< Hydrogen atom counter
int *water_count = 0;         //!< Molecule counter
int *bonded = 0;              //!< Bonded atom counter
int *bondready = 0;           //!< Ready to bond atom counter
int *action = 0;              //!< Action counter
int *watermax = NULL;         //!< Maximum number of molecules to be created

// Funkcie
void oxygen(int id_o, int atom_delay, int bond_delay);    //!< Oxygen atom process function
void hydrogen(int id_h, int atom_delay, int bond_delay);  //!< Hydrogen atom process function
int bond(int id, char type, int bond_delay);              //!< Function to simulate atom bonding, returns molecule ID
void args_validate(int NO, int NH, int TI, int TB);       //!< Validation of input arguments, exits on error
bool startup();                                           //!< Startup function (semaphore, shared resources and output file initialization)
void clean();                                             //!< Cleanup function (semaphore, shared resources and output file closing)
