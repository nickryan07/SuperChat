// from stack overflow
// http://stackoverflow.com/questions/3247861/example-of-uuid-generation-using-boost-in-c
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include "boost/date_time/posix_time/posix_time.hpp"
#include <string>
#include <zlib.h>
#include <iomanip>
#include <sstream>

#include "chat_message.hpp"

#define TRUE 1
#define FALSE 0

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

unsigned int gen_crc32(std::string data) {
  unsigned int crc;
   crc = crc32(0L, Z_NULL, 0);
   crc = crc32(crc, (const unsigned char*) data.c_str(), data.length());
   return crc;
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
int checkCheckSum(char input[],int length)
{
  //put delim in the statement because of a warning
  char buffer[length];
  char data[length]; /*info without checksum*/
  int offset;

  std::string generatedCheckSum;
  std::strncpy(buffer,input,length);

  std::string originalCheckSum(std::strtok(buffer,","));

  offset = std::strlen(buffer) + 1;  /*num chars to move over + 1 term null*/
  strcpy(data,(buffer+offset));  /*get string time,majorcommand,optional*/


  generatedCheckSum = createCheckSum(data);

  if((originalCheckSum.compare(generatedCheckSum)) == 0)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }

}

std::string format_request(std::string command, std::string data) {
  //time_t tm = time(NULL);
  std::string tm = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());
  char cmd[chat_message::max_body_length + 1];
  char req[chat_message::max_body_length + 1];
  strcpy(cmd, ",");
  strcat(cmd, tm.c_str());
  strcat(cmd, ",");
  strcat(cmd, command.c_str());
  if(data != "") {
    strcat(cmd, ",");
    strcat(cmd, data.c_str());
  }
  unsigned int chcksm = gen_crc32(std::string(cmd));
  std::stringstream sstream;
  sstream << std::hex << chcksm << "";
  std::string result = sstream.str();
  strcpy(req, result.c_str());
  strcat(req, cmd);
  return std::string(req);
}
