/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler* Scheduler::curr_scheduler = nullptr;

Thread * Scheduler::pop() {
  //Console::puts("Pop\n");
  Thread * ret = head->thread;
  thread_node * next = head->next;
  delete head;
  head = next;

  return ret;
}

void Scheduler::push(Thread * _thread) {
  //Console::puts("Push\n");
  thread_node * new_node = new thread_node;
  new_node->thread = _thread;
  new_node->next = nullptr;

  if (head == nullptr) {
    head = new_node;
    tail = new_node;
  } else {
    tail->next = new_node;
    tail = new_node;
  }
}

Scheduler::Scheduler() {
  curr_scheduler = this;
  head = nullptr;
  tail = nullptr;
  zombies = nullptr;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  Console::puts("yield: ");Console::puti(count());Console::puts("\n");
  Thread::dispatch_to(pop());
}

void Scheduler::resume(Thread * _thread) {
  Console::puts("Resume \n");
  push(_thread);

  while (zombies != nullptr) {
    //pop from stack
    thread_node * temp = zombies;
    zombies = zombies->next;

    //delete the thread
    delete temp->thread;
    delete temp;
  }

  //MANAGE DISK QUEUE

  NonBlockingDisk * curr_disk = NonBlockingDisk::curr_NB_disk;
  //if there is a thread waiting on a disk operation and its disk operation is done
  if (curr_disk->get_issued() != nullptr && curr_disk->get_status() == true) {
    queue_node * curr = curr_disk->get_issued();

    //if it is a read operation, read from port
    //else if it is a write operation, then write data to port
    if (curr->op == DISK_OPERATION::READ) {
      int i;
      unsigned short tmpw;
      for (i = 0; i < 256; i++) {
        tmpw = Machine::inportw(0x1F0);
        curr->buf[i * 2] = (unsigned char)tmpw;
        curr->buf[i * 2 + 1] = (unsigned char)(tmpw >> 8);
      }
    } else {

      int i;
      unsigned short tmpw;
      for (i = 0; i < 256; i++) {
        tmpw = curr->buf[2 * i] | (curr->buf[2 * i + 1] << 8);
        Machine::outportw(0x1F0, tmpw);
      }
    }

    resume(curr->thread); //add thread back to scheduler queue
    delete curr; //delete the node from memory now that we're done with it
    curr_disk->new_issue(); //call function to issue new operation if there is one in queue
  }
}

void Scheduler::add(Thread * _thread) {
  Console::puts("add: ");Console::puti(count());Console::puts("\n");
  push(_thread);
}


//if the current thread: add to zombie thread list, yield
//else, other thread: remove from queue, add to zombie thread list
void Scheduler::terminate(Thread * _thread) {
  //if it is the current thread
  if (_thread == Thread::CurrentThread()) {
    //create a new node and add it to zombies list
    thread_node * new_zombie = new thread_node;
    new_zombie->thread = _thread;
    new_zombie->next = nullptr;

    if (zombies == nullptr) {
      zombies = new_zombie;
    } else {
      //insert it on stack
      new_zombie->next = zombies;
      zombies = new_zombie;
    }
    //yield the processor
    yield();
    
  } else {
    //- Remove the node from the list
    thread_node * curr;
    if (_thread == head->thread) {//-- if the thread passed is the head of the list then remove it and set the head to be 2nd item in list
      curr = head;
      head = head->next;
    } else {//-- otherwise iterate through list until we find a node with the same thread
      curr = head;
      thread_node * pred = nullptr;
      while (curr->thread != _thread && curr != nullptr) {
        pred = curr;
        curr = curr->next;
      }
      //throw error if thread is not found
      if (curr == nullptr) { assert(false); }

      pred->next = curr->next;
      if (tail == curr) {
        tail = pred;
      }
    }

    //- add it to zombie list
    if (zombies == nullptr) {//-- if zombies is empty, add this as the first node
      zombies = curr;
      zombies->next = nullptr;
    } else {//-- otherwise, insert it onto stack
      curr->next = zombies;
      zombies = curr;
    }
  }
}


int Scheduler::count() {
  if (head == nullptr) {
    return 0;
  }
  int count = 1;
  thread_node * curr = head;
  while (curr->next != nullptr) {
    count++;
    curr = curr->next;
  }
  return count;
}