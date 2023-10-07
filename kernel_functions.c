#include "kernel_functions.h"
#include <stdlib.h>

int Ticks;
int KernelMode;
TCB *PreviousTask, *NextTask;
list *ReadyList, *WaitingList, *TimerList;



  //Switch to process stack of task to be loaded {switch_to_stack_of_next_task(); }
  //Remove data structures of task being terminated
  //Load context using: {LoadContext_In_Terminate();}

listobj extract(listobj *theObj) {
  ReadyList->pHead = ReadyList->pHead->pNext;
  ReadyList->pHead->pPrevious = NULL;
  return *theObj;
}

void terminate (){
  isr_off();
  listobj leavingObj;
  leavingObj = extract(ReadyList->pHead);
  /* extract() detaches the head node from the ReadyList and
  * returns the list object of the running task */
  NextTask = ReadyList->pHead->pTask;
  switch_to_stack_of_next_task();
  

  free(leavingObj.pTask); //It frees current task but that makes it not run anything
  //free(&leavingObj);
  LoadContext_In_Terminate();
/* supplied to you in the assembly file
* does not save any of the registers. Specifically, does not save the
* process stack pointer (psp), but
* simply restores registers from saved values from the TCB of NextTask
* note: the stack pointer is restored from NextTask->SP
*/
}


  //Initialize interrupt timer {Ticks = 0;} 
  //KernelMode = RUNNING; //Sets kernelmode to running
  //Set NextTask to equal TCB of the task to be loaded
  //Load context using: { LoadContext_In_Run(); }


void run (void)
{
  set_ticks(0);
  KernelMode = RUNNING;
  NextTask = ReadyList->pHead->pTask;
  LoadContext_In_Run();

/* supplied to you in the assembly file
* does not save any of the registers
* but simply restores registers from saved values
* from the TCB of NextTask
*/
}
void idle_task(){
  while (1);
}


int init_kernel(void){
  set_ticks(0);//sets ticks to 0
  
  ReadyList = malloc (sizeof(list));
  
  ReadyList->pHead = NULL; //initializes ReadyList
  ReadyList->pTail = NULL; 
  
  WaitingList = malloc (sizeof(list));
  
  WaitingList->pHead = NULL; //initializes WaitingList
  WaitingList->pTail = NULL; 
  
  TimerList = malloc (sizeof(list));
  
  TimerList->pHead = NULL; //initializes TimerList
  TimerList->pTail = NULL; 

  create_task(&idle_task,UINT_MAX);
  
  KernelMode = INIT;//indicates that kernel is initialized
  return OK;
}

exception create_task (void (*taskBody)(), unsigned int deadline)
{
  TCB *new_tcb;
  new_tcb = (TCB *) calloc (1, sizeof(TCB));
  
  if (new_tcb == NULL) {
    return FAIL;
  }

  new_tcb->PC = taskBody;
  new_tcb->SPSR = 0x21000000;
  new_tcb->Deadline = deadline;
  
  new_tcb->StackSeg [STACK_SIZE - 2] = 0x21000000;
  new_tcb->StackSeg [STACK_SIZE - 3] = (unsigned int) taskBody;
  new_tcb->SP = &(new_tcb->StackSeg [STACK_SIZE - 9]);
  
  // after the mandatory initialization you can implement the rest of the suggested pseudocode
  
  if (KernelMode == INIT){
    if (ReadyList->pHead == NULL) { //For the first run where readylist has nothing
    ReadyList->pHead = malloc (sizeof(listobj)); //Allocation
    ReadyList->pTail = ReadyList->pHead;
    ReadyList->pHead->pTask = new_tcb;
    } else {
      listobj *tempCurrent = ReadyList->pHead; //Creates new listobj if it's not the first run
      listobj *tempBeforeNext;
      if (tempCurrent->pTask->Deadline < new_tcb->Deadline) {//If it has a longer deadline
        
        listobj *new_listobj;//Allocation
        new_listobj = malloc (sizeof(listobj));
        new_listobj->pTask = new_tcb; 
        
        //Check if it has longer deadline than next
        while (tempCurrent->pTask->Deadline < new_tcb->Deadline) { 
          tempBeforeNext = tempCurrent;
          tempCurrent = tempCurrent->pNext;
        }
        //Inserts itself once deadline is shorter
        tempBeforeNext->pNext = new_listobj;
        tempCurrent->pPrevious = new_listobj;
        new_listobj->pNext = tempCurrent;
        new_listobj->pPrevious = tempBeforeNext;
        
      } else  {//if deadline is shorter insert right away
        listobj *new_listobj;
        new_listobj = malloc (sizeof(listobj));//Allocation and insertion
        new_listobj->pTask = new_tcb;
        new_listobj->pNext = ReadyList->pHead;
        ReadyList->pHead->pPrevious = new_listobj;
        ReadyList->pHead = new_listobj;
        //SwitchContext();
      }
        
    }
    
    return OK;
  }else { //Needs to check the priority
    isr_off(); //disable interrupts 
    ReadyList->pHead->pPrevious = ReadyList->pHead; //update previous task
    ReadyList->pHead->pTask = new_tcb; //insert new task in ReadyList
    NextTask = ReadyList->pHead->pTask;//update NextTask
    SwitchContext();//Switch context
  }
  return OK;
}

