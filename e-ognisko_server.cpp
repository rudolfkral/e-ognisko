// e-ognisko_server.cpp

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_server.hpp"

size_t endpoint_hash(const boost::asio::ip::tcp::endpoint& e)
{
	return std::hash<std::string>()(e.address().to_string() + ":" +	 
			std::to_string(e.port()));
}

Server::Server(unsigned short _port,
		size_t _fifo_size,
		size_t _fifo_low_watermark,
		size_t _fifo_high_watermark,
		size_t _buf_len,
		size_t _tx_interval) :
	port_(_port),
	fifo_size_(_fifo_size),
	fifo_low_watermark_(_fifo_low_watermark),
	fifo_high_watermark_(_fifo_high_watermark),
	buf_len_(_buf_len),
	tx_interval_(_tx_interval),
	io_service_(),
	acceptor_(io_service_,
			boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(),
				port_)),
	acc_socket_(io_service_),
	udp_socket_(std::make_shared<boost::asio::ip::udp::socket>(io_service_, 
			boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(),
			       	port_))),
	client_map_(10, &endpoint_hash),
	udp_router_(io_service_, udp_socket_, client_map_),
	report_sender_(io_service_, client_map_),
	udp_mixer_(io_service_, udp_socket_, client_map_,
		       	buf_len_, tx_interval_)
{
}

Server::~Server()
{
	// TODO problems with closing?
	acc_socket_.close();
	udp_socket_->close();
}

void Server::run()
{
	// start accepting new TCP connections
	acceptor_.async_accept(acc_socket_,
	       	boost::bind(&Server::handle_accept, this, _1));
	report_sender_.run();
	udp_router_.run();
	udp_mixer_.run();
	io_service_.run();
}

void Server::handle_accept(const boost::system::error_code& _err)
{
	//printf("New client: address %s\n", acc_socket_.remote_endpoint().address().to_string().c_str());

	acc_socket_.set_option(boost::asio::ip::tcp::no_delay(true));
	
	client_map_.emplace(std::piecewise_construct,
			std::make_tuple(acc_socket_.remote_endpoint()), 
			std::forward_as_tuple(io_service_,
				std::move(acc_socket_), // move
				udp_socket_, // shared_ptr
				fifo_size_,
				fifo_low_watermark_,
				fifo_high_watermark_,
				udp_mixer_.get_output_buf(),
				udp_mixer_.get_output_buf_start(),
				client_map_));
	// now, prepare to accept new connections
	acceptor_.async_accept(acc_socket_,
			boost::bind(&Server::handle_accept, this, _1));
}

int main(int argc, char** argv)
{
	namespace po = boost::program_options;
	po::options_description desc("Options");
	po::variables_map vm;

	size_t port = 14680,
	       fifo_size = 10560,
	       fifo_low_watermark = 0,
	       fifo_high_watermark = 10560,
	       buf_len = 10,
	       tx_interval = 5;

	desc.add_options()
		("PORT,p", po::value<size_t>(&port), "numer portu, z którego korzysta serwer do komunikacji (zarówno TCP, jak i UDP), domyślnie 10000 + (numer_albumu % 10000); ustawiany parametrem -p serwera, opcjonalnie też klient")
		("FIFO_SIZE,F", po::value<size_t>(&fifo_size), "rozmiar w bajtach kolejki FIFO, którą serwer utrzymuje dla każdego z klientów; ustawiany parametrem -F serwera, domyślnie 10560")
		("FIFO_LOW_WATERMARK,L", po::value<size_t>(&fifo_low_watermark), "opis w treści; ustawiany parametrem -L serwera, domyślnie 0")
		("FIFO_HIGH_WATERMARK,H", po::value<size_t>(&fifo_high_watermark), "opis w treści; ustawiany parametrem -H serwera, domyślnie równy FIFO_SIZE")
		("BUF_LEN,X", po::value<size_t>(&buf_len), "rozmiar (w datagramach) bufora pakietów wychodzących, ustawiany parametrem -X serwera, domyślnie 10")
		("TX_INTERVAL,i", po::value<size_t>(&tx_interval), "czas (w milisekundach) pomiędzy kolejnymi wywołaniami miksera, ustawiany parametrem -i serwera; domyślnie: 5ms");

	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if(vm.count("FIFO_SIZE") && !vm.count("FIFO_HIGH_WATERMARK"))
	{
		fifo_high_watermark = fifo_size;
	}

	Server server(port, fifo_size, fifo_low_watermark, fifo_high_watermark, buf_len, tx_interval);
	server.run();
}
