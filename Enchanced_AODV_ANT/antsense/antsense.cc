extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "antsense.h"
#include "priqueue.h"

#include <random.h>

#include <cmu-trace.h>
#include <address.h>
#include <mobilenode.h>
#include <energy-model.h>

#include "antsense_pkt.h"

#include <string>
#include <sstream>
#include <iostream>

#define IP_DEF_TTL   32 // default TTL
#undef TRIGGER_UPDATE_ON_FRESH_SEQNUM

//TANDRE: Visto
static inline double jitter (double max, int be_random_){
// Retorna um numero aleatorio entre 0 e max
  return (be_random_ ? Random::uniform(max) : 0);
}


void AntSense::trace (char *fmt,...){
  va_list ap;

  if (!tracetarget)
    return;

  va_start (ap, fmt);
  vsprintf (tracetarget->pt_->buffer (), fmt, ap);
  tracetarget->pt_->dump ();
  va_end (ap);
}

void AntSense::tracepkt (Packet * p, double now, int me, const char *type){
  char buf[1024];

  unsigned char *walk = p->accessdata ();

  int ct = *(walk++);
  int seq, dst, met;

  snprintf (buf, 1024, "V%s %.5f _%d_ [%d]:", type, now, me, ct);
  while (ct--)
    {
      dst = *(walk++);
      dst = dst << 8 | *(walk++);
      dst = dst << 8 | *(walk++);
      dst = dst << 8 | *(walk++);
      met = *(walk++);
      seq = *(walk++);
      seq = seq << 8 | *(walk++);
      seq = seq << 8 | *(walk++);
      seq = seq << 8 | *(walk++);
      snprintf (buf, 1024, "%s (%d,%d,%d)", buf, dst, met, seq);
    }
}

// Prints out an rtable element.
//Tandre:visto
//apenas serve para debug, vou utilizar no futuro
void AntSense::output_rte(const char *prefix, neighbour * prte, AntSense * a){
  a->trace("DFU: deimplemented");
  printf("DFU: deimplemented");

  prte = 0;
  prefix = 0;
}

class AntSenseTriggerHandler : public Handler {
  public:
    AntSenseTriggerHandler(AntSense *a_) { a = a_; }
    virtual void handle(Event *e);
  private:
    AntSense *a;
};


void AntSense::helper_callback (Event * e)
{
  //Scheduler & s = Scheduler::instance ();
  //double now = s.clock ();
  neighbour *prte;
  //rtable_ent *pr2;
  //int update_type;	 // we want periodic (=1) or triggered (=0) update?
  
  // Check for broadcast
  	if (periodic_callback_ && e == periodic_callback_)
    	{
	        if (DEBUGS) printf("A Enviar um broadcast.... do node %d\n",myaddr_);
	 
		Scheduler::instance ().schedule (helper_, periodic_callback_ , jitter (SEND_BROADCAST_JITTER, be_random_));
      		makeBroadcast();
		table_->CheckLifeNeighbors(Scheduler::instance().clock(), SEND_BROADCAST_JITTER, myaddr_);
      		//lasttup_ = now;
      		return;
    	}

	if (periodic_ants_ && e == periodic_ants_)
    	{//é o makeupdate k envia os updates de formigas
		double sss=jitter (SEND_ANTS_JITTER, be_random_);
		if (DEBUGS) printf( "----At %.2f: Em (%d), foi xamado um evento para lançar uma nova formiga em:%.2f\n", Scheduler::instance().clock(),myaddr_, sss);
      		//escolhe ja kando sera o novo envio das formigas
      		Scheduler::instance ().schedule (helper_, periodic_ants_, sss);
      		SendANT();
      		return;
    	}
	
	
	if (periodic_callback_ && e == sendPheromons)
	{
//		printf("\n\n-----------------\n.....Pheromone Table @ %.2f: .....\n-----------------\n", Scheduler::instance().clock());
		table_->ListPheromons(myaddr_);

		delete sendPheromons;
  	}
  
	if(periodic_callback_ && e == sendEnergy)
	{
  		printf("\n\n-----------------\n.....Energy Table @ %.2f: .....\n-----------------\n", Scheduler::instance().clock());
		fprintf(stderr,"Energy of node: %d->>>%f\n", myaddr_, node_energy());
		delete sendEnergy ;	
	}
}

static void mac_callback (Packet * p, void *arg){
  ((AntSense *) arg)->lost_link (p);
}

void AntSense::lost_link (Packet *p){
  hdr_cmn *hdrc = HDR_CMN (p);
  hdr_ip *iph = HDR_IP(p);
  
  int from = iph->saddr();//cant find the previous.. i will say that it was the source
  int iam = myaddr_;
  int to = hdrc->next_hop_;
  nsaddr_t dst = iph->daddr();
  
  int seqnum = hdrc->uid_;
  neighbour *next_neighbour = table_->UpdateFromLostLink(hdrc->next_hop_, dst, seqnum, from);

  
  if (!next_neighbour || (next_neighbour->metric == BIG))
    {
	if(USE_QUEUE==0)
	{
			drop(p,DROP_RTR_NO_ROUTE);
	}
	else
	{
		if (DEBUGS) printf( "----At %.2f: TEMOS UM PACOTE EM FILA DE ESPERA!!!!!!!!!!!!\n", Scheduler::instance().clock());
		Antqueue.increase_queue(p);
		return;
	}
   }
   else
   {
     	hdrc->xmit_failure_ = mac_callback;
	hdrc->xmit_failure_data_ = (void*)this;
		
	hdrc->addr_type_ = NS_AF_INET;
	hdrc->direction() = hdr_cmn::DOWN;
	hdrc->next_hop_ = next_neighbour->address;
   }
//TODO:talvez colocar aqui uma queue a funcionar??
  Scheduler::instance().schedule(target_,p, 0.0);
  
}



//TANDRE:visto
void AntSense::forwardPacket (Packet * p){
//o pacote xegou ate nos e temos de o reencaminhar...

 	hdr_ip *iph = HDR_IP(p);
  	hdr_cmn *hdrc = HDR_CMN (p);
	//Scheduler & s = Scheduler::instance ();
  	//double now = s.clock ();
	struct hdr_ip* ih	=HDR_IP(p);
  	
  	int dst = ih->daddr();
  	neighbour *prte;
	int seqnum = hdrc->uid_;
	int prev_hop = hdrc->prev_hop_;

	if (DEBUGS) printf( "----At %f: nó (%d) is going to forwardig a normal pkt\n", Scheduler::instance().clock(),myaddr_);

   	prte = table_->GetNextHop(dst, seqnum, prev_hop);//Não estamos por agora a ver o destination... são todos para o sink-node
  
	//Verificar se tem realmente algum para enviar....
	if (prte && (prte->metric != BIG))
	{
            //já existe uma rota para determinado destino vou enviar todos os pacotes k tenho para um destino
	    if(USE_QUEUE==1)
	    {
		if(Antqueue.find(dst))
		{
			if (DEBUGS) printf("Encontramos um link que estava quebrado mas agora está em condições\n");
			Packet *buffered_pkt;
			neighbour *next_neighbour;

			double delay = 0.0;
			
			while (buffered_pkt = Antqueue.take_a_package_to(dst))
			{					
				hdr_ip *iph_buff = HDR_IP(buffered_pkt);
  				hdr_cmn *hdrc_buff = HDR_CMN (buffered_pkt);
				
				int idest = iph_buff->daddr();
				int iprev_hop = hdrc_buff->prev_hop_;
				int iseqnum = hdrc_buff->uid_;
				
				delay+=0.01;
				next_neighbour = table_->GetNextHop(dst, iseqnum, iprev_hop);
				if (next_neighbour && (next_neighbour->metric != BIG))
				{
					hdrc_buff->addr_type_ = NS_AF_INET;
					hdrc_buff->xmit_failure_ = mac_callback;
  					hdrc_buff->xmit_failure_data_ =  (void*)this;
					
					hdrc_buff->addr_type_ = NS_AF_INET;
					hdrc_buff->direction() = hdr_cmn::DOWN;
					hdrc_buff->next_hop_ = next_neighbour->address;
					hdrc_buff->prev_hop_ = myaddr_;
					assert (!HDR_CMN (p)->xmit_failure_ || HDR_CMN (p)->xmit_failure_ == mac_callback);
					Scheduler::instance().schedule(target_,buffered_pkt, delay);
					if (DEBUGS) printf("Sending Packet (%i)... from the queue\n", hdrc_buff->uid_);
				}
			}
			next_neighbour=NULL;
    		}
	    }
		
  		//set direction of pkt to -1 , i.e downward
  		hdrc->xmit_failure_ = mac_callback;
		hdrc->xmit_failure_data_ = (void*)this;
		
		hdrc->addr_type_ = NS_AF_INET;
		hdrc->direction() = hdr_cmn::DOWN;
		hdrc->next_hop_ = prte->address;
		hdrc->prev_hop_ = myaddr_;
	
  		assert (!HDR_CMN (p)->xmit_failure_ ||
  			HDR_CMN (p)->xmit_failure_ == mac_callback);
  		Scheduler::instance().schedule(target_,p, 0.0);
		if (DEBUGS) printf("Sending Packet...\n");
		prte=NULL;
		return;
  	 }
	else //não existe ninguem para mandar o pacote.....
	{//TODO:ver se aplico lista....
		//TANDRE @ 26/12/2005
		//coloca em fila de espera
		//e faz o drop se necessario.... devido a tamanho limite ou tempo maximo
		if(USE_QUEUE==0)
			drop(p,DROP_RTR_NO_ROUTE);
		else
		{
			if (DEBUGS) printf("----At %.2f: \nTEMOS UM PACOTE EM FILA DE ESPERA!!!!!!!!!!!!\n", Scheduler::instance().clock());
			Antqueue.increase_queue(p);
      			return;
		}
    	}
  
}

//TANDRE:na parte do broadcast deve ser necessario alterações nomeadamente parar o broadcast apenas necessitamos de broacast com 1 TTL, apenas para os vizinhos, para saberem kem esta por perto
//Funcção xamada kando o nó recebe um pacote, poderá vir do proprio nó 
void AntSense::recv (Packet * p, Handler *){

	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_ip *iph = HDR_IP(p);

	if (DEBUGS) printf("eu no %d recebi um pacote (%i) de %d, atraves de %d, com num_forwards = %d\n", myaddr_,cmh->uid_,iph->saddr(), cmh->prev_hop_, cmh->num_forwards());

	int src = cmh->prev_hop_;
  	int dst = iph->daddr();
	int tes = cmh->next_hop_;

  
  	if(cmh->ptype() == PT_ANTSENSE) 
	{
		if (DEBUGS) printf( "----At %.2f: É do tipo PT_ANTSENSE\n", Scheduler::instance().clock());
   		iph->ttl_ -= 1;
   		recvAntSensePkt(p);
   		return;
 	}
		
  	//o pacote fui eu que originei...
	if(src == myaddr_ && cmh->num_forwards() == 0 ) 
	{//caso o pacote tenha sido enviado pela camada superior
		if (DEBUGS) printf( "----At %.2f: recebi um pacote:o pacote fui eu que originei...\n", Scheduler::instance().clock());
  		//adiciona o nosso IP
    		cmh->size() += IP_HDR_LEN;    
    		iph->ttl_ = IP_DEF_TTL;
  	}
    	else if(src == myaddr_) //o pacote é um k eu já enviei.. talvez um loop
	{
		if (DEBUGS) printf( "----At %.2f: recebi um pacote:pacote é um k eu já enviei.. talvez um loop\n", Scheduler::instance().clock());
		drop(p, DROP_RTR_ROUTE_LOOP);
    		return;
  	}
  	else //pacote para reencaminhar
	{
    		//verificar o TTL caso tenha expirado apagar o pacote
    		if(iph->ttl_-- == 0) 
		{
			if (DEBUGS) printf( "----At %.2f: recebi um pacote:TTL caso tenha expirado apagar o pacote\n", Scheduler::instance().clock());
       			drop(p, DROP_RTR_TTL);
      			return;
    		}
  	}
	
  	if ((src != myaddr_) && (iph->dport() == ROUTER_PORT))//o pacote nao fui eu k o criei.. o k vou fazer com ele?
    	{//o pacote é para mim e é normal?
    		if (DEBUGS) printf("recebi um pacote: o pacote é para mim e é normal\n");
		 Packet::free (p);
    	}
  	else 
    	{//caso o pacote nao é para mim tenho de o reencaminhar
		if (DEBUGS) printf( "----At %.2f: caso o pacote nao seja para mim tenho de o reencaminhar\n", Scheduler::instance().clock());
    	    	forwardPacket(p); //vamos encaminhar o pacote
    	}
}


static class AntSenseClass:public TclClass
{
  public:
  AntSenseClass ():TclClass ("Agent/AntSense")
  {
  }
  TclObject *create (int, const char *const *argv)
  {
//     return (new AntSense (((nsaddr_t) Address::instance().str2addr(argv[4]))));
    return (new AntSense ());
  }
} class_AntSense;


AntSense::AntSense (): Agent (PT_ANTSENSE), port_dmux_(0),
  myaddr_ (0), node_ (0),
  periodic_callback_ (0),
  perup_ (15), Antqueue(),
  min_update_periods_ (3) 
 {
  table_ = new RoutingTable ();
  helper_ = new AntSense_Helper (this);
  
  table_->DEBUGS = DEBUGS;
  table_->PHEROMONE_WEIGHT=PHEROMONE_WEIGHT;
  table_->ENERGY_WEIGHT=ENERGY_WEIGHT;
  table_->SINK_NODE_ID =SINK_NODE_ID;
  table_->maxelts=10;
  
  if(DEBUGS)
  {
  	printf("testing o k estou a mandar...%i --DEBUG\n",table_->DEBUGS);
  	printf("testing o k estou a mandar...%f --Phero\n",PHEROMONE_WEIGHT);
  	printf("testing o k estou a mandar...%f --ener\n",ENERGY_WEIGHT);
  }
 	
  Antqueue.ANTSENSE_RTQ_MAX_LEN 	= ANTSENSE_RTQ_MAX_LEN;
  Antqueue.ANTSENSE_RTQ_TIMEOUT 	= ANTSENSE_RTQ_TIMEOUT;
  Antqueue.DEBUGS 	      		= DEBUGS;
  
  //faz com que as mensagens sejam enviadas em ramdom (=1)
   be_random_ = 1;
}

AntSense::~AntSense (){
delete table_;
delete helper_;
//delete Antqueue;


delete periodic_callback_;
  
  delete periodic_ants_;
}

//TANDRE:
void AntSense::startUp()
{
//é aki que tudo começa depois do construtor
//é criado a tabela de vizinhos apenas com um nó, possivelmento o proprio
 Time now = Scheduler::instance().clock(); //regista o tempo de inicio da simulaçao

  subnet_ = Address::instance().get_subnetaddr(myaddr_);//apanha a rede do proprio
  //DEBUG
  address = Address::instance().print_nodeaddr(myaddr_);//apanha o endereço do proprio
  
  if (DEBUGS) printf("myaddress: %d -> %s\n",myaddr_,address);

  //TANDRE:5/6/2005
  //TANDRE:11/6/2005
  	Node* thisnode = MobileNode::get_node_by_address(myaddr_);
 
	double node_energy = 0.0;
	if (thisnode) {
	    if (thisnode->energy_model()) {
		    node_energy = thisnode->energy_model()->energy();
	    }
	}
 
  neighbour rte; //cria uma nova entrada
  bzero(&rte, sizeof(rte)); //apaga essa mesma entrada

  rte.dst = myaddr_;
  rte.hop = myaddr_;
  rte.metric = BIG;


  //TANDRE: 26/5/2005
  if (DEBUGS) printf("Estamos a ininiar o nó...%d", myaddr_);
  
  rte.address= myaddr_; // vamos no inicio apantar para o mesmo 
  rte.energy= node_energy; 
  rte.pheromone=  STARTUP_PHEROMONE;
  rte.advertise_ok_at = 0.0; // can always advert ourselves
  rte.advert_seqnum = true;
  rte.advert_metric = true;
  rte.changed_at = now;
  rte.new_seqnum_at = now;
  rte.wst = 0;
  
  table_->AddEntry (rte);
  
  // kick off periodic advertisments
  periodic_callback_ = new Event ();//este evento servirá para enviar brodcasts aos vizinhos com a energia do proprio

  
  periodic_ants_ = new Event ();    //este evento servirá para enviar as formigas para o sink node
  
  
  //kando será o primeiro broadcast enviado
  Scheduler::instance ().schedule (helper_, periodic_callback_, jitter (STARTUP_JITTER_BROADCAST, be_random_)); 
  //
  
  if (SINK_NODE_ID!=myaddr_) //o sink node nao envia formigas
  {
  	if (DEBUGS) printf("TESTE??? o no %d é = a no %d",myaddr_,SINK_NODE_ID);
  	double sss = jitter (STARTUP_JITTER_ANTS, be_random_);
  	Scheduler::instance ().schedule (helper_, periodic_ants_, sss); //kando será a primeira ant broadcast
	if (DEBUGS) printf("o no %d vai enviar uma formiga ás:%.2f: \n", myaddr_, sss);
  }
  
  
  if (SHOW_PHEROMONS != 0.0)
  {
  	sendPheromons =new Event ();
  	Scheduler::instance ().schedule (helper_, sendPheromons, SHOW_PHEROMONS);
 }  
  
  if (SHOW_ENERGY != 0.0)
  {
  	sendEnergy =new Event ();
  	Scheduler::instance ().schedule (helper_, sendEnergy,   SHOW_ENERGY);
  }

}

int AntSense::command (int argc, const char *const *argv){
  if (argc == 2)
    {
      if (strcmp (argv[1], "start-AntSense") == 0)
	{
	  startUp();
	  return (TCL_OK);
	}
      else if (strcmp (argv[1], "dumprtab") == 0)
	{
	  Packet *p2 = allocpkt ();
	  hdr_ip *iph2 = HDR_IP(p2);
	  neighbour *prte;

	  printf ("Table Dump %d[%d]\n----------------------------------\n",iph2->saddr(), iph2->sport());
	trace ("VTD %.5f %d:%d\n", Scheduler::instance ().clock (),
		 iph2->saddr(), iph2->sport());

	  /*
	   * Freeing a routing layer packet --> don't need to
	   * call drop here.
	   */
	Packet::free (p2);

	  for (table_->InitLoop (); (prte = table_->NextLoop ());)
	    output_rte ("\t", prte, this);
	  
	  	printf ("\n");

	  return (TCL_OK);
	}
    }
  else if (argc == 3)
    {
      if (strcasecmp (argv[1], "addr") == 0) 
      {
	 int temp;
	 temp = Address::instance().str2addr(argv[2]);
	 myaddr_ = temp;
	 return TCL_OK;
      }
      TclObject *obj;
      if ((obj = TclObject::lookup (argv[2])) == 0)
	{
		if (DEBUGS) printf ( "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
	  	return TCL_ERROR;
	}
      if (strcasecmp (argv[1], "tracetarget") == 0)
	{
	  
	  tracetarget = (Trace *) obj;
	  return TCL_OK;
	}
      else if (strcmp(argv[1], "if-queue") == 0)
        {
            ifqueue = (PriQueue *) TclObject::lookup(argv[2]);

            if (ifqueue == 0)
                return TCL_ERROR;
            return TCL_OK;
        }
      else if (strcasecmp (argv[1], "node") == 0) {
	      node_ = (MobileNode*) obj;
	      return TCL_OK;
      }
      else if (strcasecmp (argv[1], "port-dmux") == 0) {
	      port_dmux_ = (NsObject *) obj;
	      return TCL_OK;
      }
      else if(strcmp(argv[1], "drop-target") == 0) {
    		int stat = Antqueue.command(argc,argv);
      		if (stat != TCL_OK) return stat;
      		return Agent::command(argc, argv);
      }
   }
  return (Agent::command (argc, argv));
}

//TANDRE:5/6/2005
// envia um pacote formiga
void AntSense::SendANT (){
	if (DEBUGS) printf( "----At %.2f: o no %d envia uma formiga\n", Scheduler::instance().clock(), myaddr_);
	
	Packet *p = allocpkt ();
	struct hdr_cmn* ch	=HDR_CMN(p);
	struct hdr_ip* ih	=HDR_IP(p);
	struct hdr_antsense_ant_pkt* ant  = HDR_ANTSENSE_ANT_PKT(p);

	ant->currenthop				=1;
	if (DEBUGS) printf( "----At %.2f: envio de um novo pacote formiga e ja saltou ------------%d vezes\n", Scheduler::instance().clock(),ant->currenthop);
// 	verificar se podemos enviar uma formiga, isto e se existe vizinhos na tabela
		
	neighbour *prte = table_->GetNextNeighbourToSink(p);
	
	if (DEBUGS) printf( "----At %.2f: Eu sou o nó :%d e vou enviar o pacote para %d\n", Scheduler::instance().clock(), myaddr_,prte->address);
	
	if (prte->address == myaddr_)
	{
		drop(p, "DROP_RTR_NO_ROUTE");	
		if (DEBUGS) printf( "----At %.2f: pacote uma formiga dropped: nenhum vizinho (envio)\n", Scheduler::instance().clock());
		return;
	}
	
	ant->pkt_src_ 				=myaddr_;
	ant->hp_type				=AntSenseTYPE_ANT;
     	ant->antpath[ant->currenthop] 		= myaddr_;
	ant->energypath[ant->currenthop++] 	= node_energy();
	ant->pheromone				=0;
	ant->toSinkNode				=true;
	
	ch->ptype() = PT_ANTSENSE;
 	ch->size() = IP_HDR_LEN + ant->size();
 	ch->iface() = -2;
 	ch->error() = 0;
 	ch->addr_type() = NS_AF_NONE;
 	ch->prev_hop_ = myaddr_;          
 	
	ih->saddr() = myaddr_;
 	ih->daddr() = prte->address;
 	ih->sport() = RT_PORT;
 	ih->dport() = RT_PORT;
 	ih->ttl_ = IP_DEF_TTL;
	 
	table_->UpdatePheromoneToSink(prte->address, PHEROMONE_EVAPORATION);
	 
	Scheduler::instance().schedule(target_,p,0.0);
}

// //TANDRE:11/6/2005
void AntSense::makeBroadcast(){
	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antsense_hello_pkt *hello = HDR_ANTSENSE_HELLO_PKT(p);

	if (DEBUGS) printf( "----At %.2f: sending Hello from %d \n", Scheduler::instance().clock(), myaddr_);
		
 	hello->hp_type = AntSenseTYPE_HELLO;
	hello->node_energy = node_energy();
 	hello->rp_src = myaddr_;
	
 	ch->ptype() = PT_ANTSENSE;
 	ch->size() = IP_HDR_LEN + hello->size();
 	ch->iface() = -2;
 	ch->error() = 0;
 	ch->addr_type() = NS_AF_NONE;
 	ch->prev_hop_ = myaddr_;          

 	ih->saddr() = myaddr_;
 	ih->daddr() = IP_BROADCAST;
 	ih->sport() = RT_PORT;
 	ih->dport() = RT_PORT;
 	ih->ttl_ = IP_DEF_TTL;

 	Scheduler::instance().schedule(target_, p, 0.0);
 }

//TANDRE:5/6/2005
//ligação com o tipo de pacote antSense 
int hdr_antsense_pkt::offset_;
static class AntSenseHeaderClass : public PacketHeaderClass {
public:
    AntSenseHeaderClass() : PacketHeaderClass("PacketHeader/AntSense",
                                               sizeof(hdr_antsense_pkt)) {
        bind_offset(&hdr_antsense_pkt::offset_);
    }
} class_rtProtoAntsense_hdr;  

 //TANDRE: 9/12/2005 .. inputs from tcl...
//Valores a estudar!!!!!!!!!!!!!!
//pheromona colocada em cada nó quando este é descoberto
double AntSense::STARTUP_PHEROMONE 		= 100;	
//segundos ate o inicio do envio dos broadcasts
double AntSense::STARTUP_JITTER_BROADCAST 	= 0.1;
//segundos ate o inicio do envio das formigas
double AntSense::STARTUP_JITTER_ANTS 		= 10;	
//intervalo de tempo maximo para envio de formigas
double AntSense::SEND_ANTS_JITTER 		= 10; 
//intervalo de tempo maximo para envio de broadcasts
double AntSense::SEND_BROADCAST_JITTER 		= 5;
//especifica o sinknode
int AntSense::SINK_NODE_ID 			= 0;
//Utilizada na obtenção do peso da feromona
double AntSense::BIG_CONSTANT_C 			= 99999;
//Evaporação de feromonas...
double AntSense::PHEROMONE_EVAPORATION		= 0.2;
//Vamos utilizar uma fila de espera????
int AntSense::USE_QUEUE				=1;
//Qual o tamanho maximo da fila???
double AntSense::ANTSENSE_RTQ_TIMEOUT		=60; //segundos
//Qual o tempo maximo que guardamos os pacotes na fila??
int AntSense::ANTSENSE_RTQ_MAX_LEN		=5;  //pacotes
//Peso da pheromone na formula (ALFA)
double AntSense::PHEROMONE_WEIGHT			=1.5;
//Peso da energia na formula (BETA)
double AntSense::ENERGY_WEIGHT			=1.5;
//Vamos fazer debug???
int AntSense::DEBUGS				=0;


double AntSense::SHOW_PHEROMONS			=0;

double AntSense::SHOW_ENERGY			=0;

static     class     AntSenseclass:    public    TclClass
{
  public:
    AntSenseclass():
    TclClass("Agent/AntSense")
{}
    TclObject *create(int argc, const char *const *argv)
    {
        assert(argc == 5);
//         return (new AntSense((nsaddr_t) Address::instance().str2addr(argv[4])));
	return (new AntSense());
    }
    void bind()
    {
        TclClass::bind();
        add_method("dump");
	add_method("startup-pheromone");
	add_method("startup-jitter-broadcast");
	add_method("startup-jitter-ants");
	add_method("send-ants-jitter");
	add_method("send-broadcast-jitter");
	add_method("sink-node-id");
	add_method("big-constant-c");
	add_method("pheromone-evaporation");
	add_method("use-queue");
	add_method("queue-packet-timeout");
	add_method("queue-max-len");
	add_method("pheromone-weight");
	add_method("energy-weight");
	add_method("debugs");
	add_method("show-pheromons");
	add_method("show-energy");
    }
    int method(int ac, const char *const *av)
    {
        Tcl & tcl = Tcl::instance();
        int
            argc =
            ac -
            2;
        const char *const *
            argv =
            av +
            2;
        if (argc == 2)
        {
            if (strncasecmp(argv[1], "dump", 2) == 0)
            {
                stringstream
                    o;
		o << "Parameters used in simulation..."<< endl;
                o << "Agent/ANTSENSE startup-pheromone \"" << AntSense::STARTUP_PHEROMONE << "\"" << endl;
		o << "Agent/ANTSENSE startup-jitter-broadcast \"" << AntSense::STARTUP_JITTER_BROADCAST << "\"" << endl;
		o << "Agent/ANTSENSE startup-jitter-ants \"" << AntSense::STARTUP_JITTER_ANTS << "\"" << endl;
		o << "Agent/ANTSENSE send-ants-jitter \"" << AntSense::SEND_ANTS_JITTER << "\"" << endl;
		o << "Agent/ANTSENSE send-broadcast-jitter \"" << AntSense::SEND_BROADCAST_JITTER << "\"" << endl;
		o << "Agent/ANTSENSE sink-node-id \"" << AntSense::SINK_NODE_ID << "\"" << endl;
		o << "Agent/ANTSENSE big-constant-c \"" << AntSense::BIG_CONSTANT_C << "\"" << endl;
		o << "Agent/ANTSENSE pheromone-evaporation \"" << AntSense::PHEROMONE_EVAPORATION << "\"" << endl;
		o << "Agent/ANTSENSE pheromone-weight \"" << AntSense::PHEROMONE_WEIGHT << "\"" << endl;
		o << "Agent/ANTSENSE energy-weight \"" << AntSense::ENERGY_WEIGHT << "\"" << endl;
		if (AntSense::USE_QUEUE)
		{
			o << "Using queues with folowing parameters:" << endl;
			o << "Agent/ANTSENSE  queue-packet-timeout\"" << AntSense::ANTSENSE_RTQ_TIMEOUT << "\"" << endl;
			o << "Agent/ANTSENSE  queue-max-len\"" << AntSense::ANTSENSE_RTQ_MAX_LEN << "\"" << endl;
		}
		else
			o << "Not using queues "  << endl;

		if (AntSense::DEBUGS)
			o << "Debuging ON "  << endl;
		else
			o << "Debuging OFF "  << endl;
		tcl.resultf("%s", o.str().data());
                return TCL_OK;
            }
        }
        if (argc == 3)
        {
	 if (strcmp(argv[1], "startup-pheromone") == 0)
            {
		AntSense::STARTUP_PHEROMONE = atof(argv[2]);
                return (TCL_OK);
            }
        if (strcmp(argv[1], "startup-jitter-broadcast") == 0)
            {
		AntSense::STARTUP_JITTER_BROADCAST = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "startup-jitter-ants") == 0)
            {
		AntSense::STARTUP_JITTER_ANTS = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "send-ants-jitter") == 0)
            {
		AntSense::SEND_ANTS_JITTER = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "send-broadcast-jitter") == 0)
            {
		AntSense::SEND_BROADCAST_JITTER = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "sink-node-id") == 0)
            {
		AntSense::SINK_NODE_ID = int(atof(argv[2]));
                return (TCL_OK);
            }
	if (strcmp(argv[1], "big-constant-c") == 0)
            {
		AntSense::BIG_CONSTANT_C = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "pheromone-evaporation") == 0)
            {
		AntSense::PHEROMONE_EVAPORATION = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "use-queue") == 0)
            {
		AntSense::USE_QUEUE = int(atof(argv[2]));
                return (TCL_OK);
            }
	if (strcmp(argv[1], "queue-packet-timeout") == 0)
            {
		AntSense::ANTSENSE_RTQ_TIMEOUT = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "queue-max-len") == 0)
            {
		AntSense::ANTSENSE_RTQ_MAX_LEN = int(atof(argv[2]));
                return (TCL_OK);
            }
	if (strcmp(argv[1], "pheromone-weight") == 0)
            {
		AntSense::PHEROMONE_WEIGHT = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "energy-weight") == 0)
            {
		AntSense::ENERGY_WEIGHT = atof(argv[2]);
                return (TCL_OK);
            }
	if (strcmp(argv[1], "debugs") == 0)
            {
		AntSense::DEBUGS = int(atof(argv[2]));
                return (TCL_OK);
            }
	if (strcmp(argv[1], "show-pheromons") == 0)
            {
		AntSense::SHOW_PHEROMONS = int(atof(argv[2]));
                return (TCL_OK);
            }
	if (strcmp(argv[1], "show-energy") == 0)
            {
		AntSense::SHOW_ENERGY = int(atof(argv[2]));
                return (TCL_OK);
            }
	}
        return TclClass::method(ac, av);
    }
}aaa;


void AntSense::recvAntSensePkt(Packet *p) {
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antsense_pkt *ah = HDR_ANTSENSE_PKT(p);
	assert(ih->sport() == RT_PORT);
 	assert(ih->dport() == RT_PORT);
 
	if(ih->saddr() == myaddr_)//se o pacote foi enviado por mim?? 
	{
		if (DEBUGS) printf(": got my own REQUEST, Formiga, deleting packet\n");
		fprintf(stderr, "%s: got my own REQUEST, Formiga\n", __FUNCTION__);
    		drop(p,DROP_RTR_ROUTE_LOOP);
    		return;
	}
 /*
  * Incoming Packets.
  */
 	switch(ah->ah_type) 
	{
 		case AntSenseTYPE_ANT:
			if (DEBUGS) printf( "----At %.2f: É uma formiga\n", Scheduler::instance().clock());
   			recvAnt(p);
   			break;
 		case AntSenseTYPE_HELLO:
			if (DEBUGS) printf( "----At %.2f: É um Hello\n", Scheduler::instance().clock());
			recvHello(p);
   			break;
 		default:
			if (DEBUGS) printf( "----At %.2f: Invalid AntSense Package type (%x)\n", Scheduler::instance().clock(),ah->ah_type);
   			exit(1);
	 }
}

void AntSense::recvAnt(Packet *p) {
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_antsense_ant_pkt *ant = HDR_ANTSENSE_ANT_PKT(p);
	
	if(myaddr_ != ih->daddr())
	{
		if (DEBUGS) printf( "----At %.2f: O pacote não é para mim (%d), é para %d\n", Scheduler::instance().clock(), myaddr_, ih->saddr());
		drop(p,DROP_MAC_PACKET_ERROR);
    		return;
	}
	
	if(table_->AmIinLoop(p, myaddr_) && ant->toSinkNode)//se o pacote ja foi enviado por mim?? (dentro do caminho em loop)
	{
		if (DEBUGS) printf( "----At %.2f: %s: got a packet loop----------- \n", Scheduler::instance().clock(), __FUNCTION__);
		drop(p,DROP_RTR_ROUTE_LOOP);
    		return;
	}
	
	//a formiga chegou ao sink node, temos de madala para traz!!!
	if(SINK_NODE_ID == myaddr_)
	{
		if (DEBUGS) printf( "----At %.2f: Formiga chegou ao sink %d\n", Scheduler::instance().clock(), myaddr_);
		
		//é aqui que se tem de estudar a Funcção Optimização
		double aux1 = BIG_CONSTANT_C - path_energy(p);
		aux1 = 1/aux1;
				
		//reenviar o pacote para o emissor!!
		ant->hp_type	= AntSenseTYPE_ANT;
		ant->pheromone  = aux1;
		ant->toSinkNode = false;
		ih->saddr() = myaddr_;
		ih->daddr() = ant->antpath[--ant->currenthop];
		if (DEBUGS) printf( "----At %.2f: O sink (%d) reneviou o pacote para:%d\n", Scheduler::instance().clock(),myaddr_,ant->antpath[ant->currenthop]);
		ant->currenthop--;
 		ih->sport() = RT_PORT;
 		ih->dport() = RT_PORT;
 		ih->ttl_ = IP_DEF_TTL;
		ch->direction() = hdr_cmn::DOWN;
		Scheduler::instance().schedule(target_,p,0.0);
	}
	
	//o pacote vai em sentido do sinknode
	else if (ant->toSinkNode)
	{
// 		reenvio de um pacote ja enviado
		neighbour *prte = table_->GetNextNeighbourToSink(p);
		
		if (prte->address == myaddr_)
		{
			drop(p,DROP_RTR_NO_ROUTE);
			if (DEBUGS) printf( "----At %.2f: pacote uma formiga dropped:nenhum vizinho (reenvio) ou entrou em loop\n", Scheduler::instance().clock());	
			return;
		}
		
		ant->pkt_src_ 				=ant->pkt_src_;
		ant->hp_type				=AntSenseTYPE_ANT;
     		ant->antpath[ant->currenthop] 		= myaddr_;
		ant->energypath[ant->currenthop++] 	= node_energy();
		ant->pheromone				=ant->pheromone;
		ant->toSinkNode				=true;
		
		//Evaporação das feromonas kando o link e escolhido por um nó
		table_->UpdatePheromoneToSink(prte->address, PHEROMONE_EVAPORATION);
		
			
		ch->ptype() = PT_ANTSENSE;
 		ch->size() = IP_HDR_LEN + ant->size();
 		ch->iface() = -2;
 		ch->error() = 0;
 		ch->addr_type() = NS_AF_NONE;
 		ch->prev_hop_ = myaddr_; 
		ih->saddr() = myaddr_;
 		ih->daddr() = prte->address;
 		ih->sport() = RT_PORT;
 		ih->dport() = RT_PORT;
 		ih->ttl_    = --ih->ttl_;
		 
		if (DEBUGS) printf( "----At %.2f: Eu (%d), reenviei o pacote de formiga para:%d\n", Scheduler::instance().clock(),myaddr_, prte->address);
		 
		ch->direction() = hdr_cmn::DOWN;
		Scheduler::instance().schedule(target_,p,0.0);
	 
	
	}
	//o pacote vai em sentido da source e ja foi processado pelo sink node
	else 
	{//reenviar o pacote para o emissor!!
		ant->hp_type	= AntSenseTYPE_ANT;
		
		table_->UpdatePheromoneFromSink(ih->saddr(), PHEROMONE_EVAPORATION, ant->pheromone);
		if(ant->antpath[1]==myaddr_)
		{//se o pacote chegou á origem
			Packet::free (p);
			if (DEBUGS) printf( "----At %.2f: -------------------------a formiga que foi enviada por mim ja me xegou de novo--------------\n", Scheduler::instance().clock());
			return;
		}
		else
		{//ainda temos de reenviar
			ih->saddr() = myaddr_;
 			ih->daddr() = ant->antpath[ant->currenthop];
			ant->currenthop--;
 			ih->sport() = RT_PORT;
 			ih->dport() = RT_PORT;
 			ih->ttl_ = IP_DEF_TTL;
			
			ch->direction() = hdr_cmn::DOWN;
			if (DEBUGS) printf( "----At %.2f: Eu (%d), reenviei o pacote de formiga para:%d\n", Scheduler::instance().clock(),myaddr_, ih->daddr());
			Scheduler::instance().schedule(target_,p,0.0);
		}
	}
}

void AntSense::recvHello(Packet *p) {
	//struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antsense_hello_pkt *hello = HDR_ANTSENSE_HELLO_PKT(p);
	
	if(hello->rp_src==myaddr_)//se o pacote foi enviado por mim??
	{
		if (DEBUGS) printf( "----At %.2f: ", Scheduler::instance().clock());
		fprintf(stderr, " got my own REQUEST, from HELLO\n");
		fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
    		drop(p,DROP_RTR_ROUTE_LOOP);
    		return;
	}
	if (DEBUGS) printf( "----At %.2f: Updating from Hello\n", Scheduler::instance().clock());	
	
	double node_energy 	=hello->node_energy;
	nsaddr_t neighbour_id 	=hello->rp_src;
	
	bool known_neighbour = table_->FindEntry(neighbour_id);
	table_->ListTable(myaddr_, neighbour_id);
	
	if(!known_neighbour)
	{//Não existe ocurrencia deste vizinho, inserir..
		if (DEBUGS) printf( "----At %.2f: Não existe ocurrencia deste vizinho, inserir..\n", Scheduler::instance().clock());
		neighbour prte;
		prte.address=neighbour_id; // endereço do vizinho 
  		prte.energy=node_energy; // ultima energia conhecida do vizinho
  		prte.pheromone=STARTUP_PHEROMONE; // valor da feromona	
		prte.last_update_time = Scheduler::instance().clock();
		table_->AddEntry(prte);
	}
	else
	{//actualizar a energia e clock do vizinho
		if (DEBUGS) printf( "----At %.2f: actualizar a energia do vizinho\n", Scheduler::instance().clock());
		table_->UpdateEnergy(neighbour_id, node_energy);
		table_->UpdateClockNB (neighbour_id, Scheduler::instance().clock());
	}
	Packet::free (p);	
}

double AntSense::node_energy() {
	Node* thisnode = MobileNode::get_node_by_address(myaddr_);
 	double node_energy = 0.0;
	if ((thisnode) && (thisnode->energy_model())) 
	{
		    node_energy = thisnode->energy_model()->energy();
	}
	else
		if (DEBUGS) printf("Error on getting the node energy\n");
	return node_energy;
}


//TANDRE:3/7/2005
//retorna a média de energia que um determinado caminho tem...
double AntSense::path_energy(Packet *p) {

	//struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antsense_ant_pkt *ant = HDR_ANTSENSE_ANT_PKT(p);
	
	double PathEnergy=0.0;
	for (int i=1; i<ant->currenthop;i++)
	{
		PathEnergy += ant->energypath[i];
	}
	PathEnergy = PathEnergy/(ant->currenthop-1);
	return PathEnergy;
}