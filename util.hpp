// from stack overflow
// http://stackoverflow.com/questions/3247861/example-of-uuid-generation-using-boost-in-c
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <zlib.h>
#include <iomanip>
#include <sstream>

#include "chat_message.hpp"

#define DEBUG_MODE true
#define CHECKSUM_VALIDATION true

#define TRUE 1
#define FALSE 0

/*
  This function uses the boost library to generate a uuid and
  returns it in as a string.
*/
std::string gen_uuid() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

/*
  inputs
    data to be sent with checksum in format
    time,majorCommand,optionalArguements(if present)
  outputs
    string of crc32 checksum in hex

*/
std::string createCheckSum(std::string data)
{
  std::stringstream buffer;
  unsigned int crc;

  crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, (const unsigned char*) data.c_str(), data.length());

  buffer << std::hex << crc << std::dec;
  return buffer.str();

}
/*
  The gen_crc32 function takes a string [data] as a parameter and generates a
  crc32 checksum based on the string.
  Returns the crc32 in the form of an unsigned integer.
*/
unsigned int gen_crc32(std::string data) {
  unsigned int crc;
   crc = crc32(0L, Z_NULL, 0);
   crc = crc32(crc, (const unsigned char*) data.c_str(), data.length()+1);
   return crc;
}

/*
  This function takes a vector of strings passed by reference [strs] to avoid
  copying and an integer [start]. It loops through the vector from the position
  indicated by [start] and combines all the remaining indexes into one line,
  string m. It returns m.
  This use of this function is so that a message sent by the server of client
  which is split on all spaces, can be repaired.
*/
std::string build_optional_line(std::vector<std::string> &strs, int start) {
  std::string m;
  for(unsigned int j = start; j < strs.size(); j++) {
    m += (strs[j]);
    if(j != strs.size()-1) {
      m += " ";
    }
  }
  return m;
}

/*
  This function takes a vector of strings passed by reference [strs] to avoid
  copying and an integer [start]. It loops through the vector from the position
  indicated by [start] and combines all the remaining indexes into one line,
  string m. It returns m.
  This use of this function is so that a message sent by the server of client
  which is split on all commas, can be repaired.
*/
std::string build_line_no_checksum (std::vector<std::string> const &strs, int start) {
  std::string m;
  for(unsigned int j = start; j < strs.size(); j++) {
    m += (strs[j]);
    if(j != strs.size()-1) {
      m += ",";
    }
  }
  return m;
}

/*
  checks checksum data from server
  inputs:
    data sent from server in format checksum,time,majorcommand,optional
    arguement(if present)
  output:
    TRUE (1) if checksum matches data
    FALSE (0) if checksum does not match data

*/
int checkCheckSum(std::string input)
{
  std::string generatedCheckSum;

  std::vector<std::string> args;
  boost::split(args, input, boost::is_any_of(","));
  for(unsigned int i = 0; i < args.size(); i++) {
    if(args[i] == "") {
      args.erase(args.begin()+i);
    }
  }
  std::string originalCheckSum(args[0]);
  std::string line = "," + build_line_no_checksum(args, 1);
  unsigned int chcksm = gen_crc32(line);

  std::stringstream sstream;
  sstream << std::hex << chcksm << "";
  generatedCheckSum = sstream.str();
  if(DEBUG_MODE) {
    //std::cout << line << std::endl;
    //std::cout << originalCheckSum << std::endl;
    //std::cout << generatedCheckSum << std::endl;
  }
  if((originalCheckSum.compare(generatedCheckSum)) == 0)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}
/*
  The format_request function takes a string [command] and another string [data]
  as parameters and builds a message to be sent by either the server or the client
  in the format specified by the requirements document.
  Returns a string [req]
*/
std::string format_request(std::string command, std::string data) {
  std::string tm = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());
  char cmd[chat_message::max_body_length + 1];
  char req[chat_message::max_body_length + 1];

  /*strcpy(cmd, ",");
  strcat(cmd, tm.c_str());
  strcat(cmd, ",");
  strcat(cmd, command.c_str());
  if(data != "") {
    strcat(cmd, ",");
    strcat(cmd, data.c_str());
  }*/
  std::string build = "," + tm + "," + command;
  if(data != "") {
    build += "," + data;
  }
  strcpy(cmd, build.c_str());
  unsigned int chcksm = gen_crc32(std::string(build));

  std::stringstream sstream;
  sstream << std::hex << chcksm << "";
  std::string result = sstream.str();

  strcpy(req, result.c_str());
  strcat(req, cmd);

  return std::string(req);
}
