#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "chat_message.hpp"
#include <ctime>
#include <zlib.h>
#include <string>
#include <iostream>

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;
#define TRUE 1
#define FALSE 0

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
    do_connect(endpoint_iterator);
  }

  void write(const chat_message& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }
  /*
    sends data with checksum
    input:
    string command
    string optional data to go with command

  */
  void sendCheckSum (char *command, char *data = NULL)    
  {
    /*Tests*/
    command = "nickname"; 
    data ="bob";

    time_t now = time(0);    /*getting time*/
    tm *dateTime =localtime(&now);
    
    std::string time = std::to_string(dateTime->tm_hour) + ':' + std::to_string(dateTime->tm_min) + ':' + std::to_string(dateTime->tm_sec);    /*putting time in appropriate format*/ 
    
    std::string message; /*complete message with checksum to be sent*/
    
    if(data == NULL) /*optional data not present*/
    {
      message = time + ',' + command; 
    }
    else
    {
      message = time + ',' + command + ',' + data;
    }

    message =  createCheckSum(message) + ',' + message;
    
    size_t request_length = message.length();      
  
    boost::asio::write(socket_,boost::asio::buffer(message, request_length));
    /*send message^*/
    char reply[request_length];

    size_t reply_length = boost::asio::read(socket_,
    boost::asio::buffer(reply,request_length));

    if((checkCheckSum(reply,reply_length) == TRUE))
    {
       std::cout << "checkSum matches data received\n";  
    }
    else   
    {
        std::cout << "checkSum does not matches data recieved\n";
    }

    std::cout << "Reply is: ";
    std::cout.write(reply,reply_length);
    std::cout << "\n";


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
    char *delimiter = ",";
    char buffer[length];
    char data[length]; /*info without checksum*/
    int offset;

    std::string generatedCheckSum;
    std::strncpy(buffer,input,length);
    
    std::string originalCheckSum(std::strtok(buffer,delimiter)); 

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

private:
  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            socket_.close();
          }
        });
  }
  
private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};
    


int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);

    auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
    
    chat_client c(io_service, endpoint_iterator);

    std::thread t([&io_service](){ io_service.run(); });

    c.sendCheckSum("a");    /*ADDED*/

    char line[chat_message::max_body_length + 1];    /*holds written text*/
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg;
      msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

