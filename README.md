# RTOS Assignment

## Description

1. **Thread Design**
   - Create an application with three periodic threads:
     - **J1:** 300ms period
     - **J2:** 500ms period
     - **J3:** 800ms period
   - Include an aperiodic thread **J4**, triggered by **J2**.

2. **Thread Functionality**
   - Each thread will perform operations that "waste time" similar to previous exercises.

3. **Driver Design**
   - Develop a simple driver with `open`, `close`, and `write` system calls.

4. **Task Execution**
   - Each thread will:
     1. Open the special file associated with the driver.
     2. Write its identifier followed by an opening square bracket (e.g., `[1`, `[2`, `[3`, `[4`).
     3. Close the special file.
     4. Perform operations (i.e., waste time).
     5. Repeat steps 1-3, but write the identifier with a closing square bracket (e.g., `1]`, `2]`, `3]`, `4]`).

5. **Kernel Log Output**
   - The `write` system call logs the string received from the thread. A typical log output might be `[11][2[11]2][3[11]3][4]`, showing thread preemption. Adjust computational times if necessary to observe preemption.

6. **Semaphore Protection**
   - Update all tasks to use semaphores. Protect operations (steps 1-5) with semaphores to prevent preemption. Implement semaphores with a priority ceiling access protocol.




## How to Run

1. **Gain Superuser Access**
   Open a terminal and switch to superuser mode:
   ```bash
   $ sudo su
   ```
2. **Compile the Kernel Module**
Navigate to the `RTOS_Assignment1` directory and compile the kernel module using the Makefile:
 ```bash
   $ make
   ```
3. **Install the Kernel Module**
Install the compiled module into the kernel:
```bash
   $ insmod my_module.ko
   ```
4. **Verify Module Installation**
Check if the module was installed correctly:
```bash
   $ /sbin/lsmod
   ```
Ensure that the module named simple appears in the list.

5. **Check Major Device Number**
Find the major number assigned to the module:
```bash
   $ cat /proc/devices
   ```
6. **Create the Device File**
Create a special device file with the appropriate major number (replace `<major_number>` with the number found in the previous step):
```bash
   $ mknod /dev/simple c <major_number> 0
   ```
7. **Compile the User Program**
Compile the user-space program that interacts with the kernel module:
```bash
   $ gcc -lpthread assignment.c -o assignment
   ```
8. **Run the User Program**
Execute the compiled program:
```bash
   $ ./assignment
   ```
9. **View Kernel Log**
To check the kernel log for output from the driver, use:
```bash
   $ dmesg
   ```









