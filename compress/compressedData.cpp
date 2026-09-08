#include "compressedData.hpp"
#include <queue>
#include <array>

CompressedData* CompressedDataHandler::buildHuffmanTree(const std::vector<uint8_t>& inputData) {
    // Count the frequency of each character in the input data
    std::array<uint64_t, 256> frequencyMap = {0}    ;
    for (uint8_t byte : inputData) {
        frequencyMap[byte]++;
    }

    // Create a priority queue (min-heap) to build the Huffman tree
    std::priority_queue<CompressedData*, std::vector<CompressedData*>, NodeComparator> minHeap;

    // Create leaf nodes for each character and add them to the priority queue
    for (int i = 0; i < 256; ++i) {
        if (frequencyMap[i] > 0) {
            minHeap.push(new CompressedData(i, frequencyMap[i]));
        }
    }

    // Build the Huffman tree by combining nodes with the lowest frequencies
    while (minHeap.size() > 1) {
        CompressedData* left = minHeap.top();
        minHeap.pop();
        CompressedData* right = minHeap.top();
        minHeap.pop();

        uint64_t combinedFreq = left->freq + right->freq;
        CompressedData* internalNode = new CompressedData(combinedFreq, left, right);
        minHeap.push(internalNode);
    }

    // The remaining node is the root of the Huffman tree
    return minHeap.top();
}

void CompressedDataHandler::generateHuffmanCodes(CompressedData* root, const std::string& code, std::string code_table[256]) {
    if (!root) return;

    // If the current node is a leaf node, store the code for the character
    if (root->isLeaf()) {
        code_table[root->data] = code;
        return;
    }

    // Traverse the left subtree with '0' added to the code
    generateHuffmanCodes(root->left, code + "0", code_table);

    // Traverse the right subtree with '1' added to the code
    generateHuffmanCodes(root->right, code + "1", code_table);
}

void CompressedDataHandler::writeCompressedData(const CompressedData* root, BitWriter& bitWriter) {
    if (!root) return;

    // If the current node is a leaf node, write a '1' bit followed by the character
    if (root->isLeaf()) {
        bitWriter.writeBit(1);
        bitWriter.writeBits(root->data, 8); // Write the character as 8 bits
        return;
    }

    // If the current node is an internal node, write a '0' bit and traverse its children
    bitWriter.writeBit(0);
    writeCompressedData(root->left, bitWriter);
    writeCompressedData(root->right, bitWriter);
}

CompressedData* CompressedDataHandler::readCompressedData(BitReader& bitReader) {
    // Read a single bit to determine if the current node is a leaf or internal node
    int bit = bitReader.readBit();
    if (bit == -1) {
        std::cerr << "Error: Unexpected end of stream while reading compressed data." << std::endl;
        return nullptr; // Return nullptr to indicate an error
    }

    // If the bit is '1', read the next 8 bits to get the character and create a leaf node
    if (bit == 1) {
        uint8_t character;
        if (!bitReader.readBits(character)) {
            std::cerr << "Error: Unexpected end of stream while reading character." << std::endl;
            return nullptr; // Return nullptr to indicate an error
        }
        return new CompressedData(character, 0); // Frequency is not needed for decompression
    }

    // If the bit is '0', recursively read the left and right children to create an internal node
    CompressedData* leftChild = readCompressedData(bitReader);
    CompressedData* rightChild = readCompressedData(bitReader);
    return new CompressedData(0, leftChild, rightChild); // Frequency is not needed for decompression
}

void CompressedDataHandler::deleteHuffmanTree(CompressedData* root) {
    if (!root) return;

    // Recursively delete the left and right subtrees
    deleteHuffmanTree(root->left);
    deleteHuffmanTree(root->right);

    // Delete the current node
    delete root;
}

uint8_t CompressedDataHandler::compressData(const std::vector<uint8_t>& inputData, std::ostream& outputStream) {
    // Build the Huffman tree based on the input data
    CompressedData* root = buildHuffmanTree(inputData);

    // Generate Huffman codes for each character
    std::string code_table[256];
    generateHuffmanCodes(root, "", code_table);

    // Create a BitWriter to write compressed data to the output stream
    BitWriter bitWriter(outputStream);

    // Write the Huffman tree structure to the output stream
    writeCompressedData(root, bitWriter);

    // Write the actual compressed data using the generated codes
    for (uint8_t byte : inputData) {
        const std::string& code = code_table[byte];
        for (char bit : code) {
            bitWriter.writeBit(bit == '1');
        }
    }

    // Flush any remaining bits in the buffer and get the number of remaining bits
    uint8_t remainingBits = bitWriter.flush();

    // Clean up and delete the Huffman tree to free memory
    deleteHuffmanTree(root);

    return remainingBits; // Return the number of remaining bits in the last byte
}

std::vector<uint8_t> CompressedDataHandler::decompressData(std::istream& inputStream, uint64_t originalSize) {
    std::vector<uint8_t> outputData; // Vector to hold the decompressed data
    if (originalSize == 0) {
        return outputData; // Return an empty vector if the original size is zero
    }

    outputData.reserve(originalSize); // Reserve space for the original size to avoid reallocations
    BitReader bitReader(inputStream); // Create a BitReader to read bits from the input stream

    CompressedData* root = readCompressedData(bitReader); // Read the Huffman tree structure from the input stream
    if (!root) {
        throw std::runtime_error("Failed to read Huffman tree from the input stream.");
    }

    if (root->isLeaf()) {
        outputData.assign(originalSize, root->data); // If the tree has only one node, fill the output with that character
        deleteHuffmanTree(root);
        return outputData;
    }

    // Read bits from the input stream and traverse the Huffman tree to decode characters
    CompressedData* currentNode = root; // Start at the root of the Huffman tree
    while (outputData.size() < originalSize) {
        int bit = bitReader.readBit();
        if (bit == -1) {
            break; // End of stream reached
        }

        // Traverse the Huffman tree based on the read bit
        currentNode = (bit == 0) ? currentNode->left : currentNode->right;

        if (!currentNode) {
            throw std::runtime_error("Invalid bit sequence encountered during decompression.");
        }

        // If a leaf node is reached, add the character to the output data
        if (currentNode->isLeaf()) {
            outputData.push_back(currentNode->data);
            currentNode = root; // Reset to the root for the next character
        }
    }

    // Clean up and delete the Huffman tree to free memory
    deleteHuffmanTree(root);

    return outputData; // Return the decompressed data as a vector of bytes
}

