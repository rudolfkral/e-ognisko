#ifndef EOGNISKO_UDP_MIXER_HPP
#define EOGNISKO_UDP_MIXER_HPP

// e-ognisko_udp_mixer.hpp

#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_server_client.hpp"
#include "e-ognisko_mixer.hpp"

class Udp_mixer
{
	public:
		Udp_mixer(boost::asio::io_service& _io_service,
				std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
				client_map_t& _client_map,
				size_t _buf_len,
				size_t _tx_interval);
		~Udp_mixer();

		void run();

		std::queue<std::string>* get_output_buf();
		size_t* get_output_buf_start();
	private:
		void handle_timer(const boost::system::error_code& _err);

		boost::asio::io_service& io_service_;
		std::weak_ptr<boost::asio::ip::udp::socket> udp_socket_;
		client_map_t& client_map_;
		size_t buf_len_, tx_interval_;
		std::queue<std::string> output_buf_;
		size_t output_buf_start_;
		boost::asio::deadline_timer timer_;

		const size_t TX_CONSTANT = 176;
};

#endif
