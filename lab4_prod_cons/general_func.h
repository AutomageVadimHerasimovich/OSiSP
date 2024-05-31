#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define NUM_MESSAGES 15

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"


struct message{
    char type;
    short hash;
    unsigned char size;
    unsigned char data[256];
};

struct ring_buffer{
    int head;
    int tail;
    int count_added;
    int count_extracted;
    struct message messages[NUM_MESSAGES];
};
int check_buffer(struct ring_buffer* buffer);
void set_sops(struct sembuf *sops,int a, int b, int c);
unsigned char calculateHash(unsigned char *data, unsigned char size);
struct message generateMessage();
int add_message(struct ring_buffer *buffer, struct message msg);
struct message extract_message(struct ring_buffer *buffer);
void init_buffer(struct ring_buffer *buffer);
struct message create_null_message();