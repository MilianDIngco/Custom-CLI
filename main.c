#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>




#define MAX_LINE_LENGTH 100 //total length of line
#define MAX_WORD_LENGTH 20
#define MAX_NUM_WORDS 10
#define HISTORY_BUFFER 5
#define BUFFER_SIZE 1000




void printHistory(char* history[HISTORY_BUFFER][11]) {
    printf("\n---------------\n");
   
    for(int i = 0; i < HISTORY_BUFFER; i++) {
        printf("%d: ", i);
        for(int n = 0; n < MAX_NUM_WORDS; n++) {
            if(history[i][n] == NULL) {
                break;
            }
            printf("%s|", history[i][n]);
        }
        if(history[i][MAX_NUM_WORDS] != NULL)
            printf("%s", history[i][MAX_NUM_WORDS]);
        printf("\n");
    }
   
    printf("\n---------------\n");
}




void concat_array(char* arr[], int length, char* str) {
    char* space = " ";
    for(int i = 0; i < length - 1; i++) {
        strcat(str, arr[i]);
        strcat(str, " ");
    }
    strcat(str, arr[length - 1]);
}




void clear(int amount) {
    for(int i = 0; i < amount; i++) {
        printf("\b \b");
    }
}




off_t get_file_size(char* filename) {
    struct stat st;
    if(stat(filename, &st) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    return st.st_size;
}




int main(int argc, char** argv) {
   
    int run = 1;




    //SAVE SETTINGS OF OLD STDIN_FILE STUFF
    int stdin_fd = dup(STDIN_FILENO);
    int stdout_fd = dup(STDOUT_FILENO);
    int fd = -1;




    struct termios old_termios, new_termios;




    // Get current terminal settings
    if (tcgetattr(STDIN_FILENO, &old_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }




    new_termios = old_termios;




    new_termios.c_lflag &= ~(ICANON | ECHO);




    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        perror("tcsetattr");
        return 1;
    }




    pid_t pid;




    //Circular array stores last 5 commands, then overwrites (+1 bc last element stores amt)    
    char* history[HISTORY_BUFFER][MAX_NUM_WORDS + 1] = { NULL };
    int last = 0;      // points to last command
   
    //Run flag
    int should_run = 1;




    //Current working directory
    char cwd[1024];
    char* lastdir = malloc(MAX_WORD_LENGTH);




    while(should_run) {
       //prompt with current folder
        if(getcwd(cwd, sizeof(cwd)) != NULL) {
            //get last folder
            lastdir = strrchr(cwd, '/');
            if(lastdir != NULL)
                lastdir++;
        } else {
            perror("getcwd() error");
        }




        printf("osc:%s>", lastdir);
        fflush(stdout);




        //get user input
        char temp_input[MAX_LINE_LENGTH] = { '\0' };
        char copy_input[MAX_LINE_LENGTH] = { '\0' };
        int input_length = 0;
        char c;




        //do nothing flag
        int do_nothing = 0;
        int indexHistory = last;
        int up = 0;
        int down = 0;
        int right = 0;




        while(1) {
            if(read(STDIN_FILENO, &c, 1) == -1) {
                perror("read");
                break;
            }




            if(c == '\n') { // ENTER
                if(temp_input[0] == '\0')
                    do_nothing = 1;
                printf("\n");
                break;
            } else if(c == 127) { // DELETE
                if(input_length > 0) {  
                    temp_input[--input_length] = '\0';
                    printf("\b \b");
                    right--;
                }
            } else if(c == '\033') { // ARROW KEYS
                char seq[3];
                if(read(STDIN_FILENO, &seq, 2) != 2)
                    break;
                if(seq[0] == '[') {
                   
                if(seq[1] == 'A') { //UP ARROW
                 //check if prior commands exist
                 
                 if(up < HISTORY_BUFFER &&
                   history[((indexHistory == 0) ? 4 : indexHistory - 1)][0]
                   != NULL) {
                    up++;
                    indexHistory = (indexHistory == 0) ? 4 : indexHistory - 1;
                    //clear current
                    printf("\033[K");
                    clear(input_length);
                    fflush(stdout);
                    //set input length
                    temp_input[0] = '\0';
                    concat_array(history[indexHistory], atoi(history[indexHistory][MAX_NUM_WORDS]), temp_input);
                    input_length = strlen(temp_input);
                   
                    //print previous instruction
                    printf("%s", temp_input);
                    //move cursor to end of line
                    right = input_length;
                    printf("\033[K");
                 }
                } else if(seq[1] == 'B') { //DOWN ARROW
                 if(up > 1 &&
                    history[((indexHistory == 4) ? 0 : indexHistory + 1)]
                    != NULL) {
                    up--;
                    indexHistory = (indexHistory == 4) ? 0 : indexHistory + 1;
                    //clear previous input
                    printf("\033[K");
                    clear(input_length);
                    input_length = 0;
                    fflush(stdout);
                    //set input length
                    temp_input[0] = '\0';
                    concat_array(history[indexHistory], atoi(history[indexHistory][MAX_NUM_WORDS]), temp_input);
                    input_length = strlen(temp_input);
                    //print next instruction
                    printf("%s", temp_input);
                    //move cursor to end of line
                    right = input_length;
                    printf("\033[K");
                 } else if(up == 1 && history[((indexHistory == 4) ? 0 : indexHistory + 1)] != NULL) {
                    up--;
                    indexHistory = last;
                    //clear previous input
                    printf("\033[K");
                    clear(input_length);
                    input_length = 0;
                    fflush(stdout);
                    temp_input[0] = '\0';
                    printf("\033[K");
                    right = 0;
                 }
                } else if(seq[1] == 'C') { //RIGHT ARROW
                    if(right < input_length) {
                        printf("\033[1C");
                        right++;
                    }
                } else if(seq[1] == 'D') { //LEFT ARROW
                    if(right > 0) {
                        printf("\033[1D");
                        right--;
                    }
                }
                }
            } else {
                if(right == input_length) {
                    temp_input[input_length++] = c;
                    right++;
                    printf("%c", c);
                }
            }
            fflush(stdout);
        }
       
        //copy temp input to copy input
        for(int i = 0; i < input_length; i++) {
            copy_input[i] = temp_input[i];
        }




        //if nothing is entered
        if(do_nothing)
            continue;
           
        //parse user input by spaces
        int num_commands = 0;
        //split input by spaces and redirection and pipes
        char* command = strtok(temp_input, " ");
        while(command != NULL && num_commands < MAX_NUM_WORDS) {
            //store current command into history at 'last'
            history[last][num_commands] = strdup(command);
            num_commands++;
           
            command = strtok(NULL, " ");
        }
        char str_num_commands[3];
        sprintf(str_num_commands, "%d", num_commands);
        history[last][MAX_NUM_WORDS] = strdup(str_num_commands);




        //set should_run to 0 and exits application
        if(strcmp(history[last][0], "exit") == 0) {
            should_run = 0;
            break;
        }




        //clear rest of history
        for(int i = num_commands; i < MAX_NUM_WORDS; i++) {
            history[last][i] = NULL;
        }




        //check if repeating instruction !!
        if(strcmp(history[last][0], "!!") == 0) {
            int prev = (last == 0) ? 4 : (last - 1);
            if(history[prev][0] == NULL) {
                perror("No commands in history");  
                continue;
            } else {
                //copy last instructions to current
                int n = atoi(history[prev][MAX_NUM_WORDS]);
                for(int i = 0; i < n; i++) {
                    history[last][i] = history[prev][i];
                }
                history[last][MAX_NUM_WORDS] = history[prev][MAX_NUM_WORDS];
            }
        }


//check for < > redirect input / output
        int input_redir = 0;
        int output_redir = 0;
        int pipe_flag = 0;
        int fileIndex = 0;
        int concurrent = 0;
        for(int i = 0; i < input_length; i++) {
            if(copy_input[i] == '<') {
                input_redir = 1;
            }
            if(copy_input[i] == '>') {
                output_redir = 1;
            }
            if(copy_input[i] == '|') {
                pipe_flag = 1;
            }
            if(copy_input[i] == '&') {
                concurrent = 1;
            }
        }
        //input redirection
        if(input_redir) {
            //get file
            if(history[last][1] != NULL) {
                fd = open(history[last][2], O_RDONLY);
                if(fd == -1){
                    perror("open, could not find file");
                    continue;
                }
            }


            // put file contents to duplicate it to the input
            if(dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }


            if(close(fd) == -1) {
                perror("close input");
                exit(EXIT_FAILURE);
            }


        }




        //output redirection
        int out_index = 2;
        if(output_redir) {
            for(int i = 0; i < MAX_NUM_WORDS; i++) {
                if(history[last][i] == NULL) break;
                if(strcmp(history[last][i], ">") == 0) {
                    out_index = i + 1;
                    break;
                }
            }
            int output_fd;
            //get file
            if(history[last][out_index] != NULL) {
                output_fd = open(history[last][out_index], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(output_fd == -1){
                    perror("open, could not find file");
                    continue;
                }
            }
            // redirect output to file
            if(dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if(close(output_fd) == -1) {
                perror("close input");
                exit(EXIT_FAILURE);
            }
        }


        //execute command via child proc
        //FOR TESTING ONLY!!!
        if(run) {
       
        //if cd, does not require child process
        if(strcmp(history[last][0], "cd") == 0) {
            if(chdir(history[last][1]) != 0) {
                perror("chdir");
            }
           
        } else {
            pid = fork();      
           
             if(pid < 0) {
                fprintf(stderr, "Fork Failed");
                return 1;
            } else if(pid == 0) { //child process
                if(input_redir) {
                    //create separate array for arguments
                    char* args[2] = { history[last][0], NULL };
                    if(execvp(history[last][0], args) == -1) {
                        perror("execvp error - input redir");
                    }
                } else if(output_redir) {
                    char* args[MAX_NUM_WORDS] = { NULL };
					for(int i = 0; i < out_index - 1; i++) {
                        args[i] = (char*)malloc(strlen(history[last][i]) + 1);
						strcpy(args[i], history[last][i]);
					}
                    if(execvp(args[0], args) == -1) {
                        perror("execvp error - output redir");
                    }
                } else if(execvp(history[last][0], history[last]) == -1) {
                        perror("execvp error - normal");
                }


            } else {          //parent process
                if(!concurrent) {
                    wait(NULL);
                }
            }
        }

        }

        if(dup2(stdin_fd, STDIN_FILENO) == -1) {
            perror("dup2 failed to restore settings");
            exit(EXIT_FAILURE);
        }
        if(dup2(stdout_fd, STDOUT_FILENO) == -1) {
            perror("dup2 failed to restore settings");
            exit(EXIT_FAILURE);
        }




        //increment history buffer  
        last = (last + 1) % HISTORY_BUFFER;
    }

    if(close(stdin_fd) == -1) {
        perror("close in");
        exit(EXIT_FAILURE);
    }
    if(close(stdout_fd) == -1) {
        perror("close out");
        exit(EXIT_FAILURE);
    }
   
    if(tcsetattr(STDIN_FILENO, TCSANOW, &old_termios) == -1) {
        perror("tcsetattr");
        return 1;
    }




    return 0;
}









