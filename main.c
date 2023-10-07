#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"


unsigned int kernel=1, creation=0, communication=0, idle=0;

unsigned int low_deadline  = 100;    
unsigned int high_deadline = 5000;

mailbox *charMbox; 
mailbox *intMbox; 
mailbox *unsignedintMbox;


void task1(){
  char  charTest;
  int   returnValue1, intTest;
  unsigned int uintTest; 

  int                 returnValue2;
  int                 intTest2;

  returnValue1 = 100;
  returnValue2 = send_wait( intMbox, &intTest2 );
 
  terminate();   
  
}

void task2(){
  int                 returnValue2;
  int                 varInt_t2; 

  returnValue2 = receive_wait( intMbox, &varInt_t2 );
  if ( varInt_t2 != 100 )  {while(1) {}}//worked

}




void main()
{
  SystemInit(); 
  SysTick_Config(100000); 
  SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] =  (0xE0);      
  isr_off();
  
  exception returnValue = init_kernel(); 
  if (returnValue != OK) {
    kernel = FAIL; 
    while(1);
  }
  
  if ( ReadyList->pHead != ReadyList->pTail ) {
    kernel = FAIL ;}
  if ( WaitingList->pHead != WaitingList->pTail ) { 
    kernel = FAIL ;}
  if ( TimerList->pHead != TimerList->pTail ) { 
    kernel = FAIL ;}
    
  if ( kernel != OK ) { 
    while(1); //Kernel failed
  }
  
  charMbox = create_mailbox( 10 ,sizeof(char));
  intMbox = create_mailbox( 10 ,sizeof(int));
  unsignedintMbox = create_mailbox( 10 ,sizeof(unsigned int));

  create_task( task1, low_deadline );
  create_task( task2, 8*high_deadline );
  
  
  run();
  
  while(1){ /* something is wrong with run */}
}