#ifndef __antsense_pkt_h__
#define __antsense_pkt_h__
  
#include <packet.h>
  
#define HDR_ANTSENSE_PKT(p) 		((struct hdr_antsense_pkt*)hdr_antsense_pkt::access(p))
#define HDR_ANTSENSE_ANT_PKT(p)		((struct hdr_antsense_ant_pkt*)hdr_antsense_pkt::access(p))
#define HDR_ANTSENSE_HELLO_PKT(p)	((struct hdr_antsense_hello_pkt*)hdr_antsense_pkt::access(p))

#define AntSenseTYPE_ANT  	0x01
#define AntSenseTYPE_HELLO  	0x02





struct hdr_antsense_pkt {
      
     u_int8_t        ah_type;
 
     static int offset_;
     inline static int& offset() { return offset_; }
     inline static hdr_antsense_pkt* access(const Packet* p) {
         return (hdr_antsense_pkt*)p->access(offset_);
     }
 
	
 };

 struct hdr_antsense_ant_pkt
 {
 	u_int8_t        hp_type; //PARA RETIRAR??
	
 	nsaddr_t   pkt_src_;     // Node which originated this packet     
     	nsaddr_t   antpath[30];
     	//TODO colocar este array a funcionar
     	double   energypath[30];
     	double pheromone;
	int currenthop;
	bool toSinkNode;
 
	inline int size() { 
  		int sz = 0;
   		sz = sizeof(u_int8_t)		// rp_type_
	     		+ 31*sizeof(nsaddr_t) 	// rp_src_
			+ sizeof(int)
			+ sizeof(bool)
	     		+ 31*sizeof(double);		// node_energy_
			
		return sz;
	}
 };
 
 struct hdr_antsense_hello_pkt {
        
	u_int8_t        hp_type; //PARA RETIRAR??
        nsaddr_t        rp_src;                 // Source IP Address
        double	        node_energy;            // Lifetime
	//double 		pheromone;
	     
	inline int size() { 
  		int sz = 0;
  
  		sz = sizeof(u_int8_t)		// rp_type_
	     		+ sizeof(nsaddr_t) 	// rp_src_
	     		+ sizeof(double);		// node_energy_
		return sz;
	}
};
#endif

