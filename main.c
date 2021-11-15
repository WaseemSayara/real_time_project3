#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>

#define MAXTHRESHOLD 16
#define MINTHRESHOLD 14
#define numOfLoadingEmployees 4

pthread_t technical_employee[10][10], storage_employee, loading_employee;

pthread_mutex_t loading_mutex, cartonbox_mutex, storage_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t loading_threshold_cv = PTHREAD_COND_INITIALIZER;

pthread_mutex_t count_mutex[10] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t count_threshold_cv[10][2] = {{PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}};

struct technical
{
    int line;
    int worker;
} technical;

int steps_finished[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int laptops_in_carton_box, laptops_in_storage_room = 0;

/* function to be executed by the TECHNICAL EMPLOYEE thread */
void *execute_step(struct technical *data)
{

    int technical = data->worker;
    int line = data->line;

    printf("line %d , technical %d created\n", line, technical);
    int k = 20;
    while (k--)
    {

        if (pthread_mutex_lock(&count_mutex[line]) == -1)
        {
            perror("error");
        }
        if (technical <= 4)
            pthread_cond_wait(&count_threshold_cv[line][1], &count_mutex[line]);
        else
            pthread_cond_wait(&count_threshold_cv[line][0], &count_mutex[line]);

        printf("line %d , technical %d\n", line, technical);
        sleep(1);
        if (technical >= 0 && technical < 4)
        {
            pthread_cond_signal(&count_threshold_cv[line][1]);
        }
        else if (technical == 4)
        {
            pthread_cond_broadcast(&count_threshold_cv[line][0]);
        }
        steps_finished[line]++;
        if (steps_finished[line] == 10)
        {

            printf("\033[0;36mline %d , technical %d put the laptop in the carton box\n\033[0m", line, technical);
            printf("\033[0;33mlaptops_in_carton_box = %d\n\033[0m", ++laptops_in_carton_box);
            steps_finished[line] = 0;
            sleep(1);
            pthread_mutex_lock(&cartonbox_mutex);
            while (laptops_in_carton_box >= 10)
                ;
            //sleep(2);
            pthread_mutex_unlock(&cartonbox_mutex);
            pthread_cond_signal(&count_threshold_cv[line][1]);
        }
        if (laptops_in_storage_room >= MAXTHRESHOLD)
        {
            //printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
            while (laptops_in_storage_room >= MINTHRESHOLD)
                ;
        }
        pthread_mutex_unlock(&count_mutex[line]);
    }
}

/* function to be executed by the STORAGE EMPLOYEE thread */
void *collect_filled_carton(void *data)
{
    printf("Storage Employee Created!\n");
    while (1)
    {
        pthread_mutex_lock(&storage_mutex);
        if (laptops_in_carton_box >= 10)
        {

            printf("\033[0;32mThe Storage Employee collecting the filled carton and placing it in the storage room...\n\033[0m");
            sleep(2);
            printf("\033[0;32mThe Storage Employee Finished his task, laptops_in_storage_room = %d\n\033[0m", (laptops_in_storage_room + 10));

            laptops_in_storage_room += 10;

            //pthread_cond_broadcast(&loading_threshold_cv);
            laptops_in_carton_box = 0;
        }
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

            laptops_in_storage_room--;

            //cyan();
            printf("\033[0;31mLoading Emp No.%d loads 1 Laptop, laptops_in_storage_room = %d, laptops_in_carton_box = %d\n\033[0m", me, laptops_in_storage_room, laptops_in_carton_box);
            //white();
            pthread_mutex_unlock(&storage_mutex);
            sleep(6);
        }
        pthread_mutex_unlock(&loading_mutex);
        sleep(1);
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

    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
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
    for (int i = 0; i < 10; i++)
        pthread_cond_signal(&count_threshold_cv[i][1]);

    sleep(200);
    printf("thread ended with status\n");

    /* NOT REACHED */
    return 0;
}