#ifndef cmu_antsense_h_
#define cmu_antsense_h_

#include "config.h"
#include "agent.h"
#include "ip.h"
#include "delay.h"
#include "scheduler.h"
#include "queue.h"
#include "trace.h" 
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "priqueue.h"

#include "neighbour_table.h"

#if defined(WIN32) && !defined(snprintf)
#define snprintf _snprintf
#endif /* WIN32 && !snprintf */

typedef double Time;

//#define MAX_QUEUE_LENGTH 5
#define ROUTER_PORT      0xff
 
class hdr_AntSense;
 
class AntSense_Helper;
class AntSenseTriggerHandler;

class AntSense : public Agent 
{
  friend class AntSense_Helper;
  friend class AntSenseTriggerHandler;
  friend class hdr_AntSense;

public:
  AntSense();
  ~AntSense ();
  virtual int command(int argc, const char * const *);
  void lost_link(Packet *p);
  
    //TANDRE: 9/12/2005 .. inputs from tcl...
  	
	static double STARTUP_PHEROMONE;	
	//segundos ate o inicio do envio dos broadcasts
	static double STARTUP_JITTER_BROADCAST;
	//segundos ate o inicio do envio das formigas
	static double STARTUP_JITTER_ANTS;	
	//intervalo de tempo maximo para envio de formigas
	static double SEND_ANTS_JITTER; 
	//intervalo de tempo maximo para envio de broadcasts
	static double SEND_BROADCAST_JITTER;
	//especifica o sinknode
	static int SINK_NODE_ID;
	//Utilizada na obtenção do peso da feromona
	static double BIG_CONSTANT_C;
	//Evaporação de feromonas...
	static double PHEROMONE_EVAPORATION;
	//utilizamos filas de espera???
	static int USE_QUEUE;
	//Maximos elementos permitidos na fila de espera
	static int ANTSENSE_RTQ_MAX_LEN;
	//Maximo tempo permitido na fila de espera
	static double ANTSENSE_RTQ_TIMEOUT;
	//Peso da pheromone na formula (ALFA)
	static double PHEROMONE_WEIGHT;
	//Peso da energia na formula (BETA)
	static double ENERGY_WEIGHT;
	
	static int DEBUGS;

	static double SHOW_PHEROMONS;
	static double SHOW_ENERGY;
	
	
  
protected:
  void helper_callback(Event *e);
  Packet* rtable(int);
  virtual void recv(Packet *, Handler *);
  void trace(char* fmt, ...);
  void tracepkt(Packet *, double, int, const char *);
//   void needTriggeredUpdate(rtable_ent *prte, Time t);
  // if no triggered update already pending for route prte, make one so
  //void cancelTriggersBefore(Time t);
  // Cancel any triggered events scheduled to take place *before* time
  // t (exclusive)
  void makeBroadcast();
  void updateRoute(neighbour *old_rte, neighbour *new_rte);
  //void processUpdate (Packet * p);
  void forwardPacket (Packet * p);
  void startUp();
  int diff_subnet(int dst);
    
  void recvAntSensePkt(Packet *p);
  void recvAnt(Packet *p);
  void recvHello(Packet *p);
  
  //TANDRE:5/6/2005
  void SendANT ();
  
  
  double node_energy();
  double path_energy(Packet *p);
  
  // update old_rte in routing table to to new_rte
  Trace *tracetarget;       // Trace Target
  AntSense_Helper  *helper_;    // DSDV Helper, handles callbacks
 // DSDVTriggerHandler *trigger_handler;
  RoutingTable *table_;     // Routing Table
  
  
  int myaddr_;              // My address...

  
  // Extensions for mixed type simulations using wired and wireless
  // nodes
  char *subnet_;            // My subnet
  MobileNode *node_;        // My node
  // for debugging
  char *address;
  NsObject *port_dmux_;    // my port dmux

  Event *periodic_callback_;           // notify for periodic update
  
  Event *periodic_ckecksNB_;
  //TANDRE: 26/5/2005
  //evento que envia as formigas para o SR
  Event *periodic_ants_;
  
  
  Event *sendPheromons;
  Event *sendEnergy;
  
  // Randomness/MAC/logging parameters
  int be_random_;
  
  // APAGADOS
 /* int use_mac_;
  int verbose_;
  int trace_wst_;
  double alpha_;  // 0.875
  PriQueue *ll_queue;       // link level output queue
  int seqno_;  */             // Sequence number to advertise with...
  //double lasttup_;		// time of last triggered update
  //double next_tup;		// time of next triggered update
  //double wst0_;   // 6 (secs)
  double perup_;  // 15 (secs)  period between updates
  int    min_update_periods_;    // 3 we must hear an update from a neighbor
  // every min_update_periods or we declare
  // them unreachable
  
  void output_rte(const char *prefix, neighbour *prte, AntSense *a);
  
   /*
  * A pointer to the network interface queue that sits
  * between the "classifier" and the "link layer".
  */
    
  PriQueue *ifqueue;
  antsense_queue Antqueue;
};

class AntSense_Helper : public Handler {
  public:
    AntSense_Helper(AntSense *a_) { a = a_; }
    virtual void handle(Event *e) { a->helper_callback(e); }

  private:
    AntSense *a;
};

#endif
