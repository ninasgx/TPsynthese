**Synthesys Practical Work: ENSEA in the Shell**

_Objectives : Develop a tiny shell, that displays exit codes and execution times of launched programs._


For the project of implementing a simple shell, we began by outlining the basic structure of the program, including required headers and constants. Then we implemented functions to show the welcome messages, to read user commands, and to parse the commands into arguments.

Next, we added handling for running commands in the foreground using fork() and execvp(). For handling any input and output redirections, we added code to open files and do the corresponding redirection of the standard input and output.
We then had introduced background process management and typedef'd a struct, BackgroundProcess. This structure held information about the processes in the background-running processes. The PID, job ID, and command string are to be stored.

To support background execution, we modified our function that executes commands to parse the & character. We also made sure that background processes could be launched without hanging the shell. In addition, we have implemented a non blocking wait in order to check for all the finished background processes and print their statuses.
Finally, we tested various commands, including handling pipes and redirections, ensuring that our shell functions as intended.

All those steps can be seen by the history of commits made into the branch. Each commit was a step to finish the proposed laboratory. 


**Students: CONRADO Alamo and GONÇALVES Sabrina**
