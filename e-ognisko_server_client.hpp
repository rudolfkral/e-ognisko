#ifndef EOGNISKO_SERVER_CLIENT_HPP
#define EOGNISKO_SERVER_CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <memory>
#include <string>
#include <queue>
#include <unordered_map>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_fifo_man.hpp"


class Client;

typedef std::unordered_map<boost::asio::ip::tcp::endpoint, 
		Client,
		size_t(*)(const boost::asio::ip::tcp::endpoint&)
		> client_map_t;

class Client
{
	public:
		Client(boost::asio::io_service& _io_service,
				boost::asio::ip::tcp::socket _tcp_socket,
				std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
				size_t _fifo_size,
				size_t _fifo_low_watermark,
				size_t _fifo_high_watermark,
				std::queue<std::string>* _output_buf,
				size_t* _output_buf_start,
				client_map_t& _parent);
		~Client();

		bool tcp_active();
		bool udp_active();
		bool fifo_active();
		bool is_retransmitting();
		eog::client_id_t get_client_id();

		size_t get_fifo_used();
		size_t get_fifo_size();
		char* get_fifo_ptr();
		size_t get_fifo_min();
		size_t get_fifo_max();
		void fifo_pop(size_t bytes);

		void set_remote();

		void handle_data(std::shared_ptr<std::string> _data);
		void handle_hello(eog::client_id_t _client_id, boost::asio::ip::udp::endpoint _end);
		void handle_upload(eog::UDP_upload_t* _upload, std::shared_ptr<std::string> msg);
		void handle_report(std::shared_ptr<std::string> _report);
		void handle_retransmit(size_t _to_retransmit);
		void handle_keepalive();
	private:
		void an_handle_hello(const boost::system::error_code& _err,
				size_t _bytes_transferred,
				std::shared_ptr<std::string> _data);
		void an_handle_ack(const boost::system::error_code& _err,
				size_t _bytes_transferred,
				std::shared_ptr<std::string> _data);
		void an_handle_data(const boost::system::error_code& _err,
				size_t _bytes_transferred,
				std::shared_ptr<std::string> _data);
		void an_handle_report(const boost::system::error_code& _err,
				size_t _bytes_transferred,
				std::shared_ptr<std::string> _data);
		void an_handle_timer(const boost::system::error_code& _err);

		boost::asio::io_service& io_service_;
		boost::asio::ip::tcp::socket tcp_socket_;
		std::weak_ptr<boost::asio::ip::udp::socket> udp_socket_;
		boost::asio::ip::udp::endpoint udp_remote_;
		boost::asio::ip::tcp::endpoint tcp_remote_;
		size_t fifo_size_, fifo_low_watermark_, fifo_high_watermark_;
		std::queue<std::string>* output_buf_;
		size_t* output_buf_start_;

		boost::asio::deadline_timer timer_;
		Fifo_queue fifo_;

		bool is_retransmitting_, is_udp_active_, is_active_;
		eog::client_id_t client_id_;
		size_t ant_upload_, ant_data_;

		client_map_t& parent_;

		const boost::posix_time::seconds TIMEOUT_ = boost::posix_time::seconds(1);

		// TEMPORARY
		static eog::client_id_t NEW_ID_;
};

#endif
