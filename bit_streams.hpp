
/*
	Author : Dimitris Vlachos (DimitrisV22@gmail.com @ github.com/DimitrisVlachos)
*/

#ifndef _bit_streams_hpp_
#define _bit_streams_hpp_
#include "file_stream.hpp"
#include <vector>
 
namespace bit_streams {

	template <class reader_type_c>
	class bit_stream_reader_c {
		private:
		file_streams::file_stream_if* m_handle;
		uint32_t m_bit_pos,m_bit_buffer_size,m_bit_buffer_pos;
		uint64_t m_stream_len,m_stream_pos;
		uint8_t m_bit_buf;
		uint8_t* m_buffer;

		inline void fill_buf() {
			const uint64_t t = ((m_stream_pos + m_bit_buffer_size) > m_stream_len) ? 
						m_stream_len - m_stream_pos : m_bit_buffer_size;
 
			m_handle->read(&m_buffer[0],t);
			m_stream_pos += t;	
		}

		inline bool valid_state() const {
			return (0 != m_handle) && (0 != m_buffer);
		}

		public:	
		bit_stream_reader_c(const uint32_t bit_buffer_size = 32*1024) : m_handle(0),m_bit_pos(0),
		m_bit_buffer_size(bit_buffer_size),m_bit_buffer_pos(0),m_stream_len(0),m_stream_pos(0),m_bit_buf(8){
			m_buffer = new uint8_t[m_bit_buffer_size];
		}
		~bit_stream_reader_c() { close(); delete[] m_buffer; }

		void close() {
			
			delete m_handle;
			m_handle = 0;
			m_stream_len = m_stream_pos = 0;		
			m_bit_buf = 0;
			m_bit_pos = 8;
			m_bit_buffer_pos = m_bit_buffer_size;
		}

		//Allow shared handles  for embedded filesystem derived classes
		bool open(file_streams::file_stream_if* shared_handle)	{ 
			close();
			if (!m_buffer) return false;
			if (!shared_handle)
				return false;
			m_handle = shared_handle;
			if (m_handle->is_open())
				m_stream_len = m_handle->size();
			return m_handle->is_open();
		}

		bool open(const char* path)	{ 
			close();
			if (!m_buffer) return false;
			m_handle = new reader_type_c(path);
			if (!m_handle) 
				return false;
			if (!m_handle->is_open()) {
				close();
				return false;
			}
			m_stream_len = m_handle->size();

			return true;
		}

		inline bool is_open() const {
			return m_handle != 0;
		}

		inline bool eof() const {
			return ((m_stream_len == m_stream_pos) && (m_bit_buffer_pos == m_bit_buffer_size)) || (!valid_state());
		}

		uint64_t read(uint32_t bits) {	//Read up to 64bit
			register uint64_t res = 0;
			const uint64_t bits_m1 = bits - 1;

			uint32_t i = 0;
 			//Feature OPT : Do byte reads
			for (;i < bits;++i) {
				if (m_bit_pos == 8) {
					if (m_bit_buffer_pos == m_bit_buffer_size) {
						if (m_stream_len == m_stream_pos)
							return res;
						fill_buf();
						m_bit_buffer_pos = 0;
					}
					m_bit_pos = 0;
					m_bit_buf = m_buffer[m_bit_buffer_pos++];
				}

				res |= (uint64_t)(!! (m_bit_buf & (1 << (m_bit_pos++))) ) << (bits_m1 - i);
			}

			return res;
		}

		void read_p(void* dst,uint32_t bits) {
			uint8_t* ptr = static_cast<uint8_t*>(dst);
			register uint64_t u;

			//64bit units 
			while (bits >= 64U) {
				u = read(64U);
				ptr[0] = u >> (uint64_t)56U;
				ptr[1] = u >> (uint64_t)48U;
				ptr[2] = u >> (uint64_t)40U;
				ptr[3] = u >> (uint64_t)32U;
				ptr[4] = u >> (uint64_t)24U;
				ptr[5] = u >> (uint64_t)16U;
				ptr[6] = u >> (uint64_t)8U;
				ptr[7] = u & (uint64_t)0xffU;
				ptr += 8U;
				bits -= 64U;
			}

			//32bit units  
			if (bits >= 32U) {
				u = read(32U);
				ptr[0] = u >> (uint64_t)24U;
				ptr[1] = u >> (uint64_t)16U;
				ptr[2] = u >> (uint64_t)8U;
				ptr[3] = u & (uint64_t)0xffU;
				ptr += 4U;
				bits -= 32U;
			}

			//8bits
			while (bits >= 8U) {
				u = read(8U);
				*(ptr++) = u & (uint64_t)0xffU;
				bits -= 8U;
			}

			//Remainder
			if ((bits != 0U) && (bits < 8U)) {
				u = read(bits);
				*(ptr) = u & (uint64_t)0xffU;
			}
		}
	};

	template <class writer_type_c>
	class bit_stream_writer_c {
		private:
		file_streams::file_stream_if* m_handle;
		uint32_t m_bit_buffer_size;
		uint32_t m_bit_pos;
		uint8_t m_bit_buf;
		std::vector<uint8_t> m_buffer;

 
		inline void flush() {
			if (m_buffer.size() == m_bit_buffer_size) {
				m_handle->write(&m_buffer[0] ,m_bit_buffer_size );
				m_handle->flush();
				m_buffer.clear();
			}
		}

		public:	
		bit_stream_writer_c(const uint32_t bit_buffer_size = 32*1024) : m_handle(0),m_bit_buffer_size(bit_buffer_size),
		m_bit_pos(0),m_bit_buf(0) {
			m_buffer.reserve(bit_buffer_size);
 
		}
		~bit_stream_writer_c() { close(); }

		inline bool is_open() const {
			return m_handle != 0;
		}

		void close() {
			if (m_handle) {
				if (!m_buffer.empty())
					m_handle->write(&m_buffer[0],m_buffer.size());
				if (m_bit_pos) 
					m_handle->write(m_bit_buf);
				delete m_handle;
				m_handle = 0;
			}
		
			m_buffer.clear();
			m_bit_buf = 0;
			m_bit_pos = 0;
		}

		//Allow shared handles  for embedded filesystem derived classes
		bool open(file_streams::file_stream_if* shared_handle)	{ 
			close();
			if (!shared_handle)
				return false;
			m_handle = shared_handle;
			return m_handle->is_open();
		}

		bool open(const char* path)	{ 
			close();
			m_handle = new writer_type_c(path);
			if (!m_handle) 
				return false;
			return m_handle->is_open();
		}

		void write(uint64_t data,uint32_t bits) {	//Write up to 64bits
			const uint64_t bits_m1 = bits - 1;
			uint32_t i = 0;

			//Feature OPT : Do byte writes
			for (;i < bits;++i) {
				if (m_bit_pos == 8) {
					flush();
					m_buffer.push_back(m_bit_buf);
					m_bit_pos = 0;
					m_bit_buf = 0;
				}
				m_bit_buf |= (!!(data & ((uint64_t)1U << (bits_m1-i)))) << (m_bit_pos++);
			}
		}

		void write_p(const void* src,uint32_t bits) {	//Write up to Nbits
			const uint8_t* ptr = static_cast<const uint8_t*>(src);
			register uint64_t u;

			//64bit units 
			while (bits >= 64U) {
				u = ((uint64_t)ptr[0] << (uint64_t)56U);
				u |= ((uint64_t)ptr[1] << (uint64_t)48U);
				u |= ((uint64_t)ptr[2] << (uint64_t)40U);
				u |= ((uint64_t)ptr[3] << (uint64_t)32U);
				u |= ((uint64_t)ptr[4] << (uint64_t)24U);
				u |= ((uint64_t)ptr[5] << (uint64_t)16U);
				u |= ((uint64_t)ptr[6] << (uint64_t)8U);
				u |= ((uint64_t)ptr[7]);	
				write(u,64U);
				ptr += 8U;
				bits -= 64U;
			}

			//32bit units  
			if (bits >= 32U) {
				u = ((uint64_t)ptr[0] << (uint64_t)24U);
				u |= ((uint64_t)ptr[1] << (uint64_t)16U);
				u |= ((uint64_t)ptr[2] << (uint64_t)8U);
				u |= ((uint64_t)ptr[3]);	
				write(u,32U);
				ptr += 4U;
				bits -= 32U;
			}

			//8bits
			while (bits >= 8) {
				u = (uint64_t)*(ptr++);
				write(u,8U);
				bits -= 8;
			}

			//Remainder
			if ((bits != 0U) && (bits < 8)) {
				u = (uint64_t)ptr[0];
				write(u,bits);
			}
		}
	};
} 
#endif


