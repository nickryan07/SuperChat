#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Menu_Bar.H>

#include "util.hpp"


using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

std::string users, rooms;

static void cb_recv (std::string S);
static void change_room (std::string S);

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator, void (*data_recv) (std::string S))
    : io_service_(io_service),
      socket_(io_service), data_recv_ (data_recv)
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
            std::string read_line = std::string(read_msg_.body()).substr(0, read_msg_.body_length());
            //std::cout << read_line <<" <------\n";
            if(checkCheckSum(read_line.c_str())) {
              std::vector<std::string> strs;
              boost::split(strs, read_line, boost::is_any_of(", "));
              for(unsigned int i = 0; i < strs.size(); i++) {
                if(strs[i] == "") {
                  strs.erase(strs.begin()+i);
                }
              }
              if(strs[2] == "REQTEXT" && strs.size() > 3) {
                std::vector<std::string> messages;
                std::string mess = read_line.substr(read_line.find("REQTEXT,")+8, read_line.length());
                boost::split(messages, mess, boost::is_any_of(";"));
                //std::cout << mess <<" <------\n";
                for(int i = 0; i < messages.size()-1; i++) {
                  std::string s = messages[i].substr(messages[i].find(" ")+1, messages[i].length());
                  //std::cout << s <<" <------\n";
                  data_recv_(s);
                  data_recv_("\n");
                }
              } else if(strs[2] == "CHANGECHATROOM" && strs.size() > 3) {
                std::string m = build_optional_line(strs, 3);
                change_room(m);
              } else if(strs[2] == "REQUSERS" && strs.size() > 3) {
                users = read_line.substr(read_line.find("REQUSERS,")+9, read_line.length());
              } else if(strs[2] == "REQCHATROOMS") {
                rooms = read_line.substr(read_line.find("REQCHATROOMS,")+13, read_line.length());
              }
            }
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
  void (*data_recv_)(std::string S);
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};
chat_client *c = NULL;
std::thread *t = NULL;
std::thread *t_polling = NULL;
// -------------------NICKNAMES------------------
void enter_nick();
void cancel_nick();
class Change_nick {
  public:
    Change_nick() {
      dialog_b = new Fl_Window(450,50, "Change Nickname");
      b_name = new Fl_Input(125, 15, 150, 25, "Name: ");
      b_name->align(FL_ALIGN_LEFT);

      b_add = new Fl_Return_Button(305, 15, 80, 25, "Change");
      b_add->callback((Fl_Callback *)enter_nick, 0);

      cancel_b = new Fl_Button(385, 15, 60, 25, "Cancel");
      cancel_b->callback((Fl_Callback *)cancel_nick, 0);
      dialog_b->end();
      dialog_b->set_non_modal();
    }
    void show() {dialog_b->show();}
    void hide() {dialog_b->hide();}
    std::string name() {return b_name->value();}
    std::string get_input() {
      return std::string(b_name->value());
    }
    void clear() {
      b_name->value("");
    }
  private:
    Fl_Window *dialog_b;
    Fl_Input *b_name;
    Fl_Return_Button *b_add;
    Fl_Button *cancel_b;
};
Change_nick *change_nick = new Change_nick;
void enter_nick() {
  chat_message msg;
  char req[chat_message::max_body_length + 1];
  std::string s = change_nick->get_input();
  strcpy(req, format_request("NICK", s).c_str());
  msg.body_length(std::strlen(req));
  std::memcpy(msg.body(), req, msg.body_length());
  msg.encode_header();
  c->write(msg);
  change_nick->clear();
  change_nick->hide();

}
void cancel_nick() {
  change_nick->hide();
}
////////////////////////////////////////

void open_nick() {
  change_nick->show();
}
// -------------------NICKNAMES------------------

// -------------------NAMEROOM-------------------
void enter_newroom();
void cancel_newroom();
class Add_room {
  public:
    Add_room() {
      dialog_b = new Fl_Window(450,50, "Create Room");
      b_name = new Fl_Input(125, 15, 150, 25, "Name: ");
      b_name->align(FL_ALIGN_LEFT);

      b_add = new Fl_Return_Button(305, 15, 80, 25, "Create");
      b_add->callback((Fl_Callback *)enter_newroom, 0);

      cancel_b = new Fl_Button(385, 15, 60, 25, "Cancel");
      cancel_b->callback((Fl_Callback *)cancel_newroom, 0);
      dialog_b->end();
      dialog_b->set_non_modal();
    }
    void show() {dialog_b->show();}
    void hide() {dialog_b->hide();}
    std::string name() {return b_name->value();}
    std::string get_input() {
      return std::string(b_name->value());
    }
    void clear() {
      b_name->value("");
    }
  private:
    Fl_Window *dialog_b;
    Fl_Input *b_name;
    Fl_Return_Button *b_add;
    Fl_Button *cancel_b;
};
Add_room *add_room = new Add_room;
void enter_newroom() {
  chat_message msg;
  char req[chat_message::max_body_length + 1];
  std::string s = add_room->get_input();
  strcpy(req, format_request("NAMECHATROOM", s).c_str());
  msg.body_length(std::strlen(req));
  std::memcpy(msg.body(), req, msg.body_length());
  msg.encode_header();
  c->write(msg);
  add_room->clear();
  add_room->hide();
}
void cancel_newroom() {
  add_room->hide();
}

void open_newroom() {
  add_room->show();
}
// -------------------NAMEROOM-------------------

// -------------------JOINROOM-------------------
void enter_joinroom();
void cancel_joinroom();
class Join_room {
  public:
    Join_room() {
      dialog_b = new Fl_Window(450,50, "Join Room");
      b_name = new Fl_Input(125, 15, 150, 25, "Name: ");
      b_name->align(FL_ALIGN_LEFT);

      b_add = new Fl_Return_Button(305, 15, 80, 25, "Join");
      b_add->callback((Fl_Callback *)enter_joinroom, 0);

      cancel_b = new Fl_Button(385, 15, 60, 25, "Cancel");
      cancel_b->callback((Fl_Callback *)cancel_joinroom, 0);
      dialog_b->end();
      dialog_b->set_non_modal();
    }
    void show() {dialog_b->show();}
    void hide() {dialog_b->hide();}
    std::string name() {return b_name->value();}
    std::string get_input() {
      return std::string(b_name->value());
    }
    void clear() {
      b_name->value("");
    }
  private:
    Fl_Window *dialog_b;
    Fl_Input *b_name;
    Fl_Return_Button *b_add;
    Fl_Button *cancel_b;
};
Join_room *join_room = new Join_room;
void enter_joinroom() {
  chat_message msg;
  char req[chat_message::max_body_length + 1];
  std::string s = join_room->get_input();
  strcpy(req, format_request("CHANGECHATROOM", s).c_str());
  msg.body_length(std::strlen(req));
  std::memcpy(msg.body(), req, msg.body_length());
  msg.encode_header();
  c->write(msg);
  join_room->clear();

  join_room->hide();
}
void cancel_joinroom() {
  join_room->hide();
}

void open_joinroom() { //Fl_Widget* w, void* p
  join_room->show();
}
// -------------------JOINROOM-------------------

// ------------------LISTUSERS-------------------
void request_listusers() {
  chat_message msg;
  char req[chat_message::max_body_length + 1];
  std::string s = add_room->get_input();
  strcpy(req, format_request("REQUSERS", "").c_str());
  msg.body_length(std::strlen(req));
  std::memcpy(msg.body(), req, msg.body_length());
  msg.encode_header();
  c->write(msg);
}
void list_users() {
  std::vector<std::string> messages;
  boost::split(messages, users, boost::is_any_of(",;"));
  std::string output;
  for(int i = 0; i < messages.size()-1; i+=2) {
    output += std::to_string(i)+".\t("+messages[i].substr(0, 7)+") Name: "+messages[i+1]+"\n";
  }
  fl_message(output.c_str());
}
// ------------------LISTUSERS-------------------

// ------------------LISTROOMS-------------------
void request_listrooms() {
  chat_message msg;
  char req[chat_message::max_body_length + 1];
  std::string s = add_room->get_input();
  strcpy(req, format_request("REQCHATROOMS", "").c_str());
  msg.body_length(std::strlen(req));
  std::memcpy(msg.body(), req, msg.body_length());
  msg.encode_header();
  c->write(msg);
}
void list_rooms() {
  std::vector<std::string> messages;
  boost::split(messages, rooms, boost::is_any_of(";"));
  std::string output;
  for(int i = 0; i < messages.size()-1; i++) {
    output += std::to_string(i)+".\t"+messages[i]+"\n";
  }
  fl_message(output.c_str());
}
// ------------------LISTROOMS-------------------

// -----------------MAIN WINDOW------------------

bool polling = true;
/*window (width,height,name*/
Fl_Window win(832, 539, "UberChat");
void cancel_command(Fl_Widget* w, void* p) {
    win.hide();
    polling = false;
}
Fl_Menu_Bar *menubar = new Fl_Menu_Bar(0, 0, 832, 30);
Fl_Menu_Item menuitems[15] = {
		{ "&File", 0, 0, 0, FL_SUBMENU },
 		{ "&Quit", FL_ALT + 'q', (Fl_Callback *)cancel_command },
 		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "&User", 0, 0, 0, FL_SUBMENU },
 		{ "&Change Name", 0, (Fl_Callback *)open_nick},// (Fl_Callback *)enter_nick },
 		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
 		{ "&Chat Room", 0, 0, 0, FL_SUBMENU },
 		{ "&Create Room", 0, (Fl_Callback *)open_newroom },
 		{ "&Change Room", 0, (Fl_Callback *)open_joinroom },
 		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ "&View", 0, 0, 0, FL_SUBMENU },
 		{ "&List Users", 0, (Fl_Callback *)list_users },
 		{ "&List Rooms", 0, (Fl_Callback *)list_rooms },
 		{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};
/*
  (coodinates x, y, width, height, label)
  0,0 is upper left hand corner
*/
Fl_Input input1(85, 500, 600, 20, "Message:");

Fl_Output *currentRoom = new Fl_Output(370,42,200,28,"Current Chatroom: ");
;
Fl_Button enter(700, 500, 80,20,"Enter");

Fl_Text_Buffer *buff = new Fl_Text_Buffer ();

Fl_Text_Display *disp = new Fl_Text_Display (10,80,800,400);
// -----------------MAIN WINDOW------------------
static void change_room (std::string S) {
  currentRoom->value(S.c_str());
  buff->text("");
}
static void cb_recv (std::string S)
{
  // Note, this is an async callback from the perspective
  // of Fltk..
  //
  // high chance of a lock needed here if certain fltk calls
  // are made.  (like show() .... )
  std::string T = S;// + '\0';
  if (buff)
  {
    buff->append ( T.c_str () );
  }
  if (disp)
  {
    disp->show ();
  }

  win.show ();
}

static void cb_quit ( )
{
  // this is where we exit to the operating system
  // any clean up needs to happen here
  //
  win.hide();
  polling = false;
  if (c)
  {
    c->close();
  }
  if (t)
  {
     t->join();
  }
  if(t_polling)
  {
    t_polling->join();
  }
}
static void cb_input1 (Fl_Input*, void * userdata)
{
  if(std::string(input1.value()) != ""
    && std::string(input1.value()).find(",") == std::string::npos
    && std::string(input1.value()).find(";") == std::string::npos) {
    chat_message msg;
    char req[chat_message::max_body_length + 1];
    strcpy(req, format_request("SENDTEXT", input1.value()).c_str());
    msg.body_length(std::strlen(req));
    std::memcpy(msg.body(), req, msg.body_length());
    msg.encode_header();
    c->write(msg);
    input1.value("");
  } else {
    //TODO : Warning message here
  }
}

void poll() {
    while(polling) {
      chat_message msg;
      char req[chat_message::max_body_length + 1];
      strcpy(req, format_request("REQTEXT", "").c_str());
      msg.body_length(std::strlen(req));
      std::memcpy(msg.body(), req, msg.body_length());
      msg.encode_header();
      c->write(msg);
      request_listrooms();
      request_listusers();
      Fl::check();
      usleep(400000);
    }
}

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
    c = new chat_client(io_service, endpoint_iterator, &cb_recv);

    t = new std::thread([&io_service](){ io_service.run(); });
    t_polling = new std::thread(static_cast<void(*)()>(poll));

    change_room("the lobby");
    chat_message uuid_req;
    char req[chat_message::max_body_length + 1];
    strcpy(req, format_request("REQUUID", "").c_str());
    uuid_req.body_length(std::strlen(req));
    std::memcpy(uuid_req.body(), req, uuid_req.body_length());
    uuid_req.encode_header();
    c->write(uuid_req);
    currentRoom->align(FL_ALIGN_LEFT);
    win.begin ();
    win.add (input1);
    input1.callback ((Fl_Callback*)cb_input1,( void *) "Enter next:");
    input1.when ( FL_WHEN_ENTER_KEY );
    enter.callback (( Fl_Callback*) cb_input1 );
    //clear.callback (( Fl_Callback*) cb_clear );
    win.add (enter);
    disp->buffer(buff);
    win.end();
    win.show();
    menubar->menu(menuitems);

    return Fl::run();
    c->close();
    t->join();
    t_polling->join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
