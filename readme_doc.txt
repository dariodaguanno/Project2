Welcome to myshell!

Myshell is a shell within a shell. Myshell utilizes fork and execvp to run a program 
while the rest of the shell is still running. In myshell, there are two modes; 
interactive mode and batch mode. Interactive mode activates when ./myshell is used to
start the program. Batch mode activates when ./myshell <input.txt> is used to
start the program.

Built in commands:

cd <directory>- changes directory if a directory is given. If no directory is given then
myshell will print the contents of your current directory

clr - clears the screen

dir <directory> - prints the contents of the given directory. If no directory is 
given then myshell will print the current directory

environ - prints the current environment variables

echo <input> - prints <input> on myshell

help - prints the contents of this file

pause - myshell will wait for you to resume until you press enter

quit - exits the program

To redirect your input to a file of your choosing specify using '<' 
To redirect your output to a file of your choosing and truncate specify using '>'
To redirect your output to a file of your choosing and append specify using '>>'
If the file specifed does not exist it will create the file

Piping is also available in myshell! When piping is specified using '|', 
then the standard output from one process becomes the standard input of
another process.
The pipe must be given between two processes: <Process 1> | <Process 2>

If you wish to run your program in the background simply add '&' after the 
process you wish to run.

All other commands will be understood as external commands, and the main shell, bash 
will attempt to run the command instead.