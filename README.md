OSCourseDesign
==============
Operation System Course Design, BUAA

By: Hao Tong

Linux_1 : Shell
---------------

#### 1. Basic Function
This program give a command prompt with format "xsh@current_path>", such as "xsh@/home/tong/shell>", indicating that it is waiting for the user to input commands. After input, it will execute the command and output necessary information if the commands are correct, and then wait for the next command. Otherwise, it will print error information.

##### 1.1 Internal Commands
###### 1. exit  
  Kill all sub processes and exit.  
###### 2. jobs  
  Print jobs that are running in back end or suspended. The print format is
    INDEX   PID   STATE   COMMAND 
  jobs is a internal command, it won't be printed on the screen.  
###### 3. history  
  List the most recent HISTORY_LEN commands.  
###### 4. fg %\<int\>  
  Put a process identified with \<int\> in front end. The shell shouldn't print new information until this command ends.  
###### 5. bg %\<int\>  
  Put a suspended process to the back end and run.

##### 1.2 Switch Between Front and Back End

##### 1.3 I/O Redirection

#### 2. Advanced Function
##### 2.1 Pipe

##### 2.2 Wildcard

#### 3. Self-improved Function
##### 3.1 Use up and down to switch between history commands
##### 3.2 Tab to make up the command
##### 3.3 Backspace
##### 3.4 Improved Grammar
##### 3.5 Method of giving results of backend command
