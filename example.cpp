
//Simple usage example

#include "bit_streams.hpp"
#include <cstring>

void test_bit_streams() {
	uint8_t* tmp,*tmp2;
	bit_streams::bit_stream_writer_c out;
	bit_streams::bit_stream_reader_c in;

	if (!out.open("wr.bin")) {
		printf("Unable to open wr.bin!(+W)\n");
		return;
	}

	tmp = new uint8_t[256*1024];
	tmp2 = new uint8_t[256*1024];

	printf("Write : built-in types\n");

	for (uint64_t i = 0;i < 256*1024;++i) {
		 for (uint64_t x = 1;x<=64;x+=8) {
			out.write(i&((1<<x)-1),x);
		}
	}

	printf("Write : block\n");
	
	for (uint32_t i = 0;i < 256*1024;++i)
		tmp[i] = i & 0xff;

	out.write_p(tmp,256*1024*8);
	out.close();

	printf("Write : Done\n");

	if (!in.open("wr.bin")) {
		printf("Unable to open wr.bin! (+R)\n");
		delete[] tmp;
		delete[] tmp2;
		return;
	}

	
	printf("Read : built-in types\n");
	for (uint64_t i = 0;i < 256*1024;++i) {
		for (uint64_t x = 1;x<=64;x+=8) {
		 	uint64_t v = in.read(x);
			if (v != (i&((1<<x)-1))) {	
				printf("Read error %llu %llu\n",v,i );
				break;
			}
		}
		 
	}

	printf("Read : block\n");
	in.read_p(tmp2,256*1024*8);
	if (0 != memcmp(tmp,tmp2,256*1024))
		printf("Read error\n");	


	printf("\n\nDone\n");
	delete[] tmp;
	delete[] tmp2;
}

int main() {
	test_bit_streams();
	return 0;
}

