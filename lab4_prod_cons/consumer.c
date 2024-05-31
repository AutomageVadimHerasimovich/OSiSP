
#include "general_func.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
int8_t main_flag=1;

void exit_handler(int signal){
    main_flag=0;
}
int check_hash(struct message* msg){
    return msg->hash==calculateHash(msg->data, msg->size);
}
int compareMessages(struct message msg1, struct message msg2) {
    if (msg1.type != msg2.type) {
        return 0;
    }
    
    if (msg1.hash != msg2.hash) {
        return 0;
    }
    
    if (msg1.size != msg2.size) {
        return 0;
    }
    
    for (int i=0;i<msg1.size;i++){
        if (msg1.data[i]!=msg2.data[i]){
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char* argv[]){
    
    struct message null_message = create_null_message();
    struct sembuf sops = { 0, 0, 0 };

    signal(SIGUSR1, exit_handler);

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

        if (check_buffer(shared_buffer)==-1){
            printf("\n%s>---------CONSUMER %s---------<%s\n", RED,argv[0],RESET);
            printf ("Buffer is empty");
            printf("\n%s>---------CONSUMER %s---------<%s\n\n",RED, argv[0],RESET);
        }
        else {
            set_sops(&sops,0,-1,0);
            semop(full, &sops, 1);

            set_sops(&sops,0,-1,0);
            semop(is_blocked_sem, &sops, 1);
        
            struct message msg = extract_message(shared_buffer);
            if (!check_hash(&msg)){
                printf ("Error in message!\n");
            }
            else {
                printf("\n%s>---------CONSUMER %s---------<%s\n",RED, argv[0],RESET);
                printf("%stype -%s %d\n", RED, RESET, msg.type);
                printf("%shash -%s %d\n", RED, RESET, msg.hash);
                printf("%ssize -%s %d\n", RED, RESET, msg.size);
                printf("%sdata -%s ",RED,RESET);
                for (int i = 0; i < msg.size; i++) {
                    printf("%c", msg.data[i]);
                }
                printf("\n%s>---------CONSUMER %s---------<%s\n\n", RED, argv[0], RESET);
            }
        
            set_sops(&sops,0,1,0);
            semop(is_blocked_sem, &sops, 1);

            set_sops(&sops,0,1,0);
            semop(empty, &sops, 1);
        }
        
        sleep(3);
    }

    shmdt(shared_buffer); // отсоединяем сегмент от адресного пространства процесса
    return 0;
}
