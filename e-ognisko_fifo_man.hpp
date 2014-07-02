#ifndef EOGNISKO_FIFO_MAN_HPP
#define EOGNISKO_FIFO_MAN_HPP

// e-ognisko_fifo_man.hpp

#include <unordered_map>
#include "e-ognisko_protocol.hpp"

class Fifo_queue
{
	public:
		Fifo_queue(size_t _size);
		~Fifo_queue();

		void push(const char* _data, size_t _length);
		void pop(size_t _length);
		void straighten();

		char* get_buffer();
		size_t get_length() const;
		size_t get_start() const;
		size_t get_pos() const;
		size_t get_min_length() const;
		size_t get_max_length() const;
		void refresh_min_max();
	private:
		size_t size, length, min_length, max_length, start;
		char* buffer;
};

/*class Fifo_manager
{
	public:
		Fifo_manager(size_t _queue_size);
		~Fifo_manager();

		Fifo_queue& get_queue(eog::client_id_t _client_id);
		Fifo_queue& operator[](eog::client_id_t _client_id);

		void add_queue(eog::client_id_t _client_id);
		void add_to_queue(eog::client_id_t _client_id, const char* _to_add,
				size_t _len);
		void pop_from_queue(eog::client_id_t _client_id, size_t _len);
		void del_queue(eog::client_id_t _client_id);
		size_t get_queue_size();
	private:
		size_t queue_size;
		std::unordered_map<eog::client_id_t, Fifo_queue> queue_map;
};*/

#endif
