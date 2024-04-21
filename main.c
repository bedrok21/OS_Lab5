#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>


void func_f() {
    int x;
    for (;;){
        read(STDIN_FILENO, &x, sizeof(int)); 
        int result = 1;
        sleep(3);
        write(STDOUT_FILENO, &result, sizeof(int)); 
    }
}

void func_g() {
    int x;
    for (;;){
        read(STDIN_FILENO, &x, sizeof(int)); 
        int result = 1;
        sleep(10);
        write(STDOUT_FILENO, &result, sizeof(int)); 
    }
}

int main() {
    int f_pipe[2], g_pipe[2], f_pipe_out[2], g_pipe_out[2];

    pipe(f_pipe); pipe(g_pipe); pipe(f_pipe_out); pipe(g_pipe_out);

    pid_t f_pid, g_pid;
    
    f_pid = fork();
    if (f_pid == 0) {
        close(f_pipe[1]); 
        close(f_pipe_out[0]);
        dup2(f_pipe[0], STDIN_FILENO);
        dup2(f_pipe_out[1], STDOUT_FILENO);
        func_f();
    } 
    else {
        close(f_pipe[0]);  
        close(f_pipe_out[1]);

        g_pid = fork();
        if (g_pid == 0) {
            close(g_pipe[1]);
            close(g_pipe_out[0]);
            dup2(g_pipe[0], STDIN_FILENO);
            dup2(g_pipe_out[1], STDOUT_FILENO);
            func_g();
        } 
        else {
            close(g_pipe[0]);
            close(g_pipe_out[1]);
        } 
    } 

    int arg1, arg2;
    printf("Enter two arguments for f(x) and g(x): ");
    scanf("%d %d", &arg1, &arg2);

    write(f_pipe[1], &arg1, sizeof(int)); 
    write(g_pipe[1], &arg2, sizeof(int));  

    int f_result, g_result, result;

    struct pollfd fds[2];
    fds[0].fd = f_pipe_out[0];
    fds[0].events = POLLIN;  
    fds[1].fd = g_pipe_out[0];
    fds[1].events = POLLIN; 

    int timeout_ms = 5000;  int process_num = 2;
    int ask = 1; int broke = 0;

    while(1){
        int ready = poll(fds, process_num, timeout_ms);

        if (ready == -1) {
            perror("poll");
            return 1;
        } else if (ready == 0) {
            if (ask){
                int ask_res;
                printf("Timeout occurred.\n 1 - continue for 5 sec | 2 - continue forever | 3 - break\n");
                scanf("%d", &ask_res);
                switch (ask_res){
                    case 1: continue;
                    case 2: {
                        ask = 0;
                        timeout_ms = INT_MAX;
                        continue;
                    } case 3: {
                        broke = 1;
                        break;
                    } default: continue;
                } break;
            }
        } else {
            if (fds[0].revents & POLLIN) { 
                read(fds[0].fd, &f_result, sizeof(int)); 
                //printf("%d\n", f_result);
                if (f_result){
                    if (process_num == 2){
                        fds[0].fd = g_pipe_out[0];
                        process_num = 1;
                    }else{
                        result = 1;
                        break;
                    }
                }else{
                    result = 0;
                    break;
                }
            } else if (fds[1].revents & POLLIN) {  
                read(fds[1].fd, &g_result, sizeof(int));  
                //printf("%d\n", g_result);
                if (g_result){
                    process_num = 1;
                }else{
                    result = 0;
                    break;
                }
            }
        }
    }
    if (broke) printf("Process finished by user");
    else printf("RESULT is %d\n", result);

    kill(f_pid, SIGKILL);
    kill(g_pid, SIGKILL);

    return 0;
}
