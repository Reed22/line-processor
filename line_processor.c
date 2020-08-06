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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Initialize the mutex

// Initialize the condition variables
pthread_cond_t buf1_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf1_empty = PTHREAD_COND_INITIALIZER;

pthread_cond_t buf2_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf2_empty = PTHREAD_COND_INITIALIZER;

pthread_cond_t buf3_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf3_empty = PTHREAD_COND_INITIALIZER;

/***********************************************
*               int input_p()
*
* Reads user input into temporary buffer and 
* adds it to global variable buffer1
***********************************************/
int input_p(){
    //printf("    in input_p\n");
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
*          void line_sep_pc()
*
* Takes buffer1 and removes line separators 
* And adds to buffer2 
***********************************************/
void line_sep_pc(){
    //printf("    in line_sep_pc\n");
    //static int buf2_ind = 0; //Holds next available index of buffer2
    int buf2_ind = strlen(buffer2);
    //If buffer2 has been cleared, reset buf2_ind back to 0
    if(buffer2[0] == '\0')
        buf2_ind = 0;
    
    //Go through each character of buffer1
    for(int i = 0; i < strlen(buffer1); i++){
        //If char is line separator, replace with space
        if(buffer1[i] == '\n' || buffer1[i] == '\r')
            buffer2[buf2_ind] = ' ';

        //Otherwise, copy character to buffer2
        else
            buffer2[buf2_ind] = buffer1[i];

        buf2_ind++;
    }
    //Reset buffer1 after buffer2 has acquired all chars
    memset(buffer1, '\0', strlen(buffer1));
}

/***********************************************
*               void plus_replace()
*
* Takes buffer2 and replaces any instance of '++'
* with '^' and inserts into buffer3
***********************************************/
void plus_replace_pc(){
    //printf("    in plus_replace_pc\n");
    int buf3_ind = strlen(buffer3); //Holds next available index for buffer3
    //If buffer3 has been cleared, reset buf3_ind back to 0
    if(buffer3[0] == '\0')
        buf3_ind = 0;

    int i = 0;

    //Go through each character of buffer2
    while(i < strlen(buffer2)){
        //If there are a pair of consecutive plus signs,
        //insert ^ into buffer3 and increment i by 2
        //so it skips past both plus signs.
        if(buffer2[i] == '+' && buffer2[i+1] == '+'){
            buffer3[buf3_ind] = '^';
            i += 2;
        }
        //Otherwise, copy character from buffer 2 to buffer 1
        //and only increment i once
        else{
            buffer3[buf3_ind] = buffer2[i];
            i++;
        }
        buf3_ind++;
    }

    //Reset buffer2 after buffer3 has acquired all chars
    memset(buffer2, '\0', strlen(buffer2));

}

/***********************************************
*               void output()
* Will ouput 80 character lines from buffer3
* and clear the characters from buffer3
***********************************************/
void output_c(){
    //printf("    in output_c\n");
    int buffer3_len = strlen(buffer3);
    int offset = 0;  //Used for copying characters from 80th+ index to offset index

    //If buffer has at least 80 characters, print buffer
    if(buffer3_len >= CHAR_PER_LINE){
        printf("((%.80s))\n", buffer3);

        //Clear out first 80 character of buffer3
        memset(buffer3, '\0', CHAR_PER_LINE);

        //Copy 80th char on to beginning of buffer3
        //offset = 0,1,2,3...
        //i = 80, 81, 82...
        for(int i = CHAR_PER_LINE; i < buffer3_len; i++){
            buffer3[offset] = buffer3[i];
            buffer3[i] = '\0';
            offset++;
        }
    }
}
/***********************************************
*               void input()
*
* Thread handling function for input
* Producer only
***********************************************/
void *input(void *args){
    while(terminate == false){
        //printf("in *input\n");
        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);
        //printf("*input has lock\n");
        //Wait until buffer1 is empty? <--buffer will always have space. Might not need this(gets stuck on input If I remove)
        while(strlen(buffer1) > 0)
            pthread_cond_wait(&buf1_empty, &mutex);

        //Perform input
        terminate = input_p();
        //printf("Buffer1: %s\n", buffer1);

        //Signal to line consumer that buffer1 is no longer empty
        pthread_cond_signal(&buf1_full);
        
        //Unlock mutex
        pthread_mutex_unlock(&mutex);
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

        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);

        //printf("*line_Sep has lock\n");

        //Wait until buffer1 has something in it
        while(strlen(buffer1) == 0)
            pthread_cond_wait(&buf1_full, &mutex);

        //Perform line separator function
        line_sep_pc();

        //printf("Buffer2: %s\n", buffer2);

        //Signal to line consumer that buffer1 is empty 
        pthread_cond_signal(&buf1_empty);

        //Singal to plus_rep that buffer2 has something in it
        pthread_cond_signal(&buf2_full);
        
        //Unlock mutex
        pthread_mutex_unlock(&mutex);
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
        //printf("in *plus_rep\n");

        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);

        //printf("*plus_rep has lock\n");

        //Wait until buffer2 has something in it
        //Signal coming from *line_sep
        while(strlen(buffer2) == 0)
            pthread_cond_wait(&buf2_full, &mutex);

        //Perform line separator function
        plus_replace_pc();

        //printf("Buffer3: %s\n", buffer3);

        //Signal to line consumer that buffer2 is empty 
        pthread_cond_signal(&buf2_empty);

        //Singal to output that buffer3 has something in it
        pthread_cond_signal(&buf3_full);

        
        //Unlock mutex
        pthread_mutex_unlock(&mutex);
    }
}


/***********************************************
*          void *plus_rep(void *args)
*
* Thread handler for plus_rep
* Producer and Consumer
***********************************************/
void *output(void *args){
    while(terminate == false){
        //printf("in *output\n");

        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);
        
        //printf("*output has lock\n");

        //Wait until buffer3 has something in it
        while(strlen(buffer3) == 0)
            pthread_cond_wait(&buf3_full, &mutex);

        //Perform line separator function
        output_c();
        //printf("Buffer3: %s\n", buffer3);

        //Signal to line consumer that buffer1 is empty 
        pthread_cond_signal(&buf3_empty);
        
        //Unlock mutex
        pthread_mutex_unlock(&mutex);
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
    pthread_create(&out, NULL, output, NULL);


    pthread_join(in, NULL);
    pthread_join(line, NULL);
    pthread_join(plus, NULL);
    pthread_join(out, NULL);


    //while(true){
        //terminate = input();
        //if(terminate == true)
        //    break;
        //printf("Buffer1: %s\n", buffer1);
        //line_sep();
        //printf("Buffer2: %s\n", buffer2);
        //plus_replace();
        //printf("Buffer3(before output): %s\n", buffer3);
        //output();
        //printf("Buffer3(after output): %s\n", buffer3);
    //}
    free(buffer1);
    free(buffer2);
    free(buffer3);

    return 0;
}