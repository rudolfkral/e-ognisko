// e-ognisko_udp_router.cpp

#include <boost/bind.hpp>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_udp_router.hpp"

size_t endpointmap_hash(const boost::asio::ip::udp::endpoint& e)
{
	return std::hash<std::string>()(e.address().to_string() + ":" +	 
			std::to_string(e.port()));
}


Udp_router::Udp_router(boost::asio::io_service& _io_service,
		std::shared_ptr<boost::asio::ip::udp::socket> _udp_socket,
		client_map_t& _client_map) : io_service_(_io_service),
	udp_socket_(_udp_socket), client_map_(_client_map),
	endpoint_map_(10, &endpointmap_hash)
{
}

Udp_router::~Udp_router()
{
}

void Udp_router::run()
{
	if(udp_socket_.lock())
	{
		udp_socket_.lock()->async_receive_from(
				boost::asio::buffer(msg_buf, MSG_BUF_SIZE),
				endpoint_,
				boost::bind(&Udp_router::handle_receive_from,
					this, _1, _2));
	} else
	{
		// TODO throw up
	}
}

void Udp_router::handle_receive_from(const boost::system::error_code& _err,
		size_t _bytes_transferred)
{
	// TODO check if operation_aborted
	//fprintf(stderr, "Udp_router::handle_receive_from: got %lu bytes\n",
	//		_bytes_transferred);

	std::shared_ptr<std::string> msg =
			std::make_shared<std::string>(msg_buf, _bytes_transferred);
		
	eog::dgram_type_t dgram_type = eog::ident_UDP_msg(*msg);
	
	auto tcp_endp = endpoint_map_.find(endpoint_);
	void* pntr;
	
	if(tcp_endp == endpoint_map_.end())
	{
		if(dgram_type != eog::dgram_type_t::DGRAM_TYPE_HELLO)
		{
			//fprintf(stderr, "Wrong datagram from unknown client\n");
			return;
		}
		eog::client_id_t cli_id = eog::decode_UDP_hello(*msg);
		auto cli = client_map_.begin();
		for(; cli != client_map_.end(); cli++)
		{
			if(cli->second.get_client_id() == cli_id)
				break;
		}
		if(cli == client_map_.end())
		{
			//fprintf(stderr, "Client with that id not found\n");
			return;
		}
		tcp_endp = endpoint_map_.insert(std::make_pair(endpoint_, cli->first)).first;
		cli->second.handle_hello(cli_id, endpoint_);
	} else
	{
		auto client = client_map_.find(tcp_endp->second); // szukamy po TCP z pary <UDP, TCP>
		if(client == client_map_.end())
		{
			//fprintf(stderr, "Udp_router::handle_receive_from: client unknown\n");
			//fprintf(stderr, "Address: %s\n", endpoint_.address().to_string().c_str());
			// TODO throw up
		} else
		{
			switch(dgram_type)
			{
				//case eog::dgram_type_t::DGRAM_TYPE_HELLO:
				//	client->second.handle_hello(
				//			eog::decode_UDP_hello(*msg));
				//	break;
				case eog::dgram_type_t::DGRAM_TYPE_UPLOAD:
					pntr = new eog::UDP_upload_t; // MEMORY ALLOC
					eog::decode_UDP_upload(*msg, (eog::UDP_upload_t*) pntr);
					client->second.handle_upload((eog::UDP_upload_t*) pntr, msg);
					break;
				case eog::dgram_type_t::DGRAM_TYPE_RETRANSMIT:
					client->second.handle_retransmit(
							eog::decode_UDP_retransmit(*msg));
					break;
				case eog::dgram_type_t::DGRAM_TYPE_KEEPALIVE:
					//client->second.handle_keepalive();
					break;
				default:
					// TODO throw up
					//fprintf(stderr, "Weird package. Contents: \n%s\n",
					//		msg->c_str());
					break;
			}
			client->second.handle_keepalive(); // every datagram prolongs timeout
		}
	}
	// wait for another package
	if(udp_socket_.lock())
	{
		udp_socket_.lock()->async_receive_from(
				boost::asio::buffer(msg_buf, MSG_BUF_SIZE),
				endpoint_,
				boost::bind(&Udp_router::handle_receive_from,
					this, _1, _2));
	} else
	{
		// TODO throw up
	}
}
