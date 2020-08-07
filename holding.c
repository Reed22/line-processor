/***********************************************
*               void plus_replace()
*
* Takes buffer2 and replaces any instance of '++'
* with '^' and inserts into buffer3
***********************************************/
void plus_replace_pc(){
    printf("        in plus_replace_pc\n");
    int buf3_ind = strlen(buffer3); //Holds next available index for buffer3
    
    //If buffer3 has been cleared, reset buf3_ind back to 0  <-WITH STRLEN PROB DONT NEED THIS
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
    printf("        in output_c\n");
    int buffer3_len = strlen(buffer3);
    int offset = 0;  //Used for copying characters from 80th+ index to offset index

    //If buffer has at least 80 characters, print buffer
    //if(buffer3_len >= CHAR_PER_LINE){
    while(buffer3_len >= CHAR_PER_LINE){
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
        buffer3_len = strlen(buffer3);
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
        printf("in *plus_rep\n");

        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);

        printf("    *plus_rep has lock\n");

        //Wait until buffer2 has something in it
        //Signal coming from *line_sep
        while(strlen(buffer2) == 0)
            pthread_cond_wait(&buf2_full, &mutex);

        //Perform line separator function
        plus_replace_pc();

        printf("Buffer3: %s\n", buffer3);

        //Signal to line consumer that buffer2 is empty 
        //pthread_cond_signal(&buf2_empty);

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
        printf("in *output\n");

        //Lock mutex before changing buffer1
        pthread_mutex_lock(&mutex);
        
        printf("    *output has lock\n");

        //Wait until buffer3 has something in it
        //Signal coming from *plus_replace
        while(strlen(buffer3) == 0)
            pthread_cond_wait(&buf3_full, &mutex);

        //Perform line separator function
        output_c();
        printf("Buffer3: %s\n", buffer3);

        //Signal to line consumer that buffer1 is empty 
        //pthread_cond_signal(&buf3_empty);
        
        //Unlock mutex
        pthread_mutex_unlock(&mutex);
    }
}


/***********************************************
*          void line_sep_pc()
*
* Takes buffer1 and removes line separators 
* And adds to buffer2 
***********************************************/
void line_sep_pc(){
    printf("      in line_sep_pc\n");
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
    //memset(buffer1, '\0', strlen(buffer1));
}