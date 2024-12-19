**Synthesys Practical Work (tp1): ENSEA in the Shell**

_Objectives : Develop a tiny shell, that displays exit codes and execution times of launched programs._

For the project of implementing a simple shell, we began by outlining the basic structure of the program, including required headers and constants. Then we implemented functions to show the welcome messages, to read user commands, and to parse the commands into arguments.

Next, we added handling for running commands in the foreground using fork() and execvp(). For handling any input and output redirections, we added code to open files and do the corresponding redirection of the standard input and output.
We then had introduced background process management and typedef'd a struct, BackgroundProcess. This structure held information about the processes in the background-running processes. The PID, job ID, and command string are to be stored.

To support background execution, we modified our function that executes commands to parse the & character. We also made sure that background processes could be launched without hanging the shell. In addition, we have implemented a non blocking wait in order to check for all the finished background processes and print their statuses.
Finally, we tested various commands, including handling pipes and redirections, ensuring that our shell functions as intended.

All those steps can be seen by the history of commits made into the branch. Each commit was a step to finish the proposed laboratory. 


**Synthesys Practical Work (folder tp2): TFTP Client**

_Objectives: Develop a TFTP client using RFC specifications and Wireshark captures._

For this project, the goal was to create a TFTP client capable of exchanging files via the Trivial File Transfer Protocol (TFTP). This protocol is commonly used for network OS installations, diskless node operations, and firmware updates for network equipment.

The first step involved using command line arguments to obtain server and file information for the gettftp and puttftp programs. We used getaddrinfo to retrieve the server address and reserved a connection socket to communicate with the server.

For the gettftp program, we implemented the construction of a Read Request (RRQ) and its transmission to the server, and also handling the reception of single and multiple Data (DAT) packets along with their acknowledgments (ACKs).

For the puttftp program, we implemented the creation of a Write Request (WRQ) and its transmission to the server and handling the sending of single and multiple Data (DAT) packets and the reception of corresponding acknowledgments (ACKs).

We also incorporated the blocksize option, testing to find the optimal blocksize for data transfer. Additionally, we implemented error handling for packet loss and error packets (ERR).

**Students: CONRADO Alamo and GONÇALVES Sabrina**
