#pragma once
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include "general_func.h"
typedef enum{
    CREATE_PRODUCER_OPTION,
    DELETE_PRODUCER_OPTION,
    CREATE_CONSUMER_OPTION,
    DELETE_CONSUMER_OPTION,
    QUEUE_STAT_OPTION,
    PRODUCER_STAT_OPTION,
    CONSUMER_STAT_OPTION,
    Q_OPTION
}OPTIONS;

typedef struct {
    pid_t pid;          //ID процесса
    char name[12];      //название
}procces;

void delete_all_and_exit();
void create_producer();
void create_consumer();
void print_menu();
OPTIONS get_option(char* buf, int num);
void delete_last_consumer();
void delete_last_producer();
void print_queue_stat(struct ring_buffer*);
void print_producers_stat();
void print_consumers_stat();