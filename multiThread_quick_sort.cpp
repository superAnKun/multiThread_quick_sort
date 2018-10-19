#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
using namespace std;
#define MAX 100000
#define INTERVAL 10
int thread_ok = 0;

struct Data {
    Data (int* num, int l, int r) : num(num), l(l), r(r) {}
    int* num;
    int l, r;
};


struct Queue {
    Data* data;
    int head, tail;
    int size;
    sem_t* full, *empty, *PushMutex, *PopMutex;
};

Queue* initQueue(int n) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->data = (Data*)malloc(sizeof(Data) * n);
    q->full = (sem_t*)malloc(sizeof(sem_t));
    q->PushMutex = (sem_t*)malloc(sizeof(sem_t));
    q->PopMutex = (sem_t*)malloc(sizeof(sem_t));
    q->empty = (sem_t*)malloc(sizeof(sem_t));
    sem_init(q->empty, 0, n);
    sem_init(q->full, 0, 0);
    sem_init(q->PushMutex, 0, 1);
    sem_init(q->PopMutex, 0, 1);
    q->head = q->tail = 0;
    q->size = n;
    for (int i = 0; i < q->size; i++) {
        q->data[i].num = NULL;
    }
    return q;
}

void push(Queue* q, Data&& data) {
    sem_wait(q->PushMutex);
    sem_wait(q->empty);
    q->data[q->head] = data;
    ++q->head %= q->size;
    sem_post(q->full);
    sem_post(q->PushMutex);
}

Data pop(Queue* q) {
    sem_wait(q->PopMutex);
    sem_wait(q->full);
    Data ret = q->data[q->tail];
    q->data[q->tail].num = NULL;
    ++q->tail %= q->size;
    sem_post(q->empty);
    sem_post(q->PopMutex);
    return ret;
}

void clear(Queue* q) {
    free(q->PushMutex);
    free(q->PopMutex);
    free(q->full);
    free(q->empty);
    free(q->data);
    free(q);
}

void quick_sort(int* num, int l, int r) {
    if (num == NULL) return;
    if (l >= r) return;    
    while (l < r) {
        int x = l, y = r;
        int z = num[(l + r) >> 1];
        do {
            while (num[x] < z) x++;
            while (num[y] > z) y--;
            if (x <= y) {
                int temp = num[x];
                num[x] = num[y];
                num[y] = temp;
                ++x, --y;
            }
        } while (x <= y);
        quick_sort(num, x, r);
        r = y;
    }
}

void* thread_work(void* arg) {
    Queue* q = (Queue*)arg;
    while (1) {
        Data data = pop(q);
        if (thread_ok) break;
        if (data.r - data.l <= INTERVAL) {
            quick_sort(data.num, data.l, data.r);
        } else {
            int x = data.l, y = data.r;
            int ans = data.num[(data.l + data.r) >> 1];
            do {
                while (data.num[x] < ans) x++;
                while (data.num[y] > ans) y--;
                if (x <= y) {
                    int temp = data.num[x];
                    data.num[x] = data.num[y];
                    data.num[y] = temp;
                    x++, y--;
                }
            } while (x <= y);
            push(q, {data.num, data.l, y});
            push(q, {data.num, x, data.r});
        }
    }
    return NULL;
}

void* thread_check(void* arg) {
    Data* data = (Data*)arg;
    bool flag = 1;
    while (flag) {
        flag = 0;
        for (int i = data->l; i < data->r; i++) {
            if (data->num[i] <= data->num[i + 1]) continue;
            flag = 1;
        }
    }
    return NULL;
}

void with_thread_quick_sort(int* num, int len, int threadNum) {
    Queue* q = initQueue(len / 2);
    push(q, {num, 0, len - 1});
    pthread_t thread[threadNum];
    for (int i = 0; i < threadNum; i++) {
        pthread_create(thread + i, NULL, thread_work, q);
    }
    Data data = {num, 0, len - 1};
    pthread_t check_thread;
    pthread_create(&check_thread, NULL, thread_check, &data);
    pthread_join(check_thread, NULL);
    thread_ok = 1;
    for (int i = 0; i < threadNum; i++) {
        sem_post(q->full);
    }
    for (int i = 0; i < threadNum; i++) {
        pthread_join(thread[i], NULL);
    }
    clear(q);
}


int main(int argc, char** argv) {
    if (argc < 2) return 0;
    int threadNum = atoi((const char*)argv[1]);
    printf("threadNum: %d\n", threadNum);
    srand(time(NULL));
    int* num = (int*)malloc(sizeof(int) * MAX);
    for (int i = 0; i < MAX; i++) {
        num[i] = rand() % 100000 + 1;
    }
    with_thread_quick_sort(num, MAX, threadNum);
    for (int i = 0; i < MAX - 10; i++) {
        if (i % 20 == 0) printf("\n");
        printf("%d ", num[i]);
    }
    printf("\n");
    free(num);
    return 0;
}
