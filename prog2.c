#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

     Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
  int seqnum;
  int acknum;
  int checksum;
  char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define TIMEOUT 500.0f
#define WINDOW 8

struct pkt buffer[50];

unsigned int ack_sequence = 0;
unsigned int sent_sequence = 0;
unsigned int buffer_sequence = 0;


struct stats {
  int packets_sent;
  int ack_received;
  int nck_received;
  int packets_received;
  int packets_corrupted;
  int ack_corrupted;
  int timeouts;
  int packets_ignored;
} stats;
//Helper function to aid in checksums
int checksum(struct pkt packet)
{
  int i;
  unsigned int sum = 0;
  sum += (unsigned int)packet.seqnum;
  sum += (unsigned int)packet.acknum;
  for(i = 0; i < 20; i++)
    {
      sum += (unsigned int)packet.payload[i];      
    }
  return (int)sum;
}

//Helper function to create and send packets
struct pkt make_packet(int seq, int ack, char* payload)
{
  struct pkt packet_out;
  int i;

  packet_out.seqnum = seq;
  packet_out.acknum = ack;
  
  for(i = 0; i < 20; i++)
    packet_out.payload[i] = payload[i];

  packet_out.checksum = checksum(packet_out);
  return packet_out;
}

struct pkt send_packet(int target, int seq, int ack, char* payload)
{
  struct pkt packet_out;
  packet_out = make_packet(seq, ack, payload);
  printf("%c: Sending packet (%d, %d, %d ,'%s')\n",(target == 0) ? 'A' : 'B' ,packet_out.seqnum, packet_out.acknum, packet_out.checksum, packet_out.payload);
  stats.packets_sent++;
  tolayer3(target, packet_out); //send out the built packet from A
  return packet_out;
}


//This function will be called at the end of the simulation to get a report of the stats gathered
void print_stats()
{
  printf("--------STATS REPORT----------\n");
  printf("TOTAL PACKETS SENT: %i\n", stats.packets_sent);
  printf("TOTAL PACKETS IGNORNED: %i\n", stats.packets_ignored);
  printf("TOTAL PACKET LOSS: %f%%\n", 100.0f - ((float)stats.packets_received * 100.0f/(float)stats.packets_sent));
  printf("CORRUPTION %%: %f\n", (float)stats.packets_corrupted * 100.0f/(float)stats.packets_sent);
  printf("---------END REPORT-----------\n");
}

/* called from layer 5, passed the data to be sent to other side */
A_output(message)
struct msg message;
{
  struct pkt packet_out;

  if((buffer_sequence - ack_sequence) > 50)
    {
      printf("Buffer is full, ignoring packet");
      stats.packets_ignored++;
      return;
    }


  packet_out = make_packet(buffer_sequence, 0, message.data);
  if((sent_sequence - ack_sequence) < WINDOW)
    {
      if(sent_sequence == ack_sequence) 
	{
	  starttimer(0, TIMEOUT);
	}
      printf("Sending packet to B\n");

      tolayer3(0, packet_out);
      stats.packets_sent++;
      sent_sequence++;
    }

  //Save to buffer
  buffer[sent_sequence % 50] = packet_out;
  buffer_sequence++;

}

B_output(message)  /* need be completed only for extra credit */
struct msg message;
{

}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)
struct pkt packet;
{
  int i;
  stats.packets_received++;
  if(packet.checksum != checksum(packet))
    {
      //ignore this ack
      stats.packets_corrupted++;
      return;
    }
  if(packet.acknum > ack_sequence)
    {
      
      for(i = sent_sequence + 1; i <= (sent_sequence + (ack_sequence - packet.acknum)), i <= buffer_sequence; i++)
	{
	  tolayer3(0, buffer[i % 50]);
	  stats.packets_sent;
	  sent_sequence++;
	}
      printf("ACK received\n");
      ack_sequence = packet.acknum;
      stoptimer(0);

      if(sent_sequence > ack_sequence)
	{
	  //if we have packets out, then reset the timer
	  starttimer(0, TIMEOUT);
	}
      return;
    }
  return;

}

/* called when A's timer goes off */
A_timerinterrupt()
{
  int i;
  printf("A: TIMEOUT!, resending\n");
  starttimer(0, TIMEOUT);
  for(i = ack_sequence; i <= sent_sequence; i++)
    {
      stats.packets_sent++;
      tolayer3(0, buffer[i % 50]);
    }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
A_init()
{
  stats.packets_sent = 0;
  stats.ack_received = 0;
  stats.nck_received = 0;
  stats.packets_received = 0;
  stats.packets_corrupted = 0;
  stats.ack_corrupted = 0;
  stats.timeouts = 0;
  stats.packets_ignored = 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

unsigned int last_ack = 0;
/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)
struct pkt packet;
{
  int i;
  struct pkt ack_packet;
  struct pkt nck_packet;

  stats.packets_received++;

  if(checksum(packet) != packet.checksum)
    {
      printf("Packet Corrupted! Sending ACK\n");
      
      send_packet(1, last_ack, last_ack, packet.payload);
      stats.packets_corrupted++;
      
      return;
    }
  if(packet.seqnum < last_ack)
    {
      //Ignore
      return;
    }
  else if(packet.seqnum == last_ack)
    {
      last_ack++;
      send_packet(1, last_ack, last_ack, packet.payload);
      return;
    }
  else{
    printf("Out of order packet received! Sending NCK");
    send_packet(1, last_ack, last_ack, packet.payload);
    return;
  }
}

/* called when B's timer goes off */
B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
B_init()
{
}


/*****************************************************************
 ***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
  float evtime;           /* event time */
  int evtype;             /* event type code */
  int eventity;           /* entity where event occurs */
  struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
  struct event *prev;
  struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
  struct event *eventptr;
  struct msg  msg2give;
  struct pkt  pkt2give;
   
  int i,j;
  char c; 
  
  init();
  A_init();
  B_init();
   
  while (1) {
    eventptr = evlist;            /* get next event to simulate */
    if (eventptr==NULL)
      goto terminate;
    evlist = evlist->next;        /* remove this event from event list */
    if (evlist!=NULL)
      evlist->prev=NULL;
    if (TRACE>=2) {
      printf("\nEVENT time: %f,",eventptr->evtime);
      printf("  type: %d",eventptr->evtype);
      if (eventptr->evtype==0)
	printf(", timerinterrupt  ");
      else if (eventptr->evtype==1)
	printf(", fromlayer5 ");
      else
	printf(", fromlayer3 ");
      printf(" entity: %d\n",eventptr->eventity);
    }
    time = eventptr->evtime;        /* update time to next event time */
    if (nsim==nsimmax)
      break;                        /* all done with simulation */
    if (eventptr->evtype == FROM_LAYER5 ) {
      generate_next_arrival();   /* set up future arrival */
      /* fill in msg to give with string of same letter */    
      j = nsim % 26; 
      for (i=0; i<20; i++)  
	msg2give.data[i] = 97 + j;
      if (TRACE>2) {
	printf("          MAINLOOP: data given to student: ");
	for (i=0; i<20; i++) 
	  printf("%c", msg2give.data[i]);
	printf("\n");
      }
      nsim++;
      if (eventptr->eventity == A) 
	A_output(msg2give);  
      else
	B_output(msg2give);  
    }
    else if (eventptr->evtype ==  FROM_LAYER3) {
      pkt2give.seqnum = eventptr->pktptr->seqnum;
      pkt2give.acknum = eventptr->pktptr->acknum;
      pkt2give.checksum = eventptr->pktptr->checksum;
      for (i=0; i<20; i++)  
	pkt2give.payload[i] = eventptr->pktptr->payload[i];
      if (eventptr->eventity ==A)      /* deliver packet by calling */
	A_input(pkt2give);            /* appropriate entity */
      else
	B_input(pkt2give);
      free(eventptr->pktptr);          /* free the memory for packet */
    }
    else if (eventptr->evtype ==  TIMER_INTERRUPT) {
      if (eventptr->eventity == A) 
	A_timerinterrupt();
      else
	B_timerinterrupt();
    }
    else  {
      printf("INTERNAL PANIC: unknown event type \n");
    }
    free(eventptr);
  }

 terminate:
  printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);

  print_stats();
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
  printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
  printf("Enter the number of messages to simulate: ");
  scanf("%d",&nsimmax);
  printf("Enter  packet loss probability [enter 0.0 for no loss]:");
  scanf("%f",&lossprob);
  printf("Enter packet corruption probability [0.0 for no corruption]:");
  scanf("%f",&corruptprob);
  printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
  scanf("%f",&lambda);
  printf("Enter TRACE:");
  scanf("%d",&TRACE);

  srand(9999);              /* init random number generator */
  sum = 0.0;                /* test random number generator for students */
  for (i=0; i<1000; i++)
    sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
  avg = sum/1000.0;
  if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(-1);
  }

  ntolayer3 = 0;
  nlost = 0;
  ncorrupt = 0;

  time=0.0;                    /* initialize time to 0.0 */
  generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
generate_next_arrival()
{
  double x,log(),ceil();
  struct event *evptr;
  //    char *malloc();
  float ttime;
  int tempint;

  if (TRACE>2)
    printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
  x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
  /* having mean of lambda        */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime =  time + x;
  evptr->evtype =  FROM_LAYER5;
  if (BIDIRECTIONAL && (jimsrand()>0.5) )
    evptr->eventity = B;
  else
    evptr->eventity = A;
  insertevent(evptr);
} 


insertevent(p)
struct event *p;
{
  struct event *q,*qold;

  if (TRACE>2) {
    printf("            INSERTEVENT: time is %lf\n",time);
    printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
  }
  q = evlist;     /* q points to header of list in which p struct inserted */
  if (q==NULL) {   /* list is empty */
    evlist=p;
    p->next=NULL;
    p->prev=NULL;
  }
  else {
    for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
      qold=q; 
    if (q==NULL) {   /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    }
    else if (q==evlist) { /* front of list */
      p->next=evlist;
      p->prev=NULL;
      p->next->prev=p;
      evlist = p;
    }
    else {     /* middle of list */
      p->next=q;
      p->prev=q->prev;
      q->prev->next=p;
      q->prev=p;
    }
  }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
  }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
  struct event *q,*qold;

  if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      /* remove this event */
      if (q->next==NULL && q->prev==NULL)
	evlist=NULL;         /* remove first and only event on list */
      else if (q->next==NULL) /* end of list - there is one in front */
	q->prev->next = NULL;
      else if (q==evlist) { /* front of list - there must be event after */
	q->next->prev=NULL;
	evlist = q->next;
      }
      else {     /* middle of list */
	q->next->prev = q->prev;
	q->prev->next =  q->next;
      }
      free(q);
      return;
    }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

  struct event *q;
  struct event *evptr;
  //char *malloc();

  if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
  /* be nice: check to see if timer is already started, if so, then  warn */
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
    }
 
  /* create future event for when timer goes off */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime =  time + increment;
  evptr->evtype =  TIMER_INTERRUPT;
  evptr->eventity = AorB;
  insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
  struct pkt *mypktptr;
  struct event *evptr,*q;
  //char *malloc();
  float lastime, x, jimsrand();
  int i;


  ntolayer3++;

  /* simulate losses: */
  if (jimsrand() < lossprob)  {
    nlost++;
    if (TRACE>0)    
      printf("          TOLAYER3: packet being lost\n");
    return;
  }  

  /* make a copy of the packet student just gave me since he/she may decide */
  /* to do something with the packet after we return back to him/her */ 
  mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
  mypktptr->seqnum = packet.seqnum;
  mypktptr->acknum = packet.acknum;
  mypktptr->checksum = packet.checksum;
  for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
  if (TRACE>2)  {
    printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	   mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
      printf("%c",mypktptr->payload[i]);
    printf("\n");
  }

  /* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
  lastime = time;
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
  for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
  evptr->evtime =  lastime + 1 + 9*jimsrand();
 


  /* simulate corruption: */
  if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
      mypktptr->payload[0]='Z';   /* corrupt payload */
    else if (x < .875)
      mypktptr->seqnum = 999999;
    else
      mypktptr->acknum = 999999;
    if (TRACE>0)    
      printf("          TOLAYER3: packet being corrupted\n");
  }  

  if (TRACE>2)  
    printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

tolayer5(AorB,datasent)
int AorB;
char datasent[20];
{
  int i;  
  if (TRACE>2) {
    printf("          TOLAYER5: data received: ");
    for (i=0; i<20; i++)  
      printf("%c",datasent[i]);
    printf("\n");
  }
  
}
