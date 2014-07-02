#ifndef EOGNISKO_UDP_ROUTER_HPP
#define EOGNISKO_UDP_ROUTER_HPP

// e-ognisko_udp_router.hpp

#include <boost/asio.hpp>
#include <memory>
#include "e-ognisko_server_client.hpp"

size_t endpointmap_hash(const boost::asio::ip::udp::endpoint& e);

class Udp_router
{
	public:
		Udp_router(boost::asio::io_service& _io_service,
				std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
				client_map_t& _client_map);
		~Udp_router();

		void run();

	private:
		void handle_receive_from(const boost::system::error_code& _err, 
				size_t _bytes_transferred);

		static const size_t MSG_BUF_SIZE = 64 * 1024;

		boost::asio::io_service& io_service_;
		std::weak_ptr<boost::asio::ip::udp::socket> udp_socket_;
		client_map_t& client_map_;
		boost::asio::ip::udp::endpoint endpoint_;
		char msg_buf[MSG_BUF_SIZE]; 
		std::unordered_map<boost::asio::ip::udp::endpoint,
			boost::asio::ip::tcp::endpoint,
			size_t(*)(const boost::asio::ip::udp::endpoint&)> endpoint_map_;
};

#endif
