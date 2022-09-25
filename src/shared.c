/**********
 * Hayden Coffey
 *
 **********/
#include <util.h>

size_t MESSAGE_SIZE;
size_t NUM_ITERATIONS;
size_t SHARED_BUFFER_SIZE;

// mmap shared memory
// At start of buffer: 1 byte for state, 4 bytes for message size
// State
// 0: Already processed
// 1: Data ready to be read
// 2: Sender has finished, terminate
struct BufferInfo
{
    unsigned int size;
    unsigned char written;
};

struct SharedSendArgs
{
    void *sharedBuffer;
    void *data; // Data to be sent
    size_t messageSize;
    size_t numMessages;
};

struct SharedRecieveArgs
{
    void *sharedBuffer;
    void *recieveBuffer;
};

long FILE_SIZE;
pthread_mutex_t memLock;

void *allocate_buffer(size_t size)
{
    return mmap(NULL, size + sizeof(struct BufferInfo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

struct BufferInfo *get_buffer_info(void *sharedBuffer)
{
    return (struct BufferInfo *)(sharedBuffer);
}

void *get_buffer_write_address(void *sharedBuffer)
{
    return sharedBuffer + sizeof(struct BufferInfo);
}

// Return true if write was completed
int shared_write(void *buffer, void *data, size_t messageSize, struct BufferInfo *bi)
{
    int writeret = 0;
    pthread_mutex_lock(&memLock);
    // If buffer is ready to be written to, copy new data in
    if (bi->written == 0)
    {
        memcpy(buffer, data, messageSize);
        bi->size = messageSize;
        bi->written = 1;

        writeret = 1;
    }
    pthread_mutex_unlock(&memLock);
    return writeret;
}

void shared_send_messages(void *buffer, struct BufferInfo *bi, void *data, size_t messageSize, size_t numMessages)
{
    int offset = 0; // Offset in data to be sent
    int m = 0;
    while (m < numMessages)
    {
        int writeret = shared_write(buffer, data + offset, MESSAGE_SIZE, bi);
        if (writeret)
        {
            offset += messageSize;
            m++;
        }
    }

    size_t remainderSize = FILE_SIZE - offset;
    // Send remainder data
    while (1)
    {
        int writeret = shared_write(buffer, data + offset, remainderSize, bi);
        if (writeret)
        {
            break;
        }
    }
}

void shared_send_kill(struct BufferInfo *bi)
{
    while (1)
    {
        pthread_mutex_lock(&memLock);
        if (bi->written == 0)
        {
            bi->written = 2;
            pthread_mutex_unlock(&memLock);
            break;
        }
        pthread_mutex_unlock(&memLock);
    }
}

void hc_shared_write_loop(void *sharedBuffer, void *data, size_t messageSize, size_t numMessages)
{
    // Retrieve buffer state and first writable position
    struct BufferInfo *bi = get_buffer_info(sharedBuffer);
    void *bufferStart = get_buffer_write_address(sharedBuffer);

    int i = 0;
    while (i < NUM_ITERATIONS)
    {
#ifdef SHOW_ITERATION
        printf("Iteration : %d\n", i);
#endif

        shared_send_messages(bufferStart, bi, data, messageSize, numMessages);
        i++;
    }

    // When done, wait for last sent message to be processed then terminate
    shared_send_kill(bi);
}

// If buffer has been written to, copy values in.
// Send confirmation by clearing written state
int shared_read(void *sharedBuffer, void *recieveBuffer, struct BufferInfo *bi)
{
    int readret = 0;
    pthread_mutex_lock(&memLock);
    if (bi->written == 1)
    {
        memcpy(recieveBuffer, sharedBuffer, bi->size);
        bi->written = 0;
    }
    else if (bi->written == 2)
    {
        readret = -1;
    }
    pthread_mutex_unlock(&memLock);

    return readret;
}

void hc_shared_read_loop(void *sharedBuffer, void *recieveBuffer)
{
    // Retrieve buffer state
    struct BufferInfo *bi = sharedBuffer;
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);

    while (1)
    {
        if (shared_read(sharedBuffer, recieveBuffer, bi) < 0)
        {
            break;
        }
    }
}

// Sender -----------------------
void *send_thread(void *args)
{
    // Read in arguments from pointer
    void *data = ((struct SharedSendArgs *)args)->data;
    void *sharedBuffer = ((struct SharedSendArgs *)args)->sharedBuffer;
    size_t messageSize = ((struct SharedSendArgs *)args)->messageSize;
    size_t numMessages = ((struct SharedSendArgs *)args)->numMessages;

    // Example work loop
    hc_shared_write_loop(sharedBuffer, data, messageSize, numMessages);

    return NULL;
}

// Reciever -----------------------
void *recieve_thread(void *args)
{
    // Read in arguments from pointer
    void *sharedBuffer = ((struct SharedRecieveArgs *)args)->sharedBuffer;
    void *recieveBuffer = ((struct SharedRecieveArgs *)args)->recieveBuffer;

    // Example work loop
    hc_shared_read_loop(sharedBuffer, recieveBuffer);

    return NULL;
}
//==============================================

void print_log_header()
{
    printf("===============================================\n");
    printf("TYPE: SHARED MEMORY BUFFER\n");
    printf("MESSAGE SIZE : %lu B\n", MESSAGE_SIZE);
    printf("BUFFER SIZE : %lu B\n", SHARED_BUFFER_SIZE);
    printf("-----\n");
    printf("Sending : %s\n", INPUT_FILE);
    printf("File size is : %lu B\n", FILE_SIZE);
    printf("Remainder data : %lu B\n", FILE_SIZE % MESSAGE_SIZE);
    printf("Preparing to send %lu messages at size %lu B...\n", FILE_SIZE / MESSAGE_SIZE, MESSAGE_SIZE);
    printf("===============================================\n");
}

void *init_shared_buffer(size_t bufferSize)
{
    // Create shared memory buffer
    void *sharedBuffer = allocate_buffer(bufferSize);

    if (sharedBuffer == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    // Clear write state for buffer
    struct BufferInfo *bi = (struct BufferInfo *)(sharedBuffer);
    bi->written = 0;

    return sharedBuffer;
}

int main(int argc, char **argv)
{

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s message_size iterations\n", argv[0]);
        return 1;
    }

    MESSAGE_SIZE = 1ul << atoi(argv[1]);
    NUM_ITERATIONS = 1ul << atoi(argv[2]);

    SHARED_BUFFER_SIZE = MESSAGE_SIZE;

    // Init mutex lock
    if (pthread_mutex_init(&memLock, NULL) != 0)
    {
        perror("mutex init");
        return 1;
    }

    void *sharedBuffer = init_shared_buffer(SHARED_BUFFER_SIZE);

    // Prepare arguments for send thread
    struct SharedSendArgs sendArgs;
    sendArgs.data = map_file(INPUT_FILE, &FILE_SIZE);
    sendArgs.messageSize = MESSAGE_SIZE;
    sendArgs.numMessages = FILE_SIZE / MESSAGE_SIZE;
    sendArgs.sharedBuffer = sharedBuffer;

    // Prepare arguments for recieve thread
    char recieve[MESSAGE_SIZE];
    struct SharedRecieveArgs recArgs;
    recArgs.recieveBuffer = recieve;
    recArgs.sharedBuffer = sharedBuffer;

    // Print test info
    print_log_header();

    // Spin off threads
    printf("Spawning threads\n");
    pthread_t send_tid, recieve_tid;
    pthread_create(&send_tid, NULL, send_thread, &sendArgs);
    pthread_create(&recieve_tid, NULL, recieve_thread, &recArgs);

    pthread_join(send_tid, NULL);
    pthread_join(recieve_tid, NULL);

    return 0;
}
