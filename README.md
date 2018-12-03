# Final OS Project

**Author:_Erich Dingeldein_**

## 1. Building the program:
Open a command prompt in the directory containing the Makefile.
Enter the command:
```
make buildall
```
This will create a build directory containing all of the executable programs.

## 2. Input data file:
Put the input data file you wish to use into the **/data** directory.
The output data will be put in this folder as well.

## 3. Running the program:
First, if you haven't already, complete process 1 and 2.
Have a command prompt open in the directory containing the Makefile.
Enter the command:
```
make run A=./data/<name of input file> B=<name of output file>
```
Substitute the name of the appropriate files between the <> above.
Data will be read from your input file located in the data directory, translated to piglatin, and written to the output file you named above. The output file name _will_ have the directory **./data** appended to it, so only enter the name of your file. The output file will be located in the **/data/** directory upon a sucessful run of the program.

### Other information
All build process can be found in the Makefile. To run a specific make command, just enter:
```
make <process name>
```
**_IMPORTANT_**: The build directory needs to exist in order to build the program. To make the build direcotry enter:
```
make init
```
or
```
mkdir build
```
