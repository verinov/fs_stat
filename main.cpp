#include "fs_stat.h"
 
#include <cstring>
#include <memory>
#include <fstream>

void PrintBlock(std::shared_ptr<typename std::ofstream>  output, std::string fileId, uint64_t file_size,
        uint32_t start_offset, uint32_t start_phys_offset, int32_t len) {
    *output << fileId << "," << file_size << "," << start_offset << ","
                    << start_phys_offset << "," << len << "\n";
}

void PrintMetadata(std::shared_ptr<typename std::ofstream>  meta_output, 
        uint32_t inode_num, uint64_t file_size,
        bool compressed, bool encrypted, int64_t ctime, int64_t mtime, int64_t atime) {
    *meta_output << inode_num << "," << file_size << "," << compressed << ","
            << encrypted << "," << ctime << "," << mtime << "," << atime << "\n";
}

int main(int argc, char** argv) {
    std::string file_path;
    
    if (argc == 2) {
        file_path = argv[1]; 
    } else {
        file_path = "/media/alex/DATA/flash_img.img";
    }
    
    std::shared_ptr<std::ofstream> output(new std::ofstream),
            meta_output(new std::ofstream);
    
    output->open("./out.txt");
    meta_output->open("./meta_out.txt");
    
    if (!output->is_open() || !meta_output->is_open()) {
        throw std::runtime_error("ERROR WITH FILES");
    }
    
    std::shared_ptr<Disk> disk(new DiskOverRegFile(file_path));
    FSParser file_sys(disk);
    
    std::function<void(std::string, uint64_t,
        uint32_t, uint32_t, int32_t)> printBlck = std::bind(PrintBlock, output,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, 
            std::placeholders::_4, std::placeholders::_5);
    
    std::function<void(uint32_t, uint64_t, bool,
            bool, int64_t, int64_t, int64_t)> printMeta = std::bind(PrintMetadata, meta_output,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
                std::placeholders::_7);
    
    
    file_sys.Parse(printBlck, printMeta);
    
    output->close();
    meta_output->close();

    return 0;
}

