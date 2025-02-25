#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <ncurses.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_DIM 1000
#define MAX_THREADS 3
typedef struct
{
    double vector1[MAX_DIM];
    double vector2[MAX_DIM];
    double result[MAX_DIM];
    int dim;
    int thread_id;
    int num_threads;
} VectorTask;

// Vector addition
void *Vector_add(void *arg)
{
    VectorTask *task = (VectorTask *)arg;
    int start = task->thread_id * (task->dim / task->num_threads);
    int end = (task->thread_id == task->num_threads - 1) ? task->dim : (start + (task->dim / task->num_threads));

    for (int i = start; i < end; i++) // Fixed start index
    {
        task->result[i] = task->vector1[i] + task->vector2[i];
    }

    return NULL;
}

// Vector subtraction
void *Vector_sub(void *arg)
{
    VectorTask *task = (VectorTask *)arg;
    int start = task->thread_id * (task->dim / task->num_threads);
    int end = (task->thread_id == task->num_threads - 1) ? task->dim : (start + (task->dim / task->num_threads));

    for (int i = start; i < end; i++) // Fixed start index
    {
        task->result[i] = task->vector1[i] - task->vector2[i];
    }

    return NULL;
}

// Vector dot product
void *Vector_prod(void *arg)
{
    VectorTask *task = (VectorTask *)arg;
    int start = task->thread_id * (task->dim / task->num_threads);
    int end = (task->thread_id == task->num_threads - 1) ? task->dim : (start + (task->dim / task->num_threads));

    for (int i = start; i < end; i++) // Fixed start index
    {
        task->result[i] = task->vector1[i] * task->vector2[i];
    }

    return NULL;
}

// detect specific command
int errno;
int command_list(char *c, char *a)
{
    char commands[10][50] = {"cd", "help", "help&", "exit", "vi", "addvec", "subvec", "dotprod"};
    for (int i = 0; i < 9; i++)
    {
        char substring[50];
        if (i >= 5 && strcmp(commands[i], c) == 0)
            return 8;
        if (i == 4 && strcmp(commands[i], c) == 0 && strcmp(a, "dummy") == 0)
            return 6;
        else if (i == 4 && strcmp(commands[i], c) == 0)
        {
            return 7;
        }
        if (i == 0 && strcmp(commands[i], c) == 0 && strcmp(a, "dummy") != 0)
            return 4;
        if (i == 1 && strcmp(commands[i], c) == 0 && strcmp(a, "&") == 0)
            return 2;
        if (strcmp(commands[i], c) == 0 && strcmp(a, "dummy") == 0)
        {
            return i;
        }
    }
    return -1;
}

// save things written in window to file
void save_file(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        mvprintw(0, 0, "Failed to open the file for saving.");
        return;
    }

    for (int i = 0; i < LINES; i++)
    {
        char line[COLS];
        mvinnstr(i, 0, line, COLS);
        fputs(line, file);
        fputc('\n', file);
    }

    fclose(file);
}

int main()
{

    int command_val, arg_count, pid, parallel, size, pipe_count, command_count, pipe_fd[2]; // File descriptors for the pipe
    int i;
    int child_pids[100]; // Array to store child process IDs
    char command[1000];
    char *args1[100][100];

    while (1)
    {
        char *cm;
        arg_count = 0;
        command_count = 0;
        int first_arg_count = 0;

        cm = readline("\nMyShell> ");
        if (cm == NULL)
            continue;
        add_history(cm); // Add input to command history
        int mul = 0;
        // Check for multi-line command (ending with '\')
        while (cm[strlen(cm) - 1] == '\\')
        {
            mul = 1;
            char *next_line = readline("    > ");
            cm = realloc(cm, strlen(cm) + strlen(next_line) + 1);
            cm[strlen(cm) - 1] = ' ';
            strcat(cm, next_line);
            free(next_line);
        }

        memcpy(command, cm, 100);

        // tokenise using white space
        char *saveptr1; // Initialize our save pointers to save the context of tokens
        char *saveptr2;
        char *tok = strtok_r(command, " ", &saveptr1);
        while (tok != NULL)
        {
            int last = 0;
            // tokenise with |
            char *position_ptr = strchr(tok, '|');
            if (position_ptr == NULL)
            {
                args1[command_count][arg_count] = tok;
                arg_count++;
                tok = strtok_r(NULL, " ", &saveptr1);
                continue;
            }
            if (strcmp(tok, "|") == 0)
            {
                args1[command_count][arg_count] = NULL;
                arg_count = 0;
                command_count++;
                tok = strtok_r(NULL, " ", &saveptr1);
                continue;
            }
            else if (tok[0] == '|')
            {
                args1[command_count][arg_count] = NULL;
                arg_count = 0;
                command_count++;
            }
            else if (tok[strlen(tok) - 1] == '|')
                last = 1;

            char *subtok = strtok_r(tok, "|", &saveptr2);
            while (subtok != NULL)
            {
                args1[command_count][arg_count] = subtok;
                arg_count++;
                args1[command_count][arg_count] = NULL;
                if (first_arg_count == 0)
                    first_arg_count = arg_count;
                subtok = strtok_r(NULL, "|", &saveptr2);
                if (subtok != NULL)
                {
                    arg_count = 0;
                    command_count++;
                }
            }
            if (last == 1)
            {
                arg_count = 0;
                command_count++;
            }
            tok = strtok_r(NULL, " ", &saveptr1);
        }
        if (first_arg_count == 0)
            first_arg_count = arg_count;

        args1[command_count][arg_count] = NULL;
        if (command_count)
            command_val = 5; // pipe

        else
        {
            if (first_arg_count <= 1)
            {
                command_val = command_list(args1[0][0], "dummy");
            } // single commands
            else
            {
                command_val = command_list(args1[0][0], args1[0][1]); // commands with arguments
            }
        }

        // cases
        switch (command_val)
        {
        case 0: // cd without argument
            printf("argument was expected after cd <directory_name>");
            break;
        case 2: // help
            parallel = 1;
        case 1: // help
            pid = fork();
            if (pid == 0)
            {
                // child process
                printf("1. pwd - shows the present working directory\n");
                printf("2. cd <directory_name> - changes the present working directory to the directory\n");
                printf("specified in <directory_name>. A full path to a directory may be given.\n");
                printf("3. mkdir <directory_name> - creates a new directory specified by <dir>.\n");
                printf("4. ls <flag> - shows the contents of a directory based on some optional flags, like\n");
                printf("ls -al, ls, etc. See man page of ls to know more about the flags.\n");
                printf("5. exit - exits the shell\n");
                printf("6. help - prints this list of commnds\n");
                exit(errno);
            }
            else
            {
                // parent process
                if (parallel == 0)
                {
                    int status;
                    wait(&status); /*you made a exit call in child you need to wait on exit status of child*/
                    if (WIFEXITED(status))
                        printf("child exited with = %d\n", WEXITSTATUS(status));
                }
            }
            break;
        case 3: // exit
            return 0;
        case 4: // cd with argument
            if (chdir(args1[0][1]) != 0)
            {
                perror("cd");
            }
            break;
        case 5: // pipeline
            if (pipe(pipe_fd) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            for (i = 0; i <= command_count; i++)
            {
                child_pids[i] = fork();

                if (child_pids[i] == -1)
                {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }

                if (child_pids[i] == 0)
                {
                    // Child process
                    if (i == 0)
                    {
                        // First command in the pipeline
                        close(pipe_fd[0]);               // Close read end
                        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
                        close(pipe_fd[1]);               // Close duplicated pipe end
                        execvp(args1[i][0], args1[i]);   // Execute the first command
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else if (i == command_count)
                    {
                        // Middle command in the pipeline
                        close(pipe_fd[1]);              // Close write end
                        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin to pipe
                        close(pipe_fd[0]);              // Close duplicated pipe end
                        execvp(args1[i][0], args1[i]);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        // close(pipe_fd[1]); // Close write end
                        close(pipe_fd[1]); // Close write end
                        dup2(pipe_fd[0], STDIN_FILENO);
                        close(pipe_fd[0]);               // Close read end
                        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
                        close(pipe_fd[1]);
                        execvp(args1[i][0], args1[i]);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            // Close both ends of the pipe in the parent
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            // Wait for all child processes to finish
            for (int i = 0; i <= command_count; i++)
            {
                waitpid(child_pids[i], NULL, 0);
            }
            break;

        case 6: // vi without argument
            printf("usage: vi <filename>");
            break;
        case 7: // vi with argument

            initscr();
            raw();
            noecho();
            curs_set(1);
            nodelay(stdscr, TRUE);
            keypad(stdscr, TRUE);

            // Open the file for editing
            int ch;
            int x = 0, y = 0;
            FILE *file = fopen(args1[0][1], "r+");
            if (file == NULL)
            {
                // If the file doesn't exist, create it
                file = fopen(args1[0][1], "w+");
            }
            // read file text.
            while ((ch = fgetc(file)) != EOF && x != 100)
            {
                if (ch == '\n')
                {
                    y++;
                    x = 0;
                }
                else
                {
                    mvwaddch(stdscr, y, x, ch);
                    x++;
                }
            }
            // Update the cursor position
            int num_lines = y + 1;
            move(0, 0);
            refresh();
            fclose(file);

            if (file == NULL)
            {
                endwin();
                printf("Failed to open or create the file.\n");
            }

            int num_words = 0;
            int num_chars = 0;

            while (1)
            {
                ch = getch();
                if (ch == KEY_UP && y > 0)
                {
                    y--;
                }
                else if (ch == KEY_DOWN)
                {
                    y++;
                }
                else if (ch == KEY_LEFT && x > 0)
                {
                    x--;
                }
                else if (ch == KEY_RIGHT)
                {
                    x++;
                }
                else if (ch == 27)
                { // Escape key
                    break;
                }
                else if (ch == 127)
                { // Delete key
                    if (isalnum(getyx(stdscr, y, x)))
                        num_chars--;
                    if (x > 0)
                    {
                        move(y, x - 1);
                        delch();
                        x--;
                    }
                    else if (y > 0)
                    {
                        move(y - 1, COLS - 1);
                        delch();
                        x = COLS - 1;
                        y--;
                    }
                }
                else if (isalnum(ch))
                {
                    num_chars++;
                    insch(ch);
                    x++;
                }
                else if (ch == CTRL('S'))
                {
                    // Save the file

                    save_file(args1[0][1]);
                }
                else if (ch == CTRL('X'))
                {
                    // Exit the editor
                    break;
                }
                else if (ch == '\n')
                { // Enter key
                    // Insert a new line
                    mvwaddch(stdscr, y, x, ch);

                    num_lines++;
                    num_words++;
                    y++;
                    x = 0;
                }
                else if (ch == ' ')
                { // count words
                    num_words++;
                    mvwaddch(stdscr, y, x, ch);
                    x++;
                }

                // Update the cursor position
                move(y, x);
                refresh();
            }
            endwin();
            printf("Number of lines: %d\n", num_lines);
            printf("Number of words: %d\n", num_words + 1);
            printf("Number of characters: %d\n", num_chars);

            break;
        case 8: // threads
            if (first_arg_count < 3)
            {
                printf("Usage: %s <operation> <file_1> <file_2> [-<no_thread>]\n", args1[0][0]);
                break;
            }

            char *operation = args1[0][0];
            char *file1 = args1[0][1];
            char *file2 = args1[0][2];
            int num_threads = MAX_THREADS; // Default number of threads

            if (args1[0][3] && strncmp(args1[0][3], "-", 1) == 0)
            {
                num_threads = atoi(args1[0][3] + 1);
                if (num_threads <= 0 || num_threads > MAX_THREADS)
                {
                    printf("Invalid number of threads specified. Using default (%d).\n", MAX_THREADS);
                    num_threads = MAX_THREADS;
                }
            }

            FILE *fp1 = fopen(file1, "r");
            FILE *fp2 = fopen(file2, "r");

            if (fp1 == NULL || fp2 == NULL)
            {
                printf("Failed to open input files.\n");
                break;
            }

            VectorTask task;
            task.dim = 0;
            task.num_threads = num_threads;

            // Read vector data from files
            while (fscanf(fp1, "%lf", &task.vector1[task.dim]) == 1 && fscanf(fp2, "%lf", &task.vector2[task.dim]) == 1)
            {
                task.dim++;
            }
            fclose(fp1);
            fclose(fp2);

            // Initialize result vector
            for (int i = 0; i < task.dim; i++)
            {
                task.result[i] = 0.0;
            }

            pthread_t threads[num_threads];
            VectorTask thread_tasks[num_threads];

            // Create a separate task structure for each thread
            for (int i = 0; i < num_threads; i++)
            {
                thread_tasks[i] = task;
                thread_tasks[i].thread_id = i;
            }

            if (strcmp(operation, "addvec") == 0)
            {
                for (int i = 0; i < num_threads; i++)
                {
                    pthread_create(&threads[i], NULL, Vector_add, &thread_tasks[i]);
                }
            }
            else if (strcmp(operation, "subvec") == 0)
            {
                for (int i = 0; i < num_threads; i++)
                {
                    pthread_create(&threads[i], NULL, Vector_sub, &thread_tasks[i]);
                }
            }
            else if (strcmp(operation, "dotprod") == 0)
            {
                for (int i = 0; i < num_threads; i++)
                {
                    pthread_create(&threads[i], NULL, Vector_prod, &thread_tasks[i]);
                }
            }
            else
            {
                printf("Invalid operation: %s\n", operation);
                break;
            }

            // Wait for threads to finish
            for (int i = 0; i < num_threads; i++)
            {
                pthread_join(threads[i], NULL);
            }

            if (strcmp(operation, "addvec") == 0 || strcmp(operation, "subvec") == 0)
            {
                printf("Vector %s Result: ", (strcmp(operation, "addvec") == 0) ? "Addition" : "Subtraction");
                for (int i = 0; i < task.dim; i++)
                {
                    printf("%.2lf ", thread_tasks[0].result[i]); // Result will be in any thread task as they share memory
                }
                printf("\n");
            }
            else if (strcmp(operation, "dotprod") == 0)
            {
                double final_sum = 0;
                for (int i = 0; i < task.dim; i++)
                {
                    final_sum += thread_tasks[0].result[i]; // Accumulate the result
                }
                printf("Vector Dot Product Result: %.2f\n", final_sum);
            }

            break;

        default: // any other command
            parallel = 0;
            pid = fork();
            size = strlen(args1[0][first_arg_count - 1]);
            if (strcmp(args1[0][first_arg_count - 1], "&") == 0 || args1[0][first_arg_count - 1][size - 1] == '&')
            {
                parallel = 1;
            }
            if (pid == 0)
            {
                // child proces
                int status = execvp(args1[0][0], args1[0]);
                perror(args1[0][0]);
                exit(errno);
            }
            else
            {
                // parent process
                if (parallel == 0)
                {
                    int status;
                    wait(&status);
                    if (WIFEXITED(status))
                        printf("child exited with = %d\n", WEXITSTATUS(status));
                }
            }
            break;
        }
    }

    return 0;
}
