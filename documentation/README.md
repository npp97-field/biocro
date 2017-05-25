# Generating Documentation
To generate documentation for the C/C++ code, we use Doxygen.

## Installing and Running Doxygen and Related Tools

### Ubuntu Linux
1. Download doxygen and graphviz applications using a the software manager:
    ```
    sudo apt-get install doxygen graphviz
    ```

2. Compile documentation
    ```
    cd /path/to/biocro/documentation
    doxygen Doxyfile
    ```

###  Windows: 
1. Download and install the Doxygen binary distribution from <http://www.stack.nl/~dimitri/doxygen/download.html#srcbin>. After installation open the doxywizard program.

2. Select the Biocro directory as the working directory from which doxygen will run. Press ctrl+O, find the Biocro directory and select the "Doxyfile" file. Then select run and run doxygen.

### Viewing the Documentation
After Doxygen has been run, a new directory named "html" will be created under documentation directory.  Point your browser to /path/to/biocro/documentation/html/index.html

#### Options for ctags.
ctags is a program that generates tags for C and C++ files so that identifiers can be located easily.
This is useful for jumping between locations in the code with editors such as Vim.
If you use ctags, the flag "--c++-kinds=+p" will cause ctags to create tages for symbols in header files, which is often useful.
    ```
    ctags --c++-kinds=+p
    ```