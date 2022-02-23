/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef OS_SYSCALL_NUMBER_H_
#define OS_SYSCALL_NUMBER_H_

#define IGNORE 0
#define NUM_SYSCALLS 64

/* define */
#define SYSCALL_SPAWN 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7

#define SYSCALL_TASKSET_P 8
#define SYSCALL_TASKSET 9

#define SYSCALL_FUTEX_WAIT 10
#define SYSCALL_FUTEX_WAKEUP 11

#define SYSCALL_EXEC 12
#define SYSCALL_SHOW_EXEC 13

#define SYSCALL_WRITE 14
#define SYSCALL_READ 15
#define SYSCALL_CURSOR 16
#define SYSCALL_REFLUSH 17
#define SYSCALL_SERIAL_READ 18
#define SYSCALL_SERIAL_WRITE 19
#define SYSCALL_READ_SHELL_BUFF 20
#define SYSCALL_SCREEN_CLEAR 21

#define SYSCALL_GET_TIMEBASE 22
#define SYSCALL_GET_TICK 23
#define SYSCALL_GET_CHAR 24

#define MTHREAD_BARRIER_INIT 25
#define MTHREAD_BARRIER_WAIT 26
#define MTHREAD_BARRIER_DESTROY 27

#define MTHREAD_SEMAPHORE_INIT 28
#define MTHREAD_SEMAPHORE_UP 29
#define MTHREAD_SEMAPHORE_DOWN 30
#define MTHREAD_SEMAPHORE_DESTROY 31

#define MTHREAD_MUTEX_INIT 32
#define MTHREAD_MUTEX_LOCK 33
#define MTHREAD_MUTEX_UNLOCK 34

#define SYSCALL_MAILBOX_OPEN 35
#define SYSCALL_MAILBOX_CLOSE 36
#define SYSCALL_MAILBOX_SEND 37
#define SYSCALL_MAILBOX_RECV 38
#define SYSCALL_MULMAIL 39

#define SYSCALL_BINSEMGET 40
#define SYSCALL_BINSEMOP 41
#define MTHREAD_CREATE 42
#define MTHREAD_JOIN 43

#define SYSCALL_MKFS    44
#define SYSCALL_STATFS    45
#define SYSCALL_CD    46
#define SYSCALL_MKDIR    47
#define SYSCALL_RMDIR    48
#define SYSCALL_TOUCH    49

#define SYSCALL_LN 50
#define SYSCALL_CAT   51



#define SYSCALL_FOPEN 52
#define SYSCALL_FREAD 53
#define SYSCALL_FWRITE 54
#define SYSCALL_FCLOSE 55

#define SYSCALL_LS 57

#define SYSCALL_LSEEK 58
#define SYSCALL_RM 59

#define SYSCALL_LS_L 60

#endif
