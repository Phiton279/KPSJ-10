xpos; 
ypos; 
zpos;
signal_thr;
e_thr;
iEnergy; 
*iNode;
iRss;
rec_power;
rec_power_dBm;
queuelen;
pheromone_count;
rq_phcount;


pheromone_count = ((iRss + 91) * iEnergy) / (queuelen * rq->rq_hop_count); 

rep_cm_;	/* congestion metric */
rep_lm_;	/* link metric */

pheromne_count_new = srh->get_rep_lm() / (srh->route_reply_len() * srh->get_rep_cm());  