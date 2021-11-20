#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define MAXTHRESHOLD 16
#define MINTHRESHOLD 14
#define numOfLoadingEmployees 0
#define numOfLines 10
#define numOfSteps 10
#define numOfTrucks 4
#define capacityOfTruck 10
#define TruckTrevelTime 10

pthread_t technical_employee[numOfLines][numOfSteps], storage_employee, loading_employee;

pthread_mutex_t loading_mutex, cartonbox_mutex, storage_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t loading_threshold_cv = PTHREAD_COND_INITIALIZER;

pthread_mutex_t count_mutex[numOfLines] = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t count_threshold_cv[numOfLines][6] = PTHREAD_COND_INITIALIZER;
time_t current_time;

unsigned int ids[numOfLines] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int counts[numOfLines] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
pthread_mutex_t get_laptop = PTHREAD_MUTEX_INITIALIZER;

struct List *lists[numOfLines];

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

int steps_finished[numOfLines] = {0};
int laptops_in_carton_box, laptops_in_storage_room, current_truck = 0;

int trucks[4];
time_t trucks_time[4];

struct Node *newNode(laptop);
void print_list(struct List *);
void add_node(struct List *, laptop);
laptop get_free_laptop(struct List *, int);
int return_laptop(struct List *, laptop);
void remove_laptop(struct List *, int);
struct List *createList();
laptop get_laptop_with_id(struct List *, int);
laptop get_free_laptop_with_steps(struct List *, int);

laptop no_free;

/* function to be executed by the TECHNICAL EMPLOYEE thread */
void *execute_step(technical *data)
{

    int technical = data->worker;
    int line = data->line;

    printf("line %d , technical %d created\n", line, technical);
    int k = 220;
    while (k--)
    {

        if (technical == 0)
        {
            if (counts[line] < 10)
            {
                //todo: check the length of the list ( must not exceed 10)
                laptop new_laptop;
                unsigned int id = line * 10000 + ids[line] + 1;
                new_laptop.laptop_id = id;
                new_laptop.finished_steps = 0;
                for (int j = 0; j < 5; j++)
                {
                    new_laptop.visited_techs[j] = 0;
                }

                ids[line]++;
                add_node(lists[line], new_laptop);
                counts[line]++;
                new_laptop = get_laptop_with_id(lists[line], id);

                // check if its valid laptop
                if (new_laptop.laptop_id != -1)
                {
                    //step time
                    sleep(1);
                    new_laptop.finished_steps++;
                    printf("\033[0;32m line %d, technical %d, laptopd id: %d, finished step: %d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                    return_laptop(lists[line], new_laptop);
                }
                else
                {
                    printf("error in got a wronge laptop\n");
                    exit(1);
                }
            }
        }

        else if (technical >= 1 && technical <= 4)
        {
            laptop new_laptop;
            new_laptop = get_free_laptop_with_steps(lists[line], technical);
            if (new_laptop.laptop_id != -1)
            {
                //step time
                sleep(1);
                new_laptop.finished_steps++;
                printf("line %d, technical %d, laptopd id: %d, finished step: %d\n", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                return_laptop(lists[line], new_laptop);
            }
            else
            {
                sleep(1);
            }
        }

        else
        {
            laptop new_laptop;
            pthread_mutex_lock(&get_laptop);
            new_laptop = get_free_laptop(lists[line], technical);
            pthread_mutex_unlock(&get_laptop);
            if (new_laptop.laptop_id != -1)
            {
                //step time
                sleep(rand() % 5);
                int tmp = new_laptop.finished_steps;
                int index = tmp - 5;
                printf("\033[0;34m line %d, technical %d, laptopd id: %d, current step: %d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                new_laptop.visited_techs[index] = technical;
                print_array(new_laptop);
                new_laptop.finished_steps = (new_laptop.finished_steps) + 1;
                printf("\033[0;33m line %d, technical %d, laptopd id: %d, finished step: %d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                if (new_laptop.finished_steps == numOfSteps)
                {

                    printf("\033[0;36mline %d , technical %d put the laptop %d in the carton box\n\033[0m", line, technical, new_laptop.laptop_id);
                    steps_finished[line] = 0;
                    sleep(1);
                    if (pthread_mutex_lock(&cartonbox_mutex) != 0)
                        perror("error");

                    // while (laptops_in_carton_box >= numOfLines)
                    //     ;
                    printf("\033[0;33mlaptops_in_carton_box = %d\n\033[0m", ++laptops_in_carton_box);

                    if (pthread_mutex_unlock(&cartonbox_mutex) != 0)
                        perror("error");

                    if (pthread_cond_signal(&count_threshold_cv[line][0]) != 0)
                        perror("error");
                    remove_laptop(lists[line], new_laptop.laptop_id);
                    counts[line]--;
                }
                else
                {
                    return_laptop(lists[line], new_laptop);
                }
            }
            else
            {
                // printf("no free from %d\n", technical);
                sleep(1);
            }
        }
        sleep(1);
    }

    // -----------------------------------------------------------------------------
    //     if (pthread_mutex_lock(&count_mutex[line]) != 0)
    //         perror("error");

    //     if (technical <= 4)
    //     {
    //         if (pthread_cond_wait(&count_threshold_cv[line][technical], &count_mutex[line]) != 0)
    //             perror("error");
    //     }

    //     else
    //     {
    //         if (pthread_cond_wait(&count_threshold_cv[line][5], &count_mutex[line]) != 0)
    //             perror("error");
    //     }

    //     printf("line %d , technical %d\n", line, technical);

    //     // step time
    //     usleep(1000000);
    //     steps_finished[line]++;
    //     if (steps_finished[line] == numOfSteps)
    //     {

    //         printf("\033[0;36mline %d , technical %d put the laptop in the carton box\n\033[0m", line, technical);
    //         steps_finished[line] = 0;
    //         sleep(1);
    //         if (pthread_mutex_lock(&cartonbox_mutex) != 0)
    //             perror("error");

    //         // while (laptops_in_carton_box >= numOfLines)
    //         //     ;
    //         printf("\033[0;33mlaptops_in_carton_box = %d\n\033[0m", ++laptops_in_carton_box);

    //         if (pthread_mutex_unlock(&cartonbox_mutex) != 0)
    //             perror("error");

    //         if (pthread_cond_signal(&count_threshold_cv[line][0]) != 0)
    //             perror("error");
    //     }
    //     if (laptops_in_storage_room >= MAXTHRESHOLD)
    //     {
    //         //printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    //         while (laptops_in_storage_room >= MINTHRESHOLD)
    //             sleep(1);
    //     }

    //     if (technical < 4)
    //     {
    //         if (pthread_cond_signal(&count_threshold_cv[line][technical + 1]) != 0)
    //             perror("error");
    //     }
    //     else if (technical == 4)
    //     {
    //         if (pthread_cond_broadcast(&count_threshold_cv[line][5]) != 0)
    //             perror("error");
    //     }

    //     if (pthread_mutex_unlock(&count_mutex[line]) != 0)
    //         perror("error");
    // }
}

/* function to be executed by the STORAGE EMPLOYEE thread */
void *collect_filled_carton(void *data)
{
    printf("Storage Employee Created!\n");
    while (1)
    {
        pthread_mutex_lock(&storage_mutex);
        pthread_mutex_lock(&cartonbox_mutex);
        if (laptops_in_carton_box >= numOfLines)
        {

            printf("\033[0;32mThe Storage Employee collecting the filled carton and placing it in the storage room...\n\033[0m");
            sleep(2);
            printf("\033[0;32mThe Storage Employee Finished his task, laptops_in_storage_room = %d\n\033[0m", (laptops_in_storage_room + numOfLines));

            laptops_in_storage_room += numOfLines;

            //pthread_cond_broadcast(&loading_threshold_cv);
            laptops_in_carton_box = 0;
        }
        pthread_mutex_unlock(&cartonbox_mutex);
        pthread_mutex_unlock(&storage_mutex);
        sleep(1);
    }
}

/* function to be executed by the LOADING EMPLOYEE thread */
void *load_truck(int *data)
{
    int me = *((int *)data);
    printf("Loading Employee No.%d Created!\n", me);
    while (1)
    {
        pthread_mutex_lock(&loading_mutex);
        if (laptops_in_storage_room > 0)
        {
            pthread_mutex_lock(&storage_mutex);

            if (trucks[current_truck] == capacityOfTruck)
            {
                printf("\033[0;36mTruck No.%d is full\n\033[0m", current_truck);

                current_time = time(NULL);
                printf("\033[0;36mTruck No.%d is full, current time is %ld and the truck time is %ld \n\033[0m", current_truck, current_time, trucks_time[current_truck]);
                int delta_time = current_time - trucks_time[current_truck];
                printf("\033[0;36mTruck No.%d is full, delats is %d and the max is %d \n\033[0m", current_truck, delta_time, (int)TruckTrevelTime);

                if (delta_time >= (int)TruckTrevelTime)
                {
                    trucks[current_truck] = 0;
                }
            }

            if (trucks[current_truck] < capacityOfTruck)
            {
                laptops_in_storage_room--;
                trucks[current_truck]++;

                printf("\033[0;31mLoading Emp No.%d loads 1 Laptop, laptops_in_storage_room = %d, laptops_in_carton_box = %d\n\033[0m", me, laptops_in_storage_room, laptops_in_carton_box);
                printf("\033[0;31mTruck No.%d has %d laptops now\n\033[0m", current_truck, trucks[current_truck]);

                if (trucks[current_truck] == capacityOfTruck)
                {
                    printf("\033[0;32mTruck No.%d is full and going to unload\n\033[0m", current_truck);
                    current_time = time(NULL);
                    trucks_time[current_truck] = current_time;
                    current_truck = (current_truck + 1) % numOfTrucks;
                }
            }

            pthread_mutex_unlock(&storage_mutex);
        }
        pthread_mutex_unlock(&loading_mutex);
        sleep(6);
    }
}
/* like any C program, program's execution begins in main */
int main(int argc, char *argv[])
{

    int ret;
    no_free.laptop_id = -1;
    srand(getpid());

    for (int i = 0; i < numOfTrucks; i++)
    {
        trucks[i] = 0;
    }

    for (int i = 0; i < numOfLines; i++)
    {
        lists[i] = createList();
    }

    printf("hello world\n");

    laptop lap1;
    lap1.laptop_id = 1;
    lap1.finished_steps = 0;

    struct List *l1;
    l1 = createList();
    add_node(l1, lap1);

    laptop lap2;
    lap2.laptop_id = 2;
    lap2.finished_steps = 0;

    add_node(l1, lap2);

    laptop lap3;
    lap3.laptop_id = 3;
    lap3.finished_steps = 0;

    add_node(l1, lap3);

    remove_laptop(l1, 3);

    print_list(l1);

    laptop lap4;
    lap4.laptop_id = 4;
    lap4.finished_steps = 0;

    add_node(l1, lap4);

    print_list(l1);

    // laptop tmp = get_free_laptop(l1);
    // printf("the current free is laptop with id = %d with steps = %d\n", tmp.laptop_id, tmp.finished_steps);
    // laptop tmp2 = get_free_laptop(l1);
    // printf("the current free is laptop with id = %d \n", tmp2.laptop_id);
    // laptop tmp3 = get_free_laptop(l1);
    // if (tmp3.laptop_id == -1)
    // {
    //     printf("no current free laptops = %d \n", tmp3.laptop_id);
    // }
    // laptop tmp4 = get_free_laptop(l1);
    // if (tmp4.laptop_id == -1)
    // {
    //     printf("no current free laptops = %d \n", tmp4.laptop_id);
    // }

    // tmp.finished_steps++;
    // tmp2.finished_steps++;

    // return_laptop(l1, tmp);
    // return_laptop(l1, tmp2);

    // laptop tmp5 = get_free_laptop(l1);
    // if (tmp5.laptop_id == -1)
    // {
    //     printf("no current xcvxv free laptops = %d \n", tmp5.laptop_id);
    // }
    // else
    // {
    //     printf("the current free is laptop with id = %d with steps = %d\n", tmp5.laptop_id, tmp5.finished_steps);
    // }
    // laptop tmp6 = get_free_laptop(l1);
    // if (tmp6.laptop_id == -1)
    // {
    //     printf("no current xcvxv free laptops = %d \n", tmp6.laptop_id);
    // }
    // else
    // {
    //     printf("the current free is laptop with id = %d with steps = %d\n", tmp6.laptop_id, tmp6.finished_steps);
    // }

    for (int i = 0; i < numOfLines; i++)
    {
        for (int j = 0; j < numOfSteps; j++)
        {
            technical x;
            x.line = i;
            x.worker = j;

            if (pthread_create(&technical_employee[i][j], NULL, (void *)execute_step, (void *)&x) != 0)
            {
                perror("failed to create technical_employee thread");
            }

            usleep(2500);
        }
    }

    // int a = 1;
    // if (pthread_create(&storage_employee, NULL, (void *)collect_filled_carton, (void *)&a) != 0)
    // {
    //     perror("failed to create storage_employee thread");
    // }
    // for (int i = 0; i < numOfLoadingEmployees; i++)
    // {
    //     if (pthread_create(&loading_employee, NULL, (void *)load_truck, (void *)&i) != 0)
    //     {
    //         perror("failed to create loading_employee thread");
    //     }
    //     usleep(2500);
    // }
    // printf("-------------------------------\n");
    // sleep(1);
    // for (int i = 0; i < numOfLines; i++)
    //     pthread_cond_signal(&count_threshold_cv[i][0]);

    sleep(200);
    // printf("thread ended with status\n");

    /* NOT REACHED */
    return 0;
}

void add_node(struct List *l, laptop new_laptop)
{
    // Create a new LL node
    struct Node *temp = newNode(new_laptop);

    // If List is empty, then new node is front and rear both
    if (l->rear == NULL)
    {
        l->front = l->rear = temp;
        return;
    }

    // Add the new node at the end of List and change rear
    l->rear->next = temp;
    l->rear = temp;
}

// Function to return a node that contains a laptop that is not under work, if all are busy will return NULL
laptop get_free_laptop(struct List *l, int tech_id)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
    {
        printf("nul;lllllllllllllllll;\n");
        return;
    }
    struct Node *tmp = l->front;
    int visited = 0;
    while (tmp != NULL)
    {
        if (tmp->my_laptop.finished_steps >= 5)
        {
            for (int i = 0; i < 5; i++)
            {
                if (tmp->my_laptop.visited_techs[i] == tech_id)
                {
                    visited = 1;
                    break;
                }
            }
            if (visited == 0)
            {
                if (pthread_mutex_trylock(&tmp->my_laptop.laptop_mutex) == 0)
                {
                    printf(" tech %d accessed laptop %d with current steps %d =============\n", tech_id, tmp->my_laptop.laptop_id, tmp->my_laptop.finished_steps);
                    return tmp->my_laptop;
                }
                // else
                // {
                //     perror("cant lock;");
                // }
            }
        }
        tmp = tmp->next;
    }
    return no_free;
}

void print_list(struct List *l)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
    {
        printf("the list is empty\n");
        return;
    }

    struct Node *tmp = l->front;
    while (tmp != NULL)
    {
        printf("laptop id: %d -- steps finished: %d \n", tmp->my_laptop.laptop_id, tmp->my_laptop.finished_steps);
        tmp = tmp->next;
    }
    return;
}

laptop get_free_laptop_with_steps(struct List *l, int steps)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
        return no_free;

    struct Node *tmp = l->front;
    while (tmp != NULL)
    {
        if (tmp->my_laptop.finished_steps == steps)
        {
            if (pthread_mutex_lock(&tmp->my_laptop.laptop_mutex) == 0)
            {
                return tmp->my_laptop;
            }
            else
            {
                perror("error lock");
            }
        }
        else
        {
            tmp = tmp->next;
        }
    }
    return no_free;
}

laptop get_laptop_with_id(struct List *l, int id)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
        return no_free;

    struct Node *tmp = l->front;
    while (tmp != NULL)
    {
        if (tmp->my_laptop.laptop_id == id)
        {
            return tmp->my_laptop;
        }
        else
        {
            tmp = tmp->next;
        }
    }
    return no_free;
}

int return_laptop(struct List *l, laptop edited_laptop)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
        return -1;

    struct Node *tmp = l->front;
    while (tmp != NULL)
    {
        if (tmp->my_laptop.laptop_id == edited_laptop.laptop_id)
        {
            tmp->my_laptop = edited_laptop;
            pthread_mutex_unlock(&tmp->my_laptop.laptop_mutex);
            return 0;
        }
        else
        {
            tmp = tmp->next;
        }
    }
    return -1;
}

// todo: need fixing
void remove_laptop(struct List *l, int id)
{
    // If List is empty, return NULL.
    if (l->front == NULL)
        return;

    // Store previous front and move front one node ahead
    struct Node *temp = l->front;

    if (temp->my_laptop.laptop_id == id)
    {
        l->front = temp->next;
        pthread_mutex_unlock(&temp->my_laptop.laptop_mutex);
        free(temp);
        return;
    }

    struct Node *temp2;
    while (temp->next != NULL)
    {
        if (temp->next->my_laptop.laptop_id == id)
        {
            temp2 = temp->next;
            temp->next = temp2->next;
            pthread_mutex_unlock(&temp2->my_laptop.laptop_mutex);
            if (temp2->next == NULL)
            {
                l->rear = temp;
            }
            free(temp2);
            return;
        }
        temp = temp->next;
    }
}

// A utility function to create a new linked list node.
struct Node *newNode(laptop new_laptop)
{
    struct Node *temp = (struct Node *)malloc(sizeof(struct Node));
    temp->my_laptop = new_laptop;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty List
struct List *createList()
{
    struct List *l = (struct List *)malloc(sizeof(struct List));
    l->front = l->rear = NULL;
    return l;
}

void print_array(laptop l)
{
    for (int i = 0; i < 5; i++)
    {
        printf("(( for laptop %d the array is in index %d = %d ))\n", l.laptop_id, i, l.visited_techs[i]);
    }
}