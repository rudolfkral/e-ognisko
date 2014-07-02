#ifndef EOGNISKO_PROTOCOL_H
#define EOGNISKO_PROTOCOL_H

// e-ognisko_protocol.h
// ---
// Plays the role of abstract layer in communication process in e-ognisko. It
// is intended to separate such issues as formatting and decoding datagrams
// from core of program.

#include <string>
#include <cstdint>

namespace eog
{
  typedef uint32_t client_id_t;

  enum stream_type_t
  {
    STREAM_TYPE_HELLO,
    STREAM_TYPE_REPORT,
    STREAM_TYPE_UNKNOWN
  };

  enum dgram_type_t
  {
    DGRAM_TYPE_HELLO,
    DGRAM_TYPE_UPLOAD,
    DGRAM_TYPE_DATA,
    DGRAM_TYPE_ACK,
    DGRAM_TYPE_RETRANSMIT,
    DGRAM_TYPE_KEEPALIVE,
    DGRAM_TYPE_UNKNOWN
  };

  const size_t DATAGRAM_SIZE = 50000;

  stream_type_t ident_TCP_msg(std::string _msg);
  dgram_type_t ident_UDP_msg(std::string _msg);

  std::string encode_TCP_hello(client_id_t _client_id);
  client_id_t decode_TCP_hello(std::string _msg);
  //void clear_TCP_hello(std::string& _msg);

  struct TCP_report_t
  {
    std::string TCP_socket;
    size_t fifo_used, fifo_size, fifo_min, fifo_max;
  };

  std::string encode_TCP_report(TCP_report_t* _tcp_report, client_id_t clients);
  //client_id_t decode_TCP_report(TCP_report_t* _tcp_reports, std::string _msg);
  //void clear_TCP_report(std::string& _msg);

  std::string encode_UDP_hello(client_id_t _client_id);
  client_id_t decode_UDP_hello(std::string _msg);
  //void clear_UDP_hello(std::string& _msg);

  struct UDP_upload_t
  {
    const void* data; // only for encoding
    size_t data_nr, data_begin, data_len;
  };

  std::string encode_UDP_upload(UDP_upload_t* _udp_upload);
  void decode_UDP_upload(std::string _msg, UDP_upload_t* _udp_upload);
  //void clear_UDP_upload(std::string& _msg);

  struct UDP_data_t
  {
    const void* data; // only for encoding
    size_t data_nr, ack_nr, win, data_begin, data_len;
  };

  std::string encode_UDP_data(UDP_data_t* _udp_data);
  void decode_UDP_data(std::string _msg, UDP_data_t* _udp_data);
  //void clear_UDP_data(std::string& _msg);

  struct UDP_ack_t
  {
    size_t ack_nr, win;
  };

  std::string encode_UDP_ack(UDP_ack_t* _udp_ack);
  void decode_UDP_ack(std::string _msg, UDP_ack_t* _udp_ack);
  //void clear_UDP_ack(std::string& msg);

  std::string encode_UDP_retransmit(size_t _data_nr);
  size_t decode_UDP_retransmit(std::string _msg);
  //void clear_UDP_ack(std::string& _msg);

  std::string encode_UDP_keepalive();
  //void clear_UDP_keepalive(std::string& msg);

};

#endif
