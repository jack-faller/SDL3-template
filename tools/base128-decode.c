#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char illegal_table[] = {
	'\0', '\0', '\r', '\n', '"', '\'', '`', '$', '\\', '&', '>', '<',
};

uint64_t out;

void write7(uint8_t byte) { out = (out << 7) | byte; }
uint8_t out_byte(int idx) { return (out >> (idx * 8)) & 0xFF; }

#define FIRST_BYTE_MASK 0b00011110
#define SECOND_BYTE_MASK 0b00111111

uint8_t buffer[8];
int main(int argc, char **argv) {
	FILE *input = stdin;
	if (argc == 2)
		input = fopen(argv[1], "r");
	bool have_looped = false;
	int carry_bit;
	for (int read_count; (read_count = fread(buffer, 1, 8, input)) == 8;
	     have_looped = true) {
		if (have_looped) {
			for (int i = 7; i-- != 0;)
				putchar(out_byte(i));
			out = 0;
		}
		for (int i = 0; i < 8; ++i) {
			uint8_t c = buffer[i];
			switch ((c >> 6) & 0b11) {
			case 0b00:
			case 0b01: write7(c); break;
			case 0b11:
				write7(illegal_table[(FIRST_BYTE_MASK & c) >> 1]);
				carry_bit = c & 1;
				break;
			case 0b10: write7((carry_bit << 6) | (SECOND_BYTE_MASK & c)); break;
			}
		}
	}
	int final_bytes = out_byte(0);
	for (int i = 7; i > final_bytes; --i)
		putchar(out_byte(i));
	if (input != stdin)
		fclose(input);
}
