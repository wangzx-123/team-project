#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define DISK_NOT_USE  -1

// 查找最大值及其索引
static inline int  find_max(const int num[], size_t num_length) {
    
    int max = num[0];
    int max_id = 0;
    
    for (size_t i = 1; i < num_length; ++i) {
        if (num[i] > max) {
            max = num[i];
            max_id = i;
        }
    }
    
    return max_id;
}

// 检查是否存在特定键值
static inline int find_exit(const int num[], size_t num_length, int key) {
    for (size_t i = 0; i < num_length; ++i) {
        if (num[i] == key) return 0;
    }
    return 1;
}

typedef struct Request_ {
    int object_id;
    int prev_id;
    bool is_done;
    int timestamp;//时间戳
} Request;

typedef struct Object_ {
    int replica[REP_NUM + 1];
    int* unit[REP_NUM + 1];
    int size;
    int last_request_point;
    bool is_delete;
} Object;

Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];
int timestamp_current_point = 0;//当前时间戳

int T, M, N, V, G;
int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_point[MAX_DISK_NUM];//磁头位置
int disk_size_left[MAX_DISK_NUM];

void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    timestamp_current_point = timestamp;
    fflush(stdout);
}

void do_object_delete(const int* object_unit, int* disk_unit, int size)
{
    for (int i = 1; i <= size; i++) {
        disk_unit[object_unit[i]] = 0;
    }
}

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++) {
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
        }
        object[id].is_delete = true;
    }

    fflush(stdout);
}

void do_object_write(int* object_unit, int* disk_unit, int size, int object_id)
{
    int current_write_point = 0;
    for (int i = 1; i <= V; i++) {
        if (disk_unit[i] == 0) {
            disk_unit[i] = object_id;
            object_unit[++current_write_point] = i;
			
            if (current_write_point == size) {
                break;
            }
        }
    }

    assert(current_write_point == size);
}

int select_disk(int *object_replica, int replica_current_point) {
    int max_disk_index = find_max(disk_size_left, (N + 1));
    if (find_exit(object_replica, (REP_NUM + 1), max_disk_index)) {
        object_replica[replica_current_point] = max_disk_index;
    } else {
        int max_0 = DISK_NOT_USE;
        for (int j = 1; j < (N + 1); j++) {
            if (disk_size_left[j] == find_max(disk_size_left, (N + 1))) continue;
            if (max_0 < disk_size_left[j]) {
                max_0 = disk_size_left[j];
                object_replica[replica_current_point] = j;
            }
        }
    }
    return object_replica[replica_current_point];
}//选择剩余最多磁盘空间的磁盘

void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size;
        scanf("%d%d%*d", &id, &size);
        object[id].last_request_point = 0;
        memset(object[id].replica, 0, sizeof(object[id].replica));
        for (int j = 1; j <= REP_NUM; j++) {
            object[id].replica[j] = select_disk(object[id].replica, j);

            disk_size_left[object[id].replica[j]] -= size;//更新磁盘剩余空间
            object[id].unit[j] = (int*)malloc(sizeof(int) * (size + 1));
            object[id].size = size;
            object[id].is_delete = false;
			
            do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id);
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

int t_count(int req_id){//计算f(t)
    int t_real = timestamp_current_point - request[req_id].timestamp;
    if(t_real<=10)return (1-0.005*t_real);
    else if(t_real>10&&t_real<=105)return (1.05-0.01*t_real);
    else return 0;
}
int size_count(int req_id){//计算g(size)
    return ((object[request[req_id].object_id].size+1)/2);
}
int  get_point(){//获取得分最高的请求
    int point_max=0;
    int point_max_id=0;
    for(int i=1;i<MAX_REQUEST_NUM;i++){
        if(request[i].is_done==true)continue;
        int point = t_count(i)*size_count(i);
        if(point>point_max){
            point_max=point;
            point_max_id=i;
        }
    }
    return point_max_id;
}
void read_action()
{
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].timestamp = timestamp_current_point;
    }
	
    static int current_request = 0;
    static int current_phase = 0;
    if (!current_request && n_read > 0) {
        current_request = get_point();
    }
    if (!current_request) {
        for (int i = 1; i <= N; i++) {
            printf("#\n");
        }
        printf("0\n");
    } else {
        current_phase++;
        object_id = request[current_request].object_id;
        for (int i = 1; i <= N; i++) {
            if (i == object[object_id].replica[1]) {
                if (current_phase % 2 == 1) {
                    printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
                } else {
                    printf("r#\n");
                }
            } else {
                printf("#\n");
            }
        }
		
        if (current_phase == object[object_id].size * 2) {
            if (object[object_id].is_delete) {
                printf("0\n");
            } else {
                printf("1\n%d\n", current_request);
                 request[current_request].is_done = true;
            }
            current_request = 0;
            current_phase = 0;
        } else {
            printf("0\n");
        }
    }

    fflush(stdout);
}

void clean()
{
    for (int i = 1; i < MAX_OBJECT_NUM; ++i) {
        for (int j = 1; j <= REP_NUM; j++) {
            if (object[i].unit[j] == NULL)
                continue;
            free(object[i].unit[j]);
            object[i].unit[j] = NULL;
        }
    }
}

int main()
{
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

    memset(disk_size_left, DISK_NOT_USE, sizeof(disk_size_left));
    for (int i = 1; i <= N; i++) {
        disk_size_left[i] = 0.9 * V;
    }
    //初始化磁头位置
    for (int i = 1; i <= N; i++) {
        disk_point[i] = 1;
    }
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    for (int i = 1; i <= N; i++) {
        disk_point[i] = 1;
    }

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        timestamp_action();
        delete_action();
        write_action();
        read_action();
    }
    clean();

    return 0;
}