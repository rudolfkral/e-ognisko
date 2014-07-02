// e-ognisko_fifo_man.cpp

#include <cstring>
#include "e-ognisko_protocol.hpp"
#include "e-ognisko_fifo_man.hpp"

Fifo_queue::Fifo_queue(size_t _size): size(_size), length(0), min_length(0),
	max_length(0), start(0), buffer(new char[size])
{
}

Fifo_queue::~Fifo_queue()
{
	delete [] buffer;
}

void Fifo_queue::push(const char* _data, size_t _length)
{
	if(get_pos() + _length > size)
	{
		size_t copied_to_end = size - get_pos();
		size_t copied_from_begin = _length - copied_to_end;
		memcpy(buffer + get_pos(), _data, copied_to_end);
		memcpy(buffer, _data + copied_to_end, copied_from_begin);
		if(copied_from_begin > start)
		{
			start = copied_from_begin;
			length = size;
		} else
		{
			length += _length;
		}
	} else {
		memcpy(buffer + get_pos(), _data, _length);
		length += _length;
	}
	if(length > max_length)
		max_length = length;
}

void Fifo_queue::pop(size_t _length)
{
	if(_length > length)
	{
		//printf("Przeciążenie kolejki: chciano usunąć %lu bajtów, jest %lu\n",
		//		_length, length);
		_length = length;
	}
	start = (start + _length) % size;
	length -= _length;
	if(length < min_length)
		min_length = length;
}

void Fifo_queue::straighten()
{
	if(start > 0)
	{
		char* tmp = new char[start];
		memcpy(tmp, buffer, start);
		memmove(buffer, buffer + start, size - start);
		memcpy(buffer + size - start, tmp, start);
		start = 0;
		delete [] tmp;
	}
}

char* Fifo_queue::get_buffer()
{
	return (char*) buffer;
}

size_t Fifo_queue::get_length() const
{
	return length;
}

size_t Fifo_queue::get_start() const
{
	return start;
}

size_t Fifo_queue::get_pos() const
{
	return (start + length) % size;
}

size_t Fifo_queue::get_min_length() const
{
	return min_length;
}

size_t Fifo_queue::get_max_length() const
{
	return max_length;
}

void Fifo_queue::refresh_min_max()
{
	min_length = max_length = length;
}

/*Fifo_manager::Fifo_manager(size_t _queue_size): queue_size(_queue_size), queue_map()
{
}	

Fifo_queue& Fifo_manager::get_queue(eog::client_id_t _client_id)
{
	return queue_map[_client_id];
}

Fifo_queue& Fifo_manager::operator[](eog::client_id_t _client_id)
{
	return queue_map[_client_id];
}

void Fifo_manager::add_queue(eog::client_id_t _client_id)
{
	queue_map.emplace(_client_id);
}

void Fifo_manger::add_to_queue(eog::client_id_t _client_id, 
		const char* _to_add, size_t _len)
{
	queue_map[_client_id].push(_to_add, _len);
}

void Fifo_manager::pop_from_queue(eog::client_id_t _client_id, size_t _len)
{
	queue_map[_client_id].pop(_len);
}

void Fifo_manager::del_queue(eog::client_id_t _client_id)
{
	queue_map.erase(_client_id);
}
*/
