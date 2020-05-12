#include <assert.h>

#include <cmu-trace.h>
#include "antsense_queue.h"

#define CURRENT_TIME    Scheduler::instance().clock()
#define QDEBUG

/*
  Packet Queue used by AODV.
*/

antsense_queue::antsense_queue() {
  head_ = tail_ = 0;
  len_ = 0;
  
  ANTSENSE_RTQ_MAX_LEN =5;
  
  ANTSENSE_RTQ_TIMEOUT =60;
  
  limit_ = ANTSENSE_RTQ_MAX_LEN;
  
  timeout_ = ANTSENSE_RTQ_TIMEOUT;
  DEBUGS =0;
}

void antsense_queue::increase_queue(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);

 /*
  * Purge any packets that have timed out.
  */
 dropOnTimeOut();
 
 p->next_ = 0;
 ch->ts_ = CURRENT_TIME + ANTSENSE_RTQ_TIMEOUT;

 if (len_ == ANTSENSE_RTQ_MAX_LEN) 
 {
 	Packet *p0 = remove_head();	// decrements len_

   	assert(p0);
   	if(HDR_CMN(p0)->ts_ > CURRENT_TIME) 
   	{
   		if (DEBUGS) printf("ESTAMOS A DEITAR FORA UM PACOTE, MOTIVO: Queue Full\n");
     		drop(p0, DROP_RTR_QFULL);
   	}
   	else 
   	{
   		if (DEBUGS) printf("ESTAMOS A DEITAR FORA UM PACOTE, MOTIVO: Queue TIMEOUT\n");
     		drop(p0, DROP_RTR_QTIMEOUT);
   	}
 }
 
 if(head_ == 0) 
 {
   	head_ = tail_ = p;
 }
 else 
 {
   	tail_->next_ = p;
   	tail_ = p;
 }
 len_++;
 
#ifdef QDEBUG
   verifyQueue();
#endif // QDEBUG
}
                

Packet* antsense_queue::return_head_queue() {
Packet *p;

 /*
  * Purge any packets that have timed out.
  */
 dropOnTimeOut();

 p = remove_head();
#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}


Packet* antsense_queue::take_a_package_to(nsaddr_t dst) {
Packet *p, *prev;

 /*
  * Purge any packets that have timed out.
  */
 dropOnTimeOut();

 findPacketWithDst(dst, p, prev);
 assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 if(p == 0) return 0;

 if (p == head_) {
   p = remove_head();
 }
 else if (p == tail_) {
   prev->next_ = 0;
   tail_ = prev;
   len_--;
 }
 else {
   prev->next_ = p->next_;
   len_--;
 }

#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}

char antsense_queue::find(nsaddr_t dst) {
Packet *p, *prev;  
	
 findPacketWithDst(dst, p, prev);
 if (0 == p)
   return 0;
 else
   return 1;

}


/*
  Private Routines
*/

Packet* antsense_queue::remove_head() {
Packet *p = head_;
        
 if(head_ == tail_) {
   head_ = tail_ = 0;
 }
 else {
   head_ = head_->next_;
 }

 if(p) len_--;

 return p;

}

void antsense_queue::findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
	  //		if(HDR_IP(p)->dst() == dst) {
	       if(HDR_IP(p)->daddr() == dst) 
	       {
      			return;
    		}
    prev = p;
  }
}


void antsense_queue::verifyQueue() {
Packet *p, *prev = 0;
int cnt = 0;

 for(p = head_; p; p = p->next_) {
   cnt++;
   prev = p;
 }
 assert(cnt == len_);
 assert(prev == tail_);

}

bool antsense_queue::findAgedPacket(Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
    if(HDR_CMN(p)->ts_ < CURRENT_TIME) {
      return true;
    }
    prev = p;
  }
  return false;
}

void antsense_queue::dropOnTimeOut() {
Packet *p, *prev;

 while ( findAgedPacket(p, prev) ) 
 {
 	assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 	if(p == 0) return;

 	if (p == head_) 
	{
   		p = remove_head();
 	}
 	else if (p == tail_) 
	{
   		prev->next_ = 0;
   		tail_ = prev;
   		len_--;
 	}
 	else 
	{
   		prev->next_ = p->next_;
   		len_--;
 	}
	//TANDRE 26/12/2005
	//struct hdr_cmn *ch = HDR_CMN(p);
	if (DEBUGS) printf("ESTAMOS A DEITAR FORA UM PACOTE, MOTIVO: Queue TIMEOUT_(dropOnTimeOut)\n");
	drop(p, DROP_RTR_QTIMEOUT);
#ifdef QDEBUG
 	verifyQueue();
#endif // QDEBUG

	p = prev = 0;
 }
}
