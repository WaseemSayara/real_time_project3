#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>


#define MAXTHRESHOLD 40
#define MINTHRESHOLD 20

pthread_t technical_employee[10][10], storage_employee;

pthread_mutex_t carton_box_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t carton_box_threshold_cv = PTHREAD_COND_INITIALIZER;

pthread_mutex_t count_mutex[10] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t count_threshold_cv[10][2] = {{PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}};

struct technical
{
    int line;
    int worker;
} technical;

int steps_finished[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int laptops_in_carton_box, laptops_in_storage_room = 0;

/* function to be executed by the new thread */
void *execute_step(struct technical *data)
{

    int technical = data->worker;
    int line = data->line;

    printf("line %d , technical %d created\n", line, technical);
    int k = 10;
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
            printf("line %d , technical %d put the laptop in the carton box\n", line, technical);
            printf("laptops_in_carton_box = %d - laptops_in_storage_room = %d\n", ++laptops_in_carton_box, laptops_in_storage_room);
            
            steps_finished[line] = 0;
            sleep(1);

            if (laptops_in_carton_box >= 10)
            {
                pthread_cond_signal(&carton_box_threshold_cv);
                while (laptops_in_carton_box >= 10)
                    ;
                //sleep(2);
            }
            pthread_cond_signal(&count_threshold_cv[line][1]);
        }

        pthread_mutex_unlock(&count_mutex[line]);
    }
}
void *collect_filled_carton(struct technical *data)
{
    printf("Storage Employee Created!\n");
    while (1)
    {
        pthread_mutex_lock(&carton_box_mutex);
        pthread_cond_wait(&carton_box_threshold_cv, &carton_box_mutex);
        printf("The Storage Employee collecting the filled carton and placing it in the storage room...\n");
        sleep(2);
        laptops_in_storage_room+=10;
        laptops_in_carton_box = 0;
        printf("The Storage Employee Finished his task\n");
        
        pthread_mutex_unlock(&carton_box_mutex);
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
                perror("failed to create technical");
            }

            usleep(2500);
        }
    int a = 1;
    if (pthread_create(&storage_employee, NULL, (void *)collect_filled_carton, (void *)&a) != 0)
    {
        perror("failed to create thread");
    }
    sleep(1);
    for (int i = 0; i < 10; i++)
        pthread_cond_signal(&count_threshold_cv[i][1]);

    sleep(200);
    printf("thread ended with status\n");

    /* NOT REACHED */
    return 0;
}
