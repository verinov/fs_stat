#include "FS.h"

void FS::analize(std::ofstream& output, std::ofstream& meta_output) {
    if (filesystem_) {
        filesystem_->analize(output, meta_output);
    } else {
        throw std::runtime_error("ERROR: FS wasn't inited");
    }
};

FS::FS(Disk* disk) {
    int ext_sig = 0;
    disk->read(&ext_sig, 2, 1024 + 0x38);
    if (ext_sig == 0xEF53) {
        printf("found ext\n");
        filesystem_ = new Ext(disk);
        return;
    }

    int ntfs_sig = 0;
    disk->read(&ntfs_sig, 4, 3);
    if (ntfs_sig == 0x5346544e) {
        printf("found ntfs\n");
        filesystem_ = new NTFS(disk);
        return;
    }

    filesystem_ = nullptr;
}