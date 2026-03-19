#include "ext2.h"
#include "../../io/console/console.h"
#include "../../io/disk/disk.h"
#include "../../mm/mem.h"
#include "../../mm/kalloc/buddy.h"

KMemCache<Ext2Inode> *Ext2Inode::s_cache = nullptr;

static constexpr uint32_t BLOCK_BUF_SIZE = 4096;

struct BlockBuf {

    BlockBuf() : buf((uint8_t*)Buddy::alloc(BLOCK_BUF_SIZE)) {}
    ~BlockBuf() { Buddy::free(buf, BLOCK_BUF_SIZE); }

    uint8_t* buf;
};

Ext2Mount::Ext2Mount() {
    Disk::read(2, &m_sb);
    Disk::read(3, (uint8_t*)&m_sb + Disk::SECTOR_SIZE);
    if (m_sb.s_magic != EXT2_MAGIC) {
        Console::panic("Ext2Mount: bad magic number");
    }

    m_blockSize = 1024u << m_sb.s_log_block_size;
    if (m_blockSize > BLOCK_BUF_SIZE) {
        Console::panic("Ext2: block size > 4096 not supported");
    }
    m_groupCount = (m_sb.s_blocks_count + m_sb.s_blocks_per_group - 1) / m_sb.s_blocks_per_group;

    Console::kprintf("Ext2: block size=%d groups=%d inodes=%d\n",
                     m_blockSize, m_groupCount, m_sb.s_inodes_count);
}

void Ext2Mount::readBlock(uint32_t block, void* buf) const {
    uint32_t sector = block * sectorsPerBlock();
    auto dst = (uint8_t*)buf;
    for (uint32_t i = 0; i < sectorsPerBlock(); i++) {
        Disk::read(sector + i, dst + i * Disk::SECTOR_SIZE);
    }
}

void Ext2Mount::writeBlock(uint32_t block, const void* buf) const {
    uint32_t sector = block * sectorsPerBlock();
    auto src = (uint8_t*)buf;
    for (uint32_t i = 0; i < sectorsPerBlock(); i++) {
        Disk::write(sector + i, src + i * Disk::SECTOR_SIZE);
    }
}

Ext2BlockGroupDesc Ext2Mount::readGroupDesc(uint32_t group) const {
    uint32_t gdtBlock = m_sb.s_first_data_block + 1;
    uint32_t descPerBlock = m_blockSize / sizeof(Ext2BlockGroupDesc);
    uint32_t block = gdtBlock + group / descPerBlock;
    uint32_t offset = group % descPerBlock;

    Ext2BlockGroupDesc desc;
    BlockBuf buf;
    readBlock(block, buf.buf);
    memcpy(&desc, buf.buf + offset * sizeof(Ext2BlockGroupDesc), sizeof(Ext2BlockGroupDesc));
    return desc;
}

void Ext2Mount::writeGroupDesc(uint32_t group, const Ext2BlockGroupDesc& desc) const {
    uint32_t gdtBlock = m_sb.s_first_data_block + 1;
    uint32_t descPerBlock = m_blockSize / sizeof(Ext2BlockGroupDesc);
    uint32_t block = gdtBlock + group / descPerBlock;
    uint32_t offset = group % descPerBlock;

    BlockBuf buf;
    readBlock(block, buf.buf);
    memcpy(buf.buf + offset * sizeof(Ext2BlockGroupDesc), &desc, sizeof(Ext2BlockGroupDesc));
    writeBlock(block, buf.buf);
}

Ext2InodeDisk Ext2Mount::readRawInode(uint32_t num) const {
    uint32_t group = (num - 1) / m_sb.s_inodes_per_group;
    uint32_t index = (num - 1) % m_sb.s_inodes_per_group;

    Ext2BlockGroupDesc desc = readGroupDesc(group);

    uint32_t inodeSize = m_sb.s_inode_size;
    uint32_t inodesPerBlock = m_blockSize / inodeSize;
    uint32_t block = desc.bg_inode_table + index / inodesPerBlock;
    uint32_t offset = (index % inodesPerBlock) * inodeSize;

    BlockBuf buf;
    readBlock(block, buf.buf);

    Ext2InodeDisk raw;
    memcpy(&raw, buf.buf + offset, sizeof(raw));
    return raw;
}

void Ext2Mount::writeRawInode(uint32_t num, const Ext2InodeDisk& raw) const {
    uint32_t group = (num - 1) / m_sb.s_inodes_per_group;
    uint32_t index = (num - 1) % m_sb.s_inodes_per_group;

    Ext2BlockGroupDesc desc = readGroupDesc(group);

    uint32_t inodeSize = m_sb.s_inode_size;
    uint32_t inodesPerBlock = m_blockSize / inodeSize;
    uint32_t block = desc.bg_inode_table + index / inodesPerBlock;
    uint32_t offset = (index % inodesPerBlock) * inodeSize;

    BlockBuf buf;
    readBlock(block, buf.buf);
    memcpy(buf.buf + offset, &raw, sizeof(raw));
    writeBlock(block, buf.buf);
}

uint32_t Ext2Mount::allocBlock(uint32_t preferGroup) {
    for (uint32_t g = 0; g < m_groupCount; g++) {
        uint32_t group = (preferGroup + g) % m_groupCount;
        Ext2BlockGroupDesc desc = readGroupDesc(group);
        if (desc.bg_free_blocks_count == 0) continue;

        BlockBuf bitmap;
        readBlock(desc.bg_block_bitmap, bitmap.buf);

        uint32_t blockInGroup = m_sb.s_blocks_per_group;
        for (uint32_t i = 0; i < blockInGroup; i++) {
            uint32_t byte = i / 8;
            uint32_t bit = i % 8;
            if (bitmap.buf[byte] & 1 << bit) continue;

            bitmap.buf[byte] |= 1 << bit;
            writeBlock(desc.bg_block_bitmap, bitmap.buf);
            desc.bg_free_blocks_count--;
            writeGroupDesc(group, desc);
            m_sb.s_free_blocks_count--;

            Disk::write(2, &m_sb);
            Disk::write(3, (uint8_t*)&m_sb + Disk::SECTOR_SIZE);

            uint32_t block = m_sb.s_first_data_block + group * m_sb.s_blocks_per_group + i;

            BlockBuf buf;
            memset(buf.buf, 0, BLOCK_BUF_SIZE);
            writeBlock(block, buf.buf);
            return block;
        }
    }
    return 0;
}

void Ext2Mount::freeBlock(uint32_t block) {
    uint32_t group = (block - m_sb.s_first_data_block) / m_sb.s_blocks_per_group;
    uint32_t index = (block - m_sb.s_first_data_block) % m_sb.s_blocks_per_group;

    Ext2BlockGroupDesc desc = readGroupDesc(group);
    BlockBuf bitmap;
    readBlock(desc.bg_block_bitmap, bitmap.buf);

    bitmap.buf[index / 8] &= ~(1 << (index % 8));
    writeBlock(desc.bg_block_bitmap, bitmap.buf);

    desc.bg_free_blocks_count++;
    writeGroupDesc(group, desc);

    m_sb.s_free_blocks_count++;
    Disk::write(2, &m_sb);
    Disk::write(3, (uint8_t*)&m_sb + Disk::SECTOR_SIZE);
}

uint32_t Ext2Mount::allocInode(uint32_t preferGroup, bool isDir) {
    for (uint32_t g = 0; g < m_groupCount; g++) {
        uint32_t group = (preferGroup + g) % m_groupCount;
        Ext2BlockGroupDesc desc = readGroupDesc(group);
        if (desc.bg_free_inodes_count == 0) continue;

        BlockBuf bitmap;
        readBlock(desc.bg_inode_bitmap, bitmap.buf);
        for (uint32_t i = 0; i < m_sb.s_inodes_per_group; i++) {
            if (bitmap.buf[i / 8] & (1 << (i % 8))) continue;

            bitmap.buf[i / 8] |= 1 << (i % 8);
            writeBlock(desc.bg_inode_bitmap, bitmap.buf);
            desc.bg_free_inodes_count--;
            writeGroupDesc(group, desc);
            m_sb.s_free_inodes_count--;
            Disk::write(2, &m_sb);
            Disk::write(3, (uint8_t*)&m_sb + Disk::SECTOR_SIZE);

            uint32_t num = m_sb.s_inodes_per_group * group + i + 1;
            Ext2InodeDisk raw = {};
            writeRawInode(num, raw);
            return num;
        }
    }
    return 0;
}

void Ext2Mount::freeInode(uint32_t num) {
    uint32_t group = (num - 1) / m_sb.s_inodes_per_group;
    uint32_t index = (num - 1) % m_sb.s_inodes_per_group;

    Ext2BlockGroupDesc desc = readGroupDesc(group);
    BlockBuf bitmap;
    readBlock(desc.bg_inode_bitmap, bitmap.buf);

    bitmap.buf[index / 8] &= ~(1 << (index % 8));
    writeBlock(desc.bg_inode_bitmap, bitmap.buf);
    desc.bg_free_inodes_count++;
    writeGroupDesc(group, desc);
    m_sb.s_free_inodes_count++;
    Disk::write(2, &m_sb);
    Disk::write(3, (uint8_t*)&m_sb + Disk::SECTOR_SIZE);
}

int Ext2Mount::addDirEntry(Ext2Inode* dir, uint32_t inodeNum, const char* name, uint8_t fileType) const {
    DirEntry tmp;
    for (uint32_t i = 0; dir->readdir(i, &tmp) == 0; i++) {
        if (strcmp(tmp.name, name) == 0) return -1;
    }
    uint8_t nameLen = strlen(name);

    uint16_t neededLen = 8 + nameLen;
    if (neededLen % 4) neededLen += 4 - (neededLen % 4);

    uint32_t blockSize = m_blockSize;
    BlockBuf buf;

    uint64_t fileSize = dir->size();
    for (uint32_t fileOff = 0; fileOff < fileSize; fileOff += blockSize) {
        uint32_t logical = fileOff / blockSize;
        uint32_t physical = dir->blockMap(logical);
        if (!physical) continue;

        readBlock(physical, buf.buf);

        uint32_t blockOff = 0;
        while (blockOff < blockSize) {
            auto entry = (Ext2DirEntry*)(buf.buf + blockOff);
            if (entry->rec_len == 0) break;

            uint16_t actualLen = 8 + entry->name_len;
            if (actualLen % 4) actualLen += 4 - (actualLen % 4);
            uint16_t freeSpace = entry->rec_len - actualLen;

            if (entry->inode == 0) {
                freeSpace = entry->rec_len;
                actualLen = 0;
            }

            if (freeSpace >= neededLen) {
                if (actualLen > 0) {
                    entry->rec_len = actualLen;
                    blockOff += actualLen;
                }
                auto newEntry = (Ext2DirEntry*)(buf.buf + blockOff);
                newEntry->inode = inodeNum;
                newEntry->name_len = nameLen;
                newEntry->rec_len = freeSpace;
                newEntry->file_type = fileType;
                memcpy(newEntry->name, name, nameLen);
                writeBlock(physical, buf.buf);
                return 0;
            }
            blockOff += entry->rec_len;
        }
    }

    uint32_t newPhysical = dir->blockMapAlloc(fileSize / blockSize);
    if (!newPhysical) return -1;

    memset(buf.buf, 0, blockSize);
    auto newEntry = (Ext2DirEntry*)(buf.buf);
    newEntry->inode = inodeNum;
    newEntry->name_len = nameLen;
    newEntry->rec_len = blockSize;
    newEntry->file_type = fileType;
    memcpy(newEntry->name, name, nameLen);
    writeBlock(newPhysical, buf.buf);

    Ext2InodeDisk raw = readRawInode(dir->inodeNum());
    raw.i_size += blockSize;
    writeRawInode(dir->inodeNum(), raw);
    return 0;
}

int Ext2Mount::removeDirEntry(Ext2Inode* dir, const char* name) {
    uint32_t blockSize = m_blockSize;
    BlockBuf buf;
    uint64_t fileSize = dir->size();

    for (uint64_t fileOff = 0; fileOff < fileSize; fileOff += blockSize) {
        uint32_t logical = fileOff / blockSize;
        uint32_t physical = dir->blockMap(logical);
        if (!physical) return -1;

        readBlock(physical, buf.buf);

        uint32_t blockOff = 0;
        Ext2DirEntry* prev = nullptr;
        while (blockOff < blockSize) {
            auto entry = (Ext2DirEntry*)(buf.buf + blockOff);
            if (entry->rec_len == 0) break;

            if (entry->inode != 0 &&
                entry->name_len == strlen(name) &&
                memcmp(entry->name, name, entry->name_len) == 0) {
                if (prev)
                    prev->rec_len = entry->rec_len;
                else
                    entry->inode = 0;
                writeBlock(physical, buf.buf);
                return 0;
            }
            prev = entry;
            blockOff += entry->rec_len;
        }
    }
    return -1;
}

VfsInode* Ext2Mount::getRoot() {
    return getInode(EXT2_ROOT_INODE);
}

VfsInode* Ext2Mount::getInode(uint32_t num) {
    return new Ext2Inode(this, num, readRawInode(num));
}

void Ext2Mount::putInode(VfsInode* inode) {
    delete inode;
}

VfsInode* Ext2Mount::create(VfsInode* parent, const char* path) {
    auto dir = (Ext2Inode*)parent;
    uint32_t group = (dir->inodeNum() - 1) / m_sb.s_inodes_per_group;

    uint32_t num = allocInode(group, false);
    if (!num) return nullptr;

    Ext2InodeDisk raw = readRawInode(num);
    raw.i_mode = EXT2_S_IFREG | 0644;
    raw.i_links_count = 1;
    raw.i_size = 0;
    writeRawInode(num, raw);

    if (addDirEntry(dir, num, path, EXT2_FT_REG) < 0) {
        freeInode(num);
        return nullptr;
    }

    return new Ext2Inode(this, num, raw);
}

int Ext2Mount::mkdir(VfsInode* parent, const char* path) {
    auto dir = (Ext2Inode*)parent;
    uint32_t group = (dir->inodeNum() - 1) / m_sb.s_inodes_per_group;

    uint32_t num = allocInode(group, true);
    if (!num) return -1;

    uint32_t block = allocBlock(group);
    if (!block) {
        freeInode(num);
        return -1;
    }

    Ext2InodeDisk raw = {};
    raw.i_mode = EXT2_S_IFDIR | 0755;
    raw.i_links_count = 1;
    raw.i_size = 0;
    raw.i_blocks = sectorsPerBlock();
    raw.i_block[0] = block;
    writeRawInode(num, raw);

    BlockBuf buf;
    memset(buf.buf, 0, m_blockSize);

    auto dot = (Ext2DirEntry*)(buf.buf);
    dot->inode = num;
    dot->name_len = 1;
    dot->rec_len = 12;
    dot->file_type = EXT2_FT_DIR;
    dot->name[0] = '.';

    auto dotdot = (Ext2DirEntry*)(buf.buf + 12);
    dotdot->inode     = dir->inodeNum();
    dotdot->name_len  = 2;
    dotdot->file_type = EXT2_FT_DIR;
    dotdot->name[0]   = '.';
    dotdot->name[1]   = '.';
    dotdot->rec_len   = m_blockSize - 12;

    writeBlock(block, buf.buf);

    if (addDirEntry(dir, num, path, EXT2_FT_DIR) < 0) {
        freeInode(num);
        freeBlock(block);
        return -1;
    }

    Ext2InodeDisk parentRaw = readRawInode(dir->inodeNum());
    parentRaw.i_links_count++;
    writeRawInode(dir->inodeNum(), parentRaw);
    return 0;
}

int Ext2Mount::unlink(VfsInode* parent, const char* path) {
    auto dir = (Ext2Inode*)parent;

    DirEntry entry;
    bool found = false;
    for (uint32_t i = 0; dir->readdir(i, &entry) == 0; i++) {
        if (strcmp(entry.name, path) == 0) {
            found = true;
            break;
        }
    }
    if (!found) return -1;

    Ext2InodeDisk raw = readRawInode(entry.inodeNum);
    raw.i_links_count--;
    if (raw.i_links_count == 0) {
        Ext2Inode* inode = (Ext2Inode*)getInode(entry.inodeNum);
        uint32_t totalBlocks = (raw.i_size + m_blockSize - 1) / m_blockSize;
        for (uint32_t i = 0; i < totalBlocks; i++) {
            if (uint32_t block = inode->blockMap(i)) freeBlock(block);
        }
        putInode(inode);
        freeInode(entry.inodeNum);
    }
    else {
        writeRawInode(entry.inodeNum, raw);
    }
    return removeDirEntry(dir, path);
}

uint32_t Ext2Inode::blockMap(uint32_t logical) const {
    uint32_t blockSize = m_mount->blockSize();
    uint32_t ptrsPerBlock = blockSize / sizeof(uint32_t);

    if (logical < 12)
        return m_raw.i_block[logical];

    logical -= 12;

    if (logical < ptrsPerBlock) {
        if (!m_raw.i_block[12]) return 0;
        uint32_t buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[12], buf);
        return buf[logical];
    }

    logical -= ptrsPerBlock;

    if (logical < ptrsPerBlock * ptrsPerBlock) {
        if (!m_raw.i_block[13]) return 0;
        uint32_t buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[13], buf);
        uint32_t l1 = buf[logical / ptrsPerBlock];
        if (!l1) return 0;
        m_mount->readBlock(l1, buf);
        return buf[logical % ptrsPerBlock];
    }
    logical -= ptrsPerBlock * ptrsPerBlock;

    if (m_raw.i_block[14]) {
        uint32_t ptrs2 = ptrsPerBlock * ptrsPerBlock;
        uint32_t buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[14], buf);
        uint32_t l1 = buf[logical / ptrs2];
        if (!l1) return 0;
        m_mount->readBlock(l1, buf);
        uint32_t l2 = buf[(logical % ptrs2) / ptrsPerBlock];
        if (!l2) return 0;
        m_mount->readBlock(l2, buf);
        return buf[logical % ptrsPerBlock];
    }

    return 0;
}

uint32_t Ext2Inode::blockMapAlloc(uint32_t logical) {
    uint32_t blockSize = m_mount->blockSize();
    uint32_t ptrsPerBlock = blockSize / sizeof(uint32_t);
    uint32_t group = (m_num - 1) / m_mount->m_sb.s_inodes_per_group;

    if (logical < 12) {
        if (!m_raw.i_block[logical]) {
            m_raw.i_block[logical] = m_mount->allocBlock(group);
            if (!m_raw.i_block[logical]) return 0;
            m_mount->writeRawInode(m_num, m_raw);
        }
        return m_raw.i_block[logical];
    }
    logical -= 12;

    if (logical < ptrsPerBlock) {
        if (!m_raw.i_block[12]) {
            m_raw.i_block[12] = m_mount->allocBlock(group);
            if (!m_raw.i_block[12]) return 0;
            m_mount->writeRawInode(m_num, m_raw);
        }
        uint32_t buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[12], buf);
        if (!buf[logical]) {
            buf[logical] = m_mount->allocBlock(group);
            if (!buf[logical]) return 0;
            m_mount->writeBlock(m_raw.i_block[12], buf);
        }
        return buf[logical];
    }
    logical -= ptrsPerBlock;

    if (logical < ptrsPerBlock * ptrsPerBlock) {
        if (!m_raw.i_block[13]) {
            m_raw.i_block[13] = m_mount->allocBlock(group);
            if (!m_raw.i_block[13]) return 0;
            m_mount->writeRawInode(m_num, m_raw);
        }

        uint32_t l1buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[13], l1buf);

        uint32_t l1idx = logical / ptrsPerBlock;
        if (!l1buf[l1idx]) {
            l1buf[l1idx] = m_mount->allocBlock(group);
            if (!l1buf[l1idx]) return 0;
            m_mount->writeBlock(m_raw.i_block[13], l1buf);
        }

        uint32_t l2buf[ptrsPerBlock];
        m_mount->readBlock(l1buf[l1idx], l2buf);

        uint32_t l2idx = logical % ptrsPerBlock;
        if (!l2buf[l2idx]) {
            l2buf[l2idx] = m_mount->allocBlock(group);
            if (!l2buf[l2idx]) return 0;
            m_mount->writeBlock(l1buf[l1idx], l2buf);
        }
        return l2buf[l2idx];
    }
    logical -= ptrsPerBlock * ptrsPerBlock;

    uint32_t ptrs2 = ptrsPerBlock * ptrsPerBlock;
    if (logical < ptrsPerBlock * ptrs2) {
        if (!m_raw.i_block[14]) {
            m_raw.i_block[14] = m_mount->allocBlock(group);
            if (!m_raw.i_block[14]) return 0;
            m_mount->writeRawInode(m_num, m_raw);
        }

        uint32_t l1buf[ptrsPerBlock];
        m_mount->readBlock(m_raw.i_block[14], l1buf);

        uint32_t l1idx = logical / ptrs2;
        if (!l1buf[l1idx]) {
            l1buf[l1idx] = m_mount->allocBlock(group);
            if (!l1buf[l1idx]) return 0;
            m_mount->writeBlock(m_raw.i_block[14], l1buf);
        }

        uint32_t l2buf[ptrsPerBlock];
        m_mount->readBlock(l1buf[l1idx], l2buf);

        uint32_t l2idx = (logical % ptrs2) / ptrsPerBlock;
        if (!l2buf[l2idx]) {
            l2buf[l2idx] = m_mount->allocBlock(group);
            if (!l2buf[l2idx]) return 0;
            m_mount->writeBlock(l1buf[l1idx], l2buf);
        }

        uint32_t l3buf[ptrsPerBlock];
        m_mount->readBlock(l2buf[l2idx], l3buf);

        uint32_t l3idx = logical % ptrsPerBlock;
        if (!l3buf[l3idx]) {
            l3buf[l3idx] = m_mount->allocBlock(group);
            if (!l3buf[l3idx]) return 0;
            m_mount->writeBlock(l2buf[l2idx], l3buf);
        }
        return l3buf[l3idx];
    }
    return 0;
}

bool Ext2Inode::isDir() {
    return (m_raw.i_mode & 0xF000) == EXT2_S_IFDIR;
}

uint64_t Ext2Inode::size() {
    return m_raw.i_size;
}

int Ext2Inode::stat(InodeStat* out) {
    if (!out) return -1;
    out->size = m_raw.i_size;
    out->atime = m_raw.i_atime;
    out->mtime = m_raw.i_mtime;
    out->ctime = m_raw.i_ctime;
    out->gid = m_raw.i_gid;
    out->uid = m_raw.i_uid;
    out->mode = m_raw.i_mode;
    out->nlinks = m_raw.i_links_count;
    return 0;
}

int Ext2Inode::read(uint64_t offset, void* buf, uint64_t len) {
    if (!buf || len == 0) return -1;
    if (offset >= m_raw.i_size) return -1;
    if (isDir()) return -1;
    if (offset + len > m_raw.i_size)
        len = m_raw.i_size - offset;

    uint32_t blockSize = m_mount->blockSize();
    auto dst = (uint8_t*)buf;
    uint64_t remaining = len;
    uint64_t pos = offset;

    BlockBuf blockBuf;
    while (remaining > 0) {
        uint32_t logical = pos / blockSize;
        uint32_t blockOffset = pos % blockSize;
        uint32_t canRead = blockSize - blockOffset;
        if (canRead > remaining) canRead = remaining;

        uint32_t physical = blockMap(logical);
        if (!physical) {
            memset(dst, 0, canRead);
        }
        else {
            m_mount->readBlock(physical, blockBuf.buf);
            memcpy(dst, blockBuf.buf + blockOffset, canRead);
        }
        dst += canRead;
        remaining -= canRead;
        pos += canRead;
    }

    return (int)len;
}

int Ext2Inode::readdir(uint32_t index, DirEntry* dir) {
    if (!isDir() || !dir) return -1;

    uint32_t blockSize = m_mount->blockSize();
    BlockBuf blockBuf;
    uint32_t count = 0;
    uint64_t fileOff = 0;

    while (fileOff < m_raw.i_size) {
        uint32_t logical = fileOff / blockSize;
        uint32_t physical = blockMap(logical);
        if (!physical) return -1;

        m_mount->readBlock(physical, blockBuf.buf);

        uint32_t blockOff = 0;
        while (blockOff < blockSize) {
            auto entry = (Ext2DirEntry*)(blockBuf.buf + blockOff);
            if (entry->rec_len == 0) break;

            if (entry->inode != 0) {
                if (count == index) {
                    memcpy(dir->name, entry->name, entry->name_len);
                    dir->name[entry->name_len] = '\0';
                    dir->inodeNum = entry->inode;
                    return 0;
                }
                count++;
            }
            blockOff += entry->rec_len;
        }
        fileOff += blockSize;
    }
    return -1;
}

int Ext2Inode::write(uint64_t offset, const void* buf, uint64_t len) {
    if (!buf || len == 0) return -1;

    uint32_t blockSize = m_mount->blockSize();
    auto src = (uint8_t*)buf;
    uint64_t remaining = len;
    uint64_t pos = offset;
    BlockBuf blockBuf;

    while (remaining > 0) {
        uint32_t logical = pos / blockSize;
        uint32_t blockOff = pos % blockSize;
        uint32_t canWrite = blockSize - blockOff;
        if (canWrite > remaining) canWrite = remaining;

        uint32_t physical = blockMapAlloc(logical);
        if (!physical) return -1;

        if (blockOff != 0 || canWrite != blockSize) {
            m_mount->readBlock(physical, blockBuf.buf);
        }

        memcpy(blockBuf.buf + blockOff, src, canWrite);
        m_mount->writeBlock(physical, blockBuf.buf);
        src += canWrite;
        remaining -= canWrite;
        pos += canWrite;
    }

    if (offset + len > m_raw.i_size) {
        m_raw.i_size = len + offset;
        m_mount->writeRawInode(m_num, m_raw);
    }
    return (int)len;
}
