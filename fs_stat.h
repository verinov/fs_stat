/*
 * File:   fs_stat.h
 * Author: alex
 *
 * Created on November 15, 2014, 7:41 PM
 */

#ifndef FS_STAT_H
#define	FS_STAT_H


#include <stdint.h>
#include <string>
#include <fstream>
#include <functional>
#include <memory>


using BlockFunc = std::function<void(std::string, uint64_t,
        uint32_t, uint32_t, int32_t)>;
using MetadataFunc = std::function<void(uint32_t, uint64_t,
        bool, bool, int64_t, int64_t, int64_t)>;

class Disk {
public:
    virtual void read_blocks(void* buffer, size_t size, uint64_t offset) = 0;
    virtual size_t get_block_size() = 0;
    void read(void* buffer, size_t size, uint64_t offset);
};

class DiskOverRegFile: public Disk {
public:
    DiskOverRegFile(const std::string& file_path);
    ~DiskOverRegFile();

    void read_blocks(void* buffer, size_t size, uint64_t offset);
    size_t get_block_size();

private:
    std::ifstream file_;
};


class FSParser {
public:
    virtual void Parse(BlockFunc& printBlock, MetadataFunc& printMetadata);
    FSParser(std::shared_ptr<Disk> disk);
    FSParser(){filesystem_ = nullptr;}
    virtual ~FSParser();

protected:
    std::shared_ptr<Disk> disk_;

private:
    FSParser* filesystem_;
};

#endif	/* FS_STAT_H */

