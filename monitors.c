#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

typedef struct Node{ //struct za listu
    int value;
    struct Node *next;
} Node;

typedef struct Monitor{ //struct za monitor
    pthread_mutex_t mutex;
    pthread_cond_t read_cond;
    pthread_cond_t read_notempty;
    pthread_cond_t write_cond;
    pthread_cond_t delete_cond;
    int reader_count;
    int reader_waiting;
    int writer_count;
    int writer_waiting;
    int deleter_count;
    int deleter_waiting;
} Monitor;

Node *list = NULL;
Monitor monitor;
int t = 0;

void initialize_monitor(Monitor *mon){ //inicijalizacija monitora
    pthread_mutex_init(&mon->mutex, NULL); //NULL -> nikakvi dodatni atributi za mutex
    pthread_cond_init(&mon->read_cond, NULL);
    pthread_cond_init(&mon->write_cond, NULL);
    pthread_cond_init(&mon->delete_cond, NULL);
    mon->reader_count = 0;
    mon->reader_waiting = 0;
    mon->writer_count = 0;
    mon->writer_waiting = 0;
    mon->deleter_count = 0;
    mon->deleter_waiting = 0;
}

int list_size(){
    int i = 0;
    Node *tmp = list;
    while(tmp != NULL){
        i++;
        tmp = tmp->next;
    }
    return i;
}

void print_list(Node *head){
    Node *current = head;
    while(current != NULL){
        printf("%d ", current->value); //printanje elemenata liste dok trenutni element nije null
        current = current->next;
    }
    printf("\n");
}

int number_random(){
    srand(time(NULL));
    return rand() % 99 + 1; 
}

void insert(int data){
    Node *new = (Node *)malloc(sizeof(Node)); //alocira memoriju za node
    new->value = data; //node value je data u argumentu funkcije
    new->next = NULL; //next element nodea je null

    if(list == NULL) list = new; //ako je lista prazna, ona postaje taj node
    else{
        Node *tmp = list;  
        while(tmp->next != NULL) //trazi se kraj liste te se node stavlja na zadnje mjesto liste
            tmp = tmp->next;
        tmp->next = new;
    }
}

int at_index(int x){
    Node *tmp = list;
    for(int i = 0; i < x && tmp != NULL; i++)
        tmp = tmp->next;
    if (tmp == NULL) {
        printf("Error: Index out of bounds.\n");
        exit(1);
    }
    return tmp->value;
}

void delete_random(int random_index){
    if(list == NULL) return;

    Node *current = list;
    Node *previous = NULL;
    for(int i = 0; i < random_index; i++){ //trazenje elementa na mjestu random_index
        previous = current;         //te spremanje njegovog prethodnog clana
        current = current->next;
    }

    if(current != NULL){
        if(previous != NULL){
            previous->next = current->next; //next prethodnog postaje next elementa kojeg izbacujemo
        } else  
            list = current->next;
        free(current); //oslobađamo memoriju od izbačenog člana
    }
}


void *reader(void *arg){
    int id = *((int *)arg);

    while(1){
        sleep(id + 1);
        if(list_size() == 0) continue;
        int x = rand() % list_size();
        pthread_mutex_lock(&monitor.mutex); //lockamo monitor
        printf("t=%d    citac %d zeli citati element %d liste\n", t, id, x);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        monitor.reader_waiting++; //povecavamo broj citaca koji ceka
        while(monitor.deleter_waiting + monitor.deleter_count > 0) //ako ima ijedan deleter koji ceka ili radi citaci cekaju
            pthread_cond_wait(&monitor.read_cond, &monitor.mutex);
        if(x > list_size() - 1){
            monitor.reader_waiting--;
            printf("citac odustaje od citanja\n");
            pthread_mutex_unlock(&monitor.mutex);
            continue;
        }
        monitor.reader_count++; //povecavamo broj citaca koji radi
        monitor.reader_waiting--; //smanjujemo broj citaca koji ceka
        printf("t=%d    citac %d cita element %d liste (vrijednosti %d)\n", t, id, x, at_index(x));
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex); //unlockamo monitor

        sleep(rand() % 6 + 1);

        pthread_mutex_lock(&monitor.mutex);
        monitor.reader_count--; //nakon nekoliko sekundi reader izlazi
        if(monitor.reader_count == 0 && monitor.deleter_waiting > 0) 
            pthread_cond_signal(&monitor.delete_cond);
        printf("t=%d    citac %d vise ne koristi listu\n", t, id);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex);
    }
}

void *writer(void *arg){
    int id = *((int *)arg);
    
    while(1){
        sleep(id + 2);
        int number = number_random();
        pthread_mutex_lock(&monitor.mutex);
        printf("t=%d    pisac %d zeli dodati vrijednost %d u listu\n", t, id, number);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        monitor.writer_waiting++;
        while(monitor.deleter_waiting + monitor.deleter_count + monitor.writer_count > 0)
            pthread_cond_wait(&monitor.write_cond, &monitor.mutex);
        monitor.writer_count++;
        monitor.writer_waiting--;
        printf("t=%d    pisac %d zapocinje dodavanje vrijednosti %d na kraj liste\n", t, id, number);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex);
        sleep(rand() % 6 + 2);
        insert(number);

        pthread_mutex_lock(&monitor.mutex);
        monitor.writer_count--;
        if(monitor.deleter_waiting > 0)
            pthread_cond_signal(&monitor.delete_cond);
        else if(monitor.writer_waiting > 0)
            pthread_cond_signal(&monitor.write_cond);
        printf("t=%d    pisac %d dodao vrijednost %d na kraj liste\n", t, id, number);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex);
    }
}

void *deleter(void *arg){
    int id = *((int *)arg);

    while(1){ 
        sleep(id + 3);
        pthread_mutex_lock(&monitor.mutex);
        if(list_size() == 0) {
            if (monitor.deleter_waiting == 0) {
                pthread_cond_signal(&monitor.write_cond);
                pthread_cond_broadcast(&monitor.read_cond);
            } else {
                pthread_cond_broadcast(&monitor.delete_cond);
            }
            pthread_mutex_unlock(&monitor.mutex);
            continue;
        }
        int random_index = rand() % list_size();
        printf("t=%d    brisac %d zeli obrisati element %d liste\n", t, id, random_index);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        monitor.deleter_waiting++;
        while(monitor.deleter_count + monitor.reader_count + monitor.writer_count > 0)
            pthread_cond_wait(&monitor.delete_cond, &monitor.mutex);
        if(random_index > list_size() - 1){
            monitor.deleter_waiting--;
            printf("brisac odustaje od brisanja\n");
            pthread_mutex_unlock(&monitor.mutex);
            continue;
        }
        monitor.deleter_count++;
        monitor.deleter_waiting--;
        int y = at_index(random_index);
        printf("t=%d    brisac %d zapocinje s brisanjem elementa %d liste (vrijednost=%d)\n", t, id, random_index, y);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex);

        sleep(rand() % 6 + 2);
        delete_random(random_index);

        pthread_mutex_lock(&monitor.mutex);
        monitor.deleter_count--;
        if(monitor.deleter_waiting > 0)
            pthread_cond_signal(&monitor.delete_cond);
        else if(monitor.writer_waiting > 0)
            pthread_cond_signal(&monitor.write_cond);
        else if(monitor.reader_waiting > 0)
            pthread_cond_broadcast(&monitor.read_cond);
        printf("t=%d    brisac %d obrisao element liste %d (vrijednost=%d)\n", t, id, random_index, y);
        printf("t=%d    aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", t, monitor.reader_count, monitor.writer_count, monitor.deleter_count);
        printf("t=%d    Lista: ", t);
        print_list(list);
        printf("\n");
        pthread_mutex_unlock(&monitor.mutex);
    }
}



int main(){
    srand(time(NULL));
    initialize_monitor(&monitor);

    pthread_t readers[10];
    int reader_ids[10];
    for (int i = 0; i < 10; i++) {
        reader_ids[i] = i;
        pthread_create(&readers[i], NULL, reader, (void *)&reader_ids[i]);
    }

    pthread_t writers[2];
    int writer_ids[2] = {0, 1};
    for (int i = 0; i < 2; i++) {
        pthread_create(&writers[i], NULL, writer, (void *)&writer_ids[i]);
    }

    pthread_t deleters[2];
    int deleter_ids[2] = {0, 1};
    for (int i = 0; i < 2; i++) {
        pthread_create(&deleters[i], NULL, deleter, (void *)&deleter_ids[i]);
    }

    while (1) {
        sleep(1);
        t++;
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(writers[i], NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(deleters[i], NULL);
    }

    return 0;
}