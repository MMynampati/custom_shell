# custom_shell
## built for Principles in System Design @ UCI

# 53shell (icssh) - Custom Unix Shell Implementation

A fully-featured Unix shell implementation written in C that provides job control, I/O redirection, pipes, and background process management.

## 🚀 Features

### Built-in Commands
- **`exit`** - Terminate the shell (kills all background jobs with SIGKILL)
- **`cd [directory]`** - Change working directory and print new path
- **`estatus`** - Display exit status of last foreground command (red highlighting for errors)
- **`bglist`** - List all active background jobs with PID and command
- **`fg [pid]`** - Bring background job to foreground (most recent if no PID specified)

### Process Management
- **Background Jobs** - Run commands with `&` suffix
- **Job Control** - Switch between foreground/background processes
- **Signal Handling** - Proper `SIGCHLD` and `SIGUSR2` handling
- **Zombie Prevention** - Automatic reaping of terminated processes
- **Process Statistics** - Track completed programs (accessible via SIGUSR2)

### I/O Operations
- **Input Redirection** - `command < input.txt`
- **Output Redirection** - `command > output.txt`
- **Append Mode** - `command >> output.txt` 
- **Error Redirection** - `command 2> errors.txt`
- **Combined Redirection** - `command &> combined.txt`
- **Pipes** - `command1 | command2 | command3`

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Main Shell    │───▶│   Job Manager    │───▶│ Process Control │
│     Loop        │    │  (Background)    │    │  (Fork/Exec)    │
│   (readline)    │    │   (dlist_t)      │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ Command Parser  │    │ Signal Handler   │    │  I/O Manager    │
│ (validate_input)│    │  (SIGCHLD/USR2)  │    │ (Pipes/Redir)   │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 🛠️ Building

### Prerequisites
- GCC compiler
- GNU Readline library (`libreadline-dev` on Ubuntu/Debian)
- POSIX-compliant system (Linux/macOS)

### Compilation
```bash
# Standard build
make

# Debug build (with verbose output and colored prompt)
make debug

# Clean build artifacts  
make clean
```

### Output
- Executable: `bin/53shell`

## 📖 Usage

### Starting the Shell
```bash
./bin/53shell
```

### Basic Commands
```bash
# Navigate directories
cd /home/user
cd ..                    # Prints new directory path

# Check command status
ls /nonexistent
estatus                  # Shows exit code (red highlighting for errors)

# Background processes
sleep 10 &
python3 long_script.py &
bglist                   # List background jobs with PID and timestamp

# Foreground control
fg                       # Bring most recent bg job to fg
fg 1234                  # Bring specific PID to fg
```

### Advanced Features
```bash
# I/O Redirection
sort < unsorted.txt > sorted.txt
grep "error" logfile.txt 2> errors.txt
make &> build_output.txt             # Combine stdout and stderr

# Pipes
ps aux | grep python | wc -l
cat file.txt | sort | uniq | head -10

# Background job management
python3 server.py &                  # Start background job
bglist                               # List: PID, command, start time
fg 1234                              # Bring to foreground
```

### Signal Features
```bash
# Send SIGUSR2 to shell process to see statistics
kill -USR2 <shell_pid>
# Output: "Number of programs successfully completed: X"
```

### Process Flow

1. **Command Input**: `readline()` captures user input
2. **Parsing**: `validate_input()` creates `job_info` structure
3. **Built-in Check**: Handle shell built-ins (`cd`, `exit`, etc.)
4. **Process Creation**: `fork()` creates child process
5. **I/O Setup**: Child sets up redirection/pipes before `execvp()`
6. **Execution**: Child runs command, parent waits or tracks background job
7. **Cleanup**: Proper memory management and process reaping

### Example Test Session
```bash
$ ./bin/53shell
<53shell>$ echo "Hello World" > test.txt
<53shell>$ cat < test.txt
Hello World
<53shell>$ sleep 3 &
<53shell>$ bglist
[PID] [START TIME] [COMMAND]
<53shell>$ ps aux | grep sleep | wc -l
2
<53shell>$ fg
sleep 3
<53shell>$ estatus
0
<53shell>$ exit
```

## 📁 Project Structure

```
custom_shell/
├── include/
│   ├── icssh.h           # Main shell structures and constants
│   ├── helpers3.h        # Helper function prototypes  
│   ├── dlinkedlist.h     # Doubly-linked list interface
│   └── debug.h           # Debug macros and color definitions
├── src/
│   ├── icssh.c           # Main shell implementation and built-ins
│   ├── helpers3.c        # I/O, signals, job management functions
│   └── dlinkedlist.c     # Background job tracking with linked list
├── lib/
│   └── icsshlib.o        # Pre-compiled command parsing library
├── rsrc/                 # Test scripts and resources
│   ├── *.py              # Python test utilities
│   ├── input.txt         # Sample input file
│   ├── output.txt        # Sample output file
│   └── icssh.supp        # Valgrind suppression rules
├── bin/                  # Compiled executable (created by make)
├── Makefile              # Build configuration
└── README.md             # This file
```

## 🔍 Implementation Details

### Memory Management
- **Dynamic Allocation**: All structures properly allocated/freed
- **Valgrind Clean**: No memory leaks (see `rsrc/icssh.supp`)
- **Signal Safety**: Async-signal-safe operations in handlers
- **Cleanup on Exit**: Graceful shutdown kills all background processes

### Signal Handling
```c
// SIGCHLD: Asynchronous background process cleanup
void sigchld_handler(int sig) {
    chld_flag = 1;  // Set flag for main loop processing
}

// SIGUSR2: Report statistics using signal-safe functions
void sigusr2_handler(int sig) {
    write(2, "Number of programs successfully completed: ", 43);
    // ... safe integer printing
}
```

### Error Handling
- **Built-in Error Messages**: Consistent error reporting
- **Input Validation**: Robust command parsing
- **Resource Cleanup**: Proper handling of failed operations
- **Process Management**: Handles fork/exec failures gracefully

## 🎯 Key Features Demonstrated

### Systems Programming Concepts
- **Process Creation**: `fork()` and `execvp()` system calls
- **Process Synchronization**: `waitpid()` with proper status handling
- **Signal Programming**: Asynchronous event handling
- **File Descriptors**: Low-level I/O with `dup2()` and `pipe()`
- **Memory Management**: Dynamic allocation with proper cleanup

### Data Structures
- **Doubly-Linked Lists**: Background job tracking
- **Function Pointers**: Generic list operations
- **Structured Data**: Complex job and process information

### Advanced Features
- **Job Control**: Background/foreground process management
- **I/O Redirection**: Complete file descriptor manipulation
- **Pipeline Processing**: Multi-stage command execution
- **Error Recovery**: Graceful handling of system call failures

## Skills Gained

This implementation demonstrates mastery of:
- **Unix System Programming**: Low-level system call usage
- **Process Management**: Creation, monitoring, and cleanup
- **Inter-Process Communication**: Pipes and signal handling
- **File I/O Systems**: Redirection and file descriptor management
- **Memory Management**: Dynamic allocation and leak prevention
- **Software Engineering**: Modular design and error handling

**Development Environment**: C99, GCC, GNU/Linux  
**Dependencies**: `libreadline`, POSIX system calls  
**Course**: ICS 53 - Principles in System Design  
**Assignment**: Custom Shell Implementation 
