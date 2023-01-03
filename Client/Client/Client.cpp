#include <iostream>
#include <map>
#include <thread>
#include <charconv>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <boost/thread/mutex.hpp>
#include <SDKDDKVER.h> 
#include "flatbuffers/flatbuffers.h"
#include "C2S_CHATECHO_REQ_generated.h"
#include "C2S_PID_REQ_generated.h"
#include "C2S_ROOM_ENTER_REQ_generated.h"
#include "S2C_CHATECHO_ACK_generated.h"
#include "S2C_CHATECHO_NTY_generated.h"
#include "S2C_PID_ACK_generated.h"
#include "S2C_ROOM_ENTER_ACK_generated.h"
#include "S2C_ROOM_ENTER_NTY_generated.h"


void MenuSelection();
void ChattingEcho();
void ChattingRoom();
void RecvChatNTY(char* buffer);
void RecvRoomNTY(char* buffer);
void RecvCharPid(char* buffer);
void RecvRoomACK(char* buffer);
void Char_Recv(char *tmpBuffer, const boost::system::error_code& ec);
void RecvChatACK(char* buffer);
void OnAccept(const boost::system::error_code& ec);

static CHAR IP[] = "127.0.0.1";
static CHAR Port[] = "3587";
#define BUF_SIZE 1024

boost::asio::io_context io_context;
std::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(io_context));
boost::asio::io_service::strand m_strand(io_context);
boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(IP), atoi(Port));
boost::asio::ip::tcp::socket hSocket(io_context, ep.protocol());
boost::thread_group threadGroup;
boost::system::error_code error;

bool mainLoop = false;
bool mainFinish = true;

int pid;

int MenuNum = 0;

enum Code
{
	NO_CHOICE,
	CHAT_NTY,
	ROOM_NTY,
	PID,
	ROOM_ACK,
	CHAT_ACK
};

enum RoomResult
{
	FAILED_ROOM,
	SUCCESSED_ROOM
};

std::map<int, void(*)(char*)> callbackMap =
{
	{1, RecvChatNTY},
	{2, RecvRoomNTY},
	{3, RecvCharPid},
	{4, RecvRoomACK},
	{5, RecvChatACK}
};

void WorkerThread()
{
	io_context.run();
}

INT main(int argc, char* argv[])
{
	for (int i = 0; i < 2; ++i) {
		threadGroup.create_thread(WorkerThread);
	} 
	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	hSocket.async_connect(ep, OnAccept);


	while (mainFinish)
	{
		while (mainLoop)
		{
			MenuSelection();
			std::cin >> MenuNum;
			std::cin.ignore();

			if (MenuNum == 1) // Chatting
			{
				mainLoop = false;
				ChattingEcho();
			}
			else if (MenuNum == 2) // Chatting Room
			{
				mainLoop = false;
				ChattingRoom();
			}
			else if (MenuNum == 0) // close Program
			{
				mainLoop = false;
				mainFinish = false;
			}
			else
			{
				std::cout << "Please Re-Enter" << std::endl;
			}
		}
	}
	std::cout << " [ACK] Close Client" << std::endl;

	return 0;
	threadGroup.join_all();
}

void OnAccept(const boost::system::error_code& ec)
{
	char input[3000] = { 0, };
	io_context.post(m_strand.wrap(boost::bind(Char_Recv, input, error))); 
}

void MenuSelection()
{
	std::cout << "Choice Menu( Character will be finish program)" << std::endl;
	std::cout << "1. Chatting Message" << std::endl;
	std::cout << "2. Chatting Room" << std::endl;
	std::cout << "0. Close Program" << std::endl;
}

void ChattingEcho()
{
	flatbuffers::FlatBufferBuilder builder;
	std::cout << "\nChatting Program " << std::endl;
	std::cout << "\nPlease enter the character : (-1 Exit) ";

	while (1)
	{
		char input[BUF_SIZE] = { 0, };
		memset(input, 0, BUF_SIZE);
		std::cin.getline(input, BUF_SIZE, '\n');
		std::cin.clear();

		if (input[0] == '-' && input[1] == '1')
		{
			MenuNum = 0;
			mainLoop = true;
			break;
		}
		if (input[0] == '\0') 
		{
			std::cout << "you Entered a blank text. Please re-enter" << std::endl;
			std::cout << "\nPlease enter the character : (-1 Exit) ";
		}
		else
		{
			int sendReturn;

			builder.Finish(CreateC2S_CHATECHO_REQ(builder, strlen(input) + 16, MenuNum, pid, builder.CreateString(input)));
			char tmpBuffer[3000] = { 0, };
			memcpy(&tmpBuffer, builder.GetBufferPointer(), builder.GetSize());

			hSocket.write_some(boost::asio::buffer(tmpBuffer), error);
			if (error)
			{
				sendReturn = 0;
				std::cout << "Failed to send the character" << std::endl;
			}
			else
			{
				sendReturn = 1;
				std::cout << "Seccessed to send the character" << std::endl;
			}
			auto c2sEchoReq = GetC2S_CHATECHO_REQ(builder.GetBufferPointer());

			std::cout << "\n [REQ] [send] " << "Total buffer Size: \"" << c2sEchoReq->size() << "\" Code: \"" << c2sEchoReq->code() << "\" Result: \"" << sendReturn << "\" , String: \"" << c2sEchoReq->msg()->c_str() << "\" from " << pid << " client " << std::endl;
		}
		builder.Clear();
	}
	std::cout << "\nChatting Program Closed" << std::endl;
}

void ChattingRoom() 
{
	flatbuffers::FlatBufferBuilder builder;
	std::cout << "\nChatting Room Program " << std::endl;
	std::cout << "\nPlease enter the chatting room No!: (-2 Exit, When you enter character / no enter will be room no. 0 ) ";

	char input[BUF_SIZE] = { 0, };
	memset(input, 0, BUF_SIZE);
	std::cin.getline(input, BUF_SIZE, '\n');
	std::cin.clear();

	int roomNo;
	if (strlen(input) == 0)
		roomNo = 0; 
	else
		roomNo = atoi(input);

	if (roomNo == -2)  
	{
		MenuNum = 0;
		std::cout << "\nChatting room closed" << std::endl;
		mainLoop = true;
	}
	else // check the room no in the server
	{
		builder.Finish(CreateC2S_ROOM_ENTER_REQ(builder, 16, 4, roomNo, pid));
		char tmpBuffer[BUF_SIZE] = { 0, };
		memcpy(&tmpBuffer, builder.GetBufferPointer(), builder.GetSize());
		hSocket.write_some(boost::asio::buffer(tmpBuffer), error);
	}
}

void RecvChatNTY(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cEchoNty = GetS2C_CHATECHO_NTY(buffer);

	std::cout << "\n [NTY] [recv] msg received. Total Buffer size : \"" << s2cEchoNty->size() << "\", Code: \"" << s2cEchoNty->code()  << "\" , string: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << s2cEchoNty->userIdx() << "\" Client" << std::endl;
	if (MenuNum == 1)
		std::cout << "\n please enter the string: (-1 Exit) ";
	else if(MenuNum == 2)
		std::cout << "\n please enter the chatting room number!: (-2 Exit) ";
	else
		MenuSelection();
	mainLoop = true;

}

void RecvChatACK(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cEchoACK = GetS2C_CHATECHO_ACK(buffer);

	std::cout << "\n [ACK] [recv] msg received. Total Buffer size : \"" << s2cEchoACK->size() << "\", Code: \"" << s2cEchoACK->code() << "\" , string: \"" << s2cEchoACK->msg()->c_str() << "\" , Result: \"" << s2cEchoACK->result() << "\" Client" << std::endl;
	if (MenuNum == 1)
		std::cout << "\n please enter the string: (-1 Exit) ";
	else if (MenuNum == 2)
		std::cout << "\n please enter the chatting room number!: (-2 Exit) ";
	else
		MenuSelection();
	mainLoop = true;
}

void RecvRoomNTY(char* buffer)
{
	auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(buffer);

	std::cout << "\n [NTY] [recv] msg received. Total Buffer size : \"" << s2cRoomNty->size() << "\", Code: \"" << s2cRoomNty->code() << "\", Room No: \"" << s2cRoomNty->roomNo() << "\" from server \"" << s2cRoomNty->userIdx() << "\" Client" << std::endl;
	
	if (MenuNum == 1)
		std::cout << "\n please enter the string: (-1 Exit) ";
	else if (MenuNum == 2)
		std::cout << "\n please enter the chatting room number!: (-2 Exit) ";
	else
		MenuSelection();
	mainLoop = true;
}

void RecvCharPid(char* buffer)
{
	auto s2cPidAck = GetS2C_PID_ACK(buffer);
	pid = s2cPidAck->pid();
	std::cout << "\n [ACK] User No. : " << pid << std::endl;
}

void RecvRoomACK(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cRoomAck = GetS2C_ROOM_ENTER_ACK(buffer);

	if (s2cRoomAck->result() == 0)
	{
		std::cout << " [ACK] No room. please re-enter the room No " << std::endl;
	}
	else
	{
		std::cout << "\n [ACK] [recv] msg received. Total Buffer Size: \"" << s2cRoomAck->size() << "\", Code: \"" << s2cRoomAck->code() << "\" , Room No: \"" << s2cRoomAck->roomNo() << "\" , Result: \"" << s2cRoomAck->result() << std::endl;
	}

	if (MenuNum == 1)
		std::cout << "\n please enter the string: (-1 Exit) ";
	else if (MenuNum == 2)
		std::cout << "\n please enter the chatting room number!: (-2 Exit)  ";
	else
		MenuSelection();
	mainLoop = true;
}

void Char_Recv(char* tmpBuffer, const boost::system::error_code& ec)
{
	hSocket.async_read_some(boost::asio::buffer(tmpBuffer, 3000), m_strand.wrap(boost::bind(Char_Recv, tmpBuffer, ec)));
	if (ec)
	{
		throw boost::system::system_error(ec);
	}
	else if (tmpBuffer[0] == 0 && tmpBuffer[1] == 0 && tmpBuffer[2] == 0)
	{
	}
	else
	{
		auto s2cPidAck = GetS2C_PID_ACK(tmpBuffer);
		switch (s2cPidAck->code())
		{
		case Code::NO_CHOICE:
			break;
		case Code::CHAT_NTY:
			callbackMap[CHAT_NTY](tmpBuffer);
			break;
		case Code::ROOM_NTY:
			callbackMap[ROOM_NTY](tmpBuffer);
			break;
		case Code::PID:
			callbackMap[PID](tmpBuffer);
			mainLoop = true;
			break;
		case Code::ROOM_ACK:
			callbackMap[ROOM_ACK](tmpBuffer);
			break;
		case Code::CHAT_ACK:
			callbackMap[CHAT_ACK](tmpBuffer);
			break;
		}
	}
}
