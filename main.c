#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define MAXTHRESHOLD 16
#define MINTHRESHOLD 14
#define numOfLoadingEmployees 4
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

struct technical
{
    int line;
    int worker;
} technical;

int steps_finished[numOfLines] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int laptops_in_carton_box, laptops_in_storage_room, current_truck = 0;

int trucks[4];
time_t trucks_time[4];

/* function to be executed by the TECHNICAL EMPLOYEE thread */
void *execute_step(struct technical *data)
{

    int technical = data->worker;
    int line = data->line;

    printf("line %d , technical %d created\n", line, technical);
    int k = 20;
    while (k--)
    {

        if (pthread_mutex_lock(&count_mutex[line]) != 0)
            perror("error");

        if (technical <= 4)
        {
            if (pthread_cond_wait(&count_threshold_cv[line][technical], &count_mutex[line]) != 0)
                perror("error");
        }

        else
        {
            if (pthread_cond_wait(&count_threshold_cv[line][5], &count_mutex[line]) != 0)
                perror("error");
        }

        printf("line %d , technical %d\n", line, technical);

        // step time
        sleep(1);
        steps_finished[line]++;
        if (steps_finished[line] == numOfSteps)
        {

            printf("\033[0;36mline %d , technical %d put the laptop in the carton box\n\033[0m", line, technical);
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
        }
        if (laptops_in_storage_room >= MAXTHRESHOLD)
        {
            //printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
            while (laptops_in_storage_room >= MINTHRESHOLD)
                sleep(1);
        }

        if (technical < 4)
        {
            if (pthread_cond_signal(&count_threshold_cv[line][technical + 1]) != 0)
                perror("error");
        }
        else if (technical == 4)
        {
            if (pthread_cond_broadcast(&count_threshold_cv[line][5]) != 0)
                perror("error");
        }

        if (pthread_mutex_unlock(&count_mutex[line]) != 0)
            perror("error");
    }
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
        printf(" -------- loader num %d is looking ---------\n", me);
        pthread_mutex_lock(&loading_mutex);
        if (laptops_in_storage_room > 0)
        {
            printf("hi\n");
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
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newprio = 20;
    struct sched_param param;

    for (int i = 0; i < numOfTrucks; i++)
    {
        trucks[i] = 0;
    }

    /* initialized with default attributes */
    ret = pthread_attr_init(&tattr);

    /* safe to get existing scheduling param */
    ret = pthread_attr_getschedparam(&tattr, &param);

    /* set the priority; others are unchanged */
    param.sched_priority = newprio;

    /* setting the new scheduling param */
    ret = pthread_attr_setschedparam(&tattr, &param);

    /* with new priority specified */

    printf("hello world\n");

    for (int i = 0; i < numOfLines; i++)
        for (int j = 0; j < numOfSteps; j++)
        {
            struct technical x;
            x.line = i;
            x.worker = j;
            if (j <= 4)
            {
                param.sched_priority = newprio - j;

                /* setting the new scheduling param */
                ret = pthread_attr_setschedparam(&tattr, &param);
            }
            else
            {
                param.sched_priority = newprio - 10;

                /* setting the new scheduling param */
                ret = pthread_attr_setschedparam(&tattr, &param);
            }
            if (pthread_create(&technical_employee[i][j], &tattr, (void *)execute_step, (void *)&x) != 0)
            {
                perror("failed to create technical_employee thread");
            }

            usleep(2500);
        }
    int a = 1;
    if (pthread_create(&storage_employee, NULL, (void *)collect_filled_carton, (void *)&a) != 0)
    {
        perror("failed to create storage_employee thread");
    }
    for (int i = 0; i < numOfLoadingEmployees; i++)
    {
        if (pthread_create(&loading_employee, NULL, (void *)load_truck, (void *)&i) != 0)
        {
            perror("failed to create loading_employee thread");
        }
        usleep(2500);
    }
    printf("-------------------------------\n");
    sleep(1);
    for (int i = 0; i < numOfLines; i++)
        pthread_cond_signal(&count_threshold_cv[i][0]);

    sleep(200);
    printf("thread ended with status\n");

    /* NOT REACHED */
    return 0;
}