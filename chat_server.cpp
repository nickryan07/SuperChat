//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <utility>
#include <boost/algorithm/string.hpp>
#include <string>
#include <boost/asio.hpp>
#include "chat_message.hpp"
#include "util.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  void set_uuid(std::string str) {
    uuid = str;
  }
  void set_name(std::string str) {
    name = str;
  }
  std::string get_uuid() {
    return uuid;
  }
  std::string get_name() {
    return name;
  }
  std::string get_room() {
    return room;
  }
  void set_room(std::string str) {
    room = str;
    std::cout << uuid << " joined: " << room << std::endl;
  }
private:
  std::string name;
  std::string uuid;
  std::string room = "the lobby";
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------


//----------------------------------------------------------------------

class chat_room
{
public:
  chat_room(const char* nm) : name{nm} { create_room(std::string(name)); };
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    /*for (auto msg: recent_msgs_)
      participant->deliver(msg);*/
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void create_room(std::string room_name) {
    std::vector<std::string> empty_vector;
    sub_rooms[room_name].swap(empty_vector);
    chat_message_queue recent_msgs_;
    msg_queue_[room_name] = recent_msgs_;
    std::cout << room_name << ": created" << std::endl;
  }

  bool check_room(std::string name_to_check) {
    if (sub_rooms.find(name_to_check) != sub_rooms.end()) {
      return true;
    } else {
      return false;
    }
  }

  void join_room(chat_participant_ptr part, std::string name_to_check) {
    if(check_room(name_to_check)) {
      part->set_room(name_to_check);
      sub_rooms[name_to_check].push_back(part->get_uuid());
      for (auto msg: msg_queue_[name_to_check])
        part->deliver(msg);
    }
  }

  void update_messages(chat_participant_ptr part) {
    for (auto msg: msg_queue_[part->get_name()])
      part->deliver(msg);
  }

  void deliver(chat_participant_ptr part, const chat_message& msg)
  {
    std::string rm = part->get_room();
    msg_queue_[rm].push_back(msg);
    while (msg_queue_[rm].size() > max_recent_msgs)
      msg_queue_[rm].pop_front();

    for (auto participant: participants_)
      if(part->get_room() == participant->get_room())
        participant->deliver(msg);
  }
  void reply(chat_participant_ptr part, const chat_message& msg) {
    part->deliver(msg);
  }
  std::string list_users(chat_participant_ptr partic) {
    std::string list;
    for (auto part: participants_) {
      if(partic->get_room() == part->get_room()) {
        list += (part->get_uuid()+ "," + part->get_name() + ";");
      }
    }
    return list;
  }
  std::string list_rooms() {
    std::string list;
    for (const auto &str : sub_rooms ) {
      list += (str.first+";");
    }
    return list;
  }
  bool check_name(chat_participant_ptr part, std::string name_to_check) {
    for (auto part: participants_) {
      if(part->get_name() == name_to_check) {
        return true;
      }
    }
    return false;
  }
  std::string get_name() {
    return name;
  }

private:
  std::string name;
  std::set<chat_participant_ptr>  participants_;
  std::map<std::string, std::vector<std::string>> sub_rooms;
  enum { max_recent_msgs = 10000 };
  std::map<std::string, chat_message_queue> msg_queue_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    room_.join_room(shared_from_this(), "the lobby");
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::string read_line = std::string(read_msg_.body()).substr(0, read_msg_.body_length());
            std::cout << read_line << std::endl;
            std::vector<std::string> strs;
            boost::split(strs, read_line, boost::is_any_of(", "));
            for(int i = 0; i < strs.size(); i++) {
              if(strs[i] == "") {
                strs.erase(strs.begin()+i);
              }
            }
            if(strs[2] == "MYUUID") {
              std::cout << shared_from_this()->get_uuid() << std::endl;
            } else if(strs[2] == "REQCHATROOM") {
              std::string s = shared_from_this()->get_room();
              s = format_request("REQCHATROOM", s);
              char response[chat_message::max_body_length + 1];
              std::strcpy(response, s.c_str());
              chat_message res;
              res.body_length(std::strlen(response));
              std::memcpy(res.body(), response, res.body_length());
              res.encode_header();
              room_.reply(shared_from_this(), res);
            } else if(strs[2] == "REQUUID") {
              std::string s = gen_uuid();
              shared_from_this()->set_uuid(s);
              std::cout << shared_from_this()->get_uuid() << ": Connected" << std::endl;
              char response[chat_message::max_body_length + 1];
              s = format_request("REQUUID", s);
              std::strcpy(response, s.c_str());
              chat_message res;
              res.body_length(std::strlen(response));
              std::memcpy(res.body(), response, res.body_length());
              res.encode_header();
              room_.reply(shared_from_this(), res);
            } else if(strs[2] == "NICK") {
              std::string m;
              for(int j = 3; j < strs.size(); j++) {
                m += (strs[j]);
                if(j != strs.size()-1) {
                  m += " ";
                }
              }
              if(!room_.check_name(shared_from_this(), m)) {
                shared_from_this()->set_name(m);
                char response[chat_message::max_body_length + 1];
                std::string s = format_request("NICK", m);
                std::strcpy(response, s.c_str());
                chat_message res;
                res.body_length(std::strlen(response));
                std::memcpy(res.body(), response, res.body_length());
                res.encode_header();
                room_.reply(shared_from_this(), res);
                //std::cout << shared_from_this()->get_uuid() << ": " << shared_from_this()->get_name() << std::endl;
              } else {
                //TODO: Modify to ensure uniqueness if necessary
              }
            } else if(strs[2] == "SENDTEXT") {
              if(shared_from_this()->get_room() != "") {
                std::string m;
                for(int j = 3; j < strs.size(); j++) {
                  m += (strs[j]);
                  if(j != strs.size()-1) {
                    m += " ";
                  }
                }
                char response[chat_message::max_body_length + 1];
                std::strcpy(response, m.c_str());
                chat_message res;
                res.body_length(std::strlen(response));
                std::memcpy(res.body(), response, res.body_length());
                res.encode_header();
                room_.deliver(shared_from_this(), res);
              }
            } else if(strs[2] == "NAMECHATROOM") {//if(read_line.find("<NICK>") != std::string::npos) {
              std::string m;
              for(int j = 3; j < strs.size(); j++) {
                m += (strs[j]);
                if(j != strs.size()-1) {
                  m += " ";
                }
              }
              if(!room_.check_room(m)) {
                room_.create_room(m);
                char response[chat_message::max_body_length + 1];
                std::string s = format_request("NAMECHATROOM", m);
                std::strcpy(response, s.c_str());
                chat_message res;
                res.body_length(std::strlen(response));
                std::memcpy(res.body(), response, res.body_length());
                res.encode_header();
                room_.reply(shared_from_this(), res);
              }
            } else if(strs[2] == "CHANGECHATROOM") {//if(read_line.find("<NICK>") != std::string::npos) {
              std::string m;
              for(int j = 3; j < strs.size(); j++) {
                m += (strs[j]);
                if(j != strs.size()-1) {
                  m += " ";
                }
              }
              if(room_.check_room(m)) {
                room_.join_room(shared_from_this(), m);
                char response[chat_message::max_body_length + 1];
                std::string s = format_request("CHANGECHATROOM", m);
                std::strcpy(response, s.c_str());
                chat_message res;
                res.body_length(std::strlen(response));
                std::memcpy(res.body(), response, res.body_length());
                res.encode_header();
                room_.reply(shared_from_this(), res);
              }
            } else if(strs[2] == "REQUSERS") {
              std::string users = room_.list_users(shared_from_this());
              char response[chat_message::max_body_length + 1];
              std::string s = format_request("REQUSERS", users);
              std::strcpy(response, s.c_str());
              chat_message res;
              res.body_length(std::strlen(response));
              std::memcpy(res.body(), response, res.body_length());
              res.encode_header();
              room_.reply(shared_from_this(), res);
            } else if(strs[2] == "REQCHATROOMS") {
              std::string rooms = room_.list_rooms();
              char response[chat_message::max_body_length + 1];
              std::string s = format_request("REQCHATROOMS", rooms);
              std::strcpy(response, s.c_str());
              chat_message res;
              res.body_length(std::strlen(response));
              std::memcpy(res.body(), response, res.body_length());
              res.encode_header();
              room_.reply(shared_from_this(), res);
            } else if(strs[2] == "REQTEXT") {
              room_.update_messages(shared_from_this());
              char response[chat_message::max_body_length + 1];
              std::string s = format_request("REQTEXT", "");
              std::strcpy(response, s.c_str());
              chat_message res;
              res.body_length(std::strlen(response));
              std::memcpy(res.body(), response, res.body_length());
              res.encode_header();
              room_.reply(shared_from_this(), res);
            }
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket_), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_ {"the lobby"};
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_service, endpoint);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
