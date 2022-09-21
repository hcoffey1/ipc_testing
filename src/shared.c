/**********
 * Hayden Coffey
 *
 **********/
#include <util.h>

#define INPUT_FILE "brown.txt"

#define MESSAGE_SIZE (1ul << 8)
#define SHARED_BUFFER_SIZE (MESSAGE_SIZE)

#define NUM_ITERATIONS 1ul << 4 

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

// Sender -----------------------
void *send_thread(void *args)
{
    // Read in arguments from pointer
    void *data = ((struct SharedSendArgs *)args)->data;
    void *sharedBuffer = ((struct SharedSendArgs *)args)->sharedBuffer;
    size_t messageSize = ((struct SharedSendArgs *)args)->messageSize;
    size_t numMessages = ((struct SharedSendArgs *)args)->numMessages;

    // Retrieve buffer state
    struct BufferInfo *bi = (struct BufferInfo *)(sharedBuffer);
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);

    int i = 0;
    while (i < NUM_ITERATIONS)
    {
        printf("Iteration : %d\n", i);
        int offset = 0; // Offset in data to be sent

        int m = 0;
        while (m < numMessages)
        {
            pthread_mutex_lock(&memLock);
            // If buffer is ready to be written to, copy new data in
            if (bi->written == 0)
            {
                memcpy(bufferStart, data + offset, messageSize);
                bi->size = messageSize;
                bi->written = 1;

                offset += messageSize;
                m++;
            }
            pthread_mutex_unlock(&memLock);
        }

        size_t remainderSize = FILE_SIZE - offset;
        // Send remainder data
        while (1)
        {
            pthread_mutex_lock(&memLock);
            if (bi->written == 0)
            {
                memcpy(bufferStart, data + offset, remainderSize);
                bi->size = remainderSize;
                bi->written = 1;

                pthread_mutex_unlock(&memLock);
                break;
            }
            pthread_mutex_unlock(&memLock);
        }

        i++;
    }

    // When done, wait for last sent message to be processed then terminate
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

    return NULL;
}

// Reciever -----------------------
void *recieve_thread(void *args)
{
    // Read in arguments from pointer
    void *sharedBuffer = ((struct SharedRecieveArgs *)args)->sharedBuffer;
    void *recieveBuff = ((struct SharedRecieveArgs *)args)->recieveBuffer;

    // Retrieve buffer state
    struct BufferInfo *bi = sharedBuffer;
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);

    while (1)
    {
        pthread_mutex_lock(&memLock);
        // If buffer has been written to, copy values in.
        // Send confirmation by clearing written state
        if (bi->written == 1)
        {
            memcpy(recieveBuff, bufferStart, bi->size);
            bi->written = 0;
        }
        else if (bi->written == 2)
        {
            pthread_mutex_unlock(&memLock);
            break;
        }
        pthread_mutex_unlock(&memLock);
    }

    return NULL;
}
//==============================================

void print_log_header()
{
    printf("SHARED MEMORY BUFFER==========\n");
    printf("MESSAGE SIZE : %lu\n", MESSAGE_SIZE);
    printf("BUFFER SIZE : %lu\n", SHARED_BUFFER_SIZE);
    printf("-----\n");
    printf("Sending : %s\n", INPUT_FILE);
    printf("File size is : %lu B\n", FILE_SIZE);
    printf("Remainder data : %lu B\n", FILE_SIZE % MESSAGE_SIZE);
    printf("Preparing to send %lu messages at size %lu B...\n", FILE_SIZE / MESSAGE_SIZE, MESSAGE_SIZE);
    printf("==============================\n");
}

int main(int argc, char **argv)
{
    // Init mutex lock
    if (pthread_mutex_init(&memLock, NULL) != 0)
    {
        perror("mutex init");
        return 1;
    }

    // Create shared memory buffer
    void *sharedBuffer = allocate_buffer(SHARED_BUFFER_SIZE);

    if (sharedBuffer == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    // Clear write state for buffer
    struct BufferInfo *bi = (struct BufferInfo *)(sharedBuffer);
    bi->written = 0;

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

    //Print test info
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
