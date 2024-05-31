#define _GNU_SOURCE // Подключение расширений GNU.

#include <sys/wait.h> // Подключение библиотеки для работы с системными вызовами ожидания.
#include <sys/ipc.h> // Подключение библиотеки для работы с IPC (межпроцессорное взаимодействие).
#include <stdio.h> // Подключение стандартной библиотеки ввода-вывода.
#include <stdlib.h> // Подключение библиотеки для работы с памятью, процессами.
#include <stdint.h> // Подключение библиотеки для работы с целочисленными типами.
#include <unistd.h> // Подключение библиотеки для работы с системными вызовами POSIX.
#include <string.h> // Подключение библиотеки для работы со строками.
#include <semaphore.h> // Подключение библиотеки для работы с семафорами.
#include <sys/mman.h> // Подключение библиотеки для работы с памятью.
#include <fcntl.h> // Подключение библиотеки для работы с файлами и файловыми дескрипторами.
#include <time.h> // Подключение библиотеки для работы со временем.
#include <pthread.h> // Подключение библиотеки для работы с потоками.

#define MAX_QUEUE_SIZE 25 // Максимальный размер очереди.
#define MIN_QUEUE_SIZE 5 // Минимальный размер очереди.
#define MAX_CHILD_COUNT 5 // Максимальное количество дочерних процессов.
int CURRENT_MAX_QUEUE_SIZE = 15; // Текущий максимальный размер очереди.

sem_t *spare_msgs , *pick_msgs , *mutex; // Объявление семафоров.

pthread_t consumers[MAX_CHILD_COUNT]; // Массив потоков-потребителей.
char* consumers_name[MAX_CHILD_COUNT]; // Массив имен потоков-потребителей.
int consumers_count; // Количество потоков-потребителей.
pthread_t producers[MAX_CHILD_COUNT]; // Массив потоков-производителей.
char* producers_name[MAX_CHILD_COUNT]; // Массив имен потоков-производителей.
int producers_count; // Количество потоков-производителей.

typedef struct{ // Определение структуры сообщения.
    uint8_t type; // Тип сообщения.
    uint16_t hash; // Хеш сообщения.
    uint8_t size; // Размер сообщения.
    char* data; // Данные сообщения.
} message;

typedef struct{ // Определение структуры очереди.
    message* head; // Голова очереди.
    int h; // Индекс головы очереди.
    message* tail; // Хвост очереди.
    int t; // Индекс хвоста очереди.
    message* buff; // Буфер очереди.
    int count_added; // Количество добавленных сообщений.
    int count_extracted; // Количество извлеченных сообщений.
}queue;

queue *message_queue; // Очередь сообщений.

// Далее идут функции для работы с сообщениями и очередью, а также функции потоков-производителей и потоков-потребителей.


uint8_t getSize(){
    int size = 0;
    // Генерируем случайное число от 0 до 256 включительно
    while(size == 0)size = rand() % 257;
    // Если полученное число равно 256, заменяем его на 0
    if(size == 256)size=0;
    return size;
}

uint8_t getType(uint8_t size){
    // Если размер больше 128, возвращаем 1, иначе 0
    if(size>128)return 1;
    else return 0;
}

char* getData(uint8_t size){
    // Если размер равен 0, заменяем его на 256
    if(size==0)size=(uint8_t)256;
    // Выделяем память под данные
    char* data = (char*) malloc(size);
    // Если память не выделена, выводим ошибку
    if(data==NULL)perror("queue message data");
    // Генерируем случайные данные
    srand(time(NULL));
    for(int i = 0; i < size; i++)data[i] = (rand() % 26) + 'a';
    return data;
}

uint16_t FNV1_HASH(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *) data;
    uint16_t hash = 0x811C;
    // Вычисляем хеш по алгоритму FNV1
    for (size_t i = 0; i < size; ++i) {
        hash = (hash * 0x0101) ^ bytes[i];
    }
    return hash;
}

message* createMessage(){
    message* msg;
    // Выделяем память под сообщение
    msg = (message *) malloc(sizeof(*msg));
    // Заполняем поля сообщения
    msg->size = getSize();
    msg->data = getData(((msg->size + 3) / 4) * 4);
    msg->type = getType(msg->size);
    msg->hash = 0;
    msg->hash = FNV1_HASH(msg->data, strlen(msg->data));

    return msg;
}

void deleteConsumers(){
    // Если нет потребителей, выводим сообщение
    if(consumers_count == 0){
        printf("No consumers.");
        return;
    }
    // Уменьшаем количество потребителей
    consumers_count--;
    // Отменяем поток потребителя
    pthread_cancel(consumers[consumers_count]);
    // Выводим сообщение об удалении потребителя
    printf("Was delete consumer with name:%s\n",consumers_name[consumers_count]);
    // Освобождаем память от имени потребителя
    free(consumers_name[consumers_count]);
}

void deleteProducers(){
    // Если нет производителей, выводим сообщение
    if(producers_count == 0){
        printf("No consumers.");
        return;
    }
    // Уменьшаем количество производителей
    producers_count--;
    // Отменяем поток производителя
    pthread_cancel(producers[producers_count]);
    // Выводим сообщение об удалении производителя
    printf("Was delete producer with name:%s\n",producers_name[producers_count]);
    // Освобождаем память от имени производителя
    free(producers_name[producers_count]);
}

void start() {
    // Выделяем память под семафоры
    spare_msgs = malloc(sizeof(sem_t));
    pick_msgs = malloc(sizeof(sem_t));
    mutex = malloc(sizeof(sem_t));

    // Если память не выделена, выводим ошибку и завершаем программу
    if(spare_msgs == NULL || pick_msgs == NULL || mutex == NULL) {
        perror("Failed to allocate memory for semaphores");
        exit(EXIT_FAILURE);
    }

    // Выделяем память под очередь сообщений
    message_queue = malloc(sizeof(queue));
    if(message_queue == NULL) {
        perror("Failed to allocate memory for message queue");
        exit(EXIT_FAILURE);
    }

    // Инициализируем очередь нулями
    memset(message_queue, 0, sizeof(queue));

    // Выделяем память под буфер очереди
    message_queue->buff = (message *)malloc(15 * sizeof(message));

    // Инициализируем семафоры
    sem_init(spare_msgs, 0, CURRENT_MAX_QUEUE_SIZE);
    sem_init(pick_msgs, 0, 0);
    sem_init(mutex, 0, 1);
}

void fromProgExit(){
    // Удаляем всех потребителей и производителей
    while (consumers_count) deleteConsumers();
    while (producers_count) deleteProducers();

    // Уничтожаем семафоры
    sem_destroy(mutex);
    sem_destroy(spare_msgs);
    sem_destroy(pick_msgs);

    // Освобождаем память от семафоров и очереди
    free(mutex);
    free(spare_msgs);
    free(pick_msgs);
    free(message_queue);

    // Завершаем программу
    exit(0);
}

void viewStatus(){
    // Выводим статус очереди и количество потребителей и производителей
    printf("Queue max size:%d\n",CURRENT_MAX_QUEUE_SIZE);
    printf("Current size:%d\n",message_queue->count_added-message_queue->count_extracted);
    printf("Added:%d \nExtracted:%d\n",message_queue->count_added,message_queue->count_extracted);
    printf("Consumers:%d\n",consumers_count);
    printf("Producers:%d\n",producers_count);
}

int addMessage(message* msg){
    // Ожидаем, пока в очереди появится свободное место
    sem_wait(spare_msgs);
    // Блокируем доступ к очереди
    sem_wait(mutex);

    // Если очередь полная, выводим сообщение и возвращаем -1
    if(message_queue->count_added-message_queue->count_extracted==CURRENT_MAX_QUEUE_SIZE){
        printf("Cannot add message queue is full.\n");
        return -1;
    }

    // Если очередь пустая, добавляем сообщение в начало
    if(message_queue->count_added-message_queue->count_extracted==0){
        message_queue->buff[0] = *msg;
        message_queue->head = message_queue->tail = &message_queue->buff[0];
    }else message_queue->t++; // Иначе увеличиваем индекс хвоста

    // Увеличиваем количество добавленных сообщений
    message_queue->count_added++;
    // Обновляем индекс хвоста и добавляем сообщение в очередь
    message_queue->t = message_queue->t%CURRENT_MAX_QUEUE_SIZE;
    message_queue->buff[message_queue->t]=*msg;
    message_queue->tail = &message_queue->buff[message_queue->t];

    // Разблокируем доступ к очереди и увеличиваем количество сообщений для извлечения
    sem_post(pick_msgs);
    sem_post(mutex);
    return message_queue->count_added;
}

int extractedMessage(message **msg){
    // Ожидаем, пока в очереди появится сообщение для извлечения
    sem_wait(pick_msgs);
    // Блокируем доступ к очереди
    sem_wait(mutex);

    // Если очередь пустая, выводим сообщение и возвращаем -1
    if(message_queue->count_added == message_queue->count_extracted){
        printf("Queue is empty.");
        return -1;
    }
    // Извлекаем сообщение из головы очереди
    *msg = message_queue->head;
    // Увеличиваем количество извлеченных сообщений и индекс головы
    message_queue->count_extracted++;
    message_queue->h++;
    message_queue->h = message_queue->h % CURRENT_MAX_QUEUE_SIZE;
    message_queue->head = &message_queue->buff[message_queue->h];

    // Разблокируем доступ к очереди и увеличиваем количество свободных мест в очереди
    sem_post(spare_msgs);
    sem_post(mutex);
    return message_queue->count_extracted;
}

void *consumer_func(){
    message* msg;
    char name[20];
    // Формируем имя потребителя
    sprintf(name, "consumer_%02d", consumers_count);
    while(1){
        // Извлекаем сообщение из очереди
        int ret = extractedMessage(&msg);
        // Если сообщение извлечено, выводим информацию о нем
        if(ret != -1){
            printf("%s consumer message: HASH=%04X, counter_extracted=%d\n", name, msg->hash, ret);
        }
        // Задержка в 5 секунд
        sleep(5);
    }
    return NULL;
}

void *producer_func(){
    message* msg;
    char name[20];
    // Формируем имя производителя
    sprintf(name, "producer%02d", producers_count);
    while(1){
        // Создаем сообщение
        msg = createMessage();
        // Добавляем сообщение в очередь
        int ret = addMessage(msg);
        // Если сообщение добавлено, выводим информацию о нем
        if(ret != -1){
            printf("%s producer message: HASH=%04X, counter_added=%d\n", name, msg->hash, ret);
        }
        // Задержка в 5 секунд
        sleep(5);
    }
    return NULL;
}

void addConsumer(){
    // Если достигнуто максимальное количество потребителей, выводим сообщение
    if(consumers_count==MAX_CHILD_COUNT){
        printf("Max consumer size\n");
        return;
    }
    // Создаем поток потребителя
    pthread_create(&consumers[consumers_count], NULL, consumer_func, NULL);
    // Выделяем память под имя потребителя и формируем его
    consumers_name[consumers_count] = (char *) malloc(16);
    sprintf(consumers_name[consumers_count], "consumer_%02d", consumers_count);
    // Увеличиваем количество потребителей
    consumers_count++;
}

void addProducer(){
    // Если достигнуто максимальное количество производителей, выводим сообщение
    if(producers_count==MAX_CHILD_COUNT){
        printf("Max producer size\n");
        return;
    }
    // Создаем поток производителя
    pthread_create(&producers[producers_count], NULL, producer_func, NULL);
    // Выделяем память под имя производителя и формируем его
    producers_name[producers_count] = (char *) malloc(16);
    sprintf(producers_name[producers_count], "producer%02d", producers_count);
    // Увеличиваем количество производителей
    producers_count++;
}

void menu() {
    // Выводим меню
    printf("---------------------------------------\n");
    printf("'1' - to create producer.\n");
    printf("'2' - to remove producer.\n");
    printf("'3' - to create consumer.\n");
    printf("'4' - to remove consumer.\n");
    printf("'p' - view processes.\n");
    printf("'s' - view status.\n");
    printf("'-' - to decrement queue size.\n");
    printf("'+' - to increment queue size.\n");
    printf("'q' - to exit.\n");
    printf("---------------------------------------\n");
}

void viewProcesses(){
    // Если нет потребителей и производителей, возвращаемся
    if(consumers_count == 0 && producers_count == 0){
        return;
    }
    // Выводим имена всех потребителей
    for(int i = 0; i < consumers_count; i++) printf("%s\n",consumers_name[i]);
    // Выводим имена всех производителей
    for(int i = 0; i < producers_count; i++) printf("%s\n",producers_name[i]);
}
// Функция для уменьшения размера очереди
void dec_queue_func(){
    // Проверяем, достигнут ли минимальный размер очереди
    if(CURRENT_MAX_QUEUE_SIZE == MIN_QUEUE_SIZE){
        printf("Max queue size\n");
        return;
    }

    // Выделяем память под новый буфер очереди меньшего размера
    message* new_buff = (message*)malloc((CURRENT_MAX_QUEUE_SIZE - 1) * sizeof(message));
    // Проверяем, успешно ли выделена память
    if (new_buff == NULL) {
        perror("Memory with decrement queue.\n");
        return;
    }
    int new_cnt_size;

    // Если очередь пуста, обнуляем индексы головы и хвоста и ожидаем свободного места в очереди
    if((message_queue->count_added - message_queue->count_extracted) == 0){
        message_queue->h = message_queue->t = 0;
        sem_wait(spare_msgs);
    }else{

        // Если очередь полна, выводим сообщение и возвращаемся
        if((message_queue->count_added - message_queue->count_extracted) == CURRENT_MAX_QUEUE_SIZE){
            printf("Queue is full cant decrement.\n");
            return;
        }else{
            // Иначе, вычисляем новый размер очереди и ожидаем свободного места в очереди
            new_cnt_size = message_queue->count_added - message_queue->count_extracted;
            sem_wait(spare_msgs);
        }

        // Копируем сообщения из старого буфера в новый
        for(int i = 0; i < new_cnt_size; i++){
            new_buff[i] = message_queue->buff[message_queue->h];
            message_queue->h++;
            message_queue->h = message_queue->h % CURRENT_MAX_QUEUE_SIZE;
        }

        // Обнуляем индекс головы и устанавливаем индекс хвоста равным новому размеру очереди минус один
        message_queue->h = 0;
        message_queue->t = new_cnt_size - 1;

    }

    // Уменьшаем текущий максимальный размер очереди
    CURRENT_MAX_QUEUE_SIZE--;
    // Освобождаем память от старого буфера
    free(message_queue->buff);
    // Устанавливаем новый буфер
    message_queue->buff = new_buff;

}

// Функция для увеличения размера очереди
void inc_queue_func(){

    // Проверяем, достигнут ли максимальный размер очереди
    if(CURRENT_MAX_QUEUE_SIZE == MAX_QUEUE_SIZE){
        printf("Max queue size\n");
        return;
    }

    // Выделяем память под новый буфер очереди большего размера
    message* new_buff = (message*)malloc((CURRENT_MAX_QUEUE_SIZE + 1) * sizeof(message));

    // Проверяем, успешно ли выделена память
    if (new_buff == NULL) {
        perror("Memory with decrement queue.\n");
        return;
    }

    // Увеличиваем текущий максимальный размер очереди
    CURRENT_MAX_QUEUE_SIZE++;
    // Освобождаем память от старого буфера
    free(message_queue->buff);
    // Устанавливаем новый буфер
    message_queue->buff = new_buff;
    // Увеличиваем количество свободных мест в очереди
    sem_post(spare_msgs);

}

// Главная функция программы
int main(){

    // Инициализируем очередь и семафоры
    start();

    // Выводим сообщение о том, как получить информацию об использовании программы
    printf("Write 'u' to see usage.\n");

    // Основной цикл программы
    while (1) {
        char ch;
        // Очищаем буфер ввода
        rewind(stdin);
        // Читаем символ из ввода
        ch = getchar();
        // Обрабатываем введенный символ
        switch (ch) {
            case '1': {
                // Создаем производителя
                addProducer();
                break;
            }
            case '2': {
                // Удаляем производителя
                deleteProducers();
                break;
            }
            case '3': {
                // Создаем потребителя
                addConsumer();
                break;
            }
            case '4': {
                // Удаляем потребителя
                deleteConsumers();
                break;
            }
            case 'p': {
                // Выводим информацию о процессах
                viewProcesses();
                break;
            }
            case 'q': {
                // Завершаем программу
                fromProgExit();
                break;
            }
            case 's': {
                // Выводим статус очереди
                viewStatus();
                break;
            }
            case 'u': {
                // Выводим меню
                menu();
                break;
            }
            case '-': {
                // Уменьшаем размер очереди
                dec_queue_func();
                break;
            }
            case '+': {
                // Увеличиваем размер очереди
                inc_queue_func();
                break;
            }
            default:
                break;
        }
    }

    // Завершаем программу
    fromProgExit();

    return 0;
}