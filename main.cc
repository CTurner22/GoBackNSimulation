#include "includes.h"


// ******************************************************************************************
// * Utility functions for the updated version of J. Kurose's simulator.
// * - Student's should need to modify any of this code.
// *
// * Author: Phil Romig, Colorado School of Mines.
// ******************************************************************************************

simulator *simulation;

int main(int argc, char **argv) {

  int nismmax = -1;
  float lossprob = -1.0;
  float corruptprob = - 1.0;
  float lambda = -1.0;
  
  int opt = 0;
  boost::log::add_console_log(std::cout, boost::log::keywords::format = "%Message%");
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
  while ((opt = getopt(argc,argv,"n:l:c:t:d:")) != -1) {
    
    switch (opt) {
    case 'n':
      nismmax = atoi(optarg);
      break;
    case 'l':
      lossprob = atof(optarg);
      break;
    case 'c':
      corruptprob = atof(optarg);
      break;
    case 't':
      lambda = atof(optarg);
      break;
    case 'd':
      if (atoi(optarg) >= 1) boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
      if (atoi(optarg) >= 2) boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
      if (atoi(optarg) >= 3) boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -n <messages to simulate> -l <prob of loss> -c <prob of corruption> -t <avg time between messages> -d <debug level>" << std::endl;
      exit(-1);
    }
  }

  simulation = new simulator(nismmax,lossprob,corruptprob,lambda);

  A_init();
  B_init();
  simulation->go();
}


std::ostream& operator<<(std::ostream& os, const struct msg& message)
{
   for (int i=0;i<20;i++) {
    os << message.data[i];
  }
  return os;

}

std::ostream& operator<<(std::ostream& os, const struct pkt& packet)
{
  os << "(seq = " << packet.seqnum;
  os << ", ack = " << packet.acknum;
  os << ", chk = " << packet.checksum << ") ";
  for (int i=0;i<20;i++) {
    os << packet.payload[i];
  }
  return os;
}
