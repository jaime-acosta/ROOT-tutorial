## ROOT-tutorial
### Introductory C++/ROOT/Make tutorial for beginners

The purpose of this package is to serve as a starting reference to those learning how to write ROOT macros, implement them as compiled C++ executables and compile them with Make. The user may focus on only one of the three points and ignore the others.

The ROOT macro `Analyze.cc` teaches how to read data from a `.txt` file and put it into tree format, sorting the data into events. It then reads the tree and writes raw and calibrated histograms under certain conditions. There are plenty of comments throughout the code explaining each part in more detail.

The custom class to write and read the tree is placed apart as a header. The Makefile is set to link that header to the program and create a ROOT dictionary.

#### How to use:
1. Download the repository: `git clone https://github.com/jaime-acosta/ROOT-tutorial`
2. Enter the repository: `cd ROOT-tutorial`
3. Compile the code: `make`
4. Execute it: `./bin/Analyze`
 
Please note that you can replace the relative paths in `Analyze.cc` with absolute paths and add the line `export PATH=/path/to/root-tutorial/bin:$PATH` to your `.bashrc` in order to execute the program from whichever directory (`Analyze`). Furthermore, one could make use of the `argc` and `argv` variables or `CommandLineInterface.cc` to specify the input and output files as arguments to the command.
