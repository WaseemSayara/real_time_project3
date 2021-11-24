#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>
#include <time.h>

// #define MAXTHRESHOLD 16
// #define MINTHRESHOLD 5
// #define numOfLoadingEmployees 5
#define OriginalnumOfLines 10
#define numOfSteps 10
#define numOfTrucks 4
// #define capacityOfTruck 10
// #define TruckTrevelTime 10
int STORAGEMAXTHRESHOLD, STORAGEMINTHRESHOLD, StorageEmpPeriod,
    numOfLoadingEmployees, numOfLines, capacityOfTruck, TruckTrevelTime, 
    PROFITCEIL, PROFITMAXTHRESHOLD,PROFITMINTHRESHOLD, SalaryCEO, 
    SalaryHR, SalaryT, SalaryS, SalaryL, SalaryU, SalaryA, SalaryFAB, 
    SalarySELL, CostFAB, PriceSELL;
int expenses, gains;
int exportedLaptops = 0;

pthread_t technical_employee[OriginalnumOfLines][numOfSteps], storage_employee, loading_employee, accountant;
pthread_mutex_t loading_mutex, cartonbox_mutex, storage_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t loading_threshold_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t count_mutex[OriginalnumOfLines] = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t count_threshold_cv[OriginalnumOfLines][6] = PTHREAD_COND_INITIALIZER;

time_t current_time;

unsigned int ids[OriginalnumOfLines] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int counts[OriginalnumOfLines] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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