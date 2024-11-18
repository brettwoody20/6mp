/*
     File        : nonblocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"


NonBlockingDisk * NonBlockingDisk::curr_NB_disk = nullptr;


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    head = nullptr;
    tail = nullptr;
    issued = nullptr;

    curr_NB_disk = this;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void NonBlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  Console::puts("NBD Read\n");
  //if no operation has been issued (waiting for response) then we can go ahead and issue the command
  //otherwise we will add it to the queue to be issued later
  if (issued == nullptr) {
    Console::puts("Issuing read\n");
    issue_operation(DISK_OPERATION::READ, _block_no);
    set_issued(Thread::CurrentThread(), _block_no, _buf, DISK_OPERATION::READ);
    Console::puts("Issued read\n");
  } else {
    push(Thread::CurrentThread(), _block_no, _buf, DISK_OPERATION::READ);
  }

  Console::puts("Yielding\n");
  //we then yield the processor while we will wait for the disk to finish
  Scheduler::curr_scheduler->yield();
}


void NonBlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  Console::puts("NBD Write\n");
  //if no operation has been issued (waiting for response) then we can go ahead and issue the command
  //otherwise we will add it to the queue to be issued later
  if (issued == nullptr) {
    Console::puts("Issuing write \n");
    issue_operation(DISK_OPERATION::WRITE, _block_no);
    set_issued(Thread::CurrentThread(), _block_no, _buf, DISK_OPERATION::WRITE);
  } else {
    push(Thread::CurrentThread(), _block_no, _buf, DISK_OPERATION::WRITE);
  }

  //we then yield the processor while we will wait for the disk to finish
  Scheduler::curr_scheduler->yield();
}

void NonBlockingDisk::new_issue() {
  if (head != nullptr) {
    issued = pop();
    issue_operation(issued->op, issued->block_no);
  }
}


/*--------------------------------------------------------------------------*/
/* HELPER FUNCTIONS */
/*--------------------------------------------------------------------------*/
queue_node * NonBlockingDisk::pop() {
  queue_node * ret = head;
  head = head->next;

  return ret;
}

void NonBlockingDisk::push(Thread * _thread, unsigned long _block_no, unsigned char * _buf, DISK_OPERATION _op) {
  queue_node * new_node = new queue_node;
  new_node->thread = _thread;
  new_node->block_no = _block_no;
  new_node->buf = _buf;
  new_node->op = _op;
  new_node->next = nullptr;

  if (head == nullptr) {
    head = new_node;
    tail = new_node;
  } else {
    tail->next = new_node;
    tail = new_node;
  }
}

void NonBlockingDisk::set_issued(Thread * _thread, unsigned long _block_no, unsigned char* _buf, DISK_OPERATION _op) {
  queue_node * new_node = new queue_node;
  new_node->thread = _thread;
  new_node->block_no = _block_no;
  new_node->buf = _buf;
  new_node->op = _op;
  new_node->next = nullptr;

  issued = new_node;
}

void NonBlockingDisk::set_issued(queue_node * _queue_node) {
  issued = _queue_node;
}

queue_node * NonBlockingDisk::get_issued() {
  return issued;
}

bool NonBlockingDisk::get_status() {
  return is_ready();
}

bool NonBlockingDisk::queue_empty() {
  return (head == nullptr);
}


