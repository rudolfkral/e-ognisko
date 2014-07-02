// e-ognisko_server_client.cpp

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <memory>
#include <boost/bind.hpp>
#include <queue>
#include "e-ognisko_server_client.hpp"
#include <cstdio>

eog::client_id_t Client::NEW_ID_ = 0;

Client::Client(boost::asio::io_service&	_io_service,
		boost::asio::ip::tcp::socket _tcp_socket,
		std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
		size_t _fifo_size,
		size_t _fifo_low_watermark,
		size_t _fifo_high_watermark,
		std::queue<std::string>* _output_buf,
		size_t* _output_buf_start,
		client_map_t& _parent) : io_service_(_io_service),
	tcp_socket_(std::move(_tcp_socket)),
	udp_socket_(_udp_socket),
	udp_remote_(),
	tcp_remote_(tcp_socket_.remote_endpoint()),
	fifo_size_(_fifo_size),
	fifo_low_watermark_(_fifo_low_watermark),
	fifo_high_watermark_(_fifo_high_watermark),
	output_buf_(_output_buf),
	output_buf_start_(_output_buf_start),
	timer_(io_service_),
	fifo_(fifo_size_),
	is_retransmitting_(false),
	is_udp_active_(false),
	is_active_(false),
	client_id_(NEW_ID_++),
	ant_upload_(0),
	ant_data_(0),
	parent_(_parent)
{
	//if(!udp_socket_.lock()->is_open())
		//printf("UDP closed.\n");
	std::shared_ptr<std::string> msg = 
		std::make_shared<std::string>(eog::encode_TCP_hello(client_id_));	
	tcp_socket_.async_send(boost::asio::buffer(*msg),
			boost::bind(&Client::an_handle_hello,
				this, _1, _2, msg));
	// TODO handle closing connection
}

Client::~Client()
{
	//fprintf(stderr, "Destructor on client %u called\n", client_id_);
	// TODO what to do with aborted UDP async operations
	// cancelling would be too harsh, affecting all clients
	timer_.cancel();
	//if(tcp_socket_.is_open())
	//{
	//	tcp_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	//	tcp_socket_.close();
	//}
	tcp_socket_.close();
}

bool Client::tcp_active()
{
	return true; // TODO
}

bool Client::udp_active()
{
	return is_udp_active_;
}

bool Client::fifo_active()
{
	return is_udp_active_ && is_active_;
}

eog::client_id_t Client::get_client_id()
{
	return client_id_;
}

bool Client::is_retransmitting()
{
	return is_retransmitting_;
}

size_t Client::get_fifo_used()
{
	return fifo_.get_length();
}

char* Client::get_fifo_ptr()
{
	fifo_.straighten();
	return fifo_.get_buffer();
}

size_t Client::get_fifo_size()
{
	return fifo_size_;
}

size_t Client::get_fifo_min()
{
	return fifo_.get_min_length();
}

size_t Client::get_fifo_max()
{
	return fifo_.get_max_length();
}

void Client::fifo_pop(size_t bytes)
{
	fifo_.pop(bytes);
	if(fifo_.get_length() <= fifo_low_watermark_)
		is_active_ = false;
}

void Client::handle_data(std::shared_ptr<std::string> _data)
{
	//fprintf(stderr, "Sending DATA nr %lu to client %lu.\n", ant_data_, client_id_);
	if(!is_udp_active_)
	{
		// TODO
		return;
	}
	if(!udp_socket_.lock())
	{
		// TODO
		return;
	}
	eog::UDP_data_t udpdata;
	udpdata.data_nr = ant_data_++; // client doesn't ack
	udpdata.ack_nr = ant_upload_;
	udpdata.win = fifo_size_ - fifo_.get_length();
	udpdata.data = _data->c_str(); // too bored of readability
	udpdata.data_len = _data->length();

	std::shared_ptr<std::string> msg = 
		std::make_shared<std::string>(eog::encode_UDP_data(&udpdata));
	
	//boost::asio::ip::udp::endpoint remote(
	//		tcp_socket_.remote_endpoint().address(),
	//		tcp_socket_.remote_endpoint().port()
	//		);
	
	//fprintf(stderr, "DATA encoded: %s\n", msg->c_str());
	//fprintf(stderr, "DATA length: %lu\n", _data->length());

	udp_socket_.lock()->async_send_to(boost::asio::buffer(*msg),
			udp_remote_,
			boost::bind(&Client::an_handle_data, this, _1, _2, msg));
}

void Client::handle_hello(eog::client_id_t _client_id, boost::asio::ip::udp::endpoint _end)
{
	//fprintf(stderr, "Handling HELLO from client %lu.\n", _client_id);
	if(_client_id == client_id_)
	{
		is_udp_active_ = true;
		udp_remote_ = _end;
		timer_.expires_from_now(TIMEOUT_);
		timer_.async_wait(boost::bind(&Client::an_handle_timer,
					this, _1));
	} else
	{
		// TODO
	}
}

void Client::handle_upload(eog::UDP_upload_t* _upload, std::shared_ptr<std::string> _msg)
{
	//fprintf(stderr, "Client %lu Got UPLOAD. Contents: \n%s\n", client_id_, _msg->c_str());
	//fprintf(stderr, "Got UPLOAD. Length: %lu\n", _msg->length());
	if(!is_udp_active_)
	{
		// TODO
		delete _upload; // IMPORTANT
		return;
	}
	if(!udp_socket_.lock())
	{
		// TODO
		delete _upload; // IMPORTANT
		return;
	}
	if(_upload->data_nr != ant_upload_)
	{
		// TODO
		delete _upload; // IMPORTANT
		//printf("Wrong UPLOAD nr.\n");
		return;
	}
	ant_upload_++;
	
	fifo_.push(_msg->c_str() + _upload->data_begin, _upload->data_len); // copy
	if(fifo_.get_length() >= fifo_high_watermark_)
		is_active_ = true;
	
	delete _upload; // IMPORTANT

	eog::UDP_ack_t udp_ack;
	udp_ack.ack_nr = ant_upload_;
	udp_ack.win = fifo_size_ - fifo_.get_length();
	std::shared_ptr<std::string> msg = 
		std::make_shared<std::string>(encode_UDP_ack(&udp_ack));
	//boost::asio::ip::udp::endpoint remote(
	//		tcp_socket_.remote_endpoint().address(),
	//		tcp_socket_.remote_endpoint().port()
	//		);
	udp_socket_.lock()->async_send_to(boost::asio::buffer(*msg),
			udp_remote_,
			boost::bind(&Client::an_handle_ack, this, _1, _2, msg));
	}

void Client::handle_report(std::shared_ptr<std::string> _report)
{
	tcp_socket_.async_send(boost::asio::buffer(*_report),
			boost::bind(&Client::an_handle_report, this, _1, _2, _report));
}

void Client::handle_retransmit(size_t _to_retransmit)
{
	// TODO
}

void Client::handle_keepalive()
{
	// prolong timer deadline
	timer_.expires_from_now(TIMEOUT_);
	// timer cancelled, new wait...
	timer_.async_wait(boost::bind(&Client::an_handle_timer,
					this, _1));

}

void Client::an_handle_hello(const boost::system::error_code& _err,
		size_t _bytes_transferred,
		std::shared_ptr<std::string> msg)
{
	// TODO
}

void Client::an_handle_ack(const boost::system::error_code& _err,
		size_t _bytes_transferred,
		std::shared_ptr<std::string> msg)
{
	// TODO
}

void Client::an_handle_data(const boost::system::error_code& _err,
		size_t _bytes_transferred,
		std::shared_ptr<std::string> msg)
{
	// TODO
}


void Client::an_handle_report(const boost::system::error_code& _err,
		size_t _bytes_transferred,
		std::shared_ptr<std::string> msg)
{
	if(_err)
	{
		//fprintf(stderr, "Calling erase.\n");
		parent_.erase(tcp_remote_);
	} 
}

void Client::an_handle_timer(const boost::system::error_code& _err)
{
	// TODO some info
	// erase itself
	//boost::asio::ip::udp::endpoint remote(
	//		tcp_socket_.remote_endpoint().address(),
	//		tcp_socket_.remote_endpoint().port()
	//		);
	if(_err != boost::asio::error::operation_aborted)
	{
		//fprintf(stderr, "Calling erase.\n");
		parent_.erase(tcp_remote_);
	}
       	//else
	//{
	//	timer_.async_wait(boost::bind(&Client::an_handle_timer,
	//				this, _1));
	//}
}
