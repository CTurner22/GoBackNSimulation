
// ***********************************************************
// ** If you want to define other fucntions, put them here.
// ***********************************************************
void A_init();
void B_init();

int A_output(struct msg message);
int B_output(struct msg message);  /* need be completed only for extra credit */

void A_input(struct pkt packet);
void B_input(struct pkt packet);

void A_timerinterrupt();
void B_timerinterrupt();

// ***********************************************************
// ** Simple operator functions to make output look cleaner.
// ***********************************************************
std::ostream& operator<<(std::ostream& os, const struct msg& message);
std::ostream& operator<<(std::ostream& os, const struct pkt& packet);

// ***********************************************************
// ** The simulator is a global.
// ***********************************************************
extern simulator *simulation;
