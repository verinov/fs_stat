#include "fs_stat.h"

#include <cstring>
#include <memory>
#include <fstream>

#include <chrono>
#include <ctime>
#include <iostream>

void PrintBlock(std::shared_ptr<typename std::ofstream>  output, std::string fileId, uint64_t file_size,
        uint32_t start_offset, uint32_t start_phys_offset, int32_t len) {
    *output << fileId << "," << file_size << "," << start_offset << ","
                    << start_phys_offset << "," << len << std::endl;
}

void PrintMetadata(std::shared_ptr<typename std::ofstream>  meta_output,
        uint32_t inode_num, uint64_t file_size,
        bool compressed, bool encrypted, int64_t ctime, int64_t mtime, int64_t atime) {
    *meta_output << inode_num << "," << file_size << "," << compressed << ","
            << encrypted << "," << ctime << "," << mtime << "," << atime << std::endl;
}

float Test(std::string file_path) {
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

    std::chrono::time_point<std::chrono::system_clock> timerStart, timerEnd;
    timerStart = std::chrono::system_clock::now();
    file_sys.Parse(printBlck, printMeta);
    timerEnd = std::chrono::system_clock::now();
    float elapsed_seconds = ((float) std::chrono::duration_cast<std::chrono::microseconds>
                             (timerEnd-timerStart).count()) / 1000000;
    output->close();
    meta_output->close();
    return elapsed_seconds;
}

int main(int argc, char** argv) {
    std::string file_path;

    // sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'
    for (int iter = 0; iter < 10; ++iter) {
       // system("sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'");
        //std::cout << Test("/media/alex/DATA/Work/fs_stat/fs_images/big_ext") << std::endl;
        //std::cout << Test("/media/alex/DATA/Work/fs_stat/fs_images/small_ext4") << std::endl;
        std::cout << Test("/run/media/alex/DATA/Work/fs_stat/fs_images/flash_img.img") << std::endl;
        //std::cout << Test("/media/alex/DATA/Work/fs_stat/fs_images/small_ext3") << std::endl;
    }

    std::cout.flush();

    return 0;
}

