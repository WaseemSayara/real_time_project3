#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <unistd.h>
#include <sched.h>

pthread_t p_thread[10][10];
pthread_mutex_t count_mutex[10] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t count_threshold_cv[10][2] = {{PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}, {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER}};

struct technical
{
    int line;
    int worker;
} technical;

int steps_finished[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* function to be executed by the new thread */
void *execute_step(struct technical *data)
{

    int worker = data->worker;
    int line = data->line;

    printf("line %d , worker %d created\n", line, worker);
    int k = 10;
    while (k--)
    {
        if (pthread_mutex_lock(&count_mutex[line]) == -1)
        {
            perror("hi");
        }
        if (worker <= 4)
            pthread_cond_wait(&count_threshold_cv[line][1], &count_mutex[line]);
        else
            pthread_cond_wait(&count_threshold_cv[line][0], &count_mutex[line]);

        printf("line %d , worker %d\n", line, worker);
        sleep(1);
        if (worker >= 0 && worker < 4)
        {
            pthread_cond_signal(&count_threshold_cv[line][1]);
        }
        else if (worker == 4)
        {
            pthread_cond_broadcast(&count_threshold_cv[line][0]);
        }
        steps_finished[line]++;
        if (steps_finished[line] == 10)
        {
            printf("line %d , worker %d finished the last step\n", line, worker);
            steps_finished[line] = 0;
            sleep(1);
            pthread_cond_signal(&count_threshold_cv[line][1]);
        }

        pthread_mutex_unlock(&count_mutex[line]);
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
            if (pthread_create(&p_thread[i][j], &tattr, (void *)execute_step, (void *)&x) != 0)
            {
                perror("failed to create thread");
            }

            usleep(2500);
        }

    for (int i = 0; i < 10; i++)
        pthread_cond_signal(&count_threshold_cv[i][1]);

    sleep(200);
    printf("thread ended with status\n");

    /* NOT REACHED */
    return 0;
}
