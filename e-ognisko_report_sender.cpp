// e-ognisko_report_sender.cpp

#include <boost/asio.hpp>
#include <string>
#include <boost/bind.hpp>
#include <memory>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_server_client.hpp"
#include "e-ognisko_report_sender.hpp"

Report_sender::Report_sender(boost::asio::io_service& _io_service,
		client_map_t& _client_map) : io_service_(_io_service),
	client_map_(_client_map), timer_(io_service_)
{
}

Report_sender::~Report_sender()
{
}

void Report_sender::run()
{
	// set the timer out!
	timer_.expires_from_now(TIMEOUT_);
	timer_.async_wait(boost::bind(&Report_sender::handle_timer,
					this, _1));
}

void Report_sender::handle_timer(const boost::system::error_code& _err)
{
	eog::client_id_t clients = 0;
	eog::TCP_report_t* report = new eog::TCP_report_t[client_map_.size()];
	for(auto cli = client_map_.begin(); cli != client_map_.end(); cli++)
	{
		if(!cli->second.udp_active())
			continue;
		report[clients].TCP_socket = cli->first.address().to_string() +
			std::string(":") + std::to_string(cli->first.port());
		report[clients].fifo_used = cli->second.get_fifo_used();
		report[clients].fifo_size = cli->second.get_fifo_size();
		report[clients].fifo_min = cli->second.get_fifo_min();
		report[clients].fifo_max = cli->second.get_fifo_max();
		clients++;
	}
	std::shared_ptr<std::string> sreport = 
		std::make_shared<std::string>(eog::encode_TCP_report(report, clients));
	delete [] report;
	for(auto cli = client_map_.begin(); cli != client_map_.end(); cli++)
	{
		//fprintf(stderr, "Handling report.\n");
		if(!cli->second.tcp_active())
			continue;
		cli->second.handle_report(sreport);
	}
	timer_.expires_from_now(TIMEOUT_);
	timer_.async_wait(boost::bind(&Report_sender::handle_timer,
				this, _1));
}

