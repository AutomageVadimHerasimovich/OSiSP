#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "general_func.h"
#include <time.h>
#include <signal.h>

int8_t main_flag=1;

void exit_handler(int signal){
    main_flag=0;
}

int main(int argc, char* argv[]){
    srand(time(NULL));
    signal(SIGUSR1, exit_handler);
    struct sembuf sops = { 0, 0, 0 };

    key_t key = ftok("ftokfile.txt", 'A'); // генерируем ключ для сегмента общей памяти
    key_t key_sem = key + 1;
    key_t key_produser = key + 2;
    key_t key_consumer = key + 3;

    int is_blocked_sem = semget(key_sem, 1, IPC_CREAT | 0666);
    int full = semget(key_produser, 1, IPC_CREAT | 0666);
    int empty = semget(key_consumer, 1, IPC_CREAT | 0666);

    int shmid = shmget(key, sizeof(struct ring_buffer), 0666); // получаем идентификатор сегмента общей памяти
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    struct ring_buffer *shared_buffer = (struct ring_buffer *)shmat(shmid, NULL, 0); // присоединяем сегмент к адресному пространству процесса
    if (shared_buffer == (void *)-1) {
        perror("shmat");
        exit(1);
    }


    while(main_flag){
        
        if (check_buffer(shared_buffer)==1){
            printf("\n%s<---------PRODUCER %s--------->%s\n",YELLOW, argv[0],RESET);
            printf ("Buffer is full\n");
            printf("%s<---------PRODUCER %s--------->%s\n\n",YELLOW, argv[0],RESET);
        }
        else {
            struct message msg = generateMessage();
            set_sops(&sops, 0,-1,0); // уменьшение заполненности
            semop(empty, &sops, 1);

            set_sops(&sops,0,-1,0);
            semop(is_blocked_sem,&sops,1); // забираем буфер себе

            int n = add_message(shared_buffer, msg);
            
            if (n!=-1) {
                printf("%s\n<---------PRODUCER %s--------->\n%s",YELLOW, argv[0],RESET);
    
                printf("%stype -%s %d\n",YELLOW,RESET, msg.type);
                printf("%shash -%s %d\n",YELLOW,RESET, msg.hash);
                printf("%ssize -%s %d\n",YELLOW,RESET, msg.size);
                printf("%sdata - %s",YELLOW,RESET);
                for (int i = 0; i < msg.size; i++) {
                    printf("%c", msg.data[i]);
                }
                printf("%s\n<---------PRODUCER %s--------->\n\n%s",YELLOW, argv[0],RESET);
            }

            set_sops(&sops,0,1,0);
            semop(is_blocked_sem, &sops, 1);    

            set_sops(&sops,0,1,0);
            semop(full, &sops, 1);
        }

        sleep(3);       
    }
    shmdt(shared_buffer);   
    return 0;
}