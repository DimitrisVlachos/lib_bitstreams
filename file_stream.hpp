/*
	Author : Dimitris Vlachos (DimitrisV22@gmail.com @ github.com/DimitrisVlachos)
*/

#ifndef __filestream_hpp__
#define __filestream_hpp__
#include <iostream>
#include <stdint.h>
#include <fstream>
#include <cstring>
#include <string>

namespace file_streams {
	class file_stream_if {
		private:
		public:

		virtual ~file_stream_if() {}
		virtual const char* identity() const { return "file_stream_interface"; }
		virtual bool open(const char* path) { return false; }
		virtual bool close() { return true; }
		virtual bool is_open() { return false; }
		virtual bool eof() { return true; }
		virtual bool flush() { return true; }
		virtual uint64_t tell() { return 0U; }
		virtual uint64_t size() { return 0U; }
		virtual uint64_t seek(const uint64_t offs) { return 0U; }
		virtual uint64_t read(void* dest,const uint64_t len) { return 0U;}
		virtual uint64_t write(const void* src,const uint64_t len) { return 0U;}
		virtual uint8_t read() { return 0U;}
		virtual bool write(const uint8_t w) { return false;}
	};

	/*
		TODO : Buffered I/O
	*/
	class file_stream_reader_c : public file_stream_if {
		private:
		std::ifstream m_fp;
		uint64_t m_fp_size,m_fp_offs;

		public:

		file_stream_reader_c() : m_fp_size(0U),m_fp_offs(0U) {}
		file_stream_reader_c(const char* path) : m_fp_size(0U),m_fp_offs(0U) { open(path); }
		~file_stream_reader_c() { this->close(); }

		uint64_t tell() { return m_fp_offs; }
		uint64_t size() { return m_fp_size; }
		bool is_open() { return m_fp.is_open(); }
		const char* identity() const { return "file_stream_reader_c"; }
		bool eof() { return m_fp_size == m_fp_offs; }
		bool close() { m_fp.close(); return true; }

		bool open(const char* path) {
			std::streampos tmp,tmp2;
			close();
			m_fp.close();
			m_fp.open(path,std::ios::binary);

			if (!m_fp.is_open())
				return false;

			m_fp.seekg(0,std::ios::beg);
			tmp = m_fp.tellg();
			m_fp.seekg(0,std::ios::end);
			tmp2 = m_fp.tellg();
			m_fp_size = (uint64_t)(tmp2 - tmp);
			m_fp.seekg(0,std::ios::beg);
			m_fp_offs = 0U;

			return true;
		}

		uint64_t seek(const uint64_t offs) { 
			m_fp.seekg(offs,std::ios::beg);
			m_fp_offs = m_fp.tellg(); 
			return m_fp_offs; 
		}

		uint64_t read(void* dest,const uint64_t len) {
			uint64_t tmp;

			if (eof())
				return 0;

			tmp = ((m_fp_offs + len) > m_fp_size) ? m_fp_size - m_fp_offs : len;
			if (0U == tmp)
				return 0U;
			m_fp.read(static_cast<char*>(dest),tmp);
			m_fp_offs += tmp;
			return tmp;
		}

		uint8_t read() { 
			if (eof())
				return 0;

			++m_fp_offs;
			return m_fp.get();
		}
	};

	class file_stream_writer_c : public file_stream_if {
		private:
		std::ofstream m_fp;

		public:

		file_stream_writer_c()   {}
		file_stream_writer_c(const char* path)   { open(path); }
		~file_stream_writer_c() {  this->close(); }

		const char* identity() const { return "file_stream_writer_c"; }
		uint64_t tell() { return m_fp.tellp(); }
		uint64_t seek(const uint64_t offs) { m_fp.seekp(offs,std::ios::beg); return m_fp.tellp();  }
		bool is_open() { return m_fp.is_open(); }
		bool close() { m_fp.close(); return true; }
		bool write(const uint8_t v) { m_fp.put(v); return true; }

		bool open(const char* path) {
			uint64_t tmp;

			close();
			m_fp.open(path,std::ios::binary | std::ios::trunc);

			return m_fp.is_open();
		}
	 
		uint64_t write(const void* src,const uint64_t len) {
			uint64_t offs;
			if (0U == len)
				return 0U;
			offs = tell();
			m_fp.write(static_cast<const char*>(src),len);
			return tell() - offs;
		}
	};

	/*
		RAM streams
	*/
	class file_mem_reader_c : public file_stream_if {
		private:
		uint8_t* m_ram;
		uint64_t m_fp_size,m_fp_offs;
		bool m_cleanup;

		public:

		file_mem_reader_c() : m_ram(0),m_fp_size(0U),m_fp_offs(0U),m_cleanup(false) {}
		file_mem_reader_c(uint8_t* mem,uint64_t len,bool cleanup_shared_mem) : m_ram(mem) ,
	 	m_fp_size(len),m_fp_offs(0U),m_cleanup(cleanup_shared_mem) 			{   }
		~file_mem_reader_c() { this->close(); }

		uint64_t tell() { return m_fp_offs; }
		uint64_t size() { return m_fp_size; }
		bool is_open() { return true; }
		const char* identity() const { return "file_mem_reader_c"; }
		bool eof() { return m_fp_size == m_fp_offs; }
		uint8_t read() {uint8_t t; if (eof()) return 0; t = m_ram[m_fp_offs++]; return t; }

		bool close() {
			if (m_cleanup)
				delete[] m_ram;

			m_ram = 0; 
			m_fp_size = 0U;
			m_fp_offs = 0U;
			m_cleanup = false;
			return true;
		}

		bool open(uint8_t* mem,uint64_t len,bool cleanup_shared_mem) {
			close();
 			if ((!mem) || (!len))
				return false;
			m_ram = mem; 
			m_fp_size = len;
			m_fp_offs = 0U;
			m_cleanup = cleanup_shared_mem;
			return true;
		}

		uint64_t seek(const uint64_t offs) { 
			if (offs > m_fp_size) {
				m_fp_offs = m_fp_size;
				return m_fp_size;
			}
			m_fp_offs = offs;
			return m_fp_offs; 
		}

		uint64_t read(void* dest,const uint64_t len) {
			uint64_t tmp;

			if (eof())
				return 0;

			tmp = ((m_fp_offs + len) > m_fp_size) ? m_fp_size - m_fp_offs : len;
			if (0U == tmp)
				return 0U;
			memcpy(dest,(const void*)(m_ram + m_fp_offs),tmp);
			m_fp_offs += tmp;
			return tmp;
		}
	};
}
#endif
