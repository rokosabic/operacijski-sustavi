#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define BR_ULAZNIH 4
#define BR_RADNIH 6
#define BR_IZLAZNIH 3
#define BR_POD_DRETVA 8
#define VRIJEME_DOHVAT 5
#define VRIJEME_IZLAZ 5
#define VRIJEME_OBRADA 3

struct Meduspremnik {
    int ulaz;
    int izlaz;
    sem_t smijem_koristiti;
    sem_t ima_podataka;
    char spremnik[BR_POD_DRETVA];
};

struct Meduspremnik UMS[BR_RADNIH];
struct Meduspremnik IMS[BR_IZLAZNIH];

sem_t printaj;

char dohvati_ulaz() {
    return (rand() % 2) ? ('A' + rand() % 26) : ('a' + rand() % 26);
}

int obradi_ulaz(int id, char podatak) {
    return rand() % BR_RADNIH;
}

void obradi(int id, char podatak, char *rezultat, int *broj_meduspremnika) {
    sleep(VRIJEME_OBRADA); // simulira obradu podatka
    *rezultat = podatak; // "obrada podatka" zapravo ga samo prepisujemo
    *broj_meduspremnika = rand() % BR_IZLAZNIH; // bira meduspremnik u koji ce upisati izlaz
}

void print() {
    printf("UMS[]: ");
    for (int i = 0; i < BR_RADNIH; i++) {
        for (int j = 0; j < BR_POD_DRETVA; j++) {
            printf("%c", (UMS[i]).spremnik[j]);
        }
        printf(" ");
    }
    printf("\nIMS[]: ");
    for (int i = 0; i < BR_IZLAZNIH; i++) {
        for (int j = 0; j < BR_POD_DRETVA; j++) {
            printf("%c", (IMS[i]).spremnik[j]);
        }
        printf(" ");
    }
    printf("\n");  
}

void printWithPrefix(char tip, int id, char podatak, int br_meduspremnika) {
    sem_wait(&printaj);
    switch (tip) {
        case 'U':
            printf("\n(%c%d) upisao podatak: %c u ulazni meduspremnik %d \n",tip, id, podatak, br_meduspremnika);
            break;
        case 'R':
            printf("\n(%c%d) obradio podatak: %c i upisao u izlazni meduspremnik %d \n", tip, id, podatak, br_meduspremnika);
            break;
        case 'r':
            printf("\n(%c%d) uzmiam podatak: %c iz ulaznog meduspremnika %d \n", tip - 'a' + 'A', id, podatak, br_meduspremnika);
            break;
        case 'I':
            printf("\n(%c%d) stavio podatak: %c na izlaz \n", tip, id, podatak);
            break;
        default:
            break;

    }
    print();
    sem_post(&printaj);
}

void *ulazna_dretva(void *p) {
    int id = *((int *) p);
    
    while(1) {
        sleep(rand() % VRIJEME_DOHVAT + 3); // koliko vremena izmedu svakog novog podatka, 3-3+VRIJEME_DOHVAT s
        char podatak = dohvati_ulaz();
        int broj_meduspremnika = obradi_ulaz(id,podatak);
        struct Meduspremnik *meduspremnik = &UMS[broj_meduspremnika];
        sem_t *smijem_koristiti = &meduspremnik->smijem_koristiti;
        sem_t *ima_podataka = &meduspremnik->ima_podataka;
        int *ulaz = &meduspremnik->ulaz;
        int *izlaz = &meduspremnik->izlaz;
        sem_wait(smijem_koristiti);
        if (meduspremnik->spremnik[*ulaz] != '-') { // spremnik ce se popuniti, prepisuje se najstariji podatak i povecava izlaz
            *izlaz = (*izlaz + 1) % BR_POD_DRETVA;
        } else { // ako ne prepisujemo postojeci podatak povecavamo broj_podataka za 1
            sem_post(ima_podataka);
        }
        meduspremnik->spremnik[*ulaz] = podatak;
        *ulaz = (*ulaz + 1) % BR_POD_DRETVA;
        printWithPrefix('U', id, podatak, broj_meduspremnika);
        sem_post(smijem_koristiti);
    }

    return NULL;
}

void *radna_dretva(void *p) {
    int id = *((int *) p);
    struct Meduspremnik *meduspremnik_za_citanje = &UMS[id-1];
    sem_t *smijem_koristiti_citanje = &meduspremnik_za_citanje->smijem_koristiti;
    sem_t *ima_podataka = &meduspremnik_za_citanje->ima_podataka;
    int *izlaz_citanje = &meduspremnik_za_citanje->izlaz;

    while(1) {
        sem_wait(ima_podataka); // ceka podatak za procitati
        sem_wait(smijem_koristiti_citanje); // ceka da smije citati
        char podatak = meduspremnik_za_citanje->spremnik[*izlaz_citanje];
        meduspremnik_za_citanje->spremnik[*izlaz_citanje] = '-';
        *izlaz_citanje = (*izlaz_citanje + 1) % BR_POD_DRETVA;
        printWithPrefix('r', id, podatak, id-1);
        sem_post(smijem_koristiti_citanje);

        int broj_meduspremnika_za_pisanje = -1;
        char rezultat = '0';
        obradi(id, podatak, &rezultat, &broj_meduspremnika_za_pisanje);
        struct Meduspremnik *meduspremnik_za_pisanje = &IMS[broj_meduspremnika_za_pisanje];
        sem_t *smijem_koristiti_pisanje = &meduspremnik_za_pisanje->smijem_koristiti;
        int *ulaz_pisanje = &meduspremnik_za_pisanje->ulaz;
        int *izlaz_pisanje = &meduspremnik_za_pisanje->izlaz;

        sem_wait(smijem_koristiti_pisanje);
        if (meduspremnik_za_pisanje->spremnik[*ulaz_pisanje] != '-') { // spremnik ce se popuniti, prepisuje se najstariji podatak i povecava izlaz
            *izlaz_pisanje = (*izlaz_pisanje + 1) % BR_POD_DRETVA;
        }
        meduspremnik_za_pisanje->spremnik[*ulaz_pisanje] = rezultat;
        *ulaz_pisanje = (*ulaz_pisanje + 1) % BR_POD_DRETVA;
        printWithPrefix('R', id, rezultat, broj_meduspremnika_za_pisanje);
        sem_post(smijem_koristiti_pisanje);
    }

}

void *izlazna_dretva(void *p) {
    int id = *((int *) p);
    struct Meduspremnik *meduspremnik = &IMS[id-1];
    sem_t *smijem_koristiti = &meduspremnik->smijem_koristiti;
    int *izlaz = &meduspremnik->izlaz;
    char podatak = '0'; // inicijalni podatak tj. nema podatka
    while(1) {
        sleep(VRIJEME_IZLAZ); // svakih VRIJEME_IZLAZ sekundi dretva cita spremnik za izlaz
        sem_wait(smijem_koristiti);
        if (meduspremnik->spremnik[*izlaz] != '-') {
            podatak = meduspremnik->spremnik[*izlaz];
            meduspremnik->spremnik[*izlaz] = '-';
            *izlaz = (*izlaz + 1) % BR_POD_DRETVA;
        } else { // nema novog podatka za procitati
            podatak = '0';
        }
        printWithPrefix('I', id, podatak, 0);
        sem_post(smijem_koristiti);
    }
}



int main() {
    // inicijalizacija ulaznog spremnika
    for (int i=0; i<BR_RADNIH; i++) {
        struct Meduspremnik m;
        m.ulaz = 0;
        m.izlaz = 0;
        for (int i=0; i<BR_POD_DRETVA;i++) {
            m.spremnik[i] = '-';
        }
        // semafor inicijalno u 1 (nitko ne koristi meduspremnik)
        sem_init(&m.smijem_koristiti, 0, 1);
        // opci semafor inicjalno u 0 (nema podataka u spremniku)
        sem_init(&m.ima_podataka, 0, 0);
        UMS[i] = m;
    }

    // inicijalizacija izlaznog spremnika
    for (int i=0; i<BR_IZLAZNIH; i++) {
        struct Meduspremnik m;
        m.ulaz = 0;
        m.izlaz = 0;
        for (int i=0; i<BR_POD_DRETVA;i++) {
            m.spremnik[i] = '-';
        }
        // binarni semafor inicijalno u 1 (nitko ne koristi meduspremnik)
        sem_init(&m.smijem_koristiti, 0, 1);
        // opci semafor inicjalno u 0 (nema podataka u spremniku)
        sem_init(&m.ima_podataka, 0, 0);
        IMS[i] = m;
    }
    sem_init(&printaj,0,1);
    print();

    pthread_t ulazne_dretve_id[BR_ULAZNIH];
	int ulazne_id[BR_ULAZNIH];

    pthread_t radne_dretve_id[BR_RADNIH];
    int radne_id[BR_RADNIH];

    pthread_t izlazne_dretve_id[BR_IZLAZNIH];
    int izlazne_id[BR_IZLAZNIH];

    for (int i = 0; i < BR_ULAZNIH; i++ ) {
        ulazne_id[i] = i + 1;
        if ( pthread_create(&ulazne_dretve_id[i], NULL, ulazna_dretva, &ulazne_id[i]) ) {
            perror("Greska pri stvaranju dretve");
            exit(1);
        }
    }

    for (int i = 0; i < BR_RADNIH; i++ ) {
        radne_id[i] = i + 1;
        if ( pthread_create(&radne_dretve_id[i], NULL, radna_dretva, &radne_id[i]) ) {
            perror("Greska pri stvaranju dretve");
            exit(1);
        }
    }

    for (int i = 0; i < BR_IZLAZNIH; i++ ) {
        izlazne_id[i] = i + 1;
        if ( pthread_create(&izlazne_dretve_id[i], NULL, izlazna_dretva, &izlazne_id[i]) ) {
            perror("Greska pri stvaranju dretve");
            exit(1);
        }
    }
    sem_destroy(&printaj);
    pthread_join(ulazne_dretve_id[0],NULL);
    return 0;
}


