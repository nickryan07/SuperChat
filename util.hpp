// from stack overflow
// http://stackoverflow.com/questions/3247861/example-of-uuid-generation-using-boost-in-c
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <string>
#include <zlib.h>
#include <iomanip>
#include <sstream>

#include "chat_message.hpp"

std::string gen_uuid() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

unsigned int gen_crc32(std::string data) {
  unsigned int crc;
   crc = crc32(0L, Z_NULL, 0);
   crc = crc32(crc, (const unsigned char*) data.c_str(), data.length());
   return crc;
}

std::string format_request(std::string data) {
  time_t tm = time(NULL);
  char cmd[chat_message::max_body_length + 1];
  char req[chat_message::max_body_length + 1];
  sprintf(cmd, "%10d ", tm);
  strcat(cmd, data.c_str());
  unsigned int chcksm = gen_crc32(std::string(cmd));
  std::stringstream sstream;
  sstream << " " << std::hex << chcksm << " ";
  std::string result = sstream.str();
  strcpy(req, result.c_str());
  strcat(req, cmd);
  return std::string(req);
}
