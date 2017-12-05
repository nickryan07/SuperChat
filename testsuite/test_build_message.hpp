#include <string>
#include <iostream>
#include <assert.h>


/*
  This function takes a vector of strings passed by reference [strs] to avoid
  copying and an integer [start]. It loops through the vector from the position
  indicated by [start] and combines all the remaining indexes into one line,
  string m. It returns m.
  This use of this function is so that a message sent by the server of client
  which is split on all spaces, can be repaired.
*/
std::string build_optional_line_space(std::vector<std::string> &strs, int start) {
  std::string m;
  for(unsigned int j = start; j < strs.size(); j++) {
    m += (strs[j]);
    if(j != strs.size()-1) {
      m += " ";
    }
  }
  return m;
}


void test_build_message()
{
  std::string expected = "this is a unit-test";
  std::vector<std::string> sample;
  sample.push_back("ffffffff");
  sample.push_back("123456.123456");
  sample.push_back("SENDTEXT");
  sample.push_back("this");
  sample.push_back("is");
  sample.push_back("a");
  sample.push_back("unit-test");

  std::string test = build_optional_line_space(sample, 3);




  if(expected == test) {
    std::cout << "test_build_message: PASSED" << std::endl;
  } else {
    std::cout << "test_build_message: FAILED" << std::endl;
  }
}
