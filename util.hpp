// from stack overflow
// http://stackoverflow.com/questions/3247861/example-of-uuid-generation-using-boost-in-c
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <string>
#include <zlib.h>

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
