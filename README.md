# Linux-Shell-with-Advanced-Command-Execution
It is designed to handle user- defined commands, multi-threaded computations, and pipelined execution of commands.

# 🖥️ Linux Shell with Advanced Command Execution  

[![C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))  
[![Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://en.wikipedia.org/wiki/Linux)  
[![Multi-threading](https://img.shields.io/badge/Multi--Threading-pthreads-green.svg)](https://en.wikipedia.org/wiki/POSIX_Threads)  

## 📌 Overview  
This project implements a **custom Linux shell** with additional functionality for **vector operations, basic text editing, and command execution**. It is designed to handle:  
✔️ **User-defined commands**  
✔️ **Multi-threaded computations**  
✔️ **Pipelined execution of commands**  

---

## 🚀 Features  

### 🔹 **Command Handling**  
- Supports **built-in commands** like `cd`, `help`, and `exit`.  
- Allows **user-defined commands** for **vector operations** and **text editing**.  

### 🔹 **Pipeline Execution**  
- Executes **piped commands** by handling **inter-process communication**.  

### 🔹 **Vector Operations**  
- **addvec** → Performs **vector addition**.  
- **subvec** → Computes **vector subtraction**.  
- **dotprod** → Calculates the **dot product** of two vectors.  

### 🔹 **Text Editor**  
- A **terminal-based text editor** using **ncurses**.  
- Supports **word, line, and character counting**.  
- Allows **file saving** within the editor.  

### 🔹 **Multi-threading Support**  
- Uses **pthreads** for **parallel execution** of vector operations.  

### 🔹 **Command History**  
- Implements **input history tracking** using **GNU Readline**.  

---

## 🏆 Technologies Used  

### 🔹 **Languages**  
- **C**  

### 🔹 **Libraries**  
- `pthreads` → Multi-threading support.  
- `readline` → Command-line input and history management.  
- `ncurses` → Terminal-based UI for the text editor.  

### 🔹 **System Calls**  
- Uses **various Linux system calls** for process and file handling.  

---

## 🛠 How to Run  

### 🔹 **1. Clone the Repository**  
```bash
git clone https://github.com/username/repository.git
cd repository
