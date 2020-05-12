#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "neighbour_table.h"
#include <random.h>

//TANDRE: 12/06/2005
#include "antsense_pkt.h"

//TANDRE: 5/06/2005

inline neighbour& neighbour::operator = ( const neighbour &other){
	

	hop = other.hop;     // next hop
  
  	metric = other.metric;  // distance
  
  	last_message_id=other.last_message_id; // last sequence number we saw
  
  	//TANDRE 26/5/2005
  	address=other.address; // endereço do vizinho 
  	energy=other.energy; // ultima energia conhecida do vizinho
  	pheromone=other.pheromone; // valor da feromona
 
 	//TANDRE:16/7/2005
  	last_update_time=other.last_update_time;
 
  	advertise_ok_at=other.advertise_ok_at; // when is it okay to advertise this rt?
  	advert_seqnum=other.advert_seqnum;  // should we advert rte b/c of new seqnum?
  	advert_metric=other.advert_metric;  // should we advert rte b/c of new metric?



  	last_advertised_metric=other.last_advertised_metric; // metric carried in our last advert
  	changed_at=other.changed_at; // when route last changed
  	new_seqnum_at=other.new_seqnum_at;	// when we last heard a new seq number
  	wst=other.wst;     // running wst info
}


RoutingTable::RoutingTable() {
 		// Let's start with a ten element maxelts.
  elts = 0;	//numero actual de elementos vizinhos de um nó
  		//começa com 0
  
  maxelts = 10; //maximo numero de elementos no array de visinhos...
  		//este numero é actualizado caso seja necessário 
		//começa com 10 por kestoes de memoria
		
  ngbtable = new neighbour[maxelts];//Constroi uma nova tabela por nó
  ngbtableNotVisited  = new neighbour[maxelts];
   
  DEBUGS = 0; //utilizado em debug
     //antsense_queue *Antqueue;	//queue for antsense
     
  PHEROMONE_WEIGHT =1.5;
  ENERGY_WEIGHT = 1.5;
  SINK_NODE_ID = 0;
}
 
RoutingTable::~RoutingTable() {
	delete [] ngbtable;
	delete [] ngbtableNotVisited;
	ngbtable = NULL;
	ngbtableNotVisited = NULL;
}
//Tandre:Visto
void RoutingTable::AddEntry(const neighbour &ent)
//Aumenta um novo elemento à lista de vizinhos
{ 
  if (elts == maxelts) {
  	if(DEBUGS) printf("foi necessario aumentar a capacidade dos elementos do array\n");
    	neighbour tmp [maxelts];
	int oldmaxelts=maxelts;
	for(int i=0;i<maxelts;i++)
		tmp[i]= ngbtable[i];
    	maxelts *= 2;
    	//bcopy(&tmp, &ngbtable, elts*sizeof(neighbour));
	delete [] ngbtable;
	ngbtable = NULL;
	 ngbtable = new neighbour[maxelts];
	for(int i=0;i<oldmaxelts;i++)
		ngbtable[i]=tmp[i];
  }
  
   
  /*if (DEBUGS)
  {
  	printf("testing o k estou a mandar...%i--DEBUG\n",DEBUGS);
  	printf("testing o k estou a mandar...%f--Phero\n",PHEROMONE_WEIGHT);
  	printf("testing o k estou a mandar...%f--ener\n",ENERGY_WEIGHT);
  	printf("testing o k estou a mandar...%i--maxelts\n",maxelts);
  }*/
  
  if(DEBUGS) printf("AddEntry endereço:%d",ent.address);
  //printf("Encontramos 1:%d   2:%d",neighbour].address);
  
  bcopy(&ent, &ngbtable[elts], sizeof(neighbour));
  
  if(DEBUGS) printf("\nAddEntry endereço ngbtable: %d",ngbtable[elts].address);
  elts++;
  if(DEBUGS) printf("...acabei de aumentar o elts para:%i ", elts);
  return;
}

//TANDRE @ 29/12/2005
bool RoutingTable::DeleteEntry(nsaddr_t lostneighbour)
//apaga um elemento da tabela e retorna sucesso ou nao
{
	
	if(ngbtable[0].address == lostneighbour)//Nao podemos apagar o proprio
		return false;
	
	bool found=false;
	
	//delete [] tmp;
	neighbour tmp[maxelts];
	
	for ( int i=0;i<elts;i++) 
		//bcopy(&ngbtable[i], &tmp[i], sizeof(neighbour));
		tmp[i] =ngbtable[i] ; //ao contrario
	
	for ( int i=0;i<elts;i++) 
	{
		if(tmp[i].address != lostneighbour)
			//bcopy(&tmp[i], &ngbtable[i], sizeof(neighbour));
			ngbtable[i]=tmp[i];//ao contrario
		else
			found=true;
	}
	if (found)
	{
		elts--;
		return true;
	}
	else
		return false;
}	
	 
//Tandre:Visto
void RoutingTable::InitLoop() {
//funcção utilizada para loops, pesquisas na tabela
//coloca no primeiro elemento
  ctr = 0;
}

//Tandre:Visto
neighbour *RoutingTable::NextLoop() {
//funcção utilizada para loops, pesquisas na tabela
//move o ponteiro de pesquisa um elemento
  if (ctr >= elts)
    return 0;

  return &ngbtable[ctr++]; 
}

//Tandre:5/06/2005
neighbour * RoutingTable::GetNextHop(nsaddr_t dest, int last_message, int prev_hop) {
//retorna o proximo elemento a ser enviado o pacote normal...
  
	double	maxPheromone=0.0;
	int index=0;
	
   
   	//no caso de não existir ninguem para enviar, retorna o proximo mas com metric=BIG (por default)
   	if (elts==1)
   	{
   		return &ngbtable[0];
   	}
   
   	for (int i=1;i<elts;i++)
  	{
		if(DEBUGS) printf("Estamos no nó:%d... Estamos a tentar este no:%d, com pheromone=%f\n",ngbtable[0].address,  ngbtable[i].address,ngbtable[i].pheromone);
		
		if(ngbtable[i].address == SINK_NODE_ID)
		{
			if(DEBUGS) printf("Encontrei o sink.....\n");
			ngbtable[i].last_message_id = last_message;
			return &ngbtable[i];
		}
		if(ngbtable[i].pheromone>maxPheromone && ngbtable[i].last_message_id != last_message && ngbtable[i].address != prev_hop)
		{
			maxPheromone = ngbtable[i].pheromone;
			index=i;
		}
  	}
	
	//TODO: colocar na QUEUE
	if (index==0) //existe mas é um loop...
		return &ngbtable[0];
		
	ngbtable[index].last_message_id = last_message;
	return &ngbtable[index]; 
}

neighbour * RoutingTable::UpdateFromLostLink(nsaddr_t lostneighbour, nsaddr_t destinationIP, int last_message, int prev_hop)
{
	if(DeleteEntry(lostneighbour))
	{
		return GetNextHop(destinationIP, last_message, prev_hop);
	}
	else
	{
		if (DEBUGS) printf("\n [ERRO]++++++++++++++Algo se passou de muito errado...\n");
		return NULL;
	}
	
}

void RoutingTable::ConsumeNeighborsFromANearSinkNode(nsaddr_t dest)
{
	for (int i=1;i<elts;i++)
  	{
		if(ngbtable[i].address !=SINK_NODE_ID);
		{
			ngbtable[i].pheromone -= ngbtable[i].pheromone*0.1; 
		}
	}

}

//TANDRE:17/6/2005
//verifica se existe um neibogr k já um tempo nao resonde apaga esse da lista
void RoutingTable::CheckLifeNeighbors(double CurrentTime, double AntSense_BROADCAST_JITTER, double myaddress)
{
	/*delete [] tmp;
	tmp = new neighbour[maxelts]; */
	
	neighbour tmp[maxelts]; 
	    
	int finalElements=1;
	bool change=false;
	if(DEBUGS) printf("a verificar se passou algum vizinho de expiração\n");
	
	//bcopy(&ngbtable[0], &tmp[0], sizeof(neighbour));//importante o primeiro tem de ser sempre copiado pois é o proprio....
	tmp[0]=ngbtable[0];
	for ( int i=1;i<elts;i++) 
	 {	
	 	double Aux;
		Aux = CurrentTime - ngbtable[i].last_update_time;
		if(DEBUGS) printf("\nnode=%d....", ngbtable[i].address);
		if(DEBUGS) printf("AUX=%f, CURRENTTIME=%f, lastUPDATE=%f, AntSense_BROADCAST_JITTER=%f", Aux,CurrentTime, ngbtable[i].last_update_time, (AntSense_BROADCAST_JITTER*2.5));
		
		if((Aux < AntSense_BROADCAST_JITTER*2.5))//ainda esta na lista de vizinhos...
		{
			//bcopy(&ngbtable[i], &tmp[finalElements], sizeof(neighbour));
			tmp[finalElements]=ngbtable[i];
			finalElements++;
		}
		else
		{
			change=true; 
			if(DEBUGS) printf( "\nTivemos de apagar um viziho da lista... nao sabemos dele á um tempo\n");
		}
  	} 
	if (change)
	{
	
/*		delete [] ngbtable;
		ngbtable = new neighbour[maxelts];*/
		for ( int i=0;i<finalElements;i++) 
	 		//bcopy(&tmp[i], &ngbtable[i], sizeof(neighbour));
			ngbtable[i]=tmp[i];
		elts=finalElements;
	}
    	else
		if(DEBUGS) printf( "\nnao retirei nenhum elemento---vizinhos todos ainda em contacto\n");
}


//TANDRE:17/6/2005
void RoutingTable::UpdateClockNB(nsaddr_t element, double Time)
{
 	for ( int max=0;max<elts;max++) 
	{
		if (element == ngbtable[max].address) 
		{
			if(DEBUGS) printf( "encontrou o vizinho na sua tabela de endereçamento\n");
			ngbtable[max].last_update_time=Time;
		  	return;
	  	}
  	}	
	
	if(DEBUGS) printf("pesquisou mas nao encontro vizinho \n");
}

//TANDRE:12/6/2005
void RoutingTable::UpdateEnergy(nsaddr_t element, double energy)
{
 	for ( int max=0;max<elts;max++) 
	{
		if (element == ngbtable[max].address) 
		{
			if(DEBUGS) printf( "encontrou o vizinho na sua tabela de endereçamento\n");
			ngbtable[max].energy=energy;
		  	return;
	  	}
  	}	
	if(DEBUGS) printf("pesquisou mas nao encontro vizinho \n");
}

//TANDRE:16/7/2005
void RoutingTable::UpdatePheromoneToSink(nsaddr_t element,double evaporation)
{
 	for ( int i=0;i<elts;i++) 
	{
		if (element == ngbtable[i].address) 
		{
			if(DEBUGS) printf("Actualizando as feromonas .... ToSink\n Actual pheromone, %f, de (%d)->%d = %d???",ngbtable[i].pheromone, ngbtable[0].address,element,ngbtable[i].address);
			ngbtable[i].pheromone -= ngbtable[i].pheromone*evaporation;
			if(DEBUGS) printf(" next pheromone %f\n",ngbtable[i].pheromone);
		  	return;
	  	}
  	}		
}

//TANDRE:16/7/2005
void RoutingTable::UpdatePheromoneFromSink(nsaddr_t element, double evaporation, double pheromone){
 	for ( int i=0;i<elts;i++) 
	{
		if (element == ngbtable[i].address) 
		{
			if(DEBUGS) printf( "Actualizando as feromonas .... FromSink\n Actual pheromone, %f de (%d)->%d, and pheromone is %f",ngbtable[i].pheromone, ngbtable[0].address,element, pheromone);
			//TODO: verificar se a evaporação entra nesta formula.... para + ou -
			//double condensation = evaporation*0.8;  //apenas necessito de kase tudo da evaporação para restabelecer o que retirei
			ngbtable[i].pheromone = ngbtable[i].pheromone/(1-evaporation) + pheromone;//Função 3 [carreto]
			if(DEBUGS) printf(" next pheromone %f\n",ngbtable[i].pheromone);
		  	return;
	  	}
  	}

}

//TANDRE:12/6/2005
void RoutingTable::ListTable(nsaddr_t myadd, nsaddr_t search)
{
	for(int i=0;i<elts;i++) 
	{
		if(DEBUGS) printf("node %d (procuro %d)tem como vizinho ->%d\n",myadd, search, ngbtable[i].address);
	}
}

//TANDRE:26/2/2006
void RoutingTable::ListPheromons(nsaddr_t myadd)
{
	for(int i=1;i<elts;i++) {
		printf("node %d--->%d com pheromone->%f\n",myadd, ngbtable[i].address, ngbtable[i].pheromone);
	}
}

//TANDRE:12/6/2005
bool RoutingTable::FindEntry(nsaddr_t element)
{
	for ( int max=0;max<elts;max++) {
		if (element == ngbtable[max].address) {
			return true;
	  	}
  	}
	return false;
}

//TANDRE:12/6/2005//retorna o proximo elemento a ser enviado a formiga
neighbour * RoutingTable::GetNextNeighbourToSink(Packet *p) 
{
	struct hdr_antsense_ant_pkt *ant = HDR_ANTSENSE_ANT_PKT(p);
	double	sum=0.0;
  	//no caso de não existir ninguem para enviar, retorna o proximo mas com metric=BIG (por default)
  	if (elts==1)
  	{	
		if(DEBUGS) printf("Não existe ninguem para enviar as formigas\n");
  		return &ngbtable[0];
  	}
  	
	//Array com os vizinhos que ainda não foram visitados pela formiga
	delete [] ngbtableNotVisited ;
	ngbtableNotVisited = new neighbour[elts];

	bool found = false;
	int ifounded=1;
	int inewNum=1;
	
	//encontra vizinhos em que o pacote ja passou
	for (inewNum=1;inewNum<elts;inewNum++)
	{
		found=false;
		if(DEBUGS) printf("deves fazer %d loops em %i\n",ant->currenthop,inewNum);
		if(ant->currenthop>15) return &ngbtableNotVisited[1];//TODO:o k e isto?? TTL?? why retorna o novo?? ainda vazio??talvez evita loops...
		for (int newNum=1;newNum<ant->currenthop;newNum++)
		{
			if(DEBUGS) printf("Teste este :%d  é igual a %d\n",ant->antpath[newNum],ngbtable[inewNum].address);
			if(ant->antpath[newNum] == ngbtable[inewNum].address)
			{
				if(DEBUGS) printf("É mesmo igual!!! 1:%d---%d\n", ant->antpath[newNum],ngbtable[inewNum].address);
				found=true;
			}
		}
		//se não o encontrou dentro do caminho ja percurrido acrescentar aos possiveis
		if(!found)
		{
			bcopy(&ngbtable[inewNum], &ngbtableNotVisited[ifounded], sizeof(neighbour));
			if(DEBUGS) printf("Encontramos pessoal k ainda nao entrou na lista de candidatos: acrescentar node %d, com feromona=%f", ngbtableNotVisited[ifounded].address, ngbtableNotVisited[ifounded].pheromone);
			if(DEBUGS) printf(", ja encontrados %i\n",ifounded );
			if(ngbtableNotVisited[ifounded].address == SINK_NODE_ID)
			{
				if(DEBUGS) printf(".....encontrei o sink\n");
				return &ngbtableNotVisited[ifounded];;
			}
			ifounded++;
		}
	}

	//n encontrou nenhum vizinho pacivel de ser enviado o pacote, vamos colocar em queue até existir caminho
	if (ifounded==1)
	{
		if(DEBUGS) printf("Não encontrei nenhum vizinho passivel de enviar a formiga\n");
  		return &ngbtable[0];
	}
	
	if(ifounded==2)//apenas encontrou um entao vamos mandar este ja
	{
		if(DEBUGS) printf("Este é o vizinho a ser enviado: %d, pois só existe uma possibilidade\n",ngbtableNotVisited[ifounded-1].address);
  		return &ngbtableNotVisited[ifounded-1];//temos o array pronto vamos escolher o proximo hop
	
	}
	double 	NextHopProbabilities[inewNum];
	for (int i=0;i<inewNum;i++) NextHopProbabilities[i]=0.0;
  	for (int i=1;i<inewNum;i++) //Função(1)[carreto]
  	{
  		double globalvalue =0.0;
		double localvalue  =0.0;
	
		globalvalue = pow(ngbtableNotVisited[i].pheromone,PHEROMONE_WEIGHT);
		localvalue  = pow(ngbtableNotVisited[i].energy,ENERGY_WEIGHT);
		NextHopProbabilities[i]=globalvalue*localvalue;
	
		sum += NextHopProbabilities[i];
  	}
  
  	for (int i=1;i<elts;i++)
  	{
  		NextHopProbabilities[i]=NextHopProbabilities[i]/sum;
		if(DEBUGS) printf("acumulado...%d...->%f\n",i, NextHopProbabilities[i]);
  	}
	
	double target = 0.0;
	target = Random::uniform(100)/100;//numero de 0 a 1
	if(DEBUGS) printf("O numero da sorte é: %f\n",target);
	double rouleta = 0.0;

	//Processo de Roleta....
	int lucky=1;
	for (lucky=1;lucky<inewNum;lucky++)
	{
		if(DEBUGS) printf("Roleta......") ;
		rouleta += NextHopProbabilities[lucky];
		if(DEBUGS) printf("...%f\n",rouleta);
		if(rouleta > target) break;
	}
	if(DEBUGS) printf("Este é o vizinho a ser enviado: %d\n",ngbtableNotVisited[lucky].address);
  	return &ngbtableNotVisited[lucky];//temos o array pronto vamos escolher o proximo hop
}

//TANDRE:27/6/2005
//verifica se um determinado no ja foi visitado por um pacote (entrada em loop)
bool RoutingTable::AmIinLoop(Packet *p, nsaddr_t me)
{
	//struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antsense_ant_pkt *ant = HDR_ANTSENSE_ANT_PKT(p);
	
	for (int newNum=1;newNum<ant->currenthop;newNum++)
	{
		if(ant->antpath[newNum] == me)
		{
			return true;
		}
	}
	return false;
}