#ifndef BIT_READER_HPP
#define BIT_READER_HPP

#include <iostream>
#include <cstdint>
#include <stdexcept>

class BitReader {
    // Class definition for BitReader

    private:
        std::istream& inputStream;  // Reference to the input stream from which bits will be read
        uint8_t buffer;             // Internal buffer to hold the current byte being read from the input stream
        int bitCount;               // Number of bits remaining in the buffer that have not yet been read

    public:
        // Constructor to initialize the BitReader with an input stream
        explicit BitReader(std::istream& is):
            inputStream(is), buffer(0), bitCount(0) {}
        
        // Method to read a single bit from the input stream
        // This method reads a bit from the internal buffer. If the buffer is empty, it reads a new byte from the input stream.
        int readBit() {
            if (bitCount == 0) {
                char byte;
                if (!inputStream.get(byte)) {
                    throw std::runtime_error("End of stream reached while reading bits.");
                }

                buffer = static_cast<uint8_t>(byte);
                bitCount = 8;
            }

            bitCount--;
            int bit = (buffer >> bitCount) & 1;
            
            return bit;
        }

        // Method to read multiple bits at once
        // This method reads the specified number of bits from the input stream and returns them as a 64-bit integer.
        bool readBits(uint8_t& outputBits) {
            uint8_t result = 0;
            for (int i = 0; i < 8; ++i) {
                int bit = readBit();
                if (bit == -1) {
                    return false; // End of stream reached before reading the requested number of bits
                }

                result = (result << 1) | bit;
            }
            outputBits = result;
            return true;
        }

        bool isEndOfStream() const {
            if (bitCount > 0) {
                return false; // There are still bits left in the buffer
            }
            return inputStream.eof(); // Check if the underlying stream has reached EOF
        }
};

#endif // BIT_READER_HPP
