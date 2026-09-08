#ifndef BIT_WRITER_HPP
#define BIT_WRITER_HPP

#include <iostream>
#include <cstdint>
#include <string>

class BitWriter {
    // Class definition for BitWriter

    private:
        std::ostream& outputStream; // Reference to the output stream to which bits will be written
        uint8_t buffer;             // Internal buffer to hold the current byte being constructed before writing to the output stream   
        int bitCount;               // Number of bits currently in the buffer that have not yet been written to the output stream
        uint64_t totalBitsWritten;  // Total number of bits written to the output stream

    public:
        // Constructor to initialize the BitWriter with an output stream
        explicit BitWriter(std::ostream& os):
            outputStream(os), buffer(0), bitCount(0), totalBitsWritten(0) {}
        
        // Method to write a single bit to the output stream
        // This method shifts the current buffer left by 1 and adds the new bit.
        void writeBit(bool bit) {
            buffer = (buffer << 1) | (bit ? 1 : 0);
            bitCount++;

            if (bitCount == 8) {
                outputStream.put(buffer);
                buffer = 0;
                bitCount = 0;
            }
        }

        // Method to write multiple bits at once
        // This method takes a 64-bit integer and writes the specified number of bits to the output stream.
        void writeBits(uint64_t bits, int count) {
            for (int i = count - 1; i >= 0; --i) {
                writeBit((bits >> i) & 1);
            }
        }

        // Method to flush the remaining bits in the buffer to the output stream
        // This method ensures that any remaining bits in the internal buffer are written to the output stream.
        uint8_t flush() {
            uint8_t remainingBits = 0;

            if (bitCount > 0) {
                remainingBits = 8 - bitCount;
                buffer <<= remainingBits; // Shift the remaining bits to the left
                outputStream.put(buffer);
                buffer = 0;
                bitCount = 0;
            }
            return remainingBits;
        }

        // Method to get the total number of bits written to the output stream
        uint64_t getTotalBitsWritten() const {
            return totalBitsWritten;
        }
};

#endif // BIT_WRITER_HPP