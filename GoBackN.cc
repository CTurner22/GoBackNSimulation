#include "includes.h"

// ***************************************************************************
// * ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
// *
// * These are the functions you need to fill in.
// ***************************************************************************


// ***************************************************************************
// * Because of the way the simulator works you will likey need global variables
// * You can define those here.
// ***************************************************************************
std::unique_ptr<Window> a_window;
std::unique_ptr<Timer> a_timer;
int a_rx_seq = 0;

std::unique_ptr<Window> b_window;
std::unique_ptr<Timer> b_timer;
int b_rx_seq = 0;

// ***************************************************************************
// * The following routine will be called once (only) before any other
// * entity A routines are called. You can use it to do any initialization
// ***************************************************************************
void A_init() {
    a_window = std::make_unique<Window>(MAX_WINDOW_SIZE);
    a_timer = std::make_unique<Timer>(A);
}

// ***************************************************************************
// * The following rouytine will be called once (only) before any other
// * entity B routines are called. You can use it to do any initialization
// ***************************************************************************
void B_init() {


}


// ***************************************************************************
// * Called from layer 5, passed the data to be sent to other side 
// ***************************************************************************
int A_output(struct msg message) {
    std::cout << "Layer 4 on side A has recieved a message from the application that should be sent to side B: "
              << message << std::endl;

    // check if we have room
    if(!a_window->get_vacency())
        return TX_REFUSED;

    // construct packet with message data and send to side B
    std::shared_ptr<struct pkt> pckt = make_pkt(a_window->get_seq(), 0, &message);
    
    // add it to cache
    float time = simulation->getSimulatorClock();
    a_window->add_packet(pckt, time);

    // send it
    simulation->tolayer3(A, *pckt);

    // start timer
    if(!a_timer->running())
        a_timer->start(a_window->get_rto());
    
    return TX_SUCCESS;
}


// ***************************************************************************
// * Called from layer 3, when a packet arrives for layer 4 on side A
// ***************************************************************************
void A_input(struct pkt packet) {
    std::cout << "Layer 4 on side A has recieved a packet sent over the network from side B:" << packet << std::endl;

    // check integrity
    if (!vrfy_checksum(&packet)) {
        return;
    }

    // move window
    a_window->ack_packet(packet.acknum, simulation->getSimulatorClock());

    // update timer
    if(a_window->get_vacency() == a_window->WINDOW_SIZE)
        a_timer->stop();
    else
        a_timer->restart(a_window->get_rto());
}


// ***************************************************************************
// * Called from layer 5, passed the data to be sent to other side
// ***************************************************************************
int B_output(struct msg message) {
    std::cout << "Layer 4 on side B has recieved a message from the application that should be sent to side A: "
              << message << std::endl;

    return (1); /* Return a 0 to refuse the message */
}


// ***************************************************************************
// // called from layer 3, when a packet arrives for layer 4 on side B 
// ***************************************************************************
void B_input(struct pkt packet) {
    std::cout << "Layer 4 on side B has recieved a packet from layer 3 sent over the network from side A:" << packet
              << std::endl;

    // check integrity
    if (!vrfy_checksum(&packet) || packet.acknum != b_rx_seq + 1) {

        // curruption or out of order, resend ack for last valid seq
        std::shared_ptr<struct pkt> pckt = make_pkt(0, b_rx_seq, nullptr);
        simulation->tolayer3(B, *pckt);
        return;
    }


    // send valid ack
    std::shared_ptr<struct pkt> pckt = make_pkt(0, ++b_rx_seq, nullptr);
    simulation->tolayer3(B, *pckt);

    // send up stack
    auto message = std::make_unique<struct msg>();
    memcpy(message->data, packet.payload, sizeof(packet.payload));

    simulation->tolayer5(B, *message);
}


// ***************************************************************************
// * Called when A's timer goes off 
// ***************************************************************************
void A_timerinterrupt() {
    std::cout << "Side A's timer has gone off." << std::endl;

    // Retransmit cached packets
    if(a_window->get_vacency() != a_window->WINDOW_SIZE) {
        for(int i = a_window->get_base(); i < a_window->get_seq(); i++) {
            simulation->tolayer3(A, *a_window->get_packet(i, simulation->getSimulatorClock()));

        }
    }

}

// ***************************************************************************
// * Called when B's timer goes off 
// ***************************************************************************
void B_timerinterrupt() {
    std::cout << "Side B's timer has gone off." << std::endl;
}

std::unique_ptr<struct pkt> make_pkt(int seq, int ack, struct msg *message) {

    auto ret = std::make_unique<struct pkt>();
    ret->seqnum = seq;
    ret->acknum = ack;
    ret->checksum = 0;

    if(message)
        memcpy(ret->payload, message->data, sizeof(message->data));

    // generate checksum
    char *start = reinterpret_cast<char*>(ret.get());

    std::size_t hash_check = std::hash<std::string>{}( std::string(start, start + sizeof(struct pkt)));

    // truncate, not really safe and ideal but fine for this application
    ret->checksum = static_cast<int>(hash_check >> 32);
    return ret;
}


bool vrfy_checksum(struct pkt* pckt) {
    int orig = pckt->checksum;
    pckt->checksum = 0;

    char *start = reinterpret_cast<char*>(pckt);
    return std::hash<std::string>{}( std::string(start, start + sizeof(struct pkt))) >> 32 == orig;
}

Window::Window(size_t N) : pckt_cache_{ std::make_unique<pckt_ptr[]>(N) }, 
WINDOW_SIZE{N}, times_{std::make_unique<time_t[]>(N)} {

};

int Window::get_base() {
    return this->base_seq_;
}

int Window::get_seq() {
    return this->current_seq_;
}

int Window::get_vacency() {
    return WINDOW_SIZE - (this->current_seq_ - this->base_seq_);
}


Window::pckt_ptr& Window::pckt_at_(int seq) {
    return pckt_cache_[(seq - this->base_seq_) % WINDOW_SIZE];
}

Window::time_t& Window::time_at_(int seq) {
    return times_[(seq - this->base_seq_) % WINDOW_SIZE];
}


bool Window::ack_packet(int seq, time_t tm) {
    bool valid_ack = false;

    if (this->base_seq_ < seq) {
        this->base_seq_ = seq + 1;
        valid_ack = true;

        // update rtt
        this->est_rtt_ = (this->est_rtt_ * ALPHA) + ((1 - ALPHA) * (tm - this->time_at_(seq)));
    }

    if (this->current_seq_ < this->base_seq_) {
        this->current_seq_ = this->base_seq_;
    }

    return valid_ack;
}

void Window::add_packet(pckt_ptr pckt, time_t tm) {
    assert(this->get_vacency());
    this->pckt_at_(this->current_seq_) = pckt;
    this->time_at_(this->current_seq_++) = tm;
}

Window::pckt_ptr Window::get_packet(int seq, time_t tm) {
    if (seq < base_seq_ || seq >= current_seq_)
        return pckt_ptr();
    else {
        this->time_at_(seq) = tm;
        return this->pckt_at_(seq);
    }
}

Window::time_t Window::get_rto() {
    return this->est_rtt_ * BETA;
}

Timer::Timer(int side) : SIDE{side} {}

void Timer::start(time_t tm) {
    simulation->starttimer(this->SIDE, tm);
    this->running_ = true;
}

void Timer::restart(time_t tm) {
    if(this->running_)
        this->stop();

    this->start(tm);
}


void Timer::stop() {
    simulation->stoptimer(this->SIDE);
    this->running_ = false;
}

bool Timer::running() {
    return this->running_;
}