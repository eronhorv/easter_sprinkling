#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_INPUT_LENGTH 100
#define FILENAME "poems.txt"

#define MAX_MS_LENGTH 1024

struct msgbuf
{
	long mtype;
	char mtext[MAX_MS_LENGTH];
};

char *prefill_text = NULL;

void handler(int signumber)
{
	sleep(2);
	printf("\x1B[33mParent\x1B[37m - received signal %i!\n", signumber);
}

void read_str_from_pipe(int pipend, char *s)
{
	char c;
	int i = 0;
	while (read(pipend, &c, 1) > 0 && c != '\0')
	{
		s[i] = c;
		++i;
	}
	s[i] = '\0';
}

char *read_console()
{
	char *inp = NULL;
	size_t size = 0;
	ssize_t chars_read;
	chars_read = getline(&inp, &size, stdin);
	if (chars_read == -1)
	{
		fprintf(stderr, "Error reading line!\n");
	}
	return inp;
}

int poem_exists(char *target)
{
	char *title = NULL;
	size_t len_t = 0;
	ssize_t chars_read_t;

	char *body = NULL;
	size_t len_b = 0;
	ssize_t chars_read_b;

	FILE *fp = fopen(FILENAME, "a+");

	while (((chars_read_t = getline(&title, &len_t, fp)) != -1) && (chars_read_b = getline(&body, &len_b, fp) != -1))
	{
		if (!strcmp(title, target))
		{
			fclose(fp);
			free(title);
			free(body);
			return 1;
		}
	}
	fclose(fp);
	free(title);
	free(body);

	return 0;
}

int prefill()
{
	rl_insert_text(prefill_text);
	return 0;
}

int modify_poem()
{
	printf("Enter the title of the poem you want to modify: ");
	char *title = read_console();
	if (title == NULL)
	{
		free(title);
		return 1;
	}
	if (!poem_exists(title))
	{
		fprintf(stderr, "Error: Poem doesn't exist\n");
		free(title);
		return 1;
	}

	FILE *fp = fopen(FILENAME, "a+");
	FILE *temp = fopen("temp", "w");

	char *line = NULL;
	size_t len = 0;
	ssize_t chars_read;

	while ((chars_read = getline(&line, &len, fp)) != -1)
	{
		if (!strcmp(line, title))
		{
			chars_read = getline(&line, &len, fp);
			line[strcspn(line, "\r\n")] = 0;

			prefill_text = line;
			rl_startup_hook = prefill;

			printf("Modify poem: \n\x1B[35m");
			char *newp = readline(NULL);
			if (newp == NULL)
			{
				return 1;
			}

			fputs(title, temp);
			fputs(newp, temp);
			fputs("\n", temp);
			printf("\x1B[37m");

			free(newp);
		}
		else
		{
			fputs(line, temp);
		}
	}

	free(title);
	free(line);
	fclose(fp);
	fclose(temp);

	remove(FILENAME);
	rename("temp", FILENAME);

	return 0;
}

int delete_poem(char *title)
{
	if (!poem_exists(title))
	{
		fprintf(stderr, "Error: Poem doesn't exist\n");
		return 1;
	}

	FILE *fp = fopen(FILENAME, "a+");
	FILE *temp = fopen("temp", "w");

	char *line = NULL;
	size_t len = 0;
	ssize_t chars_read;

	while ((chars_read = getline(&line, &len, fp)) != -1)
	{
		if (!strcmp(line, title))
		{
			chars_read = getline(&line, &len, fp);
		}
		else
		{
			fputs(line, temp);
		}
	}

	free(line);
	fclose(fp);
	fclose(temp);

	remove(FILENAME);
	rename("temp", FILENAME);

	return 0;
}

int delete_poem_console()
{
	printf("Enter title of the poem you want to delete: ");
	char *title = read_console();

	if (title == NULL)
	{
		free(title);
		return 1;
	}
	delete_poem(title);

	free(title);
	return 0;
}

int append_poem()
{
	printf("Enter new poem's title: ");
	char *title = read_console();
	if (title == NULL)
	{
		free(title);
		return 1;
	}
	if (poem_exists(title))
	{
		fprintf(stderr, "Error: Poem already exists\n");
		free(title);
		return 1;
	}

	printf("Enter new poem: ");
	char *body = read_console();
	if (body == NULL)
	{
		free(title);
		free(body);
		return 1;
	}

	FILE *fp = fopen(FILENAME, "a");
	fprintf(fp, "%s%s", title, body);

	fclose(fp);
	free(title);
	free(body);

	return 0;
}

int list_poems()
{
	FILE *fp = fopen(FILENAME, "r");

	char *title = NULL;
	size_t len_t = 0;
	ssize_t chars_read_t;

	char *body = NULL;
	size_t len_b = 0;
	ssize_t chars_read_b;

	while ((chars_read_t = getline(&title, &len_t, fp) != -1) && (chars_read_b = getline(&body, &len_b, fp) != -1))
	{
		printf("\x1B[32m%s\x1B[37m", title);
	}

	fclose(fp);
	free(title);
	free(body);

	return 0;
}

int read_file()
{
	FILE *fp = fopen(FILENAME, "a+");

	char *title = NULL;
	size_t len_t = 0;
	ssize_t chars_read_t;

	char *body = NULL;
	size_t len_b = 0;
	ssize_t chars_read_b;
	while ((chars_read_t = getline(&title, &len_t, fp) != -1) && (chars_read_b = getline(&body, &len_b, fp) != -1))
	{
		printf("\x1B[32m%s\x1B[35m%s\n\x1B[37m", title, body);
	}
	fclose(fp);
	free(title);
	free(body);

	return 0;
}

// get title + poem chosen by user
int get_poem(char *poem)
{
	char *title = read_console();
	if (title == NULL)
	{
		free(title);
		return 1;
	}
	FILE *fp = fopen(FILENAME, "r");

	char *line = NULL;
	size_t len = 0;
	ssize_t chars_read;

	while ((chars_read = getline(&line, &len, fp)) != -1)
	{
		if (!strcmp(line, title))
		{
			chars_read = getline(&line, &len, fp);
			line[strcspn(line, "\n")] = '\0';
			snprintf(poem, MAX_MS_LENGTH, "\x1B[32m%s\x1B[35m%s\n\x1B[37m", title, line);
			free(title);
			free(line);
			fclose(fp);
			return 0;
		}
	}

	fprintf(stderr, "Error: Poem doesn't exist\n");

	free(title);
	free(line);
	fclose(fp);
	return 1;
}

int sprinkle(char *path)
{
	// preparing signals
	struct sigaction sigact;
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGUSR1, &sigact, NULL);
	signal(SIGUSR1, handler);

	// preparing pipe
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		perror("Hiba a pipe nyitaskor!");
		exit(EXIT_FAILURE);
	}

	// preparing message queue
	int uzenetsor, status;
	key_t kulcs;
	kulcs = ftok(path, 1);
	uzenetsor = msgget(kulcs, 0600 | IPC_CREAT);
	if (uzenetsor < 0)
	{
		perror("msgget");
		return 1;
	}

	pid_t child = fork();
	if (child == -1)
	{
		perror("Fork hiba");
		exit(EXIT_FAILURE);
	}

	if (child > 0) // parent
	{
		printf("\n\x1B[33mParent\x1B[37m - sending %i. child to Barátfa...\n", rand() % 4 + 1);
		// receiving signal
		sigset_t sigset;
		sigfillset(&sigset);
		sigdelset(&sigset, SIGUSR1);
		sigsuspend(&sigset); // waiting for SIGUSR1

		// writing into pipe
		sleep(3);
		printf("\x1B[33mParent\x1B[37m - listing poems...\n");

		sleep(2);
		list_poems();

		char title_buffer[MAX_MS_LENGTH];
		char buffer[MAX_MS_LENGTH];

		close(pipefd[0]);

		int need = 2;
		while (need > 0)
		{
			printf("\x1B[33m\nParent\x1B[37m - select poem for child (%i left): ", need);
			if (!get_poem(buffer))
			{
				write(pipefd[1], buffer, strlen(buffer) + 1);
				printf("\x1B[33mParent\x1B[37m - poem sent successfully to child!\n");
				need--;
			}
		}
		close(pipefd[1]);
		fflush(NULL);

		// reading from message queue
		struct msgbuf mb;
		status = msgrcv(uzenetsor, &mb, 1024, 5, 0);

		char title[MAX_MS_LENGTH];
		strcpy(title, mb.mtext); // still colorful - for printing
		char white_title[MAX_MS_LENGTH];
		strcpy(white_title, &title[5]); // removed color

		sleep(1);
		printf("\x1B[33mParent\x1B[37m - received message, deleting poem: %s\x1B[37m", title);
		delete_poem(white_title);

		status = msgctl(uzenetsor, IPC_RMID, NULL);
		if (status < 0)
			perror("msgctl");

		wait(NULL);
	}
	else // child
	{
		// sending signal
		sleep(3);
		printf("\x1B[34mChild\x1B[37m - travelling to Barátfa...\n");

		sleep(3);
		printf("\x1B[34mChild\x1B[37m - arrived, sending signal %i to parent!\n", SIGUSR1);
		kill(getppid(), SIGUSR1);

		sleep(15);

		// reading from pipe
		char poem1[MAX_MS_LENGTH];
		char poem2[MAX_MS_LENGTH];
		close(pipefd[1]);

		read_str_from_pipe(pipefd[0], poem1);
		printf("\x1B[34m\nChild\x1B[37m - received first poem:\n%s\x1B[37m", poem1);
		sleep(2);

		read_str_from_pipe(pipefd[0], poem2);
		printf("\x1B[34m\nChild\x1B[37m - received second poem:\n%s\x1B[37m", poem2);
		sleep(2);

		close(pipefd[0]);

		// sending through message queue
		char *poems[2] = {poem1, poem2};
		char *poem = poems[rand() % 2];

		char title[MAX_MS_LENGTH];
		strcpy(title, poem);
		title[strcspn(title, "\r\n")] = '\0'; // only sending title
		strcat(title, "\n");

		struct msgbuf mb;
		mb.mtype = 5;
		strcpy(mb.mtext, title);

		sleep(2);
		printf("\x1B[34m\nChild\x1B[37m - chosen poem sent back to parent: %s", title);
		sleep(3);

		msgsnd(uzenetsor, &mb, strlen(mb.mtext) + 1, 0);

		sleep(3);
		printf("\x1B[34m\nChild\x1B[37m sprinkling:\n\x1B[32m%s\x1B[35mSzabad-e locsolni?\n\x1B[37m", poem);
		sleep(5);
		printf("\x1B[34mChild\x1B[37m - coming back home...\n\n");

		exit(0);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	char c[MAX_INPUT_LENGTH];
	while (1)
	{
		printf("Choose option:  (1 - modify)  (2 - read)  (3 - list)  (4 - add)  (5 - delete)  (6 - sprinkle)  (7 - exit) --- ");

		fgets(c, MAX_INPUT_LENGTH, stdin);
		fflush(stdin);
		int s = c[0] - '0';
		switch (s)
		{
		case 1:
			modify_poem();
			break;
		case 2:
			read_file();
			break;
		case 3:
			list_poems();
			break;
		case 4:
			append_poem();
			break;
		case 5:
			delete_poem_console();
			break;
		case 6:
			sprinkle(argv[0]);
			break;
		case 7:
			return 0;
			break;
		}
	}
	return 0;
}
