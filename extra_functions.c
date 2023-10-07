
#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"


//inserts a task into the_list
void insert_into_list(TCB *new_task,list *the_list) {
  listobj *tempCurrent = the_list->pHead; //Creates new listobj if it's not the first run
  listobj *tempBeforeNext;
  if (new_task->Deadline <= Ticks) {
    return;
  }
  if (the_list->pHead == NULL) {
    the_list->pHead->pTask = new_task;
  } else {
    if (tempCurrent->pTask->Deadline < new_task->Deadline) {//If it has a longer deadline
          
      listobj *new_listobj;//Allocation
      new_listobj = malloc (sizeof(listobj));
      new_listobj->pTask = new_task; 
          
      //Check if it has longer deadline than next
      while (tempCurrent->pTask->Deadline < new_task->Deadline) { 
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
      new_listobj->pTask = new_task;
      new_listobj->pNext = the_list->pHead;
      the_list->pHead->pPrevious = new_listobj;
      the_list->pHead = new_listobj;
    }
  }
}

listobj *remove_from_list (listobj *theListObj) {
  theListObj->pPrevious->pNext = theListObj->pNext;
  theListObj->pNext->pPrevious = theListObj->pPrevious;
  theListObj->pNext = NULL;
  theListObj->pPrevious = NULL;
  return theListObj;
}

