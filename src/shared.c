/**********
 * Hayden Coffey
 *
 **********/
#include <util.h>

struct BufferInfo
{
    unsigned int size;
    unsigned char written;
};

struct SharedSendArgs
{
    void *sharedBuffer;
    void *data;
    size_t messageSize;
    size_t numMessages;
};

struct SharedRecieveArgs
{
    void *sharedBuffer;
    void *recieveBuffer;
};

pthread_mutex_t memLock;

void *allocate_buffer(size_t size)
{
    return mmap(NULL, size + sizeof(struct BufferInfo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

void *send_thread(void *args)
{
    printf("In send thread\n");
    void *data = ((struct SharedSendArgs *)args)->data;
    void *sharedBuffer = ((struct SharedSendArgs *)args)->sharedBuffer;

    size_t messageSize = ((struct SharedSendArgs *)args)->messageSize;
    size_t numMessages = ((struct SharedSendArgs *)args)->numMessages;

    printf("Done parsing arguments\n");

    struct BufferInfo *bi = (struct BufferInfo *)(sharedBuffer);
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);
    int offset = 0;

    printf("Done selecting buffer start\n");
    int i = 0;
    while (i < numMessages)
    {
        pthread_mutex_lock(&memLock);
        //printf("Aquired lock\n");
        //printf("Checking BI value\n");
        //printf("BI is : %ld\n", bi->written);
        if (bi->written == 0)
        {
            //char x[20];
            //memcpy(x, data + offset, messageSize);
            //printf("Writting message : %s\n", x);

            memcpy(bufferStart, data + offset, messageSize);
            bi->size = messageSize;
            bi->written = 1;

            offset += messageSize;
            i++;
        }
        pthread_mutex_unlock(&memLock);
    }

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

// void recieve_thread(void *sharedBuffer, void *recieveBuff)
void *recieve_thread(void *args)
{
    void *sharedBuffer = ((struct SharedRecieveArgs *)args)->sharedBuffer;
    void *recieveBuff = ((struct SharedRecieveArgs *)args)->recieveBuffer;

    struct BufferInfo *bi = sharedBuffer;
    void *bufferStart = sharedBuffer + sizeof(struct BufferInfo);
    int offset = 0;

    while (1)
    {
        pthread_mutex_lock(&memLock);
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

    printf("Last message recieved : \n");
    puts(recieveBuff);

    return NULL;
}

int main(int argc, char **argv)
{
    if (pthread_mutex_init(&memLock, NULL) != 0)
    {
        perror("mutex init");
        return 1;
    }
    size_t buffSize = 1 << 10;
    void *sharedBuffer = allocate_buffer(buffSize);

    if (sharedBuffer == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    struct BufferInfo *bi = (struct BufferInfo *)(sharedBuffer);
    bi->written = 0;

    printf("Checking BI in master\n");
    printf("BI is : %lu\n", bi->written);

    struct SharedSendArgs sendArgs;
    sendArgs.data = map_file("brown.txt");
    sendArgs.messageSize = 64;
    sendArgs.numMessages = 100;
    sendArgs.sharedBuffer = sharedBuffer;

    char recieve[1000];
    struct SharedRecieveArgs recArgs;
    recArgs.recieveBuffer = recieve;
    recArgs.sharedBuffer = sharedBuffer;

    pthread_t send_tid, recieve_tid;

    printf("Creating threads\n");
    pthread_create(&send_tid, NULL, send_thread, &sendArgs);
    pthread_create(&recieve_tid, NULL, recieve_thread, &recArgs);

    pthread_join(send_tid, NULL);
    pthread_join(recieve_tid, NULL);

    // mmap shared memory
    // At start of buffer: 1 byte for state, 4 bytes for message size
    // State
    // 0: Already processed
    // 1: Data ready to be read
    // 2: Sender has finished, terminate 

    // Sender
    // Mutex lock guards the shared memory, Sender aquires lock, if state is 0, proceed with writing new data and set state

    // Reciever
    // Reciever aquires lock, if 1, read data of size specified. Clear state

    return 0;
}
