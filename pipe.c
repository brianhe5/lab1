#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
// argc = 4
//                    1     2      3
// argv = ["./pipe", "ls", "wc", "cat" ]
/*
for n commands, we only create n-1 pipes for the first n-1 commands
nth command doesn't need a pipe because the output is stdout

as soon as you find any error, exit program
	if you do a faulty command, 
parent process responsible for creating pipes

first child: points 1 (output) into pipe #1's write
			 keeps pointing 0 (to stdin) b/c its the first input
intermittent children: points 0 (in) into pipe #1's read
					   points 1 (out) into pipe #2's write


last child: points 0 (stdin) to last pipe's read
			keeps pointing 1 to stdout b/c its the last output


37min:
	after you fork in intermittent children, can remove pointer 3 pointing
	to previous read pipe
	stdin fds(0) has to point to fds(3)
	close fds(3)
	stdout fds(1) has to point to fds(5)
	close fds(5)

	//child also wont need to read to the pipe its writing to, only the pipe before
	close fds(4)
	run execlp(intermittent child)

	Parent
	wont need to write to previous pipe only read
	close write before fork()
	pipe(pipe_fds)
	fork()


	last child:
	point fds(0) to read in of last
*/
int main(int argc, char *argv[])
{
	//check for occurence of at least 1 process
	if (argc == 1){
		perror("arguments");
		exit(EINVAL);
	}

	//init read/write of pipe instances and child id
	int pipe_fds[2];
	pid_t child_pid;

	//now, can be sure that there is at least 1 input
	// for loop for intermittent case
	for (int i = 1; i < argc; i++ )
	{
		//creates pipe and its test
		//pipe(pipe_fds) takes in integer array of size 2, creates read and write end of buffer
    	if (pipe(pipe_fds) == -1) {
        	perror("pipe");
			//change to exit with errno
    		exit(errno);
		}
		//create fork
    	if ((child_pid = fork()) == -1) {
        	perror("fork");
        	exit(errno);
		}
		fprintf(stdout, "testing");
    	// In the child process
    	if (child_pid == 0) {
			fprintf(stdout, "enters child");

			// Since the first child does not need to write (input) into the pipe, we can close write end
			//First child doesnt care about stdin, sends stdout to write
			if (i == 1) {
				fprintf(stdout, "enters i==0");

				//makes stdout pointer point to fds: 4 (write)
				dup2(pipe_fds[1], 1);
				
				//clean up unncessary/ redundant fds
				// from child's perspective, dont need to read or write
				close(pipe_fds[1]);
				close(pipe_fds[0]);
				
				//run first child, output put into same buffer
				if (execlp(argv[i], argv[i], NULL) == -1) {
					perror("execlp failed: bogus");
					exit (errno);
				}
				continue;
			}

			//last process changes stdin to read and keeps fds(1) stdout the same
			else if (i == argc-1){
				//make stdin point to last pipe's read
				//dup2(pipe_fds[0], 0);
				fprintf(stdout, "enters lastcase");
				//clean up fds
				close(pipe_fds[1]);
				close(pipe_fds[0]);

				//run last process
				if (execlp(argv[i], argv[i], NULL) == -1) {
					perror("execlp failed: bogus");
					exit (errno);
				}
				continue;
			}

			//inner case
			else {

			//stdin fds(0) should be read in already of last pipe
			//stdout fds(1) to current write fds(4)
			dup2(pipe_fds[1], 1);

			//close current write fds(4)
			close(pipe_fds[1]);

			//close current read fds(3)
			close(pipe_fds[0]);

			//run program
			if (execlp(argv[i], argv[i], NULL) == -1) {
				perror("execlp failed: bogus");
				exit (errno);
			}
			
			}
		} 

    	// In the parent process
    	else {
		fprintf(stdout, "enters parent");

		//exit status

		int status;
		//pass the address of status variable to waitpid
		//waitpid stores exit status of child process in status
		if (waitpid(child_pid, &status, 0) == -1){
			perror("wait error");
			exit(errno);
			//exits with error number
		}
		if (WIFEXITED(status) && WEXITSTATUS(status) !=0) {
			fprintf(stderr, "child exited with: %d\n", WEXITSTATUS(status));
			exit(WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			fprintf(stderr, "child signalled with: %d\n", WTERMSIG(status));
			exit(WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			fprintf(stderr, "child stopped: %d\n", WSTOPSIG(status));
			exit(WSTOPSIG(status));
		}

		//the next process would not write to first pipe, but will read
		close(pipe_fds[1]);

		//makes parent stdin: fds(0) to fds(3)

		if(dup2(pipe_fds[0],0) == -1){
			perror("pipe redirect error");
			exit(errno);
		}

		//closes fds(3) read
		close(pipe_fds[0]);
		fprintf(stdout, "end ofparent");

    	}
	}
	
	return 0;
}
