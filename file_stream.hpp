
#ifndef __filestream_hpp__
#define __filestream_hpp__
#include <iostream>
#include <stdint.h>
#include <fstream>

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

		bool close() {
			m_fp.close();
			return true;
		}

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
	 
		bool is_open() {
			return m_fp.is_open();
		}

		bool close() {
			m_fp.close();
			return true;
		}

		bool open(const char* path) {
			uint64_t tmp;

			close();
			m_fp.open(path,std::ios::binary | std::ios::trunc);

			return m_fp.is_open();
		}
	 
		uint64_t write(const void* src,const uint64_t len) {
			m_fp.write(static_cast<const char*>(src),len);
			return len;
		}

		bool write(const uint8_t v) { 
			m_fp.put(v);
		 	 
			return true;
		}
	};
}
#endif

