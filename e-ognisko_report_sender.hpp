#ifndef EOGNISKO_REPORT_SENDER_HPP
#define EOGNISKO_REPORT_SENDER_HPP

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_server_client.hpp"

class Report_sender
{
	public:
		Report_sender(boost::asio::io_service& _io_service,
				client_map_t& _client_map);
		~Report_sender();

		void run();
	private:
		void handle_timer(const boost::system::error_code& _err);

		boost::asio::io_service& io_service_;
		client_map_t& client_map_;
		boost::asio::deadline_timer timer_;

		const boost::posix_time::seconds TIMEOUT_ = boost::posix_time::seconds(1);
};

#endif
