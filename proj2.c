/**
 * @file proj2.c
 * @author GitHub User <xplagi0b@vutbr.cz>
 * @brief IOS projekt 2 (synchronizácia) - Building H2O
 * @param NO Počet atómov kyslíka
 * @param NH Počet atómov vodíka
 * @param TI Maximálny čas (ms) čakania atómov na zaradenie do fronty
 * @param TB Maximálny čas (ms) na vytvorenie molekuly
 * 
 */

#include "proj2.h"

void oxygen(int id_o, int atom_delay, int bond_delay) {
    // RNG seed
    srand(time(NULL) * getpid());

    // Oznámenie začiatku procesu
    sem_wait(mutex);
    sem_wait(outlock);
    if (*action > 0) fprintf(file, "\n");
    fprintf(file, "%d: O %d: started", ++(*action), id_o);
    sem_post(outlock);
    sem_post(mutex);

    // Zaradenie do fronty kyslíkov
    RANDSLEEP(atom_delay);
    sem_wait(oxy_barrier);
    sem_wait(mutex);
    (*oxy_count)++;
    sem_wait(outlock);
    fprintf(file, "\n%d: O %d: going to queue", ++(*action), id_o);
    sem_post(outlock);

    if (*hydro_count >= 2) {
        // Uvoľnenie atómov z fronty
        sem_post(hydro_queue);
        (*hydro_count)--;
        sem_post(hydro_queue);
        (*hydro_count)--;
        sem_post(oxy_queue);
        (*oxy_count)--;
    } else {
        // Čakanie
        sem_post(mutex);
    }

    if (*water_count < *watermax) {
        // Tvorba molekuly
        sem_wait(oxy_queue);
        int id_w = bond(id_o, 'O', bond_delay);
        sem_wait(bondlock);
        (*bonded)++;
        if (*bonded == 3) {
            sem_post(hydro_barrier);
            sem_post(hydro_barrier);
            sem_post(oxy_barrier);
            *bonded = 0;
            sem_post(moleculelock);
            sem_post(moleculelock);
            sem_post(moleculelock);
        }

        sem_post(bondlock);
        sem_post(mutex);
        sem_wait(moleculelock);

        sem_wait(outlock);
        fprintf(file, "\n%d: O %d: molecule %d created", ++(*action), id_o, id_w);
        sem_post(outlock);
    } else {
        // Už nie je možné vytvoriť viac molekúl
        sem_post(oxy_barrier);
        sem_wait(outlock);
        fprintf(file, "\n%d: O %d: not enough H", ++(*action), id_o);
        sem_post(outlock);
    }

    if (file) fclose(file);
    exit(EXIT_SUCCESS);
}

void hydrogen(int id_h, int atom_delay, int bond_delay) {
    // RNG seed
    srand(time(NULL) * getpid());

    // Oznámenie začiatku procesu
    sem_wait(mutex);
    sem_wait(outlock);
    if (*action > 0) fprintf(file, "\n");
    fprintf(file, "%d: H %d: started", ++(*action), id_h);
    sem_post(outlock);
    sem_post(mutex);

    // Zaradenie do fronty vodíkov
    RANDSLEEP(atom_delay);
    sem_wait(hydro_barrier);
    sem_wait(mutex);
    (*hydro_count)++;
    sem_wait(outlock);
    fprintf(file, "\n%d: H %d: going to queue", ++(*action), id_h);
    sem_post(outlock);

    if (*hydro_count >= 2 && *oxy_count >= 1) {
        // Uvoľnenie atómov z fronty
        sem_post(hydro_queue);
        (*hydro_count)--;
        sem_post(hydro_queue);
        (*hydro_count)--;
        sem_post(oxy_queue);
        (*oxy_count)--;
    } else {
        // Čakanie
        sem_post(mutex);
    }

    if (*water_count < *watermax) {
        // Tvorba molekuly
        sem_wait(hydro_queue);
        int id_w = bond(id_h, 'H', bond_delay);
        sem_wait(bondlock);
        (*bonded)++;
        if (*bonded == 3) {
            sem_post(hydro_barrier);
            sem_post(hydro_barrier);
            sem_post(oxy_barrier);
            *bonded = 0;
            sem_post(moleculelock);
            sem_post(moleculelock);
            sem_post(moleculelock);
        }
        sem_post(bondlock);
        sem_wait(moleculelock);

        sem_wait(outlock);
        fprintf(file, "\n%d: H %d: molecule %d created", ++(*action), id_h, id_w);
        sem_post(outlock);
    } else {
        // Už nie je možné vytvoriť viac molekúl
        sem_post(hydro_barrier);
        sem_wait(outlock);
        fprintf(file, "\n%d: H %d: not enough O or H", ++(*action), id_h);
        sem_post(outlock);
    }

    if (file) fclose(file);
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[]) {
    // Kontrola a uloženie argumentov
    if (argc != 5) {
        fprintf(stderr, "Wrong amount of arguments. Aborting!");
        exit(EXIT_FAILURE);
    }

    int NO = atoi(argv[1]); // Počet atómov kyslíka
    int NH = atoi(argv[2]); // Počet atómov vodíka
    int TI = atoi(argv[3]); // Maximálny čas (ms) čakania atómov na zaradenie do fronty
    int TB = atoi(argv[4]); // Maximálny čas (ms) na vytvorenie molekuly
    args_validate(NO, NH, TI, TB);
    
    // Inicializácia semaforov a zdieľaných zdrojov
    if (!startup()) {
        fprintf(stderr, "Couldn't initialize semaphores or shared memory. Aborting!");
        clean();
        exit(EXIT_FAILURE);
    }

    // Výpočet maximálneho počtu molekúl
    *watermax = MIN(NO, floor(NH/2));

    // Vytvorenie NO procesov kyslíka a NH procesov vodíka
    for (int i = 1; i <= NO; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            oxygen(i, TI, TB);
        } else if (pid < 0) {
            fprintf(stderr, "Forking failed. Aborting!");
            clean();
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 1; i <= NH; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            hydrogen(i, TI, TB);
        } else if (pid < 0) {
            fprintf(stderr, "Forking failed. Aborting!");
            clean();
            exit(EXIT_FAILURE);
        }
    }

    // Čakanie na ukončenie všetkých procesov
    while(wait(NULL) > 0);
    clean();
    exit(EXIT_SUCCESS);
}

int bond(int atom_id, char atom_type, int bond_delay) {
    sem_wait(bondlock);
    int next_molecule = *water_count + 1;
    (*bondready)++;
    sem_wait(outlock);
    fprintf(file, "\n%d: %c %d: creating molecule %d", ++(*action), atom_type, atom_id, next_molecule);
    sem_post(outlock);
    RANDSLEEP(bond_delay);

    if (*bondready == 3) {
        for (int i = 0; i < 3; i++) {
            sem_post(barrier);
            (*bondready)--;
        }
        (*water_count)++;
    }    

    sem_post(bondlock);
    sem_wait(barrier);

    return next_molecule;
}

void args_validate(int NO, int NH, int TI, int TB) {
    if (NO < 0) {
        fprintf(stderr, "Oxygen count cannot be negative (antimatter not yet supported). Aborting!");
        exit(EXIT_FAILURE);
    }

    if (NH < 0) {
        fprintf(stderr, "Hydrogen count cannot be negative (antimatter not yet supported). Aborting!");
        exit(EXIT_FAILURE);
    }

    if (TI < 0 || TI > 1000) {
        fprintf(stderr, "Max queue wait time must be between 0 and 1000 (including). Aborting!");
        exit(EXIT_FAILURE);
    }

    if (TB < 0 || TB > 1000) {
        fprintf(stderr, "Max molecule creation time must be between 0 and 1000 (including). Aborting!");
        exit(EXIT_FAILURE);
    }

    return;    
}

bool startup() {
    // Otvorenie výstupného súboru
    file = fopen("proj2.out", "w");
    if (file == NULL) {
        fprintf(stderr, "Could not create output file. Aborting!");
        exit(EXIT_FAILURE);
    }

    // Zdieľané premenné
    MMAP(oxy_count);
    if (oxy_count == NULL)
        return false;

    MMAP(hydro_count);
    if (hydro_count == NULL)
        return false;

    MMAP(water_count);
    if (water_count == NULL)
        return false;

    MMAP(bonded);
    if (bonded == NULL)
        return false;

    MMAP(bondready);
    if (bondready == NULL)
        return false;

    MMAP(action);
    if (action == NULL)
        return false;

    MMAP(watermax);
    if (watermax == NULL)
        return false;

    // Semafory
    mutex = sem_open("/xplagi0b.mutex", O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED)
        return false;

    barrier = sem_open("/xplagi0b.barrier", O_CREAT, 0666, 3);
    if (barrier == SEM_FAILED)
        return false;

    oxy_barrier = sem_open("/xplagi0b.oxy_barrier", O_CREAT, 0666, 1);
    if (oxy_barrier == SEM_FAILED)
        return false;

    oxy_queue = sem_open("/xplagi0b.oxy_queue", O_CREAT, 0666, 0);
    if (oxy_queue == SEM_FAILED)
        return false;

    hydro_barrier = sem_open("/xplagi0b.hydro_barrier", O_CREAT, 0666, 2);
    if (hydro_barrier == SEM_FAILED)
        return false;
    
    hydro_queue = sem_open("/xplagi0b.hydro_queue", O_CREAT, 0666, 0);
    if (hydro_queue == SEM_FAILED)
        return false;

    moleculelock = sem_open("/xplagi0b.moleculelock", O_CREAT, 0666, 0);
    if (moleculelock == SEM_FAILED)
        return false;
    
    bondlock = sem_open("/xplagi0b.bondlock", O_CREAT, 0666, 1);
    if (bondlock == SEM_FAILED)
        return false;

    outlock = sem_open("/xplagi0b.outlock", O_CREAT, 0666, 1);
    if (outlock == SEM_FAILED)
        return false;

    setbuf(file, NULL);
    return true;
}

void clean() {
    // Semafory
    sem_close(mutex);
    sem_unlink("/xplagi0b.mutex");
    sem_close(barrier);
    sem_unlink("/xplagi0b.barrier");
    sem_close(oxy_barrier);
    sem_unlink("/xplagi0b.oxy_barrier");
    sem_close(oxy_queue);
    sem_unlink("/xplagi0b.oxy_queue");
    sem_close(hydro_barrier);
    sem_unlink("/xplagi0b.hydro_barrier");
    sem_close(hydro_queue);
    sem_unlink("/xplagi0b.hydro_queue");
    sem_close(moleculelock);
    sem_unlink("/xplagi0b.moleculelock");
    sem_close(bondlock);
    sem_unlink("/xplagi0b.bondlock");
    sem_close(outlock);
    sem_unlink("/xplagi0b.outlock");

    // Zdieľané premenné
    MUNMAP(oxy_count);
    MUNMAP(hydro_count);
    MUNMAP(water_count);
    MUNMAP(bonded);
    MUNMAP(bondready);
    MUNMAP(action);
    MUNMAP(watermax);

    // Výstupný súbor
    if (file != NULL)
        fclose(file);

    return;
}
