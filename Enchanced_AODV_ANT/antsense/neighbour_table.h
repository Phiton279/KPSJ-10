
#ifndef cmu_rtable_h_
#define cmu_rtable_h_

#include "config.h"
#include "scheduler.h"
#include "queue.h"
#include "antsense_queue.h"

#define BIG   250


#ifndef uint
typedef unsigned int uint;
#endif // !uint



class neighbour {
public:
  	neighbour() { 
  		bzero(this, sizeof(neighbour));//apenas serve para termos a certeza k esta a pagado
  		pheromone=0.0;
  		energy=0.0;
  		last_message_id = 0;
  	}	
 	neighbour& operator = ( const neighbour &other);
	
	nsaddr_t     dst;     // destination
  	nsaddr_t     hop;     // next hop
  
  	uint         metric;  // distance
  
  	int 		last_message_id; // last sequence number we saw
  
  	//TANDRE 26/5/2005
  	nsaddr_t       address; // endereço do vizinho 
  	double         energy; // ultima energia conhecida do vizinho
  	double         pheromone; // valor da feromona
 
 	//TANDRE:16/7/2005
  	double	last_update_time;
 
  	double       advertise_ok_at; // when is it okay to advertise this rt?
  	bool         advert_seqnum;  // should we advert rte b/c of new seqnum?
  	bool         advert_metric;  // should we advert rte b/c of new metric?

  	Event        *trigger_event;

  	uint          last_advertised_metric; // metric carried in our last advert
  	double       changed_at; // when route last changed
  	double       new_seqnum_at;	// when we last heard a new seq number
  	double       wst;     // running wst info
  	//Event       *timeout_event; // event used to schedule timeout action
  	//PacketQueue *q;		//pkts queued for dst
};

class RoutingTable {
  public:
    
	RoutingTable();
    	~RoutingTable();
    
    	void AddEntry(const neighbour &ent);
    	void InitLoop();
    	neighbour *NextLoop();
    
    	//TANDRE:5/06/2005
    	neighbour *GetNextHop(nsaddr_t dest, int last_message_id, int prev_hop);
    
    	//TANDRE:12/6/2005
    	void UpdateEnergy(nsaddr_t element, double energy);
    	void ListTable(nsaddr_t myadd, nsaddr_t search);
	void ListPheromons(nsaddr_t myadd);
    	bool FindEntry(nsaddr_t element);
    	neighbour * GetNextNeighbourToSink(Packet *p);
    	bool AmIinLoop(Packet *p,nsaddr_t me);
    
    	//TANDRE:16/7/2005
    	void UpdatePheromoneToSink(nsaddr_t element,double evaporation);
    	void UpdatePheromoneFromSink(nsaddr_t element, double evaporation, double pheromone);
    	void UpdateClockNB(nsaddr_t element, double Time);
    
    	void CheckLifeNeighbors(double CurrentTime, double AntSense_BROADCAST_JITTER, double myaddress);
    
    	neighbour * UpdateFromLostLink(nsaddr_t lostneighbour, nsaddr_t destinationIP, int last_message_id, int prev_hop);
    
     	int         elts; //TODO:para mudar para private!!!!
     
     	int DEBUGS; //utilizado em debug
     //antsense_queue *Antqueue;	//queue for antsense
     
     	double PHEROMONE_WEIGHT;
     	double ENERGY_WEIGHT;
     	int SINK_NODE_ID;
  	int         maxelts;
  private:
    	neighbour *ngbtable;
    	neighbour *ngbtableNotVisited;
	neighbour *tmp;
    	
   
    	int         ctr;
    	//TODO: Mudar isto para algo mais bonito....tirar o excesso
    
    	void ConsumeNeighborsFromANearSinkNode(nsaddr_t dest);
    	bool DeleteEntry(nsaddr_t lostneighbour);
};
    
#endif
