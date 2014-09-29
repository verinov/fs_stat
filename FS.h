#ifndef FS_H
#define	FS_H

#include "fs_structs.h"

#include <cstring>
#include <memory>
#include <fstream>

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


class FS {
public:
    virtual void analize(std::ofstream& output, std::ofstream& meta_output);
    FS(Disk* disk);
    FS(){filesystem_ = nullptr;}
    virtual ~FS() {
        if (filesystem_) {
            delete filesystem_;
        }
    }
    
protected:
    Disk* disk_;
    
private:
    FS* filesystem_;
};

class NTFS : public FS {
public:
    NTFS(Disk* disk);
    ~NTFS();
    void analize(std::ofstream& output, std::ofstream& meta_output);
    
private:
    friend class FS;
    
    size_t read_attr(char* attr, uint64_t offset, size_t count, char* str);
    size_t read_runlist(uint64_t offset, size_t count, char* str, NTFSRunlistEntry* run_format);
    size_t read_al(NTFSMftEntry *fr, NTFSAttribute* attr, uint64_t fr_num, uint32_t type,
                    char* name, size_t offset, size_t count, char* str);
    size_t analize_nonres_attr(std::ofstream& output, std::ofstream& meta_output,
                    uint64_t fr_num, NTFSNonresidentAttr* attr, uint64_t base_fr_num);
    size_t analize_res_attr(std::ofstream& output, std::ofstream& meta_output,
                    uint64_t fr_num, NTFSResidentAttr* attr, uint64_t base_fr_num);
    size_t read_fr(uint64_t fr_num, uint32_t type, char* name, size_t offset, size_t count, char* str);
    uint64_t read_fr_for_attr_size(uint64_t base_fr_num, uint32_t type, char* name);
    uint64_t read_al_for_attr_size(NTFSAttribute *attr, NTFSMftEntry *fr, uint64_t base_fr_num,
                    uint32_t type, char* name); 
    size_t analize_fr(std::ofstream& output, std::ofstream& meta_output, uint64_t fr_num);
    void fixup(char * start);
      
    uint32_t sector_size_;
    uint32_t sectors_per_cluster_;
    uint32_t cluster_size_;
    uint32_t irecord_size_;
    uint64_t total_sectors_;
    uint64_t mft_cluster_;
    uint64_t mftmirr_cluster_;
    size_t fr_size_;
    NTFSMftEntry *mft_fr_;
    NTFSMftEntry *tmp_fr_;
};

class Ext : public FS {
public:
    Ext(Disk* disk);
    ~Ext();
    void analize(std::ofstream& output, std::ofstream& meta_output);

private:
    friend class FS;

    void analize_desc(std::ofstream& output, std::ofstream& meta_output, const ExtGroupDesc& desc, uint32_t group_num);
    void analize_inode(std::ofstream& output, std::ofstream& meta_output, const ExtInode* inode, uint32_t inode_num);
    void print_inode_metadata(std::ofstream& meta_output, const ExtInode* inode, uint32_t inode_num);
    void analize_block(std::ofstream& output, uint32_t& curr_offset, uint32_t block_phys_offset,
            uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
            uint64_t file_size, int depth, uint32_t inode_num);
    void analize_extent_node(std::ofstream& output, uint32_t& curr_offset, char* entry,
            uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
            uint64_t file_size, int depth, uint32_t inode_num);
    void analize_extent(std::ofstream& output, uint32_t& curr_offset, const ExtExtent* extent,
        uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
        uint64_t file_size, uint32_t inode_num);
    
    uint32_t inodes_count_;
    uint64_t blocks_count_;
    uint32_t first_block_;
    uint32_t block_size_;
    uint32_t cluster_size_;
    uint32_t blocks_per_group_;
    uint32_t frags_per_group_;
    uint32_t inodes_per_group_;
    uint32_t rev_level_;

    uint32_t inode_size_;
    uint32_t feature_compat_;
    uint32_t feature_incompat_;
    uint32_t feature_ro_compat_;
    uint16_t desc_size_;
    uint32_t first_meta_bg_;
    uint64_t groups_per_flex_;

    uint64_t kbytes_written_;
};


#endif	/* FS_H */
