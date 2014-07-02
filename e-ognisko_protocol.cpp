// e-ognisko-protokol.cpp
// ---
// Implementation of E-ognisko protocol-specific functions.

#include <cstring>
#include <cstdlib>
#include "e-ognisko_protocol.hpp"

using namespace eog;

stream_type_t eog::ident_TCP_msg(std::string _msg)
{
  if(strncmp("CLIENT", _msg.c_str(), 6) == 0)
    return stream_type_t::STREAM_TYPE_HELLO;
  if(strncmp("\n", _msg.c_str(), 1) == 0)
    return stream_type_t::STREAM_TYPE_REPORT;
  return stream_type_t::STREAM_TYPE_UNKNOWN;
}

dgram_type_t eog::ident_UDP_msg(std::string _msg)
{
  if(strncmp("CLIENT", _msg.c_str(), 6) == 0)
    return dgram_type_t::DGRAM_TYPE_HELLO;
  if(strncmp("UPLOAD", _msg.c_str(), 6) == 0)
    return dgram_type_t::DGRAM_TYPE_UPLOAD;
  if(strncmp("DATA", _msg.c_str(), 4) == 0)
    return dgram_type_t::DGRAM_TYPE_DATA;
  if(strncmp("ACK", _msg.c_str(), 3) == 0)
    return dgram_type_t::DGRAM_TYPE_ACK;
  if(strncmp("RETRANSMIT", _msg.c_str(), 10) == 0)
    return dgram_type_t::DGRAM_TYPE_RETRANSMIT;
  if(strncmp("KEEPALIVE", _msg.c_str(), 9) == 0)
    return dgram_type_t::DGRAM_TYPE_KEEPALIVE;
  return dgram_type_t::DGRAM_TYPE_UNKNOWN;
}

std::string eog::encode_TCP_hello(client_id_t _client_id)
{
  return std::string("CLIENT ") + std::to_string(_client_id) +
    std::string("\n");
}

client_id_t eog::decode_TCP_hello(std::string _msg)
{
  return strtoull(_msg.c_str() + 7, NULL, 10);
}

std::string eog::encode_TCP_report(TCP_report_t* _tcp_report, client_id_t clients)
{
  std::string result("\n");
  for(client_id_t client = 0; client < clients; client++)
  {
    result += _tcp_report[client].TCP_socket;
    result += std::string(" FIFO: ");
    result += std::to_string(_tcp_report[client].fifo_used);
    result += std::string("\\");
    result += std::to_string(_tcp_report[client].fifo_size);
    result += std::string(" (min. ");
    result += std::to_string(_tcp_report[client].fifo_min);
    result += std::string(", max. ");
    result += std::to_string(_tcp_report[client].fifo_max);
    result += std::string(")\n");
  }
  return result;
}

std::string eog::encode_UDP_hello(client_id_t _client_id)
{
  return std::string("CLIENT ") + std::to_string(_client_id) +
    std::string("\n");
}

client_id_t eog::decode_UDP_hello(std::string _msg)
{
  return strtoull(_msg.c_str() + 7, NULL, 10);
}

std::string eog::encode_UDP_upload(UDP_upload_t* _udp_upload)
{
  return std::string("UPLOAD ") +
    std::to_string(_udp_upload->data_nr) +
    std::string("\n") +
    std::string((const char*) _udp_upload->data, _udp_upload->data_len);
}

void eog::decode_UDP_upload(std::string _msg, UDP_upload_t* _udp_upload)
{
  char* afternum;
  _udp_upload->data_nr = strtoull(_msg.c_str() + 7,
      &afternum, 10);
  _udp_upload->data_begin = afternum - _msg.c_str() + 1;
  _udp_upload->data_len = _msg.size() - _udp_upload->data_begin;
  return;
}

std::string eog::encode_UDP_data(UDP_data_t* _udp_data)
{
  return std::string("DATA ") +
    std::to_string(_udp_data->data_nr) + std::string(" ") +
    std::to_string(_udp_data->ack_nr) + std::string(" ") +
    std::to_string(_udp_data->win) + std::string("\n") + 
    std::string((const char*)_udp_data->data, _udp_data->data_len);
}

void eog::decode_UDP_data(std::string _msg, UDP_data_t* _udp_data)
{
  char* afternum;
  _udp_data->data_nr = strtoull(_msg.c_str() + 5,
      &afternum, 10);
  _udp_data->ack_nr = strtoull(afternum, 
      &afternum, 10);
  _udp_data->win = strtoull(afternum,
      &afternum, 10);
  _udp_data->data_begin = afternum - _msg.c_str() + 1;
  _udp_data->data_len = _msg.size() - _udp_data->data_begin;
  return;
}

std::string eog::encode_UDP_ack(UDP_ack_t* _udp_ack)
{
  return std::string("ACK ") +
    std::to_string(_udp_ack->ack_nr) + std::string(" ") +
    std::to_string(_udp_ack->win) + std::string("\n");
}

void eog::decode_UDP_ack(std::string _msg, UDP_ack_t* _udp_ack)
{
  char* win_begin;
  _udp_ack->ack_nr = strtoull(_msg.c_str() + 4, &win_begin, 10);
  _udp_ack->win = strtoull(win_begin, NULL, 10);
  return;
}

std::string eog::encode_UDP_retransmit(size_t _data_nr)
{
  return std::string("RETRANSMIT ") +
    std::to_string(_data_nr) + std::string("\n");
}

size_t eog::decode_UDP_retransmit(std::string _msg)
{
  return strtoull(_msg.c_str() + 11, NULL, 10);
}

std::string eog::encode_UDP_keepalive()
{
  return std::string("KEEPALIVE\n");
}

