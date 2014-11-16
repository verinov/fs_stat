#ifndef FS_H
#define	FS_H

#include "fs_stat.h"
#include "fs_structs.h"

#include <cstring>
#include <memory>
#include <fstream>



class NTFS : public FSParser {
public:
    NTFS(std::shared_ptr<Disk> disk);
    ~NTFS();
    void Parse(BlockFunc& printBlock, MetadataFunc& printMetadata);
    
private:
    friend class FSParser;
    
    size_t read_attr(char* attr, uint64_t offset, size_t count, char* str);
    size_t read_runlist(uint64_t offset, size_t count, char* str, NTFSRunlistEntry* run_format);
    size_t read_al(NTFSMftEntry *fr, NTFSAttribute* attr, uint64_t fr_num, uint32_t type,
                    char* name, size_t offset, size_t count, char* str);
    size_t analize_nonres_attr(BlockFunc& printBlock, uint64_t fr_num, 
                                 NTFSNonresidentAttr* attr, uint64_t base_fr_num);
    size_t analize_res_attr(MetadataFunc& printMetadata, uint64_t fr_num,
                              NTFSResidentAttr* attr, uint64_t base_fr_num);
    size_t read_fr(uint64_t fr_num, uint32_t type, char* name, size_t offset, size_t count, char* str);
    uint64_t read_fr_for_attr_size(uint64_t base_fr_num, uint32_t type, char* name);
    uint64_t read_al_for_attr_size(NTFSAttribute *attr, NTFSMftEntry *fr, uint64_t base_fr_num,
                    uint32_t type, char* name); 
    size_t analize_fr(BlockFunc& printBlock, MetadataFunc& printMetadata, uint64_t fr_num);
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

class Ext : public FSParser {
public:
    Ext(std::shared_ptr<Disk> disk);
    ~Ext();
    void Parse(BlockFunc& printBlock, MetadataFunc& printMetadata);

private:
    friend class FSParser;

    void analize_desc(BlockFunc& printBlock, MetadataFunc& printMetadata, const ExtGroupDesc& desc, uint32_t group_num);
    void analize_inode(BlockFunc& printBlock, MetadataFunc& printMetadata, const ExtInode* inode, uint32_t inode_num);
    void print_inode_metadata(MetadataFunc& printMetadata, const ExtInode* inode, uint32_t inode_num);
    void analize_block(BlockFunc& printBlock, uint32_t& curr_offset, uint32_t block_phys_offset,
            uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
            uint64_t file_size, int depth, uint32_t inode_num);
    void analize_extent_node(BlockFunc& printBlock, uint32_t& curr_offset, char* entry,
            uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
            uint64_t file_size, int depth, uint32_t inode_num);
    void analize_extent(BlockFunc& printBlock, uint32_t& curr_offset, const ExtExtent* extent,
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
