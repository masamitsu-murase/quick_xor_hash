/**
   method block zero():
     returns a block with all zero bits.

   method block reverse(block b)
     returns a block with all of the bytes reversed (00010203... => ...03020100)

   method block extend8(byte b):
     returns a block with all zero bits except for the lower 8 bits which come from b.

   method block extend64(int64 i):
     returns a block of all zero bits except for the lower 64 bits which come from i
     and are in little-endian byte order.

   method block rotate(block bl, int n):
     returns bl rotated left by n bits.

   method block xor(block bl1, block bl2):
     returns a bitwise xor of bl1 with bl2

   method block XorHash0(byte[] rgb):
     block ret = zero()
     for (int i = 0; i < rgb.Length; i ++)
       ret = xor(ret, rotate(extend8(rgb[i]), i * 11))
     returns reverse(ret)

   entrypoint block XorHash(byte[] rgb):
     returns xor(extend64(rgb.Length), XorHash0(rgb))
  */
#include <cstdint>
#include <array>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>


const size_t BYTES_PER_BLOCK = 20;
typedef std::array<uint8_t, BYTES_PER_BLOCK> block_t;


void rotate_block(block_t &block, size_t shift)
{
    block_t::value_type carry = 0;
    for (size_t i=0; i<block.size(); i++) {
        const uint8_t temp_carry = (block[i] >> (sizeof(uint8_t) * 8 - shift));
        block[i] = (block[i] << shift) | carry;
        carry = temp_carry;
    }
    block[0] |= carry;
}

void reverse_block(block_t &block)
{
    for (size_t i=0; i<block.size()/2; i++) {
        std::swap(block[i], block[block.size() - i - 1]);
    }
}

void xor(block_t &lhs, const block_t &rhs)
{
    for (size_t i=0; i<lhs.size(); i++) {
        lhs[i] ^= rhs[i];
    }
}

block_t quick_xor_hash(std::istream &input)
{
    uint64_t count = 0;

    const size_t BLOCK_UNIT_SIZE = BYTES_PER_BLOCK * 8;
    std::array<uint8_t, BLOCK_UNIT_SIZE> hash_data = {};

    bool loop = true;
    while (loop) {
        std::array<uint8_t, BLOCK_UNIT_SIZE> input_data;
        input.read(reinterpret_cast<char*>(&input_data[0]), input_data.size());
        count += input.gcount();
        if (input.gcount() != input_data.size()) {
            std::fill(std::begin(input_data) + static_cast<std::ptrdiff_t>(input.gcount()),
                      std::end(input_data), 0);
            loop = false;
        }
        for (size_t i=0; i<hash_data.size(); i++) {
            hash_data[i] ^= input_data[i];
        }
    }

    std::array<block_t, 8> blocks = {};

    for (size_t i=0; i<BLOCK_UNIT_SIZE; i++) {
        const size_t mod_index = (i * 11) % BLOCK_UNIT_SIZE;
        const size_t blocks_index = mod_index % 8;
        const size_t byte_index = mod_index / 8;
        blocks[blocks_index][byte_index] = hash_data[i];
    }

    block_t &block = blocks[0];
    for (size_t i=1; i<blocks.size(); i++) {
        rotate_block(blocks[i], i);
        xor(block, blocks[i]);
    }

    reverse_block(block);

    block_t length_block = {};
    for (size_t i=0; i<sizeof(uint64_t); i++) {
        length_block[sizeof(uint64_t) - i - 1] = static_cast<uint8_t>(count >> (i * 8));
    }
    xor(block, length_block);

    return block;
}

block_t quick_xor_hash_file(const std::string &filename)
{
    std::ifstream input(filename.c_str(), std::ios::binary);
    return quick_xor_hash(input);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: quick_xor_hash.exe filename" << std::endl;
        return 1;
    }

    try {
        block_t result = quick_xor_hash_file(argv[1]);
        for (auto it=std::crbegin(result); it!=std::crend(result); it++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(*it);
        }
        std::cout << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }

    return 0;
}
