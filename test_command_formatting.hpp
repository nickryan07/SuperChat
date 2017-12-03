#include <string>
#include <iostream>
#include <assert.h>


#include "util.hpp"

void test_formatting()
{
  std::string tm = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());
  std::string sample = ","+tm+",SENDTEXT,This is a test message.";

  std::string test = format_request_nochecksum(tm, "SENDTEXT", "This is a test message.");

  if(sample == test) {
    std::cout << "test_command_formatting: PASSED" << std::endl;
  } else {
    std::cout << "test_command_formatting: FAILED" << std::endl;
  }
}
