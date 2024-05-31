#include "general_func.h"
#include "main_func.h"

// Глобальные переменные
procces* consumers_array;
procces* producers_array;
int current_num_producers =0;
int current_num_consumers =0;
struct message null_message;

void sigint_handler(int signal){
    delete_all_and_exit(); // Удаление всех процессов и выход из программы
}

int main() {
    srand(time(NULL)); // Инициализация генератора случайных чисел
    signal(SIGINT, sigint_handler); // Установка обработчика сигнала SIGINT

    struct message null_message = create_null_message(); // Создание пустого сообщения
    key_t key = ftok("ftokfile.txt", 'A'); // Генерация ключа для сегмента общей памяти
    key_t key_sem = key + 1; // Генерация ключа для семафора
    key_t key_produser = key + 2; // Генерация ключа для производителя
    key_t key_consumer = key + 3; // Генерация ключа для потребителя

    int is_blocked_sem = semget(key_sem, 1, IPC_CREAT | 0666); // Создание семафора
    int full = semget(key_produser, 1, IPC_CREAT | 0666); // Создание семафора для производителя
    int empty = semget(key_consumer, 1, IPC_CREAT | 0666); // Создание семафора для потребителя

    union semun { // Объявление объединения для управления семафорами
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;

    arg.val = 1; // Инициализация одноместного семафора для доступа к буферу
    if (semctl(is_blocked_sem, 0, SETVAL, arg) == -1) { // Установка значения семафора
        perror("semctl");
        exit(1);
    }
    arg.val = NUM_MESSAGES-3; // Инициализация семафора для отслеживания свободных мест в буфере
    if (semctl(empty, 0, SETVAL, arg) == -1) { // Установка значения семафора
        perror("semctl");
        exit(1);
    }
    arg.val = 3; // Инициализация семафора для отслеживания сообщения в буфере
    if (semctl(full, 0, SETVAL, arg) == -1) { // Установка значения семафора
        perror("semctl");
        exit(1);
    }

    int shmid = shmget(key, sizeof(struct ring_buffer)+256*NUM_MESSAGES, IPC_CREAT | 0666); // Создание сегмента общей памяти
    if (shmid == -1) { // Проверка на успешное создание сегмента общей памяти
        perror("shmget"); // если создание сегмента общей памяти не удалось
        exit(1);
    }
    struct ring_buffer *shared_buffer = (struct ring_buffer *)shmat(shmid, NULL, 0); // Присоединение сегмента к адресному пространству процесса
    if (shared_buffer == (void *)-1) { // Проверка на успешное присоединение сегмента
        perror("shmat"); // если присоединение сегмента не удалось
        exit(1);
    }

    init_buffer(shared_buffer); // Инициализация буфера
    add_message(shared_buffer, generateMessage()); // Добавление сообщения в буфер
    add_message(shared_buffer, generateMessage());
    add_message(shared_buffer, generateMessage());

    printf("\n%s>---------MAIN--------<\n", GREEN); // Вывод информационного сообщения

    for (int i=0;i<3;i++){ // Цикл для вывода информации о сообщениях в буфере

        printf ("%sType:%s %d\n",GREEN,RESET, shared_buffer->messages[i].type); // Вывод типа сообщения
        printf ("%sHash%s: %d\n",GREEN,RESET, shared_buffer->messages[i].hash); // Вывод хеша сообщения
        printf ("%sSize:%s %d\n",GREEN,RESET, shared_buffer->messages[i].size); // Вывод размера сообщения
        printf ("%sData: %s",GREEN,RESET); // Вывод данных сообщения
        for (int j=0;j<shared_buffer->messages[i].size;j++){ // Цикл для вывода данных сообщения
            printf ("%c", shared_buffer->messages[i].data[j]); // Вывод данных сообщения
        }
        printf ("\n");
    }
    printf("%s>---------MAIN---------<%s\n\n",GREEN,RESET);
    print_menu();

    char* buf = (char*)malloc(sizeof(char)*10); // Выделение памяти под буфер для ввода
    while(1){
        OPTIONS num = get_option(buf,10); // Получение выбранного пользователем пункта меню
        switch (num){ // Обработка выбранного пункта меню
            case CREATE_PRODUCER_OPTION:
                create_producer(); // Создание производителя
                break;
            case CREATE_CONSUMER_OPTION:
                create_consumer(); // Создание потребителя
                break;
            case DELETE_CONSUMER_OPTION:
                delete_last_consumer(); // Удаление последнего потребителя
                break;
            case DELETE_PRODUCER_OPTION:
                delete_last_producer(); // Удаление последнего производителя
                break;
            case QUEUE_STAT_OPTION:
                print_queue_stat(shared_buffer); // Вывод статистики очереди
                break;
            case PRODUCER_STAT_OPTION:
                print_producers_stat(); // Вывод статистики производителей
                break;
            case CONSUMER_STAT_OPTION:
                print_consumers_stat(); // Вывод статистики потребителей
                break;
            case Q_OPTION:
                delete_all_and_exit(); // Удаление всех процессов и выход из программы
                break;
        }
    }
}