#include "FS.h"

#include <memory>
#include <fstream>


DiskOverRegFile::DiskOverRegFile(const std::string& file_path) {
    file_.open(file_path, std::ifstream::binary | std::ifstream::in);
}

DiskOverRegFile::~DiskOverRegFile(){
    if (file_.is_open()) {
        file_.close();
    }
}

void Disk::read(void* buffer, size_t size, uint64_t offset) {
    if (size == 0) {
        return;
    }
    
    size_t block_size = get_block_size();
    std::unique_ptr<char> tmp_block(new char[block_size]);
    
    uint64_t middle_offset = (offset + block_size - 1) / block_size;
    size_t left_chunk_size = middle_offset * block_size - offset;
    if (left_chunk_size >= size){
        read_blocks(tmp_block.get(), 1, middle_offset - 1);
        memcpy(buffer, tmp_block.get() + block_size - left_chunk_size, size);
        return;
    }
    if (left_chunk_size){
        read_blocks(tmp_block.get(), 1, middle_offset - 1);
        memcpy(buffer, tmp_block.get() + block_size - left_chunk_size, left_chunk_size);
    }
    
    size_t middle_chunk_block_count = (size - left_chunk_size) / block_size;
    read_blocks((char*)buffer + left_chunk_size, middle_chunk_block_count, middle_offset);
    
    size_t right_chunk_size = (size - left_chunk_size) % block_size;

    if (right_chunk_size) {
        read_blocks(tmp_block.get(), 1, middle_offset + middle_chunk_block_count);
        memcpy((char*)buffer + size - right_chunk_size, tmp_block.get(), right_chunk_size);
    }
}


void DiskOverRegFile::read_blocks(void* buffer, size_t size, uint64_t offset) {
    if (size == 0) {
        return;
    }
    size_t block_size = get_block_size();
    file_.seekg(offset * block_size, file_.beg);
    file_.read((char*)buffer, size * block_size);
}


inline size_t DiskOverRegFile::get_block_size() {
    return 512;
}


