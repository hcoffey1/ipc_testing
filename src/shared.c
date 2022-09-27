/**********
 * Hayden Coffey
 *
 **********/
#include <util.h>

size_t MESSAGE_SIZE;
size_t SHARED_BUFFER_SIZE;

void * sharedBuffer2;

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
    unsigned char id;
};

struct SharedSendArgs
{
    void *sharedBuffer;
    void *data; // Data to be sent
    size_t messageSize;
    size_t numMessages;
};

struct SharedReceiveArgs
{
    void *sharedBuffer;
    void *receiveBuffer;
    size_t numMessages;
    size_t messageSize;
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

// If buffer has been written to, copy values in.
// Send confirmation by clearing written state
int shared_read(void *sharedBuffer, void *receiveBuffer, struct BufferInfo *bi, int isHost)
{
    int readret = 0;
    while(1)
    {
        pthread_mutex_lock(&memLock);
        if (bi->written == 1)
        {
            memcpy(receiveBuffer, sharedBuffer, bi->size);
            bi->written = 0;
            bi->id = isHost;
            pthread_mutex_unlock(&memLock);
            break;
        }
        else if (bi->written == 2)
        {
            readret = -1;
            bi->written = 0;
            bi->id = isHost;
            pthread_mutex_unlock(&memLock);
            break;
        }
        pthread_mutex_unlock(&memLock);
    }

    return readret;
}

// Return true if write was completed
int shared_write(void *buffer, void *data, size_t messageSize, struct BufferInfo *bi, int isHost)
{
    int writeret = 0;
    while(1)
    {
        pthread_mutex_lock(&memLock);
        // If buffer is ready to be written to, copy new data in
        if (bi->written == 0)
        {
            memcpy(buffer, data, messageSize);
            bi->size = messageSize;
            bi->written = 1;
            bi->id = isHost;

            writeret = 1;
            pthread_mutex_unlock(&memLock);
            break;
        }
        pthread_mutex_unlock(&memLock);
    }

    return writeret;
}

void shared_send_messages(void *buffer, struct BufferInfo *bi, void *data, size_t messageSize, size_t numMessages)
{
    int m = 0;
    while (m < numMessages)
    {
        shared_write(buffer, data, MESSAGE_SIZE, bi, 1);
        m++;
    }

    size_t remainderSize = FILE_SIZE;
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

void sync2(struct BufferInfo * bi, int id, int written)
{
    while(1){
        pthread_mutex_lock(&memLock);
        if(bi->id == id && bi->written == written)
        {
            pthread_mutex_unlock(&memLock);
            break;
        }
        pthread_mutex_unlock(&memLock);
    }
}

void hc_shared_latency_loop(void* sharedBuffer, void * data, size_t messageSize, size_t numMessages, int isHost)
{
    struct BufferInfo *bi = sharedBuffer;
    struct BufferInfo *bi2 = sharedBuffer2;

    void *bufferStart = get_buffer_write_address(sharedBuffer);
    void *bufferStart2 = get_buffer_write_address(sharedBuffer2);

    char * inbuf = malloc(messageSize);

    if(isHost)
    {
        printf("Running latency test\n");
        for(int i = 0; i < numMessages; i++)
        {
            shared_write(bufferStart, data, messageSize, bi, isHost);
            shared_read(bufferStart2, inbuf, bi2, isHost);
        }
    }
    else
    {
        for(int i = 0; i < numMessages; i++)
        {
            shared_read(bufferStart, inbuf, bi, isHost);
            shared_write(bufferStart2, inbuf, messageSize, bi2, isHost);
        }
    }
}

// B/W
void hc_shared_write_loop(void *sharedBuffer, void *data, size_t messageSize, size_t numMessages)
{
    printf("Running B/W loop, message size: %d\n", messageSize);
    // Retrieve buffer state and first writable position
    struct BufferInfo *bi = get_buffer_info(sharedBuffer);
    void *bufferStart = get_buffer_write_address(sharedBuffer);

    shared_send_messages(bufferStart, bi, data, messageSize, numMessages);

    // When done, wait for last sent message to be processed then terminate (waits for ack)
    shared_send_kill(bi);

    printf("Done\n");

    hc_shared_latency_loop(sharedBuffer, data, messageSize, numMessages, 1);
}


void hc_shared_read_loop(void *sharedBuffer, void *receiveBuffer, size_t messageSize, size_t numMessages)
{
    // Retrieve buffer state
    struct BufferInfo *bi = sharedBuffer;
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);

    while (1)
    {
        if (shared_read(sharedBuffer, receiveBuffer, bi, 0) < 0)
        {
            break;
        }
    }

    hc_shared_latency_loop(sharedBuffer, NULL, messageSize, numMessages, 0);
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

// Receiver -----------------------
void *receive_thread(void *args)
{
    // Read in arguments from pointer
    void *sharedBuffer = ((struct SharedReceiveArgs *)args)->sharedBuffer;
    void *receiveBuffer = ((struct SharedReceiveArgs *)args)->receiveBuffer;
    size_t numMessages = ((struct SharedReceiveArgs *)args)->numMessages;
    size_t messageSize= ((struct SharedReceiveArgs *)args)->messageSize;

    // Example work loop
    hc_shared_read_loop(sharedBuffer, receiveBuffer, messageSize, numMessages);

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
    bi->id = 0;

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

    SHARED_BUFFER_SIZE = MESSAGE_SIZE;

    // Init mutex lock
    if (pthread_mutex_init(&memLock, NULL) != 0)
    {
        perror("mutex init");
        return 1;
    }

    void *sharedBuffer = init_shared_buffer(SHARED_BUFFER_SIZE);
    sharedBuffer2 = init_shared_buffer(SHARED_BUFFER_SIZE);

    size_t numMessages = 1ul<<10;

    // Prepare arguments for send thread
    struct SharedSendArgs sendArgs;
    sendArgs.data = map_file(INPUT_FILE, &FILE_SIZE);
    sendArgs.messageSize = MESSAGE_SIZE;
    sendArgs.numMessages = numMessages;
    sendArgs.sharedBuffer = sharedBuffer;

    // Prepare arguments for receive thread
    char receive[MESSAGE_SIZE];
    struct SharedReceiveArgs recArgs;
    recArgs.messageSize = MESSAGE_SIZE;
    recArgs.receiveBuffer = receive;
    recArgs.sharedBuffer = sharedBuffer;
    recArgs.numMessages = numMessages;

    // Spin off threads
    printf("Spawning threads\n");
    pthread_t send_tid, receive_tid;
    pthread_create(&send_tid, NULL, send_thread, &sendArgs);
    pthread_create(&receive_tid, NULL, receive_thread, &recArgs);

    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    return 0;
}
