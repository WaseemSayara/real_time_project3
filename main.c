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

    sleep(2);

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
                        printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m is created \n", line, technical, new_laptop.laptop_id);
                        // commit changes
                        return_laptop(lists[line], new_laptop);
                        pthread_cond_signal(&finised_step_cond[line][technical]);
                    }
                    else // There's no vaild laptop
                    {
                        printf("error got a wronge laptop\n");
                        exit(1);
                    }
                }
                else // All lines are full so wait until one of them be empty
                {
                    printf("line %d is full of laptops\n", line);
                    pthread_cond_wait(&line_full_cond[line], &line_full_mutex[line]);
                    printf("line %d is available\n", line);
                }
                pthread_mutex_unlock(&line_full_mutex[line]);
            }
            // Technical from 1 to 4 do their job in order
            else if (technical >= 1 && technical <= 4)
            {
                laptop new_laptop;
                // get the laptop with finished steps as his number
                new_laptop = get_free_laptop_with_steps(lists[line], technical);
                // Finish work on laptop in a random processing time then increment finished steps
                if (new_laptop.laptop_id != -1)
                {
                    //step time
                    int sleep_time = (rand() % ((step_max_time + 1) - step_min_time)) + step_min_time;
                    sleep(sleep_time);
                    new_laptop.finished_steps++;
                    // Show finished laptop information
                    // printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished step: \033[0;31m%d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);

                    return_laptop(lists[line], new_laptop);
                    // Thread finish this step and send signal to the other threads in condition
                    pthread_cond_signal(&finised_step_cond[line][technical]);
                }
                else
                {
                    pthread_mutex_lock(&finised_step_mutex[line][technical - 1]);
                    pthread_cond_wait(&finised_step_cond[line][technical - 1], &finised_step_mutex[line][technical - 1]);
                }
                pthread_mutex_unlock(&finised_step_mutex[line][technical - 1]);
            }
            // Laptop go to technical workers from 5 to 9
            // work on the first valid laptop order isn't important
            else
            {
                laptop new_laptop;
                if (pthread_mutex_lock(&get_laptop) != 0)
                    perror("error");
                new_laptop = get_free_laptop(lists[line], technical);
                if (pthread_mutex_unlock(&get_laptop) != 0)
                    perror("error");
                // Check if there's laptop wait those technical workers
                if (new_laptop.laptop_id != -1)
                {
                    //step time
                    int sleep_time = (rand() % ((step_max_time + 1) - step_min_time)) + step_min_time;
                    sleep(sleep_time);
                    int index = new_laptop.finished_steps - 5;
                    new_laptop.visited_techs[index] = technical;
                    // print_array(new_laptop);
                    new_laptop.finished_steps = (new_laptop.finished_steps) + 1;
                    // printf("line \033[0;32m%d\033[0m, technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished step: \033[0;31m%d\n\033[0m", line, technical, new_laptop.laptop_id, new_laptop.finished_steps);

                    /*
                    * Check if the laptop finished all manufacturing steps
                    */
                    if (new_laptop.finished_steps == numOfSteps)
                    {
                        printf("line \033[0;32m%d\033[0m, Last technical \033[0;31m%d\033[0m, laptop id: \033[0;32m%5d\033[0m, finished steps: \033[0;31m0\033[0m -> \033[0;31m1\033[0m -> \033[0;31m2\033[0m -> \033[0;31m3\033[0m -> \033[0;31m4\033[0m -> \033[0;31m%d\033[0m -> \033[0;31m%d\033[0m -> \033[0;31m%d\033[0m -> \033[0;31m%d\033[0m -> \033[0;31m%d\033[0m\n", line, technical, new_laptop.laptop_id, new_laptop.visited_techs[0], new_laptop.visited_techs[1], new_laptop.visited_techs[2], new_laptop.visited_techs[3], new_laptop.visited_techs[4]);
                        printf("\033[0;36mline %d , technical %d put the laptop %d in the carton box\n\033[0m", line, technical, new_laptop.laptop_id);
                        steps_finished[line] = 0; // Reset manufacturing steps counter

                        if (pthread_mutex_lock(&cartonbox_mutex) != 0)
                            perror("error");

                        printf("\033[0;31m*********** laptops_in_carton_box = %d ***********\n\033[0m", ++laptops_in_carton_box);
                        if (pthread_mutex_unlock(&cartonbox_mutex) != 0)
                            perror("error");

                        // Boxing Time
                        sleep(1);
                        // remove laptop from linked list
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
                    // sleep for 1 seconds when all laptops are under manifacturing
                    sleep(1);
                }
            }

            if (laptops_in_storage_room >= STORAGEMAXTHRESHOLD)
            {
                printf("THE STORAGE HAS REACHED MAX THRESHOLD ( STOP ACCEPTING LAPTOPS )\n");
                while (laptops_in_storage_room >= STORAGEMINTHRESHOLD)
                    sleep(1);
                printf("THE STORAGE HAS REACHED MIN THRESHOLD ( CONTINUE ACCEPTING LAPTOPS )\n");
            }
        }
        else
        {
            // check if line is up again
            sleep(2);
        }
    }
}

/* function to be executed by the STORAGE EMPLOYEE thread */
void *collect_filled_carton(void *data)
{
    while (1)
    {
        if (pthread_mutex_lock(&cartonbox_mutex) != 0)
            perror("error");
        if (laptops_in_carton_box >= numOfLines)
        {
            printf("\033[0;32mThe Storage Employee collecting..\n\033[0m");
            laptops_in_carton_box -= numOfLines;
            sleep(StorageEmpPeriod); // Time until the storage employee finish his work
            printf("\033[0;32mThe Storage Employee Finished, laptops_in_storage_room = %d\n\033[0m", (laptops_in_storage_room + numOfLines));

            if (pthread_mutex_lock(&storage_mutex) != 0)
                perror("error");
            laptops_in_storage_room += numOfLines; // Increment storage counter for each laptop stored in the room

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
    /*
    * Load laptops to truck
    */
    while (1)
    {
        if (pthread_mutex_lock(&loading_mutex) != 0)
            perror("error");
        /*
        * Check if there's a ready laptops in the storage room
        */
        if (laptops_in_storage_room > 0)
        {
            if (pthread_mutex_lock(&storage_mutex) != 0)
                perror("error");
            if (trucks[current_truck] == capacityOfTruck)
            {
                printf("\033[0;36mTruck No.%d is full\n\033[0m", current_truck);
                current_time = time(NULL);
                int delta_time = current_time - trucks_time[current_truck];
                if (delta_time >= (int)TruckTrevelTime)
                {
                    trucks[current_truck] = 0;
                }
            }
            /*
            * Check if there's an empty space into truck or not
            */
            if (trucks[current_truck] < capacityOfTruck)
            {
                laptops_in_storage_room--;
                trucks[current_truck]++;
                exportedLaptops++;
                // Print loading information
                printf("\033[0;35mLoading Emp No.%d loading, storage_room = %d, carton_box = %d\n\033[0m", me, laptops_in_storage_room, laptops_in_carton_box);
                printf("\033[0;32mTruck No.%d has %d laptops now\n\033[0m", current_truck, trucks[current_truck]);
                // When truck is full it will unload the laptops
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

/* Function to calculate profits of the manufacturing process */

void *calculate_profit(int *data)
{
    while (1)
    {
        // month period
        sleep(30);
        printf("Calculating..\n\n");
        expenses = (exportedLaptops * CostFAB) + SalaryCEO + SalaryHR +
                   (SalaryT * 10 * numOfLines) + SalaryS +
                   (SalaryL * numOfLoadingEmployees) +
                   (SalaryU * numOfTrucks) + (SalaryA * 0);
        gains_this_month = exportedLaptops * PriceSELL;

        int month_profit = gains_this_month - expenses;

        if (month_profit > PROFITMAXTHRESHOLD)
        {
            printf("Profit This Month: \033[0;32m%d\033[0m\n", month_profit);
        }
        else if (month_profit < PROFITMINTHRESHOLD)
        {
            printf("Profit This Month: \033[0;31m%d\033[0m\n", month_profit);
        }
        else{
            printf("Profit This Month: %d\n", month_profit);
        }
        if ((gains_this_month - expenses) <= PROFITMINTHRESHOLD)
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

        if (numOfLines < (OriginalnumOfLines / (float)2))
        {
            return 5;
        }

        if (total_gains >= GAINCEIL)
        {
            return 6;
        }
    }
}
/* like any C program, program's execution begins in main */
int main(int argc, char *argv[])
{

    char tmp[20];
    FILE *arguments;
    int a = 1;

    /*
    * Read given values from argument.txt file 
    */
    arguments = fopen("arguments.txt", "r");
    if (arguments == NULL)
    {
        perror("CANT OPEN FILE");
        exit(1);
    }
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

    // Initialize all trucks to 0
    for (int i = 0; i < numOfTrucks; i++)
    {
        trucks[i] = 0;
    }

    for (int i = 0; i < numOfLines; i++)
    {
        lists[i] = createList();
    }

    printf("Welcome to Our Factory!\n");

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

    // Number of lines greater than half of them
    if (status == 5)
    {
        printf("Number of lines fgreater than half of them\n");
    }

    if (status == 6)
    {
        printf("Profits reach the required value\n");
    }
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