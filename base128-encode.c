#include <memory.h>
#include <stdio.h>

// Padding scheme:
// Pad the input up to a multiple of 7 with the pading being the amount of
// padding added. So the final nibble of the output always tells you how much
// padding there was. For input [1, 2], padded would be [1, 2, 5, 5, 5, 5, 5].
// A multiple of 7 input would be padded with 7s.

// You can have 16 of these.
// They are encoded as the first three bytes of a UTF-8 2 byte point.
// So 110xxxxy 10yyyyyy where x is the illegal char and y is the next.
// This is designed to avoid all HTML and JS special characters.
// Also, the implementation would break if any character in 1..8 is used here.
int illegal_table[128] = {
	['\0'] = 1, ['\r'] = 2, ['\n'] = 3, ['"'] = 4,  ['\''] = 5, ['`'] = 6,
	['$'] = 7,  ['\\'] = 8, ['&'] = 9,  ['>'] = 10, ['<'] = 11,
};

#define FIRST_BYTE 0b11000000
#define SECOND_BYTE 0b10000000
#define SECOND_BYTE_MASK 0b00111111
#define MASK 0b01111111
int previous = -1;
void write_char(int c) {
	if (previous != -1) {
		int code = illegal_table[previous];
		if (code) {
			putchar(FIRST_BYTE | ((code - 1) << 1) | (c >> 6));
			putchar(SECOND_BYTE | (SECOND_BYTE_MASK & c));
			previous = -1;
			return;
		} else {
			putchar(previous);
		}
	}
	previous = c;
}

char buffer[7];

void write_buffer() {
	write_char(buffer[0] >> 1);
	for (int i = 1; i <= 6; ++i)
		write_char(
			MASK & ((buffer[i - 1] << (7 - i)) | (buffer[i] >> (i + 1)))
		);
	write_char(buffer[6] & MASK);
}

int main(int argc, char **argv) {
	int read_count;
	while ((read_count = fread(buffer, 1, 7, stdin)) == 7)
		write_buffer();
	int padding_count = 7 - read_count;
	memset(&buffer[read_count], padding_count, padding_count);
	write_buffer();
	write_char(-1);
}
