#ifndef EOGNISKO_SERVER_HPP
#define EOGNISKO_SERVER_HPP

#include <boost/asio.hpp>
#include <memory>
#include "e-ognisko_server_client.hpp"
#include "e-ognisko_report_sender.hpp"
#include "e-ognisko_udp_router.hpp"
#include "e-ognisko_udp_mixer.hpp"

size_t endpoint_hash(const boost::asio::ip::udp::endpoint& e);

class Server
{
	public:
		explicit Server(unsigned short _port,
				size_t _fifo_size,
				size_t _fifo_low_watermark,
				size_t _fifo_high_watermark,
				size_t _buf_len,
				size_t _tx_interval);
		~Server();

		void run();
	private:
		void handle_accept(const boost::system::error_code& _err);

		unsigned short port_;
		size_t fifo_size_, fifo_low_watermark_, fifo_high_watermark_,
		       buf_len_, tx_interval_;
		boost::asio::io_service io_service_;
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket acc_socket_;
		std::shared_ptr<boost::asio::ip::udp::socket> udp_socket_;

		client_map_t client_map_;

		Udp_router udp_router_;

		Report_sender report_sender_;

		Udp_mixer udp_mixer_;
};

#endif
