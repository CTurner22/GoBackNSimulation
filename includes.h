#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>


#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

#define TRACE BOOST_LOG_TRIVIAL(trace) 
#define DEBUG BOOST_LOG_TRIVIAL(debug)  
#define INFO BOOST_LOG_TRIVIAL(info)  
#define WARNING BOOST_LOG_TRIVIAL(warning) 
#define ERROR BOOST_LOG_TRIVIAL(error) 
#define FATAL BOOST_LOG_TRIVIAL(fatal)  
#define ENDL  " (" << __FILE__ << ":" << __LINE__ << ")"

#include "simulator.h"
#include "main.h"
#include "GoBackN.h"
