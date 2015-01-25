#include "FS.h"

#include <memory>

enum ExtConsts {
    COMPAT_DIR_PREALLOC = 0x1,
    COMPAT_IMAGIC_INODES = 0x2,
    COMPAT_HAS_JOURNAL = 0x4,
    COMPAT_EXT_ATTR = 0x8,
    COMPAT_RESIZE_INODE = 0x10,
    COMPAT_DIR_INDEX = 0x20,
    COMPAT_LAZY_BG = 0x40,
    COMPAT_EXCLUDE_INODE = 0x80,
    COMPAT_EXCLUDE_BITMAP = 0x100,
    COMPAT_SPARSE_SUPER2 = 0x200,

    INCOMPAT_COMPRESSION = 0x1,
    INCOMPAT_FILETYPE = 0x2,
    INCOMPAT_RECOVER = 0x4,
    INCOMPAT_JOURNAL_DEV = 0x8,
    INCOMPAT_META_BG = 0x10,
    INCOMPAT_EXTENTS = 0x40,
    INCOMPAT_64BIT = 0x80,
    INCOMPAT_MMP = 0x100,
    INCOMPAT_FLEX_BG = 0x200,
    INCOMPAT_EA_INODE = 0x400,
    INCOMPAT_DIRDATA = 0x1000,
    INCOMPAT_BG_USE_META_CSUM = 0x2000,
    INCOMPAT_LARGEDIR = 0x4000,
    INCOMPAT_INLINE_DATA = 0x8000,

    RO_COMPAT_SPARSE_SUPER = 0x1,
    RO_COMPAT_LARGE_FILE = 0x2,
    RO_COMPAT_BTREE_DIR = 0x4,
    RO_COMPAT_HUGE_FILE = 0x8,
    RO_COMPAT_GDT_CSUM = 0x10,
    RO_COMPAT_DIR_NLINK = 0x20,
    RO_COMPAT_EXTRA_ISIZE = 0x40,
    RO_COMPAT_HAS_SNAPSHOT = 0x80,
    RO_COMPAT_QUOTA = 0x100,
    RO_COMPAT_BIGALLOC = 0x200,
    RO_COMPAT_METADATA_CSUM = 0x400, //implies RO_COMPAT_GDT_CSUM, though GDT_CSUM must not be set

    EXT4_BG_INODE_UNINIT = 0x1,
    EXT4_BG_BLOCK_UNINIT = 0x2,
    EXT4_BG_INODE_ZEROED = 0x4
};

Ext::Ext(std::shared_ptr<Disk> disk) {
    disk_ = disk;

    std::unique_ptr<ExtSuperBlock> sb(new ExtSuperBlock);
    disk_->read(sb.get(), 1024, 1024);

    inodes_count_ = sb->s_inodes_count;
    blocks_count_ = sb->s_blocks_count_lo;
    first_block_ = sb->s_first_data_block;
    block_size_ = 1 << (sb->s_log_block_size + 10);
    cluster_size_ = 1 << (sb->s_log_cluster_size);
    blocks_per_group_ = sb->s_blocks_per_group;
    frags_per_group_ = sb->s_clusters_per_group;
    inodes_per_group_ = sb->s_inodes_per_group;
    rev_level_ = sb->s_rev_level;

    if (rev_level_) {
        inode_size_ = sb->s_inode_size;
        feature_compat_ = sb->s_feature_compat;
        feature_incompat_ = sb->s_feature_incompat;
        feature_ro_compat_ = sb->s_feature_ro_compat;
        desc_size_ = INCOMPAT_64BIT & feature_incompat_ ?
                sb->s_desc_size : 32;
        first_meta_bg_ = INCOMPAT_META_BG & feature_incompat_ ?
                sb->s_first_meta_bg : (blocks_count_ - 1) / blocks_per_group_ + 1;

        if (INCOMPAT_64BIT & feature_incompat_) {
            blocks_count_ += ((uint64_t) sb->s_blocks_count_hi << 32);
        }

        groups_per_flex_ = 1 << sb->s_log_groups_per_flex;
        kbytes_written_ = sb->s_kbytes_written;
    } else {
        inode_size_ = 128;
        feature_compat_ = feature_incompat_ = feature_ro_compat_ = 0;
        desc_size_ = 32;
        first_meta_bg_ = (blocks_count_ - 1) / blocks_per_group_ + 1;
        kbytes_written_ = 0;
        groups_per_flex_ = 1;
    }

    if (~(~feature_incompat_ | INCOMPAT_FILETYPE | INCOMPAT_META_BG | INCOMPAT_RECOVER | // TODO: deal with RECOVER
            INCOMPAT_EXTENTS | INCOMPAT_64BIT | INCOMPAT_FLEX_BG | INCOMPAT_INLINE_DATA)) {
        throw std::runtime_error("can't read this fs, some incompatible features are not supported");
    }
}

Ext::~Ext() {
}

void Ext::Parse(BlockFunc& printBlock, MetadataFunc& printMetadata) {
    ExtGroupDesc bg_desc;

    int bg_per_metabg = block_size_ / desc_size_;
    uint64_t meta_bg_start = first_meta_bg_ ? first_meta_bg_ : bg_per_metabg;

    //first groups in the beginning are in the same "meta_bg"
    for (int bg = 0; bg < meta_bg_start; bg++) {
        disk_->read(&bg_desc, desc_size_, (first_block_ + 1) * block_size_ + bg * desc_size_);
        analize_desc(printBlock, printMetadata, bg_desc, bg);
    }

    //process metablocks
    for (uint64_t metabg_first_bg = meta_bg_start; blocks_per_group_ * metabg_first_bg < blocks_count_;
            metabg_first_bg += bg_per_metabg) {
        for (int bg = 0; bg < bg_per_metabg && blocks_per_group_ * (metabg_first_bg + bg - 1) < blocks_count_; bg++) {
            disk_->read(&bg_desc, desc_size_, (1 + first_block_ + metabg_first_bg * blocks_per_group_) * block_size_ +
                    bg * desc_size_);
            analize_desc(printBlock, printMetadata, bg_desc, metabg_first_bg + bg);
        }
    }
}

void Ext::analize_desc(BlockFunc& printBlock, MetadataFunc& printMetadata, const ExtGroupDesc& desc, uint32_t group_num) {
    if (desc.bg_flags & EXT4_BG_INODE_UNINIT) {
        return;
    }

    uint64_t inode_bitmap_off = desc.bg_inode_bitmap_lo;
    uint64_t inode_table_off = desc.bg_inode_table_lo;
    uint32_t free_inodes_count = desc.bg_free_inodes_count_lo;

    if ((INCOMPAT_64BIT & feature_incompat_) && desc_size_ > 32) {
        inode_bitmap_off += ((uint64_t) desc.bg_inode_bitmap_hi << 32);
        inode_table_off += ((uint64_t) desc.bg_inode_table_hi << 32);
        free_inodes_count += ((uint32_t) desc.bg_free_inodes_count_hi << 16);
    }

    int byte_count = inodes_per_group_ / 8 < block_size_ ? inodes_per_group_ / 8 : block_size_;
    
    std::unique_ptr<char> bitmap_chunk(new char[byte_count]);
    std::unique_ptr<char> inode(new char[inode_size_]);

    for (uint64_t k = 0; 8 * k < inodes_per_group_; k += byte_count) {
        disk_->read(bitmap_chunk.get(), byte_count, (first_block_ + inode_bitmap_off) * block_size_ + k);
        for (int i = 0; i < byte_count; i++) {
            if (bitmap_chunk.get()[i]) {
                for (int j = 0; j < 8; j++) {
                    if (bitmap_chunk.get()[i] & (1 << j)) {
                        disk_->read(inode.get(), inode_size_,
                                (first_block_ + inode_table_off) * block_size_ + inode_size_ * (8 * i + j));
                        analize_inode(printBlock, printMetadata, (ExtInode*) inode.get(),
                                group_num * inodes_per_group_ + 8 * (k + i) + j + 1);
                    }
                }
            }
        }
    }
}

void Ext::print_inode_metadata(MetadataFunc& printMetadata, const ExtInode* inode, uint32_t inode_num) {
    return; // approx. -15% time
    uint64_t file_size = inode->i_size_lo;
    file_size += ((uint64_t) inode->i_size_high << 32);

    int64_t ctime, atime, mtime, crtime;
    bool encrypt_flag, compressed_flag;
    compressed_flag = 0x4 & inode->i_flags;
    encrypt_flag = 0;

    ctime = 1000000000 * (int64_t) inode->i_ctime;
    atime = 1000000000 * (int64_t) inode->i_atime;
    mtime = 1000000000 * (int64_t) inode->i_mtime;
    crtime = 1000000000 * (int64_t) inode->i_crtime;
    if (inode->i_extra_isize >= 24) {
        ctime = ctime + (inode->i_ctime_extra % 4) * 0x100000000 + (inode->i_ctime_extra >> 2);
        atime = atime + (inode->i_atime_extra % 4) * 0x100000000 + (inode->i_atime_extra >> 2);
        mtime = mtime + (inode->i_mtime_extra % 4) * 0x100000000 + (inode->i_mtime_extra >> 2);
        crtime = crtime + (inode->i_crtime_extra % 4) * 0x100000000 + (inode->i_crtime_extra >> 2);
    }
    
    printMetadata(inode_num, file_size, compressed_flag, encrypt_flag, ctime, mtime, atime);
}

void Ext::analize_inode(BlockFunc& printBlock, MetadataFunc& printMetadata, const ExtInode* inode, uint32_t inode_num) {
    if (inode->i_links_count == 0) {
        return;
    }

    uint64_t file_size = inode->i_size_lo;
    file_size += ((uint64_t) inode->i_size_high << 32); // in ext2/3 should be 0

    bool extents_flag = 0x80000 & inode->i_flags;
    bool huge_file_flag = 0x40000 & inode->i_flags;
    bool ea_inode_flag = 0x200000 & inode->i_flags; // TODO: do we need extended attributes?
    bool inline_data_flag = 0x10000000 & inode->i_flags;

    print_inode_metadata(printMetadata, inode, inode_num);

    if (inline_data_flag) {
        // there are no blocks for this inode
        return;
    }

    //all values are in blocks
    uint32_t curr_offset = 0, start_offset = 0, start_phys_offset = 0, next_phys_offset = 0;

    if (extents_flag) {
        //extents
        ExtExtentHeader* extent_header = (ExtExtentHeader*) inode->i_block;
        int entry_count = extent_header->eh_entries;
        int depth = extent_header->eh_depth;
        for (int i = 1; i <= entry_count; ++i) {
            analize_extent_node(printBlock, curr_offset, (char*) inode->i_block + 12 * i,
                    start_offset, start_phys_offset, next_phys_offset, file_size, depth, inode_num);
        }

        if (start_phys_offset != 0) {
            printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
        }
    } else {
        //block map
        for (uint32_t record = 0; record < 12 && curr_offset * block_size_ < file_size; ++record) {
            analize_block(printBlock, curr_offset, inode->i_block[record],
                    start_offset, start_phys_offset, next_phys_offset, file_size, 0, inode_num);
        }

        for (int i = 1; i <= 3 && curr_offset * block_size_ < file_size; ++i) {
            analize_block(printBlock, curr_offset, inode->i_block[11 + i],
                    start_offset, start_phys_offset, next_phys_offset, file_size, i, inode_num);
        }

        if (start_phys_offset != 0) {
            printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
        }
    }
}

inline uint32_t power(uint32_t base, int power) {
    uint32_t potential = 1;
    for (int i = 0; i < power; i++) {
        potential *= base;
    }
    return potential;
}

//mapping only applicable for lower 2^32 blocks
void Ext::analize_block(BlockFunc& printBlock, uint32_t& curr_offset, uint32_t block_phys_offset,
        uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
        uint64_t file_size, int depth, uint32_t inode_num) {

    if (curr_offset * block_size_ >= file_size) {
        throw std::runtime_error("went out of the file, should never happen");
    }

    if (block_phys_offset == 0) {
        //everything in this subtree is zeroes
        if (start_phys_offset != 0) {
            printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
        }

        start_offset = -1; //just for debug, this value shouldn't be used anywhere
        next_phys_offset = start_phys_offset = 0;
        curr_offset += power(block_size_ / 4, depth);
        return;
    }

    if (depth == 0) {
        //leaf node of extent tree
        if (block_phys_offset == next_phys_offset) {
            next_phys_offset++;
        } else {
            if (start_phys_offset != 0) {
                printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
            }
            start_offset = curr_offset;
            start_phys_offset = block_phys_offset;
            next_phys_offset = start_phys_offset + 1;
        }
        curr_offset++;

    } else {
        //interior node of extent tree
        std::unique_ptr<char> block(new char[block_size_]);
        disk_->read(block.get(), block_size_, block_phys_offset * block_size_);

        for (uint32_t record = 0; record < block_size_ && curr_offset * block_size_ < file_size; record += 4) {
            analize_block(printBlock, curr_offset, *((uint32_t*) (block.get() + record)), start_offset, start_phys_offset,
                    next_phys_offset, file_size, depth - 1, inode_num);
        }
    }
}

void Ext::analize_extent_node(BlockFunc& printBlock, uint32_t& curr_offset, char* entry,
        uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
        uint64_t file_size, int depth, uint32_t inode_num) {

    if (curr_offset * block_size_ >= file_size) {
        throw std::runtime_error("went out of the file, should never happen");
    }

    if (depth == 0) {
        analize_extent(printBlock, curr_offset, (ExtExtent*) entry,
                start_offset, start_phys_offset, next_phys_offset,
                file_size, inode_num);
    } else {
        //read header and go through entries
        ExtExtentIndex* extent_index = (ExtExtentIndex*) entry;
        uint64_t node_phys_offset = extent_index->ei_leaf_lo + ((uint64_t) extent_index->ei_leaf_hi << 32);
        std::unique_ptr<char> node_block(new char[block_size_]);
        disk_->read(node_block.get(), block_size_, node_phys_offset * block_size_);

        ExtExtentHeader* extent_header = (ExtExtentHeader*) node_block.get();
        int entry_count = extent_header->eh_entries;
        int depth = extent_header->eh_depth;
        for (int i = 1; i <= entry_count; i++) {
            analize_extent_node(printBlock, curr_offset, node_block.get() + 12 * i,
                    start_offset, start_phys_offset, next_phys_offset, file_size, depth, inode_num);
        }
    }
}

void Ext::analize_extent(BlockFunc& printBlock, uint32_t& curr_offset, const ExtExtent* extent,
        uint32_t& start_offset, uint32_t& start_phys_offset, uint32_t& next_phys_offset,
        uint64_t file_size, uint32_t inode_num) {

    uint16_t len = extent->ee_len;

    if (len <= 32768) {
        // initialized extent
        uint64_t extent_phys_offset = extent->ee_start_lo + ((uint64_t) extent->ee_start_hi << 32);

        if (extent_phys_offset == next_phys_offset) {
            // append this extent to the row
            next_phys_offset += len;
        } else {
            // row breaks
            if (start_phys_offset != 0) {
                printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
            }
            start_offset = curr_offset;
            start_phys_offset = extent_phys_offset;
            next_phys_offset = start_phys_offset + len;

        }
        curr_offset += len;
    } else {
        // uninitialized extent
        len -= 32768;
        if (start_phys_offset != 0) {
            printBlock(std::to_string(inode_num), file_size, start_offset,
                    start_phys_offset, curr_offset - start_offset);
        }

        start_offset = -1; // just for debug, this value shouldn't be used anywhere
        next_phys_offset = start_phys_offset = 0;
        curr_offset += len;
        return;
    }
}
