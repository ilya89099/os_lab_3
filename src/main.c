#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "adjacency_list.h"
#include "queue.h"
// лаба 3, вариант 21
// поиск кратчайшего пути в графе поиском в ширину
// граф задается матрицей смежности

typedef struct ThreadParams thread_params;
struct ThreadParams {
    int current_vertex;
    adjacency_list *list;
    int *max_thread_count;
    pthread_mutex_t *mutex;
    int *result;
    pthread_mutex_t *thread_count_mutex;
};

int min(int lhs, int rhs) {
    return lhs > rhs ? rhs : lhs;
}

//каждый раз новый поток принимает thread_params
//current_vertex - текущая вершина в обходе
//result - там хранятся подсчитанные расстояния до вершин, или -1 если не подсчитано
//max_thread_count - эта переменная уменьшается при создании нового потока,
//таким образом будет создано столько потоков сколько нужно
//mutexes - мьютексы для синхронизации доступа к result
//list - список смежности для графа, будем считать, что граф связный
void *bfs(void *args) {
    thread_params* params = (thread_params*) args;
    vector recount;
    v_init(&recount);
    pthread_mutex_lock(params->mutex);
    for (int i = 0; i < params->list->vecs[params->current_vertex].size; ++i) {
        
        int n_v = params->list->vecs[params->current_vertex].data[i].vertex_number;
        int n_vl = params->list->vecs[params->current_vertex].data[i].length;
        if (params->result[n_v] == -1 || params->result[n_v] > params->result[params->current_vertex] + n_vl) {
            params->result[n_v] = params->result[params->current_vertex] + n_vl;
            v_push(&recount, (pair){.vertex_number = n_v, .length = n_vl});
        }
        
    }
    pthread_mutex_unlock(params->mutex);

    pthread_mutex_lock(params->thread_count_mutex);
    int can_create_threads = min(*(params->max_thread_count), recount.size);
    *(params->max_thread_count) -= can_create_threads;
    pthread_mutex_unlock(params->thread_count_mutex);

    pthread_t threads[can_create_threads];
    thread_params* new_params = malloc(sizeof(thread_params) * can_create_threads);
    for (int i = 0; i < can_create_threads; ++i) {
        new_params[i] = *params;
        new_params[i].current_vertex = recount.data[i].vertex_number;
        pthread_create(&threads[i], NULL, bfs, &new_params[i]);
    }

    queue q;
    q_init(&q);
    for (int i = can_create_threads; i < recount.size; ++i) {
        q_push(&q, recount.data[i].vertex_number);
    }
    pthread_mutex_lock(params->mutex);
    while (!q_empty(&q)) {
        int cur_vertex = q_pop(&q);
        
        for (int i = 0; i < params->list->vecs[cur_vertex].size; ++i) {
            int neighbour_vertex = params->list->vecs[cur_vertex].data[i].vertex_number;
            int neighbour_distance = params->list->vecs[cur_vertex].data[i].length;
            if (params->result[neighbour_vertex] == -1 ||
                params->result[neighbour_vertex] > params->result[cur_vertex] + neighbour_distance) {
                params->result[neighbour_vertex] = params->result[cur_vertex] + neighbour_distance;
                q_push(&q, neighbour_vertex);
            }
        }
        
    }
    pthread_mutex_unlock(params->mutex);

    for (int i = 0; i < can_create_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_lock(params->thread_count_mutex);
    params->max_thread_count += can_create_threads;
    pthread_mutex_unlock(params->thread_count_mutex);
    v_destroy(&recount);
    free(new_params);
    pthread_exit(NULL);
}

int main() {
    int n;
    printf("Enter size of matrix\n");
    scanf("%d", &n);
    int matrix[n][n];
    printf("Enter matrix\n");
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            scanf("%d", &matrix[i][j]);
        }
    }

    adjacency_list *list = malloc(sizeof(adjacency_list));
    a_init(list);
    a_resize(list, n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (matrix[i][j] != 0) {
                v_push(&list->vecs[i], (pair) {j, matrix[i][j]});
            }
        }
    }
    size_t vertex_number;
    int *max_thread_count = malloc(sizeof(int));
    int *result = malloc(sizeof(int) * n);
    for (int i = 0; i < n; ++i) {
        result[i] = -1;
    }

    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t) * n);
    pthread_mutex_init(mutex, NULL);
    pthread_mutex_t* thread_count_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(thread_count_mutex, NULL);

    printf("Enter vertex to start search\n");
    scanf("%ld", &vertex_number);
    while (vertex_number > n || vertex_number == 0) {
        printf("Vertex number must be < %d\n and > 0", n);
        printf("Enter vertex to start search\n");
        scanf("%ld", &vertex_number);
    }
    printf("Enter max thread count\n");
    scanf("%d", max_thread_count);
    while (*max_thread_count < 1) {
        printf("Max thread count must be not less than 1");
        printf("Enter max thread count\n");
        scanf("%d", max_thread_count);
    }

    vertex_number--;
    result[vertex_number] = 0;
    (*max_thread_count)--;

    thread_params parameters = (thread_params) {.max_thread_count = max_thread_count,
            .result = result,
            .current_vertex = vertex_number,
            .list = list,
            .mutex = mutex,
            .thread_count_mutex = thread_count_mutex};

    pthread_t first_thread;
    pthread_create(&first_thread, NULL, bfs, &parameters);
    pthread_join(first_thread, NULL);

    for (int i = 0; i < n; ++i) {
        printf("%d:%d ", i + 1, result[i]);
    }
    return 0;
}

//0 3 5 0 0
//3 0 0 6 1
//5 0 0 4 0
//0 6 4 0 1
//0 1 0 1 0