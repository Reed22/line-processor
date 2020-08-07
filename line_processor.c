#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE 1000           //Max characters per line
#define LINES 10                //Max lines in 5 second interval
#define SIZE MAX_LINE * LINES   //Size of buffers
#define CHAR_PER_LINE 80        //Output line length

#define  _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L

char* buffer1;  //Shared between input and line_sep
char* buffer2;  //Shared between line_sep and plus_replace
char* buffer3;  //Shared between plus_replace and output

bool terminate = false; //Flag that determines whether terminating string found

//Initialize mutexes 
pthread_mutex_t buffer1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer3_mutex = PTHREAD_MUTEX_INITIALIZER;


// Initialize the condition variables
pthread_cond_t buf1_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf1_empty = PTHREAD_COND_INITIALIZER;

pthread_cond_t buf2_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf2_empty = PTHREAD_COND_INITIALIZER;

pthread_cond_t buf3_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf3_empty = PTHREAD_COND_INITIALIZER;

/****************************************************
*       Start of *line_sep producer, process, and
*       consumer functions
*****************************************************/

//Consumer function: Extracts buffer1 into a temporary
//char array, then emptys buffer1
char* get_buffer1(){
    char temp[strlen(buffer1)];
    strcpy(temp, buffer1);
    memset(buffer1, '\0', strlen(buffer1));
    return temp;
}

//Processor function: Goes through temp and changes
//any line separators to space characters
char* replace_line_separators(char* temp){
    for(int i = 0; i < strlen(temp); i++){
        if(temp[i] == '\r' || temp[i] == '\n')
            temp[i] = ' ';
    }
    return temp;
}

//Producer function: Copies or concatenate 
//temp to buffer2
void fill_buffer2(char* temp){
    if(strlen(buffer2) == 0){
        strcpy(buffer2, temp);
    }
    else{
        strcat(buffer2, temp);
    }
}

/****************************************************
*       End of *line_sep producer, process, and
*       consumer functions
*****************************************************/

/****************************************************
*       Start of *line_sep producer, process, and
*       consumer functions
*****************************************************/

//Consumer function: Extracts buffer1 into a temporary
//char array, then emptys buffer1
char* get_buffer2(){
    char temp[strlen(buffer2)];
    strcpy(temp, buffer2);
    memset(buffer2, '\0', strlen(buffer2));
    return temp;
}

//Processor function: Goes through temp and changes
//pairs of plus signs to ^ character
char* replace_plus_signs(char* temp){    
    int temp_ind = 0;
    int new_temp_ind = 0;
    int temp_len = strlen(temp);

    char new_temp[temp_len];
    memset(new_temp, '\0', strlen(new_temp));

    printf("temp_len = %d\n", temp_len);

    while(new_temp_ind < temp_len){
        if(temp[temp_ind] == '+' && temp[temp_ind + 1] == '+'){
            new_temp[new_temp_ind] = '^';
            temp_ind += 2;
        }
        else{
            new_temp[new_temp_ind] = temp[temp_ind];
            temp_ind++;
        }
        new_temp_ind++;
    }
    return new_temp;
}

//Producer function: Copies or concatenate 
//temp to buffer2
void fill_buffer3(char* temp){
    if(strlen(buffer3) == 0){
        strcpy(buffer3, temp);
    }
    else{
        strcat(buffer3, temp);
    }
}

/***********************************************
*               bool input_p()
*
* Reads user input into temporary buffer and 
* adds it to global variable buffer1
*
* Returns true if terminating string found,
* false otherwise
***********************************************/
bool input_p(){
    size_t len = SIZE;  
    char* tempBuf = (char *)malloc(MAX_LINE * sizeof(char)); //Temporary buffer that will add to buffer1
    char buf[256];
    getline(&tempBuf, &len, stdin);

    //sscanf will strip the carriage return from tempBuf before placing in buf
    //This will make the strcmp work as expected
    sscanf(tempBuf, "%s", buf);

    //If terminating input line is found, 
    //Need to account for input like "DONE more stuff"
    //sscanf will strip off first word and mistake above example as terminating
    if(strcmp(buf, "DONE") == 0){
        return true;
    }

    //Otherwise, insert string into buffer1
    else{
        //If buffer is empty, perform strcpy, else strcat
        if(strlen(buffer1) == 0)
            strcpy(buffer1, tempBuf);
        else
            strcat(buffer1, tempBuf);
    }
    free(tempBuf);
    return false;
}

/***********************************************
*               void *input()
*
* Thread handling function for input
* Producer only
***********************************************/
void *input(void *args){
    while(terminate == false){
        //printf("in *input\n");

        //Lock buffer1_mutex before changing buffer1
        pthread_mutex_lock(&buffer1_mutex);

        //printf("    *input has lock\n");

        //SHOULDN'T NEED THIS(WAITS FOR BUFFER1 TO BE EMPTY)
        while(strlen(buffer1) > 0){
            //printf("        *input waiting for buf1_empty, giving up lock\n");
            pthread_cond_wait(&buf1_empty, &buffer1_mutex);
        }

        //Perform input
        terminate = input_p();
        printf("Buffer1: %s\n", buffer1);

        //Signal to line consumer that buffer1 is no longer empty
        pthread_cond_signal(&buf1_full);
        //printf("*input signalled buf1_full\n");

        //Unlock mutex
        pthread_mutex_unlock(&buffer1_mutex);
        //printf("*input unlocked buffer1_mutext\n");
    }
}

/***********************************************
*          void *line_sep(void *args)
*
* Thread handler for line_sep
* Producer and Consumer
***********************************************/
void *line_sep(void *args){
    while(terminate == false){
        //printf("in *line_sep\n");

        //Lock buffer1_mutex before using buffer1
        pthread_mutex_lock(&buffer1_mutex);
        //printf("    *line_sep consumer has buffer1_mutex\n");

        //Wait until buffer1 has something in it
        //Signal coming from *input
        while(strlen(buffer1) == 0){
            //printf("        *line_Sep waiting for buf1_full, giving up buffer1_mutex\n");
            pthread_cond_wait(&buf1_full, &buffer1_mutex);
        }

        //Acquire buffer1
        char* temp = get_buffer1();

        //SHOULDN'T NEED TO DO THIS (SIGNALS TO *INPUT THAT BUFFER 1 IS EMPTY)
        pthread_cond_signal(&buf1_empty);

        //Unlock buffer1_mutex so *input can do its thing
        pthread_mutex_unlock(&buffer1_mutex);
        //printf("*line_sep has unlocked buffer1_mutex\n");

       
        //Process temp
        replace_line_separators(temp);


        //Producer Buffer2
        //Try to take buffer2_mutex if plus_rep doesn't have it
        pthread_mutex_lock(&buffer2_mutex);

        //printf("line_sep has buffer2_mutex\n");
        //printf("strlen(buffer2) = %d\n", strlen(buffer2));

        //SHOULDN'T NEED THIS(WAITS FOR BUFFER2 TO BE EMPTY)
        while(strlen(buffer2) > 0){
            //printf("        *input waiting for buf1_empty, giving up lock\n");
            pthread_cond_wait(&buf2_empty, &buffer2_mutex);
        }

        //Put temp variable in buffer2
        fill_buffer2(temp);
        printf("Buffer2: %s\n", buffer2);

        //Singal to plus_rep that buffer2 is no longer empty
        pthread_cond_signal(&buf2_full);

        //Unlock buffer2_mutex so plus_rep can do its thing
        pthread_mutex_unlock(&buffer2_mutex);
        //printf("                line_sep is unlocking buffer2_mutex\n");    
    }
}
/***********************************************
*          void *plus_rep(void *args)
*
* Thread handler for plus_rep
* Producer and Consumer
***********************************************/
void *plus_rep(void *args){
    while(terminate == false){

        //Lock buffer2_mutex before using buffer1
        pthread_mutex_lock(&buffer2_mutex);

        //Wait until buffer2 has something in it
        //Signal coming from *line_sep
        while(strlen(buffer2) == 0){
            pthread_cond_wait(&buf2_full, &buffer2_mutex);
        }

        //Acquire buffer2
        char* temp = get_buffer2();

        //SHOULDN'T NEED TO DO THIS (SIGNALS TO *INPUT THAT BUFFER 2 IS EMPTY)
        pthread_cond_signal(&buf2_empty);

        //Unlock buffer2_mutex so *input can do its thing
        pthread_mutex_unlock(&buffer2_mutex);

        printf("temp in plus_rep before replace: %s\n", temp);

        replace_plus_signs(temp);

        printf("temp in plus_rep after replace: %s\n", temp);

        fill_buffer3(temp);
        printf("Buffer3: %s\n", buffer3);
    }
}

/***********************************************
*               int main()
***********************************************/
int main(){
    pthread_t in, line, plus, out;
    buffer1 = (char *)malloc(SIZE * sizeof(char));
    buffer2 = (char *)malloc(SIZE * sizeof(char));
    buffer3 = (char *)malloc(SIZE * sizeof(char));
    
    //Create threads
    pthread_create(&in, NULL, input, NULL);
    pthread_create(&line, NULL, line_sep, NULL);
    pthread_create(&plus, NULL, plus_rep, NULL);
    //pthread_create(&out, NULL, output, NULL);


    pthread_join(in, NULL);
    pthread_join(line, NULL);
    pthread_join(plus, NULL);
    //pthread_join(out, NULL);

    free(buffer1);
    free(buffer2);
    free(buffer3);

    return 0;
}