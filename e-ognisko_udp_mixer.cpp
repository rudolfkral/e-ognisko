// e-ognisko_udp_mixer.cpp

#include <string>
#include <boost/bind.hpp>
#include <memory>
#include <boost/asio.hpp>
#include <cstdio>
#include "e-ognisko_udp_mixer.hpp"

Udp_mixer::Udp_mixer(boost::asio::io_service& _io_service,
		std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
		client_map_t& _client_map,
		size_t _buf_len,
		size_t _tx_interval) : io_service_(_io_service),
	udp_socket_(_udp_socket),
       	client_map_(_client_map),
	buf_len_(_buf_len),
	tx_interval_(_tx_interval),
	output_buf_(),
	output_buf_start_(0),
	timer_(io_service_)
{
}

Udp_mixer::~Udp_mixer()
{
}

void Udp_mixer::run()
{
	timer_.expires_from_now(boost::posix_time::millisec(tx_interval_));
	timer_.async_wait(boost::bind(&Udp_mixer::handle_timer,
				this, _1));
}

std::queue<std::string>* Udp_mixer::get_output_buf()
{
	return &output_buf_;
}

size_t* Udp_mixer::get_output_buf_start()
{
	return &output_buf_start_;
}

void Udp_mixer::handle_timer(const boost::system::error_code& _err)
{
	// TODO check if operation aborted
	mixer_input* inputs = new mixer_input[client_map_.size()];
	size_t n = 0;
	std::queue<client_map_t::iterator> active_cli;
	std::queue<client_map_t::iterator> filling_cli;
	for(auto cli = client_map_.begin(); cli != client_map_.end(); cli++)
	{
		if(!cli->second.udp_active() || cli->second.is_retransmitting())
			continue;
		if(cli->second.fifo_active())
		{
			inputs[n].data = cli->second.get_fifo_ptr();
			inputs[n].len = cli->second.get_fifo_used();
			//fprintf(stderr, "Submitted data that begins with: \n%s\n",
			//		std::string((const char*)inputs[n].data,
			//			880).c_str());
			inputs[n].consumed = 0;
			active_cli.push(cli);
			n++;
		} else
		{
			filling_cli.push(cli);
		}
	}

	//if(n > 0)
		//fprintf(stderr, "Udp_mixer::handle_timer: %lu active clients, %lu filling.\n", n,
	//			filling_cli.size());

	size_t buf_size = TX_CONSTANT * tx_interval_;
	char msg_buf[buf_size];

	mixer(inputs, n, msg_buf, &buf_size, tx_interval_);
	//if(n > 0)
	//{
	//	memcpy(msg_buf, inputs[0].data, buf_size);
	//	inputs[0].consumed = buf_size;
	//}

	std::shared_ptr<std::string> data = 
		std::make_shared<std::string>((const char*)msg_buf, buf_size);

	if(output_buf_.size() == buf_len_)
	{
		output_buf_start_++;
		output_buf_.pop();
	}
	output_buf_.push(*data);

	//fprintf(stderr, "SENT DATA: \n%s\n", data->c_str());

	for(size_t i = 0; i < n; i++)
	{
		//fprintf(stderr, "Consumed %lu.\n", inputs[i].consumed);
		//fprintf(stderr, "Consumed data: \n%s\n",
		//		std::string((const char*)inputs[i].data,
		//			inputs[i].consumed).c_str());
		active_cli.front()->second.fifo_pop(inputs[i].consumed);
		active_cli.front()->second.handle_data(data);
		active_cli.pop();
		//fprintf(stderr, "popped\n");
	}

	while(!filling_cli.empty())
	{
		filling_cli.front()->second.handle_data(data);
		filling_cli.pop();
	}

	delete [] inputs;

	timer_.expires_from_now(boost::posix_time::millisec(tx_interval_));
	timer_.async_wait(boost::bind(&Udp_mixer::handle_timer,
				this, _1));
}

