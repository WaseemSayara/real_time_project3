#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define OriginalnumOfLines 10
#define numOfSteps 10
#define numOfTrucks 4

int STORAGEMAXTHRESHOLD, STORAGEMINTHRESHOLD, StorageEmpPeriod,
    numOfLoadingEmployees, numOfLines, capacityOfTruck, TruckTrevelTime,
    GAINCEIL, PROFITMAXTHRESHOLD, PROFITMINTHRESHOLD, SalaryCEO,
    SalaryHR, SalaryT, SalaryS, SalaryL, SalaryU, SalaryA, SalaryFAB,
    SalarySELL, CostFAB, PriceSELL, step_min_time, step_max_time;
int expenses, gains_this_month, total_gains;
int exportedLaptops = 0;

pthread_t technical_employee[OriginalnumOfLines][numOfSteps], storage_employee, loading_employee, accountant;

pthread_mutex_t loading_mutex, cartonbox_mutex, storage_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t finised_step_cond[OriginalnumOfLines][4];
pthread_mutex_t finised_step_mutex[OriginalnumOfLines][4] = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t line_full_cond[OriginalnumOfLines];
pthread_mutex_t line_full_mutex[OriginalnumOfLines];

time_t current_time;

unsigned int ids[OriginalnumOfLines];
unsigned int counts[OriginalnumOfLines];
unsigned int off_line[OriginalnumOfLines];
pthread_mutex_t get_laptop = PTHREAD_MUTEX_INITIALIZER;

struct List *lists[OriginalnumOfLines];

typedef struct technical
{
    int line;
    int worker;
} technical;

typedef struct laptop
{
    unsigned int laptop_id;
    int finished_steps;
    int visited_techs[5];
    pthread_mutex_t laptop_mutex;
} laptop;

struct Node
{
    laptop my_laptop;
    struct Node *next;
};

// The List, front stores the front node of LL and rear stores the
// last node of LL
struct List
{
    struct Node *front, *rear;
};

int steps_finished[OriginalnumOfLines] = {0};
int laptops_in_carton_box, laptops_in_storage_room, current_truck = 0;

int trucks[numOfTrucks];
time_t trucks_time[numOfTrucks];