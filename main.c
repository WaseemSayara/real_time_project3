/*
* Laptop-manufacturing factory
*/

#include "local.h"

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
    // Get thread line and worker number
    int technical = data->worker;
    int line = data->line;

    srand(pthread_self());

    while (1)
    {
        // check if the line is on
        if (off_line[line] == 0)
        {
            // The first technical worker choose the line work for laptop
            if (technical == 0)
            {
                pthread_mutex_lock(&line_full_mutex[line]);
                // check the current created laptops in the this line
                printf("line %d current count = %d\n", line, counts[line]);
                if (counts[line] < 10)
                {
                    // create new laptop
                    laptop new_laptop;
                    unsigned int id = line * 10000 + ids[line] + 1; // linenumber * 10000 + current id for this line is used to identify laptops
                    new_laptop.laptop_id = id;
                    new_laptop.finished_steps = 0;
                    // make the array of visited steps all 0 at start
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
                        //step time with range ( min to max ) in arguments file
                        int sleep_time = (rand() % ((step_max_time + 1) - step_min_time)) + step_min_time;
                        sleep(sleep_time);
                        new_laptop.finished_steps++;
                        printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished step: \033[0;31m%d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                        // commit changes
                        return_laptop(lists[line], new_laptop);
                        pthread_cond_signal(&finised_step_cond[line][technical]);
                    }
                    else
                    {
                        printf("error got a wronge laptop\n");
                        exit(1);
                    }
                }
                else
                {
                    printf("line %d is full\n", line);
                    pthread_cond_wait(&line_full_cond[line], &line_full_mutex[line]);
                    printf("line %d is nott full\n", line);
                }
                pthread_mutex_unlock(&line_full_mutex[line]);
            }

            else if (technical >= 1 && technical <= 4)
            {
                laptop new_laptop;
                // get the laptop with finished steps as his number
                new_laptop = get_free_laptop_with_steps(lists[line], technical);
                if (new_laptop.laptop_id != -1)
                {
                    //step time
                    int sleep_time = (rand() % ((step_max_time + 1) - step_min_time)) + step_min_time;
                    sleep(sleep_time);
                    new_laptop.finished_steps++;
                    printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished step: \033[0;31m%d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                    return_laptop(lists[line], new_laptop);
                    pthread_cond_signal(&finised_step_cond[line][technical]);
                }
                else
                {
                    pthread_mutex_lock(&finised_step_mutex[line][technical - 1]);
                    pthread_cond_wait(&finised_step_cond[line][technical - 1], &finised_step_mutex[line][technical - 1]);
                }
                pthread_mutex_unlock(&finised_step_mutex[line][technical - 1]);
            }

            else
            {
                laptop new_laptop;
                if (pthread_mutex_lock(&get_laptop) != 0)
                    perror("error");
                new_laptop = get_free_laptop(lists[line], technical);
                if (pthread_mutex_unlock(&get_laptop) != 0)
                    perror("error");
                if (new_laptop.laptop_id != -1)
                {
                    //step time
                    int sleep_time = (rand() % ((step_max_time + 1) - step_min_time)) + step_min_time;
                    sleep(sleep_time);
                    int index = new_laptop.finished_steps - 5;
                    new_laptop.visited_techs[index] = technical;
                    // print_array(new_laptop);
                    new_laptop.finished_steps = (new_laptop.finished_steps) + 1;
                    printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished step: \033[0;31m%d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);
                    if (new_laptop.finished_steps == numOfSteps)
                    {
                        printf("\033[0;36mline %d , technical %d put the laptop %d in the carton box\n\033[0m", line, technical, new_laptop.laptop_id);
                        steps_finished[line] = 0;

                        if (pthread_mutex_lock(&cartonbox_mutex) != 0)
                            perror("error");

                        // while (laptops_in_carton_box >= numOfLines)
                        //     ;
                        printf("\033[0;31m*********** laptops_in_carton_box = %d ***********\n\033[0m", ++laptops_in_carton_box);
                        if (pthread_mutex_unlock(&cartonbox_mutex) != 0)
                            perror("error");
                        sleep(1);
                        // if (pthread_cond_signal(&count_threshold_cv[line][0]) != 0)
                        //     perror("error");
                        remove_laptop(lists[line], new_laptop.laptop_id);
                        counts[line]--;
                        pthread_cond_signal(&line_full_cond[line]);
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
            //printf("laptops_in_storage_room : %d\n", laptops_in_storage_room);
            if (laptops_in_storage_room >= STORAGEMAXTHRESHOLD)
            {
                while (laptops_in_storage_room >= STORAGEMINTHRESHOLD)
                    sleep(1);
            }
        }
        else
        {
            sleep(5);
        }
    }
}

/* function to be executed by the STORAGE EMPLOYEE thread */
void *collect_filled_carton(void *data)
{
    printf("Storage Employee Created!\n");
    while (1)
    {
        if (pthread_mutex_lock(&cartonbox_mutex) != 0)
            perror("error");
        if (laptops_in_carton_box >= numOfLines)
        {
            printf("\033[0;32mThe Storage Employee collecting..\n\033[0m");
            laptops_in_carton_box -= numOfLines;
            sleep(StorageEmpPeriod);
            printf("\033[0;32mThe Storage Employee Finished, laptops_in_storage_room = %d\n\033[0m", (laptops_in_storage_room + numOfLines));
            if (pthread_mutex_lock(&storage_mutex) != 0)
                perror("error");
            laptops_in_storage_room += numOfLines;
            if (pthread_mutex_unlock(&storage_mutex) != 0)
                perror("error");
        }
        if (pthread_mutex_unlock(&cartonbox_mutex) != 0)
            perror("error");
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
        if (pthread_mutex_lock(&loading_mutex) != 0)
            perror("error");
        if (laptops_in_storage_room > 0)
        {
            if (pthread_mutex_lock(&storage_mutex) != 0)
                perror("error");
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
                exportedLaptops++;
                printf("\033[0;35mLoading Emp No.%d loading, storage_room = %d, carton_box = %d\n\033[0m", me, laptops_in_storage_room, laptops_in_carton_box);
                printf("\033[0;32mTruck No.%d has %d laptops now\n\033[0m", current_truck, trucks[current_truck]);
                if (trucks[current_truck] == capacityOfTruck)
                {
                    printf("\033[0;32mTruck No.%d is full and going to unload\n\033[0m", current_truck);
                    current_time = time(NULL);
                    trucks_time[current_truck] = current_time;
                    current_truck = (current_truck + 1) % numOfTrucks;
                }
            }
            if (pthread_mutex_unlock(&storage_mutex) != 0)
                perror("error");
        }
        if (pthread_mutex_unlock(&loading_mutex) != 0)
            perror("error");
        sleep(10);
    }
}
void *calculate_profit(int *data)
{
    while (1)
    {
        // TODO: add ifs to check if the program ends use the comment below to know the states, let each one retuen a different status and check the status in the main.
        //    while (numOfLines >= (OriginalnumOfLines / (float)2) && (gains - expenses) < PROFITCEIL);
        sleep(30);
        printf("Calculating..\n\n");
        expenses = (exportedLaptops * CostFAB) + SalaryCEO + SalaryHR +
                   (SalaryT * 10 * numOfLines) + SalaryS +
                   (SalaryL * numOfLoadingEmployees) +
                   (SalaryU * numOfTrucks) + (SalaryA * 0);
        gains_this_month = exportedLaptops * PriceSELL;
        printf("Profit: %d\n", gains_this_month - expenses);
        if ((gains_this_month - expenses) < PROFITMINTHRESHOLD)
        {
            numOfLines--;
            printf("Suspend a Line! ( LINE %d is going DOWN )\n\n", numOfLines);
            off_line[numOfLines] = 1;
        }
        if ((gains_this_month - expenses) > PROFITMAXTHRESHOLD && numOfLines != OriginalnumOfLines)
        {
            off_line[numOfLines] = 0;
            printf("Cancel one Suspended Line! ( LINE %d is going UP )\n\n", numOfLines);
            numOfLines++;
        }
        total_gains += gains_this_month;
        exportedLaptops = 0;
    }
}
/* like any C program, program's execution begins in main */
int main(int argc, char *argv[])
{

    char tmp[20];
    FILE *arguments;

    arguments = fopen("arguments.txt", "r");
    fscanf(arguments, "%s %d\n", tmp, &STORAGEMAXTHRESHOLD);
    fscanf(arguments, "%s %d\n", tmp, &STORAGEMINTHRESHOLD);
    fscanf(arguments, "%s %d\n", tmp, &StorageEmpPeriod);
    fscanf(arguments, "%s %d\n", tmp, &numOfLoadingEmployees);
    fscanf(arguments, "%s %d\n", tmp, &capacityOfTruck);
    fscanf(arguments, "%s %d\n", tmp, &TruckTrevelTime);
    fscanf(arguments, "%s %d\n", tmp, &GAINCEIL);
    fscanf(arguments, "%s %d\n", tmp, &PROFITMAXTHRESHOLD);
    fscanf(arguments, "%s %d\n", tmp, &PROFITMINTHRESHOLD);
    fscanf(arguments, "%s %d\n", tmp, &SalaryCEO);
    fscanf(arguments, "%s %d\n", tmp, &SalaryHR);
    fscanf(arguments, "%s %d\n", tmp, &SalaryT);
    fscanf(arguments, "%s %d\n", tmp, &SalaryS);
    fscanf(arguments, "%s %d\n", tmp, &SalaryL);
    fscanf(arguments, "%s %d\n", tmp, &SalaryU);
    fscanf(arguments, "%s %d\n", tmp, &SalaryA);
    fscanf(arguments, "%s %d\n", tmp, &CostFAB);
    fscanf(arguments, "%s %d\n", tmp, &PriceSELL);
    fscanf(arguments, "%s %d %d\n", tmp, &step_min_time, &step_max_time);
    fclose(arguments);
    numOfLines = OriginalnumOfLines;

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

    printf("Welcome to Our Factory!\n");

    /*
    * Reduce codue using array of laptop struct !!!!!!!!!!!!!!!!!!
    */

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
    if (pthread_create(&accountant, NULL, (void *)calculate_profit, (void *)&a) != 0)
    {
        perror("failed to create storage_employee thread");
    }

    void *status;

    pthread_join(accountant, &status);
    printf("status is : %d \n", status);
    
    // TODO: check why ir ended to print it, and kill all , use status from the line above
    printf("\n\n TOTAL GAIN IS %d", total_gains);
    printf("\n\n$$$$$$$$$$$$$$$$$$$$$  The END  $$$$$$$$$$$$$$$$$$$$$\n\n");
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
        printf("NULL;\n");
        return no_free;
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
                    return tmp->my_laptop;
                }
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
            if (pthread_mutex_unlock(&tmp->my_laptop.laptop_mutex) != 0)
                perror("error");
            return 0;
        }
        else
        {
            tmp = tmp->next;
        }
    }
    return -1;
}

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
        if (pthread_mutex_unlock(&temp->my_laptop.laptop_mutex) != 0)
            perror("error");
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
            if (pthread_mutex_unlock(&temp2->my_laptop.laptop_mutex) != 0)
                perror("error");
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
    printf("--------------------------------------------------\n");
    for (int i = 0; i < 5; i++)
    {
        printf("|| for laptop %5d the array is in index %d = %d ||\n", l.laptop_id, i, l.visited_techs[i]);
    }
    printf("--------------------------------------------------\n");
}