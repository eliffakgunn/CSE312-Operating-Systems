/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>


#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string>  
#include <ucontext.h>
#include <vector>

  
// number of elements in array
#define MAX_NUM 20
// number of threads
#define THREAD_MAX 4
//stack size
#define STACK_SIZE 4096
//buffer size for producer-consumer problem
#define BUFFER_SIZE 2

using namespace std;
  

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/



void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif

struct ThreadFunction{
  void *(*funcName)(void *);
  void *arg;
};

struct ThreadTable{
  int id;
  char name[15]; 
  unsigned long PC;
  unsigned long SP;
  reg_word registers[R_LENGTH];
  char state[15]; //RUNNING, READY, BLOCKED, TERMINATED
  ThreadFunction* function;
  void* stack; 
  int stackSize;
  ucontext_t context;
};

struct Mutex{
  int lock;
};


void SPIM_timerHandler();
void initThreads();
void thread_create(void *(*thrFunc)(void *), void *arg);
int thread_join(int tid);
void thread_yield();
int thread_exit();
void mutex_lock(Mutex* mutex);
void mutex_unlock(Mutex* mutex);
void mutex_init(Mutex *mutex);
void run_function(void (*thrFunc)(void));
void contextSwitch();
void printTable();
void *producer(void *ptr);
void *consumer(void *ptr);
void merge(int low, int mid, int high);
void merge_sort(int low, int high);
void *merge_sort(void* arg);
void mergeSort();
void producerConsumer();


// array of size MAX_NUM
int a[MAX_NUM];
int part = 0;
int join_id = 0;
int threadNum = 0;

list<ThreadTable*> readyQueue, threadTable;
ThreadTable* runningThread = NULL;

//producer-consumer
int buffer = BUFFER_SIZE;
Mutex* mutex = new Mutex();

//increases buffer as BUFFER_SIZE
void *producer(void *ptr){
  for(int i = 0; i<BUFFER_SIZE; ++i){
    cout<<"Producer: "<<runningThread->name<<" trying to get the lock."<<endl;
    mutex_lock(mutex);
    cout<<"Producer: "<<runningThread->name<<" got the lock."<<endl;
    cout<<"Producer: Buffer size before producing:"<<buffer<<endl;
    ++buffer;
    SPIM_timerHandler();
    cout<<"Producer: Producing..."<<endl;
    cout<<"Producer: Buffer size after producing:"<<buffer<<endl;
    cout<<"Producer: "<<runningThread->name<<" released the lock."<<endl;
    mutex_unlock(mutex);
  }
  thread_exit();
}

//decreases buffer as BUFFER_SIZE
void *consumer(void *ptr){
  for(int i = 0; i<BUFFER_SIZE; ++i){
    cout<<"Consumer: "<<runningThread->name<<" trying to get the lock."<<endl;
    mutex_lock(mutex);
    cout<<"Consumer: "<<runningThread->name<<" got the lock."<<endl;
    cout<<"Consumer: Buffer size before producing: "<<buffer<<endl;
    --buffer;
    cout<<"Consumer: Consuming..."<<endl;
    cout<<"Consumer: Buffer size after consuming: "<<buffer<<endl;
    cout<<"Consumer: "<<runningThread->name<<" released the lock."<<endl;
    mutex_unlock(mutex);
  }
  thread_exit();
}

// merge function for merging two parts
void merge(int low, int mid, int high)
{
    int* left = new int[mid - low + 1];
    int* right = new int[high - mid];
  
    // n1 is size of left part and n2 is size
    // of right part
    int n1 = mid - low + 1, n2 = high - mid, i, j;
  
    // storing values in left part
    for (i = 0; i < n1; i++)
        left[i] = a[i + low];
  
    // storing values in right part
    for (i = 0; i < n2; i++)
        right[i] = a[i + mid + 1];
  
    int k = low;
    i = j = 0;
  
    // merge left and right in ascending order
    while (i < n1 && j < n2) {
        if (left[i] <= right[j])
            a[k++] = left[i++];
        else
            a[k++] = right[j++];
    }
  
    // insert remaining values from left
    while (i < n1) {
        a[k++] = left[i++];
    }
  
    // insert remaining values from right
    while (j < n2) {
        a[k++] = right[j++];
    }
}

// merge sort function
void merge_sort(int low, int high)
{
    // calculating mid point of array
    int mid = low + (high - low) / 2;
    if (low < high) {
  
        // calling first half
        merge_sort(low, mid);
  
        // calling second half
        merge_sort(mid + 1, high);
  
        // merging the two halves
        merge(low, mid, high);
    }
}
  
// thread function for multi-threading
void* merge_sort(void* arg)
{
  // which part out of 4 parts
  int low, high, mid, thread_part = part++;

  // calculating low and high
  low = thread_part * (MAX_NUM / THREAD_MAX);
  if(thread_part == THREAD_MAX-1)
    high = MAX_NUM - 1;
  else
    high = (thread_part + 1) * (MAX_NUM / THREAD_MAX) - 1;

  // evaluating mid point
  mid = low + (high - low) / 2;
  if (low < high) {
      merge_sort(low, mid);
      SPIM_timerHandler();
      merge_sort(mid + 1, high);
      merge(low, mid, high);
  }

  thread_exit();
}

//it prints current thread table, then does context switch
void SPIM_timerHandler()
{
  cout<<"\nTime interrupt occured in "<<runningThread->name<<". Thread Table Informations:"<<endl;
  printTable();

  if(!readyQueue.empty())
    contextSwitch();
}

//it does context switchins with round robin schedulin.
void contextSwitch(){
  list<ThreadTable*>::iterator iter;

  ThreadTable* runningThreadCopy = runningThread;

  //it thread did not terminate, then adds it to readyQueue
  int res = strcmp(runningThread->state, "TERMINATED"); 
  if(res != 0){
    strcpy(runningThread->state, "READY");
    readyQueue.push_back(runningThread);
  }

  //if all processes are done, then it returns
  if(readyQueue.empty())
    return;

  //next thread is the head of queue because RR scheduling
  iter = readyQueue.begin();
  
  ThreadTable* nextThread = (ThreadTable*)malloc(sizeof(ThreadTable));
  nextThread = (*iter);

  strcpy(nextThread->state, "RUNNING");
  readyQueue.pop_front();

  cout<<"\nContext switch occured from "<<runningThread->name<<" to "<<nextThread->name<<". Thread Table Informations:"<<endl;
  printTable();

  //changes thread context with next thread
  runningThread = nextThread;
  swapcontext(&(runningThreadCopy->context),&(nextThread->context));
}

//it prints some informations of threads
void printTable(){
  list<ThreadTable*>::iterator iter = threadTable.begin();
  for (unsigned int i = 0; i < threadTable.size(); ++i)
  {
    cout<<"Name: "<<(*iter)->name<<"\tID: "<<(*iter)->id <<"\tState: "<<(*iter)->state<<"\tPC: "<<(*iter)->PC<<"\tSP: "<<(*iter)->SP<<endl;
    ++iter;
  }
  cout<<endl;
}

//inits the mutex as 0
void mutex_init(Mutex *mtx){
  mtx->lock = 0;
}

//if it locks, it returns; otherwise it calls thread_yield func for context switching
void mutex_lock(Mutex* mtx){
  if(mtx->lock == 0){
    mtx->lock = 1;
    return;
  }

  string name;

  if(runningThread->id == 1)
    name = "Producer: ";
  else
    name = "Consumer: ";

  cout<<name<<runningThread->name<<" could not get the lock."<<endl;

  thread_yield();
}

//it unlocks the lock
void mutex_unlock(Mutex* mtx){
  mtx->lock = 0;
}

//it creates threads and sets their informations
void thread_create(void *(*thrFunc)(void *), void *arg){
  string name = "Thread-";

  //if it is first thread, it inits the thread table
  if(threadNum == 0)
    initThreads();

  ThreadTable* newThread = (ThreadTable*)malloc(sizeof(ThreadTable));

  string num = to_string(threadNum);
  name += num;
  strcpy(newThread->name,name.c_str());

  //sets thread's fields
  newThread->id = threadNum++;
  newThread->stack = (char*)malloc(STACK_SIZE);
  newThread->stackSize = STACK_SIZE;
  newThread->SP = (unsigned long)newThread->stack+STACK_SIZE-sizeof(int);
  newThread->PC = (long)thrFunc;

  //sets thread's function
  newThread->function = (ThreadFunction*)malloc(sizeof(ThreadFunction));
  newThread->function->funcName = thrFunc;
  newThread->function->arg = arg;

  //sets thread's stack pointer
  getcontext(&(newThread->context)); 
  newThread->context.uc_link = 0;    
  newThread->stack = malloc(STACK_SIZE);
  newThread->context.uc_stack.ss_sp = newThread->stack;
  newThread->context.uc_stack.ss_size = STACK_SIZE;
  newThread->context.uc_stack.ss_flags = 0;
 
  strcpy(newThread->state,"READY");
  readyQueue.push_back(newThread);
  threadTable.push_back(newThread);

  //sets the context and runs the thread function
  makecontext( &newThread->context, (void(*)(void))&run_function, 1, thrFunc);
}

//inits the thread table
void initThreads(){
  ThreadTable* firstThread = (ThreadTable*)malloc(sizeof(ThreadTable));
  strcpy(firstThread->name, "Main Thread");

  getcontext(&(firstThread->context)); 
  firstThread->stack = firstThread->context.uc_stack.ss_sp;
  firstThread->id = threadNum++;
  firstThread->SP = (unsigned long)firstThread->stack+STACK_SIZE-sizeof(int);

  firstThread->function = (ThreadFunction*)malloc(sizeof(ThreadFunction));
  firstThread->function->funcName = NULL;
  firstThread->function->arg = NULL;
  
  runningThread = firstThread;
  strcpy(firstThread->state,"RUNNING");
}

//runs the thread function
void run_function(void (*thrFunc)(void)) {
  thrFunc();
}

//it finds current thread from thread table, then it checks this thread.
//if thread's state is TERMINATED, so if it exits, it returns
//else it calls thread_yield func for context switching
int thread_join(int i) {
  list<ThreadTable*>::iterator iter = threadTable.begin();
  for(int j=0; j<i-1; ++j)
    ++iter;

  while(1){
    int res = strcmp((*iter)->state, "TERMINATED");
    if(res == 0){
      return 0;
    }
    else
      thread_yield();
  }
}

//it calls context switch
void thread_yield(){
  contextSwitch();
}

//it finds current thread from thread table, then sets its state to TERMINATED
int thread_exit(){
  list<ThreadTable*>::iterator iter = threadTable.begin();

  for(unsigned int i =0; i<threadTable.size(); ++i){
    if((*iter)-> id == runningThread->id){

      strcpy((*iter)->state, "TERMINATED");
      thread_yield();

      return 0;
    }
    ++iter;
  }
  return 1;
}

void mergeSort(){
  srand (time(NULL));
  clock_t t1, t2;

  for (int i = 0; i < MAX_NUM; i++)
    a[i] = rand() % 100;

  cout << "Array: ";
  for (int i = 0; i < MAX_NUM; i++)
    cout << a[i] << " ";
  cout<<endl;

  t1 = clock();

  for(int i = 0; i<THREAD_MAX; ++i){
    thread_create(merge_sort, NULL);
  }

  for(int i = 0; i<THREAD_MAX; ++i){
    thread_join(i);
  }


  merge(0, (MAX_NUM / 2 - 1) / 2, MAX_NUM / 2 - 1);
  merge(MAX_NUM / 2, MAX_NUM/2 + (MAX_NUM-1-MAX_NUM/2)/2, MAX_NUM - 1);
  merge(0, (MAX_NUM - 1)/2, MAX_NUM - 1);


  t2 = clock();

  // displaying sorted array
  cout << "Sorted array: ";
  for (int i = 0; i < MAX_NUM; i++)
    cout << a[i] << " ";
  cout<<endl;

  // time taken by merge sort in seconds
  cout << "Time taken: " << (t2 - t1)/(double)CLOCKS_PER_SEC << endl;
}

void producerConsumer(){
  mutex_init(mutex);
  thread_create(producer, NULL);
  thread_create(consumer, NULL);

  for(int i = 0; i<2; ++i)
    thread_join(i);
}


/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */
int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
  float val = FPR_S (REG_FA0);

  write_output (console_out, "%.8f", val);
  break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
  static char str [256];

  read_input (str, 256);
  R[REG_RES] = atol (str);
  break;
      }

    case READ_FLOAT_SYSCALL:
      {
  static char str [256];

  read_input (str, 256);
  FPR_S (REG_FRES) = (float) atof (str);
  break;
      }

    case READ_DOUBLE_SYSCALL:
      {
  static char str [256];

  read_input (str, 256);
  FPR [REG_FRES] = atof (str);
  break;
      }

    case READ_STRING_SYSCALL:
      {
  read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
  data_modified = true;
  break;
      }

    case SBRK_SYSCALL:
      {
  mem_addr x = data_top;
  expand_data (R[REG_A0]);
  R[REG_RES] = x;
  data_modified = true;
  break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
  static char str [2];

  read_input (str, 2);
  if (*str == '\0') *str = '\n';      /* makes xspim = spim */
  R[REG_RES] = (long) str[0];
  break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];  /* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
  R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
  break;
      }

    case READ_SYSCALL:
      {
  /* Test if address is valid */
  (void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
  R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
  R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
  data_modified = true;
  break;
      }

    case WRITE_SYSCALL:
      {
  /* Test if address is valid */
  (void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
  R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
  R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
  break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
  R[REG_RES] = _close(R[REG_A0]);
#else
  R[REG_RES] = close(R[REG_A0]);
#endif
  break;
      }

      case thr_create:
      {
        thread_create(merge_sort, NULL);
        break;
      }

      case thr_join:
      {
        thread_join(join_id++);
        break;
      }

      case thr_exit:
      {
        thread_exit();
        break;
      }

      case mtx_lock:
      {
        mutex_lock(mutex);
        break;
      }

      case mtx_unlock:
      {
        mutex_unlock(mutex);
        break;
      }

      case init_program:
      {
        int kernel = R[REG_A0];

        if(kernel == 1)
          mergeSort();
        else if(kernel == 2)
          producerConsumer();
        else{
          cout<<"Yanlış numara girdiniz."<<endl;
          cout<<"a0 == 1, if kernel is SPIMOS_GTU_1"<<endl;
          cout<<"a0 == 2, if kernel is SPIMOS_GTU_2"<<endl;
        }

        break;
      }


    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}


void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
  error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
  error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
  error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
  error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
  error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
  error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
  error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
  error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
  error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
  error ("  Floating point\n");
      break;

    default:
      if (!quiet)
  error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}