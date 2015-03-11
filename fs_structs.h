#ifndef FS_STRUCTS_H
#define	FS_STRUCTS_H

#include <stdint.h>

struct __attribute__((__packed__)) NTFSMftEntry {
    char signature[4]; // 0-3
    uint16_t fixup_offset; // 4-5
    uint16_t fixup_count; // 6-7
    uint64_t lsn; // 8-15
    uint16_t seq_value; // 16-17
    uint16_t link_count; // 18-19
    uint16_t first_attr_offset; // 20-21
    uint16_t flags; // 22-23
    uint32_t used_entry_size; // 24-27
    uint32_t alloc_entry_size; // 28-31
    uint64_t base_fr : 48; // 32-37
    uint16_t base_fr_seq_number;




    uint16_t next_attr_id; // 40-41
    //attributes and fixup values are omitted here
};

struct __attribute__((__packed__)) NTFSAttribute {
    uint32_t type_id;
    uint32_t attr_len;
    char nonresident_flag;
    char name_len;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attr_id;
};

struct __attribute__((__packed__)) NTFSResidentAttr {
    uint32_t type_id;
    uint32_t attr_len;
    char nonresident_flag;
    char name_len;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attr_id;
    uint32_t content_size;
    uint16_t content_offset;
};

struct __attribute__((__packed__)) NTFSNonresidentAttr {
    uint32_t type_id;
    uint32_t attr_len;
    char nonresident_flag;
    char name_len;
    uint16_t name_offset;
    uint16_t flags;
    uint16_t attr_id;
    uint64_t start_vcn;
    uint64_t end_vcn;
    uint16_t runlist_offset;
    uint16_t compress_unit_size;
    uint32_t unused;
    uint64_t allocated_content_size;
    uint64_t actual_content_size;
    uint64_t inited_content_size;
};

struct __attribute__((__packed__)) NTFSBootSector {
    char jump_intsuction[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    char sectors_per_cluster;
    uint16_t reserved_sectors;
    char unused1[5];
    char media_desc;
    char unused2[18];
    uint64_t total_sectors;
    uint64_t mft_cluster_address;
    uint64_t mft_mirror_cluster_address;
    char filerecord_size;
    char unused3[3];
    char index_record_size;
    char unused4[3];
    uint64_t serial_numer;
};

struct __attribute__((__packed__)) NTFSRunlistEntry {
    uint8_t runlen_length : 4;
    uint8_t offset_length : 4;
};

struct __attribute__((__packed__)) NTFSAttrListEntry {
    uint32_t type_id;
    uint16_t entry_len;
    uint8_t name_len;
    uint8_t name_offset;
    uint64_t start_vcn;
    uint64_t fr : 48;
    uint16_t fr_seq_number;
    uint8_t attr_id;
};

struct __attribute__((__packed__)) NTFSStdInfo {
    int64_t ctime;
    int64_t mtime;
    int64_t mtf_mtime;
    int64_t atime;
    uint32_t flags;
};

struct __attribute__((__packed__)) ExtSuperBlock {
    uint32_t s_inodes_count; /* Inodes count */
    uint32_t s_blocks_count_lo; /* Blocks count */
    uint32_t s_r_blocks_count_lo; /* Reserved blocks count */
    uint32_t s_free_blocks_count_lo; /* Free blocks count */
    uint32_t s_free_inodes_count; /* Free inodes count */
    uint32_t s_first_data_block; /* First Data Block */
    uint32_t s_log_block_size; /* Block size */
    uint32_t s_log_cluster_size; /* Allocation cluster size */
    uint32_t s_blocks_per_group; /* # Blocks per group */
    uint32_t s_clusters_per_group; /* # Clusters per group */
    uint32_t s_inodes_per_group; /* # Inodes per group */
    int32_t s_mtime; /* Mount time */
    int32_t s_wtime; /* Write time */
    uint16_t s_mnt_count; /* Mount count */
    uint16_t s_max_mnt_count; /* Maximal mount count */
    uint16_t s_magic; /* Magic signature */
    uint16_t s_state; /* File system state */
    uint16_t s_errors; /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level; /* minor revision level */
    int32_t s_lastcheck; /* time of last check */
    uint32_t s_checkinterval; /* max. time between checks */
    uint32_t s_creator_os; /* OS */
    uint32_t s_rev_level; /* Revision level */
    uint16_t s_def_resuid; /* Default uid for reserved blocks */
    uint16_t s_def_resgid; /* Default gid for reserved blocks */
    /*These fields are for EXT4_DYNAMIC_REV superblocks only.*/
    uint32_t s_first_ino; /* First non-reserved inode */
    uint16_t s_inode_size; /* size of inode structure */
    uint16_t s_block_group_nr; /* block group # of this superblock */
    uint32_t s_feature_compat; /* compatible feature set */
    uint32_t s_feature_incompat; /* incompatible feature set */
    uint32_t s_feature_ro_compat; /* readonly-compatible feature set */
    uint8_t s_uuid[16]; /* 128-bit uuid for volume */
    char s_volume_name[16]; /* volume name */
    char s_last_mounted[64]; /* directory where last mounted */
    uint32_t s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
     */
    uint8_t s_prealloc_blocks; /* Nr of blocks to try to preallocate*/
    uint8_t s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    uint16_t s_reserved_gdt_blocks; /* Per group desc for online growth */
    /*
     * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    uint8_t s_journal_uuid[16]; /* uuid of journal superblock */
    uint32_t s_journal_inum; /* inode number of journal file */
    uint32_t s_journal_dev; /* device number of journal file */
    uint32_t s_last_orphan; /* start of list of inodes to delete */
    uint32_t s_hash_seed[4]; /* HTREE hash seed */
    uint8_t s_def_hash_version; /* Default hash version to use */
    uint8_t s_jnl_backup_type;
    uint16_t s_desc_size; /* size of group descriptor */
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg; /* First metablock block group */
    int32_t s_mkfs_time; /* When the filesystem was created */
    uint32_t s_jnl_blocks[17]; /* Backup of the journal inode */
    /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
    uint32_t s_blocks_count_hi; /* Blocks count */
    uint32_t s_r_blocks_count_hi; /* Reserved blocks count */
    uint32_t s_free_blocks_count_hi; /* Free blocks count */
    uint16_t s_min_extra_isize; /* All inodes have at least # bytes */
    uint16_t s_want_extra_isize; /* New inodes should reserve # bytes */
    uint32_t s_flags; /* Miscellaneous flags */
    uint16_t s_raid_stride; /* RAID stride */
    uint16_t s_mmp_update_interval; /* # seconds to wait in MMP checking */
    uint64_t s_mmp_block; /* Block for multi-mount protection */
    uint32_t s_raid_stripe_width; /* blocks on all data disks (N*stride)*/
    uint8_t s_log_groups_per_flex; /* FLEX_BG group size */
    uint8_t s_checksum_type; /* metadata checksum algorithm used */
    uint16_t s_reserved_pad;
    uint64_t s_kbytes_written; /* nr of lifetime kilobytes written */
    uint32_t s_snapshot_inum; /* Inode number of active snapshot */
    uint32_t s_snapshot_id; /* sequential ID of active snapshot */
    uint64_t s_snapshot_r_blocks_count; /* reserved blocks for active
                                             snapshot's future use */
    uint32_t s_snapshot_list; /* inode number of the head of the
                                           on-disk snapshot list */
#define EXT4_S_ERR_START offsetof(struct ext4_super_block, s_error_count)
    uint32_t s_error_count; /* number of fs errors */
    uint32_t s_first_error_time; /* first time an error happened */
    uint32_t s_first_error_ino; /* inode involved in first error */
    uint64_t s_first_error_block; /* block involved of first error */
    uint8_t s_first_error_func[32]; /* function where the error happened */
    uint32_t s_first_error_line; /* line number where error happened */
    int32_t s_last_error_time; /* most recent time of an error */
    uint32_t s_last_error_ino; /* inode involved in last error */
    uint32_t s_last_error_line; /* line number where error happened */
    uint64_t s_last_error_block; /* block involved of last error */
    uint8_t s_last_error_func[32]; /* function where the error happened */
#define EXT4_S_ERR_END offsetof(struct ext4_super_block, s_mount_opts)
    uint8_t s_mount_opts[64];
    uint32_t s_usr_quota_inum; /* inode for tracking user quota */
    uint32_t s_grp_quota_inum; /* inode for tracking group quota */
    uint32_t s_overhead_clusters; /* overhead blocks/clusters in fs */
    uint32_t s_backup_bgs[2]; /* groups with sparse_super2 SBs */
    uint32_t s_reserved[106]; /* Padding to the end of the block */
    uint32_t s_checksum; /* crc32c(superblock) */
};

struct __attribute__((__packed__)) ExtGroupDesc {
    uint32_t bg_block_bitmap_lo; /* Blocks bitmap block */
    uint32_t bg_inode_bitmap_lo; /* Inodes bitmap block */
    uint32_t bg_inode_table_lo; /* Inodes table block */
    uint16_t bg_free_blocks_count_lo; /* Free blocks count */
    uint16_t bg_free_inodes_count_lo; /* Free inodes count */
    uint16_t bg_used_dirs_count_lo; /* Directories count */
    uint16_t bg_flags; /* EXT4_BG_flags (INODE_UNINIT, etc) */
    uint32_t bg_exclude_bitmap_lo; /* Exclude bitmap for snapshots */
    uint16_t bg_block_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bbitmap) LE */
    uint16_t bg_inode_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+ibitmap) LE */
    uint16_t bg_itable_unused_lo; /* Unused inodes count */
    uint16_t bg_checksum; /* crc16(sb_uuid+group+desc) */
    uint32_t bg_block_bitmap_hi; /* Blocks bitmap block MSB */
    uint32_t bg_inode_bitmap_hi; /* Inodes bitmap block MSB */
    uint32_t bg_inode_table_hi; /* Inodes table block MSB */
    uint16_t bg_free_blocks_count_hi; /* Free blocks count MSB */
    uint16_t bg_free_inodes_count_hi; /* Free inodes count MSB */
    uint16_t bg_used_dirs_count_hi; /* Directories count MSB */
    uint16_t bg_itable_unused_hi; /* Unused inodes count MSB */
    uint32_t bg_exclude_bitmap_hi; /* Exclude bitmap block MSB */
    uint16_t bg_block_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+bbitmap) BE */
    uint16_t bg_inode_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+ibitmap) BE */
    uint32_t bg_reserved;
};

struct __attribute__((__packed__)) ExtInode {
    uint16_t i_mode; /* File mode */
    uint16_t i_uid; /* Low 16 bits of Owner Uid */
    uint32_t i_size_lo; /* Size in bytes */
    int32_t i_atime; /* Access time */
    int32_t i_ctime; /* Inode Change time */
    int32_t i_mtime; /* Modification time */
    int32_t i_dtime; /* Deletion Time */
    uint16_t i_gid; /* Low 16 bits of Group Id */
    uint16_t i_links_count; /* Links count */
    uint32_t i_blocks_lo; /* Blocks count */
    uint32_t i_flags; /* File flags */

    union {

        struct {
            uint32_t l_i_version;
        } linux1;

        struct {
            uint32_t h_i_translator;
        } hurd1;

        struct {
            uint32_t m_i_reserved1;
        } masix1;
    } osd1; /* OS dependent 1 */
    uint32_t i_block[15]; /* Pointers to blocks */
    uint32_t i_generation; /* File version (for NFS) */
    uint32_t i_file_acl_lo; /* File ACL */
    uint32_t i_size_high;
    uint32_t i_obso_faddr; /* Obsoleted fragment address */

    union {

        struct {
            uint16_t l_i_blocks_high; /* were l_i_reserved1 */
            uint16_t l_i_file_acl_high;
            uint16_t l_i_uid_high; /* these 2 fields */
            uint16_t l_i_gid_high; /* were reserved2[0] */
            uint16_t l_i_checksum_lo; /* crc32c(uuid+inum+inode) LE */
            uint16_t l_i_reserved;
        } linux2;

        struct {
            uint16_t h_i_reserved1; /* Obsoleted fragment number/size which are removed in ext4 */
            uint16_t h_i_mode_high;
            uint16_t h_i_uid_high;
            uint16_t h_i_gid_high;
            uint32_t h_i_author;
        } hurd2;

        struct {
            uint16_t h_i_reserved1; /* Obsoleted fragment number/size which are removed in ext4 */
            uint16_t m_i_file_acl_high;
            uint32_t m_i_reserved2[2];
        } masix2;
    } osd2; /* OS dependent 2 */
    uint16_t i_extra_isize;
    uint16_t i_checksum_hi; /* crc32c(uuid+inum+inode) BE */
    uint32_t i_ctime_extra; /* extra Change time      (nsec << 2 | epoch) */
    uint32_t i_mtime_extra; /* extra Modification time(nsec << 2 | epoch) */
    uint32_t i_atime_extra; /* extra Access time      (nsec << 2 | epoch) */
    int32_t i_crtime; /* File Creation time */
    uint32_t i_crtime_extra; /* extra FileCreationtime (nsec << 2 | epoch) */
    uint32_t i_version_hi; /* high 32 bits for 64-bit version */
};

struct __attribute__((__packed__)) ExtExtentHeader {
    uint16_t eh_magic;
    uint16_t eh_entries; // number of valid entries
    uint16_t eh_max; // capacity of store in entries
    uint16_t eh_depth;
    uint32_t eh_generation; // generation of the tree
};

struct __attribute__((__packed__)) ExtExtent {
    uint32_t ee_block; // first logical block extent covers
    uint16_t ee_len; // number of blocks covered by extent
    uint16_t ee_start_hi; // high 16 bits of physical block
    uint32_t ee_start_lo; // low 32 bits of physical block
};

struct __attribute__((__packed__)) ExtExtentIndex {
    uint32_t ei_block; // index covers logical blocks from 'block'
    uint32_t ei_leaf_lo; // pointer to the physical block of the next level
    uint16_t ei_leaf_hi; // high 16 bits of physical block
    uint16_t ei_unused;
};

#endif	/* FS_STRUCTS_H */

