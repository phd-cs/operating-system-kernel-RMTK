#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"


void set_ticks(uint no_of_ticks) {
  Ticks = no_of_ticks;
}

uint ticks() {
  return Ticks;
}

uint deadline() {
  return CurrentTask->Deadline;
}

void set_deadline(uint deadline) {
  isr_off();
  ReadyList->pHead->pTask->Deadline = deadline;
  PreviousTask = ReadyList->pHead->pTask;
  
  listobj *tempListObj = ReadyList->pHead;
  listobj *tempListObj2 = ReadyList->pHead->pNext;
  while (tempListObj->pTask->Deadline > tempListObj2->pTask->Deadline) { //Reschedules ready list
    tempListObj->pPrevious->pNext = tempListObj2; //if deadline is longer for the one before sets it to next
    tempListObj2->pNext->pPrevious = tempListObj;
    tempListObj2->pPrevious = tempListObj->pPrevious;
    tempListObj->pNext = tempListObj2->pNext;
    tempListObj2->pNext = tempListObj;
    tempListObj->pPrevious = tempListObj2;
    
    tempListObj2 = tempListObj->pNext; //to go to next
  }
  NextTask = ReadyList->pHead->pTask;
  SwitchContext();
}

void TimerInt() {
  
  Ticks++;
  listobj *tempListObj = TimerList->pHead;
  while (tempListObj != NULL) { // Check if tasks done sleeping or deadline expired
    if (tempListObj->nTCnt <= ticks() || tempListObj->pTask->Deadline <= ticks()) {
      insert_into_list(remove_from_list(tempListObj)->pTask,ReadyList);
    }
    tempListObj = tempListObj->pNext;
  }
  
  tempListObj = WaitingList->pHead;
  while (tempListObj != NULL) { // Check if tasks done sleeping or deadline expired
    if (tempListObj->pTask->Deadline <= ticks()) {
      insert_into_list(remove_from_list(tempListObj)->pTask,ReadyList);
      //mBox *tempBox = clean mailbox
      //while (
    }
    tempListObj = tempListObj->pNext;
  }

}