#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"



mailbox* create_mailbox( uint nMessages, uint nDataSize ) {
  //Allocates and initializes mailbox including head and tail messages
  mailbox *new_mailbox;
  new_mailbox =  calloc (1, sizeof(mailbox));
  
  if (new_mailbox == NULL) {
    return NULL;
  }
  
  msg *new_mailbox_head;
  new_mailbox_head =  calloc (1, sizeof(mailbox));
  if (new_mailbox_head == NULL) {
    return NULL;
  }
  
  msg *new_mailbox_tail;
  new_mailbox_tail =  calloc (1, sizeof(mailbox));
  if (new_mailbox_tail == NULL) {
    return NULL;
  }
  
  new_mailbox->nMaxMessages = nMessages;// Initialization
  new_mailbox->nDataSize = nDataSize;
  
  new_mailbox_tail->pPrevious = new_mailbox_head;
  new_mailbox_tail->pNext = new_mailbox_tail;
  new_mailbox_head->pNext = new_mailbox_tail;
  new_mailbox_head->pPrevious = new_mailbox_head;
  
  new_mailbox->pHead = new_mailbox_head;
  new_mailbox->pTail = new_mailbox_tail;
  
  return new_mailbox;
}

exception remove_mailbox( mailbox* mBox) {
  if (mBox->nMessages == 0) {
    free(mBox);
    return OK;
  } else {
  return NOT_EMPTY;
  }
}

exception send_wait( mailbox* mBox, void* pData ) { //Sends wait and blocks self if there is no recieve in mailbox
  isr_off();
  msg* tempMsg;
  if (mBox->nMessages < 0 && mBox->pHead->pNext->pBlock != NULL) {//If a Reciever (of type wait) is ->waiting<-
    tempMsg = mBox->pHead->pNext; //Reciever pointer (Located at ->pNext not Head
    memcpy(mBox->pHead->pNext->pData, pData, sizeof(char)); //Copies send data to reciever
    mBox->pHead->pNext->pNext->pPrevious = tempMsg->pPrevious;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext; //Removes recieveing msg from mailbox

    PreviousTask = ReadyList->pHead->pTask; //Previous task is sender
    insert_into_list(mBox->pHead->pBlock->pTask, ReadyList); //inserts recieving task to mailbox
    NextTask = ReadyList->pHead->pTask; 
    
  } else {//Sends message and blocks self
    msg *new_msg = malloc (sizeof(msg)); //Allocation
    if (new_msg == NULL) {
      return FAIL;
    }
    new_msg->pData = pData; 
    listobj *new_listobj = malloc (sizeof(listobj)); //Allocation
    if (new_listobj == NULL) {
      return FAIL;
    }
    new_listobj = ReadyList->pHead;
    new_msg->pBlock = new_listobj;
    
    
    if (mBox->nMessages == 0) {
      mBox->pTail->pPrevious->pNext = new_msg;//inserts into the empty mailbox
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
    } else {
      mBox->pTail->pNext = new_msg; //if not empty also inserts
      new_msg->pPrevious = mBox->pTail;
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
    }
    
    PreviousTask = ReadyList->pHead->pTask;
    
    ReadyList->pHead->pNext->pPrevious = ReadyList->pHead->pNext; //Inserts task into waiting list and removes it from readylist
    ReadyList->pHead = ReadyList->pHead->pNext;
    WaitingList->pTail->pPrevious->pNext = new_listobj;
    WaitingList->pTail = new_listobj;
    
    NextTask = ReadyList->pHead->pTask; 
    mBox->nMessages += SENDER;
    tempMsg = new_msg;
  }
  SwitchContext();
  if (tempMsg->pBlock->pTask->Deadline <= 0) {
    isr_off();
    //remove tempMsg
    isr_on();
    return DEADLINE_REACHED;
  } else {
    return OK;
  }
}

exception receive_wait( mailbox* mBox, void* pData ){ 
  isr_off();
  msg *tempMsg = mBox->pHead->pNext;
  listobj *theBlock = tempMsg->pBlock;
  if (mBox->nMessages > 0) {
    memcpy(pData, tempMsg->pData, mBox->nDataSize);
    
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    //free(tempMsg);//Removes the message from mailbox (Might need to set pNext and pPrevious
    if(tempMsg->pBlock != NULL) {//If it was of type wait 
      PreviousTask = ReadyList->pHead->pTask;
      insert_into_list(theBlock->pTask, ReadyList);
      NextTask = ReadyList->pHead->pTask; 
    } else { //FIX Free senders data area
      free(pData);
    }
  } else {
    msg *new_msg = malloc (sizeof(msg)); //Allocation
    if (new_msg == NULL) {
      return FAIL;
    }

    if (mBox->nMessages == 0) {
      mBox->pTail->pPrevious->pNext = new_msg;//inserts into the empty mailbox
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
    } else {
      mBox->pTail->pNext = new_msg; //if not empty also inserts
      new_msg->pPrevious = mBox->pTail;
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
    }
    PreviousTask = ReadyList->pHead->pTask; //updates previous task  
    listobj *thelistobj = ReadyList->pHead;
    
     //Inserts task into waiting list and removes it from readylist
    ReadyList->pHead = ReadyList->pHead->pNext;
    ReadyList->pHead->pPrevious = ReadyList->pHead;
    
    insert_into_list(thelistobj->pTask, WaitingList);
    WaitingList->pTail->pPrevious->pNext = thelistobj;
    WaitingList->pTail = thelistobj;
    
    tempMsg = new_msg;
    
    NextTask = ReadyList->pHead->pTask;
    
    mBox->nMessages += RECEIVER; 
    
  }
  SwitchContext();
  if (theBlock->pTask->Deadline >= Ticks) {
    isr_off();
    tempMsg->pNext->pPrevious = tempMsg->pPrevious;
    tempMsg->pPrevious->pNext = tempMsg->pNext;
    free(tempMsg);
    isr_on();
    return DEADLINE_REACHED;
  } else {
    return OK;
  }
}

exception send_no_wait( mailbox* mBox, void* pData ){ //Sends message and doesn't block
  isr_off();
  exception status = FAIL;
  msg *tempMsg = mBox->pHead->pNext;
  listobj *tempListObj;
  if (mBox->nMessages < 0) {//if there is a recieving message insert task into ReadyList
    
    memcpy(tempMsg->pData, pData, sizeof(pData));
    TCB *theTask = tempMsg->pBlock->pTask;
    free(pData);
    free(tempMsg);
    PreviousTask = ReadyList->pHead->pTask; //Updates previous task
    insert_into_list(theTask, ReadyList);
    NextTask = ReadyList->pHead->pTask; //Updates next task
    status = OK;
    SwitchContext();
  } else {
    msg *new_msg = malloc (sizeof(msg)); //Allocation
    if (new_msg == NULL){
      return FAIL;
    }
    new_msg->pData = malloc (mBox->nDataSize);
    if (new_msg->pData == NULL){
      return FAIL;
    }
    memcpy(new_msg->pData, pData, mBox->nDataSize);
    if (new_msg->pData == pData) {
      while(1);
    }
    //new_msg->pData = pData;
    if (mBox->nMessages == mBox->nMaxMessages) { //if full
      mBox->pHead->pNext = mBox->pHead->pNext->pNext;//Free's oldest message
      free(mBox->pHead->pNext->pPrevious);
    }
    if (mBox->nMessages == 0){//if mBox is empty msg should become tail
      mBox->pTail->pPrevious->pNext = new_msg;
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
      new_msg->pBlock = NULL;
      //Mailbox is Head(Empty) -> pTail (Msg)
    } else {
      mBox->pTail->pNext = new_msg;
      new_msg->pPrevious = mBox->pTail;
      mBox->pTail = new_msg;
      mBox->pTail->pNext = mBox->pTail;
      //Mailbox is Head(Empty) -> pNext(Msg)... -> pTail (Msg)
      
      new_msg->pBlock = NULL;
    }
  mBox->nMessages += SENDER; 
  status = OK;
  }
  
  return status;
}

exception receive_no_wait( mailbox* mBox, void* pData ){//recieves and if mBox is empty doesn't wait but
  //but still places recieve message
  isr_off();//Disable interrupt
  
  msg *tempMsg = mBox->pHead->pNext;
  listobj *tempObj = tempMsg->pBlock;
  if (mBox->nMessages > 0) { //Looking to recieve a message
    memcpy(pData, tempMsg->pData, mBox->nDataSize);
    
    mBox->pHead->pNext = tempMsg->pNext;
    tempMsg->pNext->pPrevious = mBox->pHead;
    tempMsg->pPrevious = NULL;
    tempMsg->pNext = NULL;
    //free(mBox->pHead->pNext);//Removes the message from mailbox
  
    if (tempObj != NULL) {//pBlock will only have anything if it was a send_wait message (currently blocked)
      PreviousTask = ReadyList->pHead->pTask;
      insert_into_list(tempMsg->pBlock->pTask, ReadyList);
      NextTask = ReadyList->pHead->pTask;
      SwitchContext();
    } else {
      free(tempMsg->pData);
      mBox->nMessages += RECEIVER;
    }
    
  } else {
    return FAIL;
  }
  return OK; //Status on recieved message
  

}

exception wait(uint nTicks) {
  exception status;
  PreviousTask = ReadyList->pHead->pTask;
  insert_into_list(remove_from_list(ReadyList->pHead)->pTask, TimerList);
  NextTask = ReadyList->pHead->pTask;
  SwitchContext();
  if (ReadyList->pHead->pTask->Deadline <= Ticks) {
    status = DEADLINE_REACHED;
  } else {
    status = OK;
  }
  return status;
}



//void task2(void) {
//receive_wait(mb, msg); /* 8 */
/* do something */ /* 11 */
//terminate(); /* 12 */
//}
//void task1(void) {
//create_task(task2, 200); /* 6 */
//send_wait(mb,msg); /* 7 */
/* do something */ /* 9 */
//terminate(); /* 10 */
//}
//void idle() { while(1); /* 5 */ }
//main() {
//init_kernel(); /* 1 */
//mb = create_mailbox(1,15); /* 2 */
//create_task(task1, 100); /* 3 */
//run(); /* 4 */
//}

