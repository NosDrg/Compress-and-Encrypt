#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include "bitReader.hpp"
#include "bitWriter.hpp"
#include <cstdint>
#include <vector>
#include <string>


//Use Huffman tree to store the compressed data. 

//Each node in the tree represents a character and its corresponding frequency. 
//The left and right pointers are used to traverse the tree during encoding and decoding.
struct HuffmanNode {
    uint8_t data;               // The character represented by this node
    uint64_t freq;              // Total number of bits in the compressed data
    HuffmanNode* left;          // Pointer to the left child node (for tree structures)
    HuffmanNode* right;         // Pointer to the right child node (for tree structures)

    // Constructor to initialize a leaf node with a character and its frequency
    HuffmanNode(uint8_t val, uint64_t frequency) : data(val), freq(frequency), left(nullptr), right(nullptr) {};

    // Constructor to initialize an internal node with a frequency and child nodes
    HuffmanNode(uint64_t frequency, HuffmanNode* l, HuffmanNode* r) : data(0), freq(frequency), left(l), right(r) {};

    // Method to check if the current node is a leaf node (i.e., has no children)
    bool isLeaf() const {
        return (left == nullptr && right == nullptr);
    }
};

struct NodeComparator {
    // Comparator for priority queue to order nodes based on frequency
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->freq > b->freq; // Min-heap based on frequency
    }
};

class HuffmanHandler {
    // Class to handle reading and writing compressed data using BitReader and BitWriter
    public:
        // Method to compress input data and write it to an output stream
        static uint8_t compressData(const std::vector<uint8_t>& inputData, std::ostream& outputStream);

        // Method to decompress data from an input stream and return the original data
        static std::vector<uint8_t> decompressData(std::istream& inputStream, uint64_t originalSize);

    private:
        // Helper methods for building the Huffman tree, generating codes, and reading/writing compressed data  
        static HuffmanNode* buildHuffmanTree(const std::vector<uint8_t>& inputData);

        // Recursive method to generate Huffman codes for each character based on the tree structure
        static void generateHuffmanCodes(HuffmanNode* root, const std::string& code, std::string code_table[256]);

        // Recursive method to read the compressed data from the input stream and reconstruct the Huffman tree
        static void writeCompressedData(const HuffmanNode* root, BitWriter& bitWriter);

        // Recursive method to read the compressed data from the input stream and reconstruct the Huffman tree
        static HuffmanNode* readCompressedData(BitReader& bitReader);

        // Recursive method to delete the Huffman tree and free memory
        static void deleteHuffmanTree(HuffmanNode* root);
};

#endif // HUFFMAN_HPP