
// ***********************************************************
// * Any additional include files should be added here.
// ***********************************************************
#include <memory>
#include <functional>

// ***********************************************************
// * Any functions you want to add should be included here.
// ***********************************************************

#define MAX_WINDOW_SIZE 10
#define INIT_RTT 50.0
#define MIN_RTT 15.0
#define MAX_RTT 100.0

#define ALPHA  0.125
#define BETA 1.5

#define TX_SUCCESS 1
#define TX_REFUSED 0

std::unique_ptr<struct pkt> make_pkt(int seq,int ack,struct msg *message);
bool vrfy_checksum(struct pkt* pckt);


class Window {
public:
    using pckt_ptr = std::shared_ptr<struct pkt>;
    using time_t = decltype(std::declval<simulator>().getSimulatorClock());
    
    Window(size_t N);

    int get_seq();
    int get_base();
    int get_vacency();
    
    time_t get_rto();

    bool ack_packet(int seq, time_t tm);
    void add_packet(pckt_ptr pckt, time_t tm);

    pckt_ptr get_packet(int seq, time_t tm);

    const size_t WINDOW_SIZE;

private:

    int base_seq_ = 1;
    int current_seq_ = 1;
    time_t est_rtt_ = INIT_RTT;


    std::unique_ptr<pckt_ptr[]> pckt_cache_;
    std::unique_ptr<time_t[]> times_;


    pckt_ptr& pckt_at_(int seq);   
    time_t& time_at_(int seq);    
 
};

class Timer {
public:
    using time_t = decltype(std::declval<simulator>().getSimulatorClock());

    Timer(int side);

    void restart(time_t tm);
    void start(time_t tm);
    void stop();
    bool running();
    void set_expired();


    private:
        const int SIDE;
        bool running_ = false;
        
};
