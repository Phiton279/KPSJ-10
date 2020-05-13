#ifndef __antsense_queue_h_
#define __antsense_queue_h_

#include <ip.h>
#include <agent.h>

class antsense_queue : public Connector {
 public:
        antsense_queue();
	
        void            recv(Packet *, Handler*) { abort(); }

        void            increase_queue(Packet *p);

	inline int      command(int argc, const char * const* argv) 
	  { return Connector::command(argc, argv); }

        /*
         *  Returns a packet from the head of the queue.
         */
        Packet*         return_head_queue(void);

        /*
         * Returns a packet for destination "D".
         */
        Packet*         take_a_package_to(nsaddr_t dst);
  /*
   * Finds whether a packet with destination dst exists in the queue
   */
        char            find(nsaddr_t dst);

   //The maximum number of packets that we allow a routing protocol to buffer.
	int ANTSENSE_RTQ_MAX_LEN;	//packets
   //The maximum period of time that a routing protocol is allowed to buffer a packet for.
        double ANTSENSE_RTQ_TIMEOUT;	// seconds
	
	int DEBUGS;
 private:
        Packet*         remove_head();
        void            dropOnTimeOut(void);
	void		findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev);
	bool 		findAgedPacket(Packet*& p, Packet*& prev); 
	void		verifyQueue(void);

        Packet          *head_;
        Packet          *tail_;

        int             len_;

        int             limit_;
        double          timeout_;

	
};

#endif 
