#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* fje za obradu dogadaja, sigterma i siginta */
void obradi_sigterm(int sig);
void obradi_sigint(int sig);
void obradi_sigusr1(int sig);
void obradi_sigusr2(int sig);

int nije_kraj = 1;

int main(){
    struct sigaction act;

    /* sigusr1 */
    act.sa_handler = obradi_sigusr1;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGUSR2);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    /* sigusr2 */
    act.sa_handler = obradi_sigusr2;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGINT);
    act.sa_flags = 0;
    sigaction(SIGUSR2, &act, NULL);

    /* sigterm */
    act.sa_handler = obradi_sigterm;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGUSR2);
    sigaddset(&act.sa_mask, SIGINT);
    act.sa_flags = 0;
    sigaction(SIGTERM, &act, NULL);

    /* sigint */
    act.sa_handler = obradi_sigint;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGUSR2);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    printf("Program PID: %ld krece s radom\n", (long)getpid());

    //simulacija posla koji program radi
    int i = 1;
    while(nije_kraj){
        printf("Program: iteracija %d\n", i++);
        sleep(1);
    }

    printf("Program PID: %ld zavrsio s radom\n", (long)getpid());
    
    return 0;
}

void obradi_sigusr1(int sig){
    printf("Pocetak obrade signala SIGUSR1 %d\n", sig);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    for(int i = 1; i <= 10; i++){
        printf("Obrada signala SIGUSR1 %d: (%d)\n", sig, i);
        sleep(1);
    }

    printf("Kraj obrade signala SIGUSR1 %d\n", sig);
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void obradi_sigusr2(int sig){
    printf("Pocetak obrade signala SIGUSR2 %d\n", sig);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    for(int i = 1; i <= 10; i++){
        printf("Obrada signala SIGUSR2 %d: (%d)\n", sig, i);
        sleep(1);
    }

    printf("Kraj obrade signala SIGUSR2 %d\n", sig);
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void obradi_sigterm(int sig){
    printf("Pocetak obrade signala SIGTERM %d\n", sig);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    for(int i = 1; i <= 10; i++){
        printf("Obrada signala SIGTERM %d: (%d)\n", sig, i);
        sleep(1);
    }

    printf("Kraj obrade signala SIGTERM %d, spremam i izlazim\n", sig);
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    nije_kraj = 0;
}

void obradi_sigint(int sig){
    printf("Pocetak obrade signala SIGINT %d\n", sig);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    for(int i = 1; i <= 10; i++){
        printf("Obrada signala SIGINT %d: (%d)\n", sig, i);
        sleep(1);
    }

    printf("Kraj obrade signala SIGINT %d\n", sig);
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    exit(1);
}