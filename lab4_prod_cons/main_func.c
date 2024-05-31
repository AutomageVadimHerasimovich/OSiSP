
#include "main_func.h"
extern procces* consumers_array;
extern procces* producers_array;
extern int current_num_producers;
extern int current_num_consumers;

void print_menu(){
    printf ("Menu:\n");
    printf ("+p - create producer\n");
    printf ("-p - create producer\n");
    printf ("+c - create consumer\n");
    printf ("-c - create consumer\n");
    printf ("ps - print producer stat\n");
    printf ("cs - print consumer stat\n");
    printf ("qs - print queue stat\n");
    printf ("q - exit\n");
}

void delete_all_and_exit(){
    while(current_num_consumers>0){
        delete_last_consumer();
    }
    while(current_num_producers>0){
        delete_last_producer();
    }
    exit(0);
}

OPTIONS get_option(char* buf, int num){
    while (1){
        fgets(buf, num, stdin);

        if (buf[0]=='+' && buf [1]=='p' && strlen(buf)==3){
            return CREATE_PRODUCER_OPTION;
        }
        else if (buf[0]=='-' && buf[1]=='p' && strlen(buf)==3){
            return DELETE_PRODUCER_OPTION;
        }
        else if (buf[0]=='+' && buf [1]=='c' && strlen(buf)==3){
            return CREATE_CONSUMER_OPTION;
        }
        else if (buf[0]=='-' && buf[1]=='c' && strlen(buf)==3){
            return DELETE_CONSUMER_OPTION;
        }
        else if (buf[0]=='q' && buf[1]=='s' && strlen(buf)==3){
            return QUEUE_STAT_OPTION;
        }
        else if (buf[0]=='p' && buf[1] == 's' && strlen(buf)==3){
            return PRODUCER_STAT_OPTION;
        }
        else if (buf[0]=='c' && buf[1] == 's' && strlen(buf)==3){
            return CONSUMER_STAT_OPTION;
        }
        else if (buf[0]=='q' && strlen(buf)==2){
            return Q_OPTION;
        }
        printf ("Error input - try again:\n");
    }
}

void create_producer(){
    //ls -l /proc/<pid>/fd/0 - в ответ придет строка, которую указываем в open
    int new_terminal = open("/dev/pts/1", O_RDWR);

    pid_t temp_pid = fork();      
    current_num_producers++;
    producers_array = (procces*)realloc(producers_array, current_num_producers * sizeof(procces));  

    sprintf(producers_array[current_num_producers-1].name, "P_%d", current_num_producers);
    producers_array[current_num_producers-1].pid = temp_pid;

    if (temp_pid < 0) {
        fprintf(stderr,"%s\n" ,strerror(errno));
        exit(1);
    } else if (temp_pid == 0) {
        if (new_terminal!=-1){
            dup2(new_terminal, STDIN_FILENO);
            dup2(new_terminal, STDOUT_FILENO);
            dup2(new_terminal, STDERR_FILENO);
        }
        execl("./producer.exe", producers_array[current_num_producers-1].name, NULL);
    } else {
        printf("Producer with PID %d(%s) - created\n", temp_pid, producers_array[current_num_producers-1].name);
        printf("Count of producers proceses - %d\n", current_num_producers);
    }
}

void create_consumer(){
    int new_terminal = open("/dev/pts/2", O_RDWR);

    pid_t temp_pid = fork();      
    current_num_consumers++;
    consumers_array = (procces*)realloc(consumers_array, current_num_consumers * sizeof(procces));  

    sprintf(consumers_array[current_num_consumers-1].name, "C_%d", current_num_consumers);
    consumers_array[current_num_consumers-1].pid = temp_pid;

    if (temp_pid < 0) {
        fprintf(stderr,"%s\n" ,strerror(errno));
        exit(1);
    } else if (temp_pid == 0) {
        if (new_terminal!=-1){
            dup2(new_terminal, STDIN_FILENO);
            dup2(new_terminal, STDOUT_FILENO);
            dup2(new_terminal, STDERR_FILENO);
        }
        execl("./consumer.exe", consumers_array[current_num_consumers-1].name, NULL);
    } else {
        printf("Consumer with PID %d(%s) - created\n", temp_pid, consumers_array[current_num_consumers-1].name);
        printf("Count of consumers proceses - %d\n", current_num_consumers);
    }
}

void delete_last_consumer(){
    if (current_num_consumers>0){
        int status;
        kill(consumers_array[current_num_consumers-1].pid,SIGUSR1);
        waitpid(consumers_array[current_num_consumers-1].pid, &status, 0);
        printf ("Consumer %s deleted\n",consumers_array[current_num_consumers-1].name);
        current_num_consumers--;
        consumers_array = (procces*)realloc(consumers_array, current_num_consumers*sizeof(procces));
    }
    else {
        printf("There are no consumers to delete\n");
    }
}
void delete_last_producer(){
    if (current_num_producers > 0) {
        int status;
        kill(producers_array[current_num_producers - 1].pid, SIGUSR1);
        waitpid(producers_array[current_num_producers - 1].pid, &status, 0);
        printf("Producer %s deleted\n", producers_array[current_num_producers - 1].name);
        current_num_producers--;
        producers_array = (procces*)realloc(producers_array, current_num_producers * sizeof(procces));
    } else {
        printf("There are no producers to delete\n");
    }
}
void print_queue_stat(struct ring_buffer* buffer){
    printf ("Stat of queue:\n");
    int num_in_queue  = buffer->count_added-buffer->count_extracted;
    printf ("Num elements in queue: %d\n", num_in_queue);
    printf ("Num of free slots %d\n", NUM_MESSAGES-num_in_queue);
    printf ("Count_added - %d\n", buffer->count_added);
    printf ("Count_extrected - %d\n", buffer->count_extracted);
    /*int n = buffer->count_added - buffer->count_extracted;
    for (int i=0,j=buffer->head;i<n;i++){
        printf ("Message %d\n", i+1);
        printf ("   Type: %d\n", buffer->messages[j].type);
        printf ("   Hash: %d\n", buffer->messages[j].hash);
        printf ("   Size: %d\n   Data: ", buffer->messages[j].size);
        for (int k=0;k<buffer->messages[j].size;k++){
            printf ("%c", buffer->messages[j].data[k]);
        }
        printf ("\n");
        j++;
        if (j==NUM_MESSAGES){
            j=0;
        }
    }*/
}
void print_producers_stat(){
    printf ("Producers processes:\n");
    if (current_num_producers>0){
        for (int i=0;i<current_num_producers;i++){
            printf ("Name: %s, ppid: %d, pid: %d\n", producers_array[i].name, getpid(), producers_array[i].pid);
        }
    }
    else {
        printf ("There are no producers\n");
    }
}
void print_consumers_stat(){
    printf("Consumers processes:\n");
    if (current_num_consumers > 0) {
        for (int i = 0; i < current_num_consumers; i++) {
            printf("Name: %s, ppid: %d, pid: %d\n", consumers_array[i].name, getpid(), consumers_array[i].pid);
        }
    } else {
     printf("There are no consumers\n");
    }
}