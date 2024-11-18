/*
     File        : nonblocking_disk.H

     Author      : 

     Date        : 
     Description : 

*/

#ifndef _NONBLOCKING_DISK_H_
#define _NONBLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"
#include "scheduler.H"

class Scheduler;
class Thread;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
struct queue_node {
   Thread * thread;
   unsigned long block_no;
   unsigned char* buf;
   DISK_OPERATION op;
   queue_node * next;
};

/*--------------------------------------------------------------------------*/
/* N o n B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/


class NonBlockingDisk : public SimpleDisk {
private:

   queue_node * head;
   queue_node * tail;

   queue_node * issued;

   queue_node * pop();
   void push(Thread * _thread, unsigned long _block_no, unsigned char* _buf, DISK_OPERATION _op);

   void set_issued(Thread * _thread, unsigned long _block_no, unsigned char* _buf, DISK_OPERATION _op);

public:
   static NonBlockingDisk * curr_NB_disk;

   NonBlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a NonBlockingDisk device with the given size connected to the 
      MASTER or DEPENDENT slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

   void new_issue();
   
   void set_issued(queue_node * _queue_node);
   queue_node * get_issued();

   bool get_status();

   bool queue_empty();
};

#endif
