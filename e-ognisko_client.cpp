// e-ognisko_client.cpp

#include <string>
#include <ctime>
#include <memory>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdio>
#include "e-ognisko_protocol.hpp"

std::string SERVER = "::1";
size_t PORT = 14680;
boost::asio::ip::address serv_addr;
const size_t UDP_BUF_SIZE = 1024;
const boost::posix_time::milliseconds KEEPALIVE_TIMEOUT = boost::posix_time::milliseconds(100);

boost::asio::io_service io_service;
eog::client_id_t id;
boost::asio::deadline_timer keepalive_timer(io_service);
boost::asio::ip::tcp::socket tcp_socket(io_service, boost::asio::ip::tcp::v6());
boost::asio::ip::udp::socket udp_socket(io_service, boost::asio::ip::udp::v6());
boost::asio::posix::stream_descriptor input(io_service, ::dup(STDIN_FILENO));
boost::asio::posix::stream_descriptor output(io_service, ::dup(STDOUT_FILENO));
std::string last_package; // should be empty when server sends ack
size_t to_send = 0, to_get = 0, win = 0; // all previous to to_send have been ack'd
boost::asio::streambuf tcp_stream, stdin_stream;
char udp_buf[UDP_BUF_SIZE];
bool sending_upload = false;

void handle_connect(const boost::system::error_code& err);
void handle_tcp_read_until(const boost::system::error_code& err,
		size_t bytes_transferred);
void handle_udp_connect(const boost::system::error_code& err);
void handle_udp_hello(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg);
void handle_read_stdin(const boost::system::error_code& err,
		size_t bytes_transferred);
void handle_write_stdout(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg);
void handle_upload();
void handle_send_upload(const boost::system::error_code& err,
		size_t bytes_transferred);
void handle_udp_receive(const boost::system::error_code& err,
		size_t bytes_transferred);
void handle_udp_keepalive(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg);
void process_data(size_t bytes_transferred);
void process_ack(size_t bytes_transferred);
void handle_timer(const boost::system::error_code& err);
int main(int argc, char** argv);

void handle_connect(const boost::system::error_code& err)
{
	if(err)
	{
		//fprintf(stderr, "Connection refused.\n");
		return;
	}
	
	boost::asio::async_read_until(tcp_socket, tcp_stream, '\n',
			&handle_tcp_read_until);
}

void handle_tcp_read_until(const boost::system::error_code& err,
		size_t bytes_transferred)
{
	if(err)
	{
		//fprintf(stderr, "tcp_read_until error!\n");
		return;
	}

	std::istream is(&tcp_stream);
	std::string msg;
	std::getline(is, msg);

	eog::stream_type_t tcptype = eog::ident_TCP_msg(msg);
	if(tcptype != eog::stream_type_t::STREAM_TYPE_HELLO)
	{
		//fprintf(stderr, "report encountered?\n");
		return;
	}

	id = eog::decode_TCP_hello(msg);
	//fprintf(stderr, "Got ID %lu\n", id);

	boost::asio::ip::udp::endpoint udp_endp(serv_addr, PORT);
	udp_socket.async_connect(udp_endp, &handle_udp_connect);
}

void handle_udp_connect(const boost::system::error_code& err)
{
	if(!err)
	{
		//fprintf(stderr, "UDP connected.\n");

		std::shared_ptr<std::string> msg_hello =
			std::make_shared<std::string>(eog::encode_UDP_hello(id));
		
		udp_socket.async_send(boost::asio::buffer(*msg_hello),
				boost::bind(&handle_udp_hello, _1, _2, msg_hello));
	}
}

void handle_udp_hello(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg)
{
	if(!err)
	{
		//fprintf(stderr, "UDP HELLO succesfully sent.\n");
		//std::shared_ptr<std::string> read_stdin =
		//	std::make_shared<std::string>();
		//boost::asio::async_read(input,
		//		stdin_stream,
		//		boost::asio::transfer_exactly(std::min(eog::DATAGRAM_SIZE,
		//				win)), // too big?
		//		&handle_read_stdin);
		
		udp_socket.async_receive(boost::asio::buffer(udp_buf, UDP_BUF_SIZE),
				&handle_udp_receive);
		keepalive_timer.expires_from_now(KEEPALIVE_TIMEOUT);
		keepalive_timer.async_wait(&handle_timer);
	}
}

void handle_upload()
{
	if(stdin_stream.size() == 0)
	{
		boost::asio::async_read(input, stdin_stream, &handle_read_stdin);
		return;
	}

	//fprintf(stderr, "Preparing upload package nr %lu.\n", to_send);

	size_t bytes = std::min(stdin_stream.size(), win);

	//fprintf(stderr, "Exactly %lu bytes.\n", bytes);

	char buff[bytes]; // store all data
	std::istream is(&stdin_stream);
	is.read(buff, bytes); // jaki delimiter

	if(is.gcount() != bytes)
	{
		//fprintf(stderr, "Extracted insufficient data from stdin...\n");
	}

	eog::UDP_upload_t up;
	up.data = &buff;
	up.data_nr = to_send;
	up.data_len = is.gcount();
	
	last_package = eog::encode_UDP_upload(&up);

	//fprintf(stderr, "UPLOAD contents: \n%s\n", last_package.c_str());
	//fprintf(stderr, "UPLOAD length: %lu\n", last_package.length());
	
	udp_socket.async_send(boost::asio::buffer(last_package),
			&handle_send_upload);
}

void handle_read_stdin(const boost::system::error_code& err,
		size_t bytes_transferred)
{
	//if(bytes_transferred == 0)
	//{
		// retry
		//boost::asio::async_read(input,
		//		stdin_stream,
		//		boost::asio::transfer_exactly(std::min(eog::DATAGRAM_SIZE,
		//				win)), // too big?
		//		&handle_read_stdin);
		//boost::asio::async_read(input, stdin_stream, &handle_read_stdin);
		//		
		//return;

	//}
	if(!err || err == boost::asio::error::eof)
	{
		//fprintf(stderr, "READ %lu bytes.\n", bytes_transferred);
		//fprintf(stderr, "Preparing upload package nr %lu.\n", to_send);
		//
		//size_t bytes = std::min(bytes_transferred, win);
		//
		//char buff[bytes]; // store all data
		//std::istream is(&stdin_stream);
		//is.read(buff, bytes); // jaki delimiter

		//if(is.gcount() != bytes)
		//{
		//	fprintf(stderr, "Extracted insufficient data from stdin...\n");
		//}

		//eog::UDP_upload_t up;
		//up.data = &buff;
		//up.data_nr = to_send;
		//up.data_len = is.gcount();
		
		//last_package = eog::encode_UDP_upload(&up);

		//fprintf(stderr, "UPLOAD contents: \n%s\n", last_package.c_str());
		//fprintf(stderr, "UPLOAD length: %lu\n", last_package.length());
		
		//udp_socket.async_send(boost::asio::buffer(last_package),
		//		&handle_send_upload);

		handle_upload();
	} else if(err != boost::asio::error::operation_aborted)
	{
		//fprintf(stderr, "error occured when reading from stdin\n");
	}
}

void handle_udp_receive(const boost::system::error_code& err,
		size_t bytes_transferred)
{
	if(err)
	{
		//fprintf(stderr, "Connection aborted.\n");
		udp_socket.cancel();
		tcp_socket.cancel();
		io_service.stop();
		return;
	}
	eog::dgram_type_t dgram_type = eog::ident_UDP_msg(udp_buf);
	switch(dgram_type)
	{
		case eog::dgram_type_t::DGRAM_TYPE_ACK:
			//fprintf(stderr, "got ACK datagram.\n");
			process_ack(bytes_transferred);
			break;
		case eog::dgram_type_t::DGRAM_TYPE_DATA:
			//fprintf(stderr, "got DATA datagram.\n");
			process_data(bytes_transferred);
			break;
		default:
			break;
	}

	udp_buf[0] = '\0'; // empty the buffer?

	udp_socket.async_receive(boost::asio::buffer(udp_buf, UDP_BUF_SIZE),
			&handle_udp_receive);

}

void handle_send_upload(const boost::system::error_code& err,
		size_t bytes_transferred)
{
	//fprintf(stderr, "UPLOAD successfully sent(%lu bytes). Incrementing to_send.\n",
	//		bytes_transferred);
	if(err)
		return;
	to_send++;
	sending_upload = false;
}

void process_data(size_t bytes_transferred)
{
	eog::UDP_data_t data;
	eog::decode_UDP_data(std::string(udp_buf, bytes_transferred), &data);
	//fprintf(stderr, "DATA(%lu bytes): %s\n", bytes_transferred,
	//		std::string(udp_buf, bytes_transferred).c_str());
	//fprintf(stderr, "Got DATA. Length: %lu bytes.\n", bytes_transferred);
	// retransmisja gdy dwa razy ten sam ack
	// i to jest do obsługi normalnego flow
	win = data.win;
	//fprintf(stderr, "WINDOW: %lu\n", data.win);
	if(data.data_nr != to_get)
	{
		//fprintf(stderr, "Wrong DATA nr, got %lu, anticipated %lu. Ignoring.\n",
		//		data.data_nr, to_get);
		return;
	}
	to_get++;

	if(to_send != data.ack_nr)
	{
		//fprintf(stderr, "Wrong DATA ack, got %lu, anticipated %lu.\n", data.ack_nr, to_send);
	} else
	{
		// server is ready to get new datagram, if we aren't
		// already sending it after ACK message
		if(!sending_upload)
		{
			sending_upload = true;
			//boost::asio::async_read(input,
			//		stdin_stream,
			//		boost::asio::transfer_exactly(std::min(eog::DATAGRAM_SIZE,
			//				win)), // too big?
			//		&handle_read_stdin);
			//boost::asio::async_read(input, stdin_stream, &handle_read_stdin);
			handle_upload();
		}
	}


	//fprintf(stderr, "Printing DATA(begin= %lu, len = %lu).\n", data.data_begin, data.data_len);

	std::shared_ptr<std::string> to_write = 
		std::make_shared<std::string>(udp_buf + data.data_begin, data.data_len);

	//fprintf(stderr, "to_write(length %lu): %s\n", to_write->length(), to_write->c_str());

	boost::asio::async_write(output,
			boost::asio::buffer(*to_write),
		       boost::bind(&handle_write_stdout, _1, _2, to_write));	
}

void process_ack(size_t bytes_transferred)
{
	eog::UDP_ack_t sack;
	eog::decode_UDP_ack(std::string(udp_buf, bytes_transferred), &sack);
	win = sack.win;
	if(to_send != sack.ack_nr)
	{
		//fprintf(stderr, "Wrong ACK ack, got %lu, anticipated %lu.\n", sack.ack_nr, to_send);
	} else
	{
		// server is ready to get new datagram, if we aren't
		// already sending it after ACK message
		if(sending_upload)
			return;
		sending_upload = true;
		//boost::asio::async_read(input,
		//		stdin_stream,
		//		boost::asio::transfer_exactly(std::min(eog::DATAGRAM_SIZE,
		//				win)), // too big?
		//		&handle_read_stdin);
		//boost::asio::async_read(input, stdin_stream, &handle_read_stdin);
		handle_upload();
	}


}

void handle_write_stdout(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg)
{
	if(err && err != boost::asio::error::operation_aborted)
	{
		//fprintf(stderr, "Error occured.\n");
	} else
	{
		//fprintf(stderr, "Written %lu bytes.\n", bytes_transferred);
	}
}

void handle_udp_keepalive(const boost::system::error_code& err,
		size_t bytes_transferred, std::shared_ptr<std::string> msg)
{
}

void handle_timer(const boost::system::error_code& err)
{
	// check err
	if(err)
	{
		if(err != boost::asio::error::operation_aborted)
			//fprintf(stderr, "Connection aborted.\n");
		return;
	}
	std::shared_ptr<std::string> msg =
		std::make_shared<std::string>(eog::encode_UDP_keepalive());

	udp_socket.async_send(boost::asio::buffer(*msg),
			boost::bind(&handle_udp_keepalive, _1, _2, msg));
	keepalive_timer.expires_from_now(KEEPALIVE_TIMEOUT);
	keepalive_timer.async_wait(&handle_timer);
}

int main(int argc, char** argv)
{
	namespace po = boost::program_options;
	po::options_description desc("Options");
	po::variables_map vm;

	size_t retransmit_limit = 5;

	desc.add_options()
		("PORT,p", po::value<size_t>(&PORT), "numer portu, z którego korzysta serwer do komunikacji (zarówno TCP, jak i UDP), domyślnie 10000 + (numer_albumu % 10000); ustawiany parametrem -p serwera, opcjonalnie też klient")
		("SERVER_NAME,s", po::value<std::string>(&SERVER), "nazwa lub adres IP serwera, z którym powinien połączyć się klient, ustawiany parametrem -s klienta")
		("RETRANSMIT_LIMIT,X", po::value<size_t>(&retransmit_limit), "opis w treści; ustawiany parametrem -X klienta, domyślnie 10");

	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	serv_addr = boost::asio::ip::address::from_string(SERVER);

	tcp_socket.set_option(boost::asio::ip::tcp::no_delay(true));
	boost::asio::ip::tcp::endpoint tcp_endp(serv_addr, PORT);
	tcp_socket.async_connect(tcp_endp,  &handle_connect);
	io_service.run();
}
