#include "includes.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

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

simulator::simulator(int n, float l, float c, float t) {


    // ********************************************************************
    // * User configureable Variables.
    // ********************************************************************
    nsimmax = n;
    lossprob = l;
    corruptprob = c;
    lambda = t;


    // ***************************************************************************
    // * Internal variables.
    // ***************************************************************************
    evlist = NULL;
    nsim = 0;
    kr_time = 0.000;
    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;
    kr_time = 0.0;
    srandom(time(NULL));
    generate_next_arrival();


    // ***************************************************************************
    // * Basic Sanity Checks
    // ***************************************************************************
    if (nsimmax <= 0) {
        FATAL << "Can't have a simulation without at least 1 message." << std::endl;
        exit(-1);
    }
    if ((lossprob < 0) || (lossprob > 1)) {
        FATAL << "Invalid loss probability (" << lossprob << ")." << std::endl;
        exit(-1);
    }
    if ((corruptprob < 0) || (corruptprob > 1)) {
        FATAL << "Invalid corruption probability (" << corruptprob << ")." << std::endl;
        exit(-1);
    }
    if (lambda < 0) {
        FATAL << "Invalid average delay (" << lambda << ")." << std::endl;
        exit(-1);
    }


    INFO << "-----  Stop and Wait Network Simulator Version 1.1 -------- ";
    INFO << "Number of messages to simulate: " << nsimmax;
    INFO << "Packet loss probability: " << lossprob;
    INFO << "Packet corruption probability [0.0 for no corruption]: " << corruptprob;
    INFO << "Average time between messages from sender's layer5: " << lambda;

}


void simulator::go() {
    struct event *eventptr;

    struct pkt pkt2give;

    int i, j;
    char c;

    while (1) {


        //
        // Pop the next event off the list.
        //
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;

        INFO << "MAINLOOP: Triggering event at time: " << eventptr->evtime << "type: " << eventptr->evtype
             << EVENT_NAMES[eventptr->evtype] << ", entity: " << SIDE_NAMES[eventptr->eventity];

        //
        // Jump the clock forward to the time the next event needs to happen.
        //
        kr_time = eventptr->evtime;

        //
        // I think it is a bug to stop here. Need to look more closely.
        //
        // if (nsim==nsimmax)
        // break; 	/* all done with simulation */


        //
        // Process the event.
        //
        if ((eventptr->evtype == FROM_LAYER5) && (nsim != nsimmax)) {

            // This adds the next FROM_LAYER5 event to the event list.
            generate_next_arrival();

            /* fill in msg to give with string of same letter */
            struct msg msg2give;
            j = nsim % 26;
            for (int i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;
            INFO << "MAINLOOP: Passing data down from layer 5 to layer 4: " << msg2give;


            // Pass the message down to the student.
            if (eventptr->eventity == A) {
                if (A_output(msg2give)) { nsim++; }

            } else {
                if (B_output(msg2give)) { nsim++; };
            }
            // nsim++ used to be here.

        }


        if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];

            INFO << "MAINLOOP: Passing packet up from layer 3 to layer 4: " << pkt2give;
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);


            free(eventptr->pktptr);          /* free the memory for packet */

        }


        if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }


        free(eventptr);
    }

    terminate:
    INFO << "MAINLOOP: Simulator terminated at time " << kr_time << " after sending " << nsim << " msgs from layer5.";
}




/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
double simulator::jimsrand() {
    return ((double) random() / (double) RAND_MAX);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
void simulator::generate_next_arrival() {
    double x;
    struct event *evptr;
    float ttime;
    int tempint;


    INFO << "GENERATE NEXT ARRIVAL: creating new arrival.";

    x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtime = kr_time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


void simulator::insertevent(struct event *p) {
    struct event *q, *qold;

    INFO << "INSERTEVENT: Current time is " << kr_time << ", inserting event to happen at " << p->evtime;

    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    } else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        } else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        } else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void simulator::printevlist() {
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void simulator::stoptimer(int AorB) {
    struct event *q, *qold;

    INFO << "STOP TIMER: stopping timer at " << kr_time;

    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            } else {     /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    WARNING << "Warning: unable to cancel your timer. It wasn't running.";
}


void simulator::starttimer(int AorB, float increment) {
    struct event *q;
    struct event *evptr;

    INFO << "START TIMER: starting timer at " << kr_time << std::endl;

    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            WARNING << "Warning: attempt to start a timer that is already started";
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtime = kr_time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void simulator::tolayer3(int AorB, struct pkt packet) {
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        INFO << "TOLAYER3: Loosing packet: " << packet;
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *) malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (int i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];


    /* create future event for arrival of packet at the other side */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */


    /* finally, compute the arrival time of packet at the other end.
     medium can not reorder, so make sure packet arrives between 1 and 10
     time units after the latest arrival time of packets
     currently in the medium on their way to the destination */
    lastime = kr_time;
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();


    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        INFO << "TOLAYER3: Corrupting packet " << packet << " as " << *mypktptr;
    }


    INFO << "TOLAYER3: Scheduling " << packet << " to arrvie on side " << SIDE_NAMES[(AorB + 1) % 2] << ".";
    insertevent(evptr);
}


void simulator::tolayer5(int AorB, struct msg message) {

    INFO << "TOLAYER5: Data received at application layer on side " << SIDE_NAMES[AorB]
         << ", (" << message << ").";
}

float simulator::getSimulatorClock() {

    return(kr_time);
}
