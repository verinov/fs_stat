#include "FS.h"

#include <memory>

enum {
    ATTR_COMPRESSED = 1,
    ATTR_ENCRYPTED = 0x4000,
    ATTR_SPARSE = 0x8000
};

inline uint64_t MIN(uint64_t x, uint64_t y){
    return x < y ? x : y;
}

NTFS::NTFS(std::shared_ptr<Disk> disk) {
    disk_ = disk;

    std::unique_ptr<NTFSBootSector> boot(new NTFSBootSector);
    disk_->read(boot.get(), sizeof(NTFSBootSector), 0);

    sector_size_ = boot->bytes_per_sector;
    sectors_per_cluster_ = boot->sectors_per_cluster;
    cluster_size_ = sector_size_ * sectors_per_cluster_;
    total_sectors_ = boot->total_sectors;
    mft_cluster_ = boot->mft_cluster_address;
    mftmirr_cluster_ = boot->mft_mirror_cluster_address;

    {
        int tmp = boot->filerecord_size;
        fr_size_ = tmp < 0 ? 1 << (-tmp) : tmp * cluster_size_;
        tmp = boot->index_record_size;
        irecord_size_ = tmp < 0 ? 1 << (-tmp) : tmp * cluster_size_;
    }

    mft_fr_ = (NTFSMftEntry *) new char[fr_size_];
    tmp_fr_ = (NTFSMftEntry *) new char[fr_size_];

    disk_->read(mft_fr_, fr_size_, mft_cluster_ * cluster_size_);

    fixup((char*) mft_fr_);
}

NTFS::~NTFS() {
    delete mft_fr_;
    delete tmp_fr_;
}

void NTFS::fixup(char * start) {
    unsigned fixup_off = *((unsigned*) (start + 4)) % 0x10000; //offset to fixup value and array
    unsigned fixup_count = *((unsigned*) (start + 6)) % 0x10000 - 1; //entries in fixup array
    for (unsigned i = 0; i < fixup_count; i++) {
        if (start[(i + 1) * sector_size_ - 2] != start[fixup_off] ||
                start[(i + 1) * sector_size_ - 1] != start[fixup_off + 1]) {
            throw std::runtime_error("fixup mismatch");;
        }
        start[(i + 1) * sector_size_ - 2] = start[fixup_off + 2 + i * 2];
        start[(i + 1) * sector_size_ - 1] = start[fixup_off + 3 + i * 2];
    }
    if (!memcmp(start, "BAAD", 4)) {
        throw std::runtime_error("BAAD");
    }
}

size_t utf16_to_utf8(uint16_t* start16, uint16_t* end16, char* start8, char* end8) {

    uint16_t* ptr16 = start16;
    char* ptr8 = start8;
    uint32_t chr;
    while (ptr16 < end16 && ptr8 < end8) {
        if (((*ptr16) & 0xd800) == 0xd800) {
            if (ptr8 + 3 >= end8 || ptr16 + 1 >= end16) {
                break;
            }
            chr = ((*ptr16 % 0x400) << 10) + *(ptr16 + 1) % 0x400 + 0x10000;
            ptr16 += 2;

            *(ptr8++) = 0xf0 + (chr >> 18);
            *(ptr8++) = 0x80 + (chr >> 12) % 0x40;
            *(ptr8++) = 0x80 + (chr >> 6) % 0x40;
            *(ptr8++) = 0x80 + chr % 0x40;
        } else {
            chr = *ptr16;

            if (chr < 0x80) {
                *(ptr8++) = chr;
            } else if (chr < 0x800) {
                if (ptr8 + 1 >= end8) {
                    break;
                }
                *(ptr8++) = 0xc0 + (chr >> 6);
                *(ptr8++) = 0x80 + chr % 0x40;
            } else {
                if (ptr8 + 2 >= end8) {
                    break;
                }
                *(ptr8++) = 0xe0 + (chr >> 12);
                *(ptr8++) = 0x80 + (chr >> 6) % 0x40;
                *(ptr8++) = 0x80 + chr % 0x40;
            }

            ptr16 += 1;
        }
    }
    return ptr8 - start8;
}

inline NTFSAttribute* attr_shift(NTFSAttribute* &attr, size_t value) {
    return attr = (NTFSAttribute*) ((char*) attr + value);
}

size_t NTFS::read_fr(uint64_t fr_num, uint32_t type, char* name, size_t offset, size_t count, char* str) {
    //careful!! don't call read_fr or read_attr_size from read_fr for non-zero fr_num!!
    NTFSMftEntry *fr = tmp_fr_;
    if (fr_num == 0) {
        fr = mft_fr_;
    } else {
        read_fr(0, 128, nullptr, fr_num * fr_size_, fr_size_, (char*) fr);
        fixup((char*) fr);
    }

    NTFSAttribute* attr = (NTFSAttribute*) ((char*) fr + fr->first_attr_offset);

    for (; attr->type_id <= 32 || attr->type_id <= type; attr_shift(attr, attr->attr_len)) {
        if (attr->flags & ATTR_COMPRESSED || attr->flags & ATTR_ENCRYPTED) {
            throw std::runtime_error("compressed or encrypted!!!");
        }

        char attr_name[256];
        if (attr->name_len) {
            memcpy(attr_name, (char*) attr + attr->name_offset, attr->name_len * 2);
        }

        if (attr->type_id == type && (!(name || attr->name_len) || 
                (name && attr->name_len && !memcmp(attr_name, name, attr->name_len * 2)))) {
            return read_attr((char*) attr, offset, count, str);
        }

        if (attr->type_id == 32) {
            return read_al(fr, attr, fr_num, type, name, offset, count, str);
        }
    }
    //Couldn't find this attribute
    return 0;
}

size_t NTFS::read_al(NTFSMftEntry *fr, NTFSAttribute* attr, uint64_t fr_num, uint32_t type, 
                     char* name, size_t offset, size_t count, char* str) {
    NTFSAttrListEntry* list_entry = (NTFSAttrListEntry*) std::unique_ptr<char>(new char[280]).get();
    size_t list_entry_offset = 0;
    size_t bytes_read = 0;

    while (count && read_attr((char*) attr, list_entry_offset, 280, (char*) list_entry)) {
        list_entry_offset += list_entry->entry_len;

        if (list_entry->type_id == 0) {
            break;
        }
        if (list_entry->type_id != type) {
            continue;
        }
        if (memcmp(name, (char*) list_entry + list_entry->name_offset, list_entry->name_len)) {
            continue;
        }

        uint64_t vcn_start = list_entry->start_vcn;
        if (offset < vcn_start * cluster_size_) {
            throw std::runtime_error("entry is missing or not in order of vcn's");
        }

        NTFSMftEntry * nonbase_fr;
        uint64_t nonbase_fr_num = list_entry->fr;
        unsigned attr_id = list_entry->attr_id;
        
        std::unique_ptr<char> nbfr_placefolder;

        if (fr_num == nonbase_fr_num) {
            nonbase_fr = fr;
        } else {
            nbfr_placefolder = std::unique_ptr<char>(new char[fr_size_]);
            nonbase_fr = (NTFSMftEntry*) nbfr_placefolder.get();
            read_fr(0, 128, nullptr, nonbase_fr_num * fr_size_, fr_size_, (char*) nonbase_fr);
            fixup((char*) nonbase_fr);
        }

        NTFSAttribute* tmp_attr = (NTFSAttribute*) ((char*) nonbase_fr + nonbase_fr->first_attr_offset);
        unsigned tmp_attr_offset = nonbase_fr->first_attr_offset;
        for (; tmp_attr->type_id != 0xffffffff && count; attr_shift(tmp_attr, tmp_attr->attr_len)) {
            if (tmp_attr->attr_id == attr_id) {
                size_t tmp_bytes_read = read_attr((char*) tmp_attr,
                        offset - vcn_start*cluster_size_, count, str);
                offset += tmp_bytes_read;
                count -= tmp_bytes_read;
                str += tmp_bytes_read;
                bytes_read += tmp_bytes_read;
            }
        }
    }
    return bytes_read;
}

size_t NTFS::read_attr(char* attr_header, uint64_t offset, size_t count, char* str) {
    if (((NTFSAttribute*) attr_header)->nonresident_flag) {
        //non-resident
        NTFSNonresidentAttr* attr = (NTFSNonresidentAttr*) attr_header;
        if (attr->actual_content_size < offset) {
            return 0; //no bytes on this offset in the attribute
        }
        return read_runlist(offset, count, str, (NTFSRunlistEntry*) attr + attr->runlist_offset);
    } else {
        NTFSResidentAttr* attr = (NTFSResidentAttr*) attr_header;
        uint32_t content_size = attr->content_size;
        uint32_t content_offset = attr->content_offset;
        if (count + offset > content_size) {
            throw;
        }
        memcpy(str, (char*) attr_header + content_offset + offset, count);
        return count;
    }
}

size_t NTFS::read_runlist(uint64_t offset, size_t count, char* str, NTFSRunlistEntry* run_format) {
    uint length_mod = (1 << (8 * run_format->runlen_length));
    uint64_t run_length = (*((uint64_t*) (run_format + 1))) % length_mod;
    int offset_size = 8 * run_format->offset_length;
    uint64_t run_offset = (((*((int64_t*) (run_format + 1 +
            run_format->runlen_length))) << (64 - offset_size)) >> (64 - offset_size));
    size_t bytes_read = 0;
    uint64_t vcn = 0;

    while (*(char*) run_format && count) {
        if (offset < vcn * cluster_size_) {
            throw std::runtime_error("DATA MISSING");
        }
        size_t tmp_bytes_read = 0;

        size_t len = 0;
        if ((vcn + run_length) * cluster_size_ > offset) {
            len = MIN(count,(run_length + vcn) * cluster_size_ - offset);
            if (offset_size) {
                disk_->read(str, len, (run_offset - vcn) * cluster_size_ + offset);
            } else {
                //sparse
                memset(str, 0, len);
            }
        }
        //go to the next entry in the run list
        run_format = run_format + 1 + run_format->runlen_length + run_format->offset_length;
        vcn += run_length;
        length_mod = (1 << (8 * run_format->runlen_length));
        run_length = (*((uint64_t*) (run_format + 1))) % length_mod;
        offset_size = 8 * run_format->offset_length;
        run_offset = run_offset + (((*((int64_t*) (run_format + 1 +
                run_format->runlen_length))) << (64 - offset_size)) >> (64 - offset_size));

        offset += len;
        count -= len;
        str += len;
        bytes_read += len;
    } // while run list isn't read

    return bytes_read;
}

size_t NTFS::analize_fr(BlockFunc& printBlock, MetadataFunc& printMetadata, uint64_t fr_num) {
    std::unique_ptr<char> fr_placefolder;
    NTFSMftEntry *fr;
    if (fr_num == 0) {
        fr = mft_fr_;
    } else {
        fr_placefolder = std::unique_ptr<char>(new char[fr_size_]);
        fr = (NTFSMftEntry*) fr_placefolder.get();
        read_fr(0, 128, nullptr, fr_num * fr_size_, fr_size_, (char*) fr);
        fixup((char*) fr);
    }

    NTFSAttribute* basic_attr = (NTFSAttribute*) ((char*) fr + fr->first_attr_offset);
    uint64_t base_fr_num = fr->base_fr == 0 ? fr_num : fr->base_fr;

    for (; basic_attr->type_id != 0xffffffff; attr_shift(basic_attr, basic_attr->attr_len)) {
        if (basic_attr->nonresident_flag) {
            analize_nonres_attr(printBlock, fr_num, (NTFSNonresidentAttr*) basic_attr, base_fr_num);
        } else {
            analize_res_attr(printMetadata, fr_num, (NTFSResidentAttr*) basic_attr, base_fr_num);
        }
    }
    
    return 0;
}

size_t NTFS::analize_nonres_attr(BlockFunc& printBlock, uint64_t fr_num, 
                                 NTFSNonresidentAttr* attr, uint64_t base_fr_num) {
    uint16_t attr_name[127];
    if (attr->name_len) {
        memcpy((char*) attr_name, (char*) attr + attr->name_offset, attr->name_len * 2);
    }
    attr_name[attr->name_len] = 0;

    uint64_t actual_size;
    if (base_fr_num == fr_num) {
        actual_size = attr->actual_content_size;
    } else {
        actual_size = read_fr_for_attr_size(base_fr_num, attr->type_id, attr->name_len ? (char*) attr_name : nullptr);
    }

    NTFSRunlistEntry* run_format = ((NTFSRunlistEntry*) attr + attr->runlist_offset);
    uint length_mod = (1 << (8 * run_format->runlen_length));
    uint64_t run_length = (*((uint64_t*) (run_format + 1))) % length_mod;
    int offset_size = 8 * run_format->offset_length;
    uint64_t run_offset = (((*((int64_t*) (run_format + 1 +
            run_format->runlen_length))) << (64 - offset_size)) >> (64 - offset_size));

    uint64_t vcn = attr->start_vcn;
    
    std::string fileId = std::to_string(base_fr_num) + ":" + 
            std::to_string(attr->type_id);
    
    // TODO: deal with attr name
/*    
    std::unique_ptr<char> attr_name8(new char[attr->name_len * 2]);
    size_t str_len = utf16_to_utf8(attr_name, attr_name + attr->name_len,
            attr_name8.get(), attr_name8.get() + attr->name_len * 2);
    output.write(attr_name8.get(), str_len);
*/
    while (*(char*) run_format) {
        
        printBlock(fileId, actual_size, vcn, run_offset, run_length);

        run_format = run_format + 1 + run_format->runlen_length + run_format->offset_length;
        vcn += run_length;
        length_mod = (1 << (8 * run_format->runlen_length));
        run_length = (*((uint64_t*) (run_format + 1))) % length_mod;
        offset_size = 8 * run_format->offset_length;
        run_offset = run_offset + (((*((int64_t*) (run_format + 1 +
                run_format->runlen_length))) << (64 - offset_size)) >> (64 - offset_size));
    }//while run list isn't read
}

size_t NTFS::analize_res_attr(MetadataFunc& printMetadata, uint64_t fr_num,
                              NTFSResidentAttr* attr, uint64_t base_fr_num) {
    if (attr->type_id == 16) {
        NTFSStdInfo* std_info = (NTFSStdInfo*) ((char*) attr + attr->content_offset);

        uint32_t flags = std_info->flags;
        bool compressed_flag = flags & 0x800;
        bool encrypt_flag = flags & 0x4000;

        if (base_fr_num == fr_num) {
            
            printMetadata(fr_num, read_fr_for_attr_size(fr_num, 128, nullptr),
        compressed_flag, encrypt_flag, std_info->ctime, std_info->mtime, std_info->atime);
        }
    }
}

void NTFS::Parse(BlockFunc& printBlock, MetadataFunc& printMetadata) {
    const unsigned byte_count = 512;
    std::unique_ptr<char[]> bitmap_block(new char[byte_count]);

    uint64_t bitmap_size = read_fr_for_attr_size(0, 176, nullptr);
    //type == 176 for $BITMAP attribute
    for (uint64_t offset = 0; offset < bitmap_size; offset += byte_count) {
        size_t br = read_fr(0, 176, 0, offset, MIN(byte_count, bitmap_size - offset), bitmap_block.get());
        for (int i = 0; i < br; i++) {
            if (bitmap_block[i]) {
                for (int j = 0; j < 8; j++) {
                    if (bitmap_block[i] & (1 << j)) {
                        analize_fr(printBlock, printMetadata, 8 * (offset + i) + j);
                    }
                }
            }
        }
    }
}

uint64_t NTFS::read_fr_for_attr_size(uint64_t base_fr_num, uint32_t type, char* name) {
    //careful!! don't call read_fr or read_attr_size from read_attr_size for non-zero fr_num!!
    NTFSMftEntry *fr = tmp_fr_;
    if (base_fr_num == 0) {
        fr = mft_fr_;
    } else {
        read_fr(0, 128, nullptr, base_fr_num * fr_size_, fr_size_, (char*) fr);
        fixup((char*) fr);
    }
    NTFSAttribute *attr = (NTFSAttribute*) ((char*) fr + fr->first_attr_offset);

    for (; attr->type_id <= 32 || attr->type_id <= type; attr_shift(attr, attr->attr_len)) {
        char attr_name[258];
        if (attr->name_len) {
            memcpy(attr_name, (char*) attr + attr->name_offset, attr->name_len * 2);
        }
        if (attr->type_id == type && (!(name || attr->name_len) || (name && attr->name_len && !memcmp(attr_name, name, 2 * attr->name_len)))) {
            if (attr->nonresident_flag) {
                return ((NTFSNonresidentAttr*) attr)->actual_content_size;
            } else {
                return ((NTFSResidentAttr*) attr)->content_size;
            }
        }
        if (attr->type_id == 32) {
            return read_al_for_attr_size(attr, fr, base_fr_num, type, name);
        }
    }
    return 0;
}

uint64_t NTFS::read_al_for_attr_size(NTFSAttribute *attr, NTFSMftEntry *fr, 
                                     uint64_t base_fr_num, uint32_t type, char* name) {
    char list_entry_buffer[280];
    NTFSAttrListEntry* list_entry = (NTFSAttrListEntry*) list_entry_buffer;
    size_t list_entry_offset = 0;

    for (; read_attr((char*) attr, list_entry_offset, 280, (char*) list_entry); list_entry_offset += list_entry->entry_len) {
        if ((list_entry->type_id != type) || (list_entry->start_vcn != 0) ||
                memcmp(name, (char*) list_entry + list_entry->name_offset, list_entry->name_len)) {
            continue;
        }

        NTFSMftEntry* nonbase_fr;
        uint64_t nonbase_fr_num = list_entry->fr;
        
        std::unique_ptr<char> nbfr_placefolder;

        if (base_fr_num == nonbase_fr_num) {
            nonbase_fr = fr;
        } else {
            nbfr_placefolder = std::unique_ptr<char>(new char[fr_size_]);
            nonbase_fr = (NTFSMftEntry*) nbfr_placefolder.get();
            read_fr(0, 128, nullptr, nonbase_fr_num * fr_size_, fr_size_, (char*) nonbase_fr);
            fixup((char*) nonbase_fr);
        }

        NTFSAttribute* tmp_attr = (NTFSAttribute*) ((char*) nonbase_fr + nonbase_fr->first_attr_offset);
        for (; tmp_attr->type_id != 0xffffffff; attr_shift(tmp_attr, tmp_attr->attr_len)) {
            if (tmp_attr->attr_id == list_entry->attr_id) {
                uint64_t result;
                if (tmp_attr->nonresident_flag) {
                    result = ((NTFSNonresidentAttr*) tmp_attr)->actual_content_size;
                } else {
                    result = ((NTFSResidentAttr*) tmp_attr)->content_size;
                }
                return result;
            }
        }
    }
    throw std::runtime_error("attr missing");
}
