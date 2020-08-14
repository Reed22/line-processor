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

//Initialize mutexes 
pthread_mutex_t buffer1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer3_mutex = PTHREAD_MUTEX_INITIALIZER;


// Initialize the condition variables
pthread_cond_t buf1_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf2_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf3_full = PTHREAD_COND_INITIALIZER;

/***************************************
* Consumer functions for all threads
*
* Each function performs the same, but 
* on a different buffer. Copies the buffer 
* into a temporary variable and clears
* out the buffer. Returns the contents
* of the buffer
***************************************/

void get_buffer1(char* temp){
    strcpy(temp, buffer1);
    memset(buffer1, '\0', strlen(buffer1));
}

void get_buffer2(char* temp){
    strcpy(temp, buffer2);
    memset(buffer2, '\0', strlen(buffer2));
}

void get_buffer3(char* temp){
    strcpy(temp, buffer3);
    memset(buffer3, '\0', strlen(buffer3));
}

/***************************************
* Processor functions for all threads
*
* Each function is different and performs
* certain processes on the char* variable
* that is passed to it. Returns the char*
* after it is finished altering it based
* on the function's responsibilities. 
*
* None of these functions are in critical
* sections of any thread
***************************************/

//Reads user input, checks for null terminating character,
void get_user_input(char* tempBuf){
    size_t len = SIZE;  
    getline(&tempBuf, &len, stdin);
}

//Goes through temp and changes any line 
//separators to space characters
void replace_line_separators(char* temp){
    for(int i = 0; i < strlen(temp); i++){
        //Checks for newline and carriage return
        if(temp[i] == '\r' || temp[i] == '\n')
            temp[i] = ' ';
    }
}

//Goes through temp and changes pairs of 
//plus signs to ^ character
void replace_plus_signs(char* temp){ 
    int temp_len = strlen(temp);
    int index = 0;
    
    //Go through each character of temp
    while(index < temp_len){
        //Found pair
        if(temp[index] == '+' && temp[index + 1] == '+'){
            //Replace first '+' with '^'
            temp[index] = '^';

            //Shift all other characters down
            for(int j = index + 1; j < temp_len; j++){
                temp[j] = temp[j + 1];
            }

            //Put null terminating charcter where last char was
            temp[temp_len - 1] = '\0';

            //Decreminate temp_len since we removed a char
            temp_len --;
        }
        index++;
    }
}

//Outputs if 80 characters are in buffer
void output_c(char* buffer4){
    int buffer4_len = strlen(buffer4);  //Number of characters in buffer4
    int offset;                         //Used for copying characters from 80th+ index to offset index

    //If buffer has at least 80 characters, print buffer
    while(buffer4_len >= CHAR_PER_LINE){
        printf("%.80s\n", buffer4);

        //Clear out first 80 character of buffer3
        memset(buffer4, '\0', CHAR_PER_LINE);

        //Copy 80th char on to beginning of buffer4
        //offset = 0,1,2,3...
        //i = 80, 81, 82...
        offset = 0;
        for(int i = CHAR_PER_LINE; i < buffer4_len; i++){
            buffer4[offset] = buffer4[i];
            buffer4[i] = '\0';
            offset++;
        }

        buffer4_len = buffer4_len - CHAR_PER_LINE;

    }
}

/***************************************
* Producer functions for all threads
* Each function performs the same, but 
* on a different buffer. If the buffer 
* is empty, it performs a strcpy, if 
* it is not empty then it performs a 
* strcat.
*
* Parameters: char* that will be
* inserted into buffer
*
***************************************/

void fill_buffer1(char* temp){
    if(strlen(buffer1) == 0)
        strcpy(buffer1, temp);
    else
        strcat(buffer1, temp);
}

void fill_buffer2(char* temp){
    if(strlen(buffer2) == 0){
        strcpy(buffer2, temp);
    }
    else{
        strcat(buffer2, temp);
    }
}

void fill_buffer3(char* temp){
    if(strlen(buffer3) == 0){
        strcpy(buffer3, temp);
    }
    else{
        strcat(buffer3, temp);
    }
}

//This buffer is not global, so buffer4 needs to be passed in
void fill_buffer4(char* temp, char* buffer4){
    if(strlen(buffer4) == 0){
        strcpy(buffer4, temp);
    }
    else{
        strcat(buffer4, temp);
    }
}

/***********************************************
*               void *input()
*
* Thread handling function for input
* Producer only
***********************************************/
void *input(void *args){
    char* tempBuf = (char *)malloc(MAX_LINE * sizeof(char)); //Temporary buffer that will add to buffer1
    bool input_terminate = false;                           //Flag used to exit while loop if terminating character found

    while(input_terminate == false){
        get_user_input(tempBuf);

        //Lock buffer1_mutex before changing buffer1
        pthread_mutex_lock(&buffer1_mutex);

        //Perform input
        fill_buffer1(tempBuf);

        //Signal to line consumer that buffer1 is no longer empty
        pthread_cond_signal(&buf1_full);

        //Unlock mutex
        pthread_mutex_unlock(&buffer1_mutex);

        //Check for terminating line
        if(strcmp(tempBuf, "DONE\n") == 0){
            input_terminate = true;
        }
        memset(tempBuf, '\0', strlen(tempBuf));

    }
    free(tempBuf);
}

/***********************************************
*          void *line_sep(void *args)
*
* Thread handler for line_sep
* Producer and Consumer
***********************************************/
void *line_sep(void *args){
    bool line_sep_terminate = false;                    //Flag used to exit while loop if terminating character found
    char* temp = (char *)malloc(SIZE * sizeof(char));   //Temporary buffer used to hold buffer1
    while(line_sep_terminate == false){

        //Lock buffer1_mutex before using buffer1
        pthread_mutex_lock(&buffer1_mutex);

        //Wait until buffer1 has something in it
        //Signal coming from *input
        while(strlen(buffer1) == 0){
            pthread_cond_wait(&buf1_full, &buffer1_mutex);
        }

        //Acquire buffer1
        get_buffer1(temp);


        //Unlock buffer1_mutex so *input can do its thing
        pthread_mutex_unlock(&buffer1_mutex);
        
        //Check for terminating line
        //If buffer was not flushed prior to getting buffer, then terminating line will be last 5 char of temp
        if(strlen(temp) > strlen("DONE\n")){
            if (strcmp(temp + strlen(temp)-5, "DONE\n") == 0 ){
                line_sep_terminate = true;
            }
        }
        //If buffer was flushed, temp will just hold terminating line if terminating line was inputted
        else if(strcmp(temp, "DONE\n") == 0){
            line_sep_terminate = true;
        }

        //Process temp
        replace_line_separators(temp);

        //Producer Buffer2
        //Try to take buffer2_mutex if plus_rep doesn't have it
        pthread_mutex_lock(&buffer2_mutex);

        //Put temp variable in buffer2
        fill_buffer2(temp);

        //Singal to plus_rep that buffer2 is no longer empty
        pthread_cond_signal(&buf2_full);

        //Unlock buffer2_mutex so plus_rep can do its thing
        pthread_mutex_unlock(&buffer2_mutex);
        memset(temp, '\0', strlen(temp));

    }
    free(temp);
}
/***********************************************
*          void *plus_rep(void *args)
*
* Thread handler for plus_rep
* Producer and Consumer
***********************************************/
void *plus_rep(void *args){
    bool plus_rep_terminate = false;                    //Flag used to exit while loop if terminating character found
    char* temp = (char *)malloc(SIZE * sizeof(char));   //Temporary buffer used to hold buffer2
    while(plus_rep_terminate == false){

        //Lock buffer2_mutex before using buffer1
        pthread_mutex_lock(&buffer2_mutex);

        //Wait until buffer2 has something in it
        //Signal coming from *line_sep
        while(strlen(buffer2) == 0){
            pthread_cond_wait(&buf2_full, &buffer2_mutex);
        }

        //Acquire buffer2
        get_buffer2(temp);

        //Unlock buffer2_mutex so *input can do its thing
        pthread_mutex_unlock(&buffer2_mutex);
       
        //Check for terminating line
        //Have to check for DONE with a space character since line_sep changes newline to space
        //If buffer was not flushed prior to getting buffer, then terminating line will be last 5 char of temp
        if(strlen(temp) > strlen("DONE ")){
            if (strcmp(temp + strlen(temp)-5, "DONE ") == 0 ){
                plus_rep_terminate = true;
            }
        }
        //If buffer was flushed, temp will just hold terminating line if terminating line was inputted
        else if(strcmp(temp, "DONE ") == 0){
            plus_rep_terminate = true;
        }
        //Process temp
        replace_plus_signs(temp);

        //Producer Buffer3
        //Try to take buffer3_mutex if output doesn't have it
        pthread_mutex_lock(&buffer3_mutex);

        //Put temp variable in buffer3
        fill_buffer3(temp);

        //Singal to output that buffer3 is no longer empty
        pthread_cond_signal(&buf3_full);

        //Unlock buffer3_mutex so output can do its thing
        pthread_mutex_unlock(&buffer3_mutex);
        
        memset(temp, '\0', strlen(temp));

    }
    free(temp);
}
/***********************************************
*          void *output(void *args)
*
* Thread handler for output
* Consumer only
***********************************************/
void *output(void *args){
    char* buffer4 = (char *)malloc(SIZE * sizeof(char)); //Holds buffer3 until ready for output
    bool output_terminate = false;                       //Flag used to exit while loop if terminating character found
    char* temp = (char *)malloc(SIZE * sizeof(char));   //Temporary buffer that will hold buffer3
    while(output_terminate == false){

        //Lock buffer3_mutex before using buffer1
        pthread_mutex_lock(&buffer3_mutex);

        //Wait until buffer3 has something in it
        //Signal coming from *line_sep
        while(strlen(buffer3) == 0){
            pthread_cond_wait(&buf3_full, &buffer3_mutex);
        }

        //Acquire buffer3
        get_buffer3(temp);

        //Unlock buffer2_mutex so *input can do its thing
        pthread_mutex_unlock(&buffer3_mutex);
       
        //Check for terminating line
        //Have to check for DONE with a space character since line_sep changes newline to space
        //If buffer was not flushed prior to getting buffer, then terminating line will be last 5 char of temp
        if(strlen(temp) > strlen("DONE ")){
            if (strcmp(temp + strlen(temp)-5, "DONE ") == 0 ){
                output_terminate = true;
            }
        }
        //If buffer was flushed, temp will just hold terminating line if terminating line was inputted
        else if(strcmp(temp, "DONE ") == 0){
            output_terminate = true;
        }

        //Place temp in buffer4
        fill_buffer4(temp, buffer4);

        //Print buffer4 and modify based on characters printed
        output_c(buffer4);

        memset(temp, '\0', strlen(temp));
    }
    free(buffer4);
    free(temp);
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
    pthread_create(&out, NULL, output, NULL);


    pthread_join(in, NULL);
    pthread_join(line, NULL);
    pthread_join(plus, NULL);
    pthread_join(out, NULL);

    free(buffer1);
    free(buffer2);
    free(buffer3);

    return 0;
}