#include "general_func.h" // Подключение заголовочного файла с общими функциями

unsigned char calculateHash(unsigned char *data, unsigned char size) {
    unsigned int hash = 2166136261;  // Начальное значение хеша
    for (unsigned int i = 0; i < size; i++) {
        hash ^= data[i]; // Применение операции XOR к каждому байту данных
        hash *= 16777619; // Умножение хеша на простое число
    }
    return hash;
}

int check_buffer(struct ring_buffer* buffer){
    int num_in_queue = buffer->count_added-buffer->count_extracted; // Вычисление числа элементов в очереди
    if (num_in_queue==0){
        return -1; // Очередь пуста
    }
    if (num_in_queue>=NUM_MESSAGES){
        return 1;  // Очередь полная
    }
    return 0; // В очереди есть место
}

char generateRandomLetter() {
    char letter;
    int randomCase = rand() % 2; // Генерация случайного числа 0 или 1

    if (randomCase == 0) {
        letter = 'a' + rand() % 26; // Генерация случайной строчной буквы
    } else {
        letter = 'A' + rand() % 26; // Генерация случайной заглавной буквы
    }

    return letter; // Возвращение сгенерированной буквы
}

struct message generateMessage() {
    struct message msg;

    msg.type = rand() % 2; // Генерация случайного типа сообщения

    int size = rand() % 257; // Генерация случайного размера сообщения
    while(size == 0){
        size = rand() % 257; // Повторная генерация, если размер равен 0
    }
    if (size == 256){
        size = 0; // Если размер равен 256, устанавливаем его равным 0
    }
    msg.size = size;

    int i;
    for ( i = 0; i < size-1; i++) {
        msg.data[i] = generateRandomLetter(); // Генерация случайного байта данных
    }
    msg.data[i] = '\0'; // Добавление нулевого символа в конец данных

    msg.hash = calculateHash(msg.data, size); // Вычисление хеша сообщения

    return msg; // Возвращение сгенерированного сообщения
}

void init_buffer(struct ring_buffer *buffer){
    buffer->head = 0; // Инициализация головы буфера
    buffer->tail = 0; // Инициализация хвоста буфера
    buffer->count_added = 0; // Инициализация счетчика добавленных сообщений
    buffer->count_extracted = 0; // Инициализация счетчика извлеченных сообщений
}

struct message create_null_message(){
    struct message msg;
    msg.hash=0; // Установка хеша сообщения равным 0
    msg.size=0; // Установка размера сообщения равным 0
    msg.type=0; // Установка типа сообщения равным 0
    for (int i=0;i<256;i++){
        msg.data[i]=0; // Установка каждого байта данных равным 0
    }
    return msg; // Возвращение созданного сообщения
}

void set_sops(struct sembuf *sops,int a, int b, int c){
    sops->sem_num = a; // Установка номера семафора
    sops->sem_op = b; // Установка операции над семафором
    sops->sem_flg = c; // Установка флагов семафора
}

int add_message(struct ring_buffer* buffer, struct message msg) {
    if (buffer->count_added-buffer->count_extracted>=NUM_MESSAGES){ // Если в буфере нет места
        return -1; // Возвращение ошибки
    }
    else {
        buffer->count_added++; // Увеличение счетчика добавленных сообщений
        buffer->messages[buffer->tail]=msg; // Добавление сообщения в буфер
        buffer->tail++; // Увеличение индекса хвоста буфера
        if (buffer->tail == NUM_MESSAGES){
            buffer->tail = 0; // Если индекс хвоста достиг максимального значения, устанавливаем его равным 0
        }
        return 0; // Возвращение успешного результата
    }
}

struct message extract_message(struct ring_buffer* buffer) {
    struct message null_message = create_null_message(); // Создание пустого сообщения

    if (buffer->count_added - buffer->count_extracted > 0) { // Если в буфере есть сообщения
        struct message msg = buffer->messages[buffer->head]; // Извлечение сообщения из буфера
        buffer->messages[buffer->head] = null_message; // Замена извлеченного сообщения на пустое
        buffer->count_extracted++; // Увеличение счетчика извлеченных сообщений
        buffer->head++; // Увеличение индекса головы буфера
        if (buffer->head==NUM_MESSAGES){
            buffer->head=0; // Если индекс головы достиг максимального значения, устанавливаем его равным 0
        }

        return msg; // Возвращение извлеченного сообщения
    } else {
        return null_message; // Возвращение пустого сообщения, если в буфере нет сообщений
    }
}
