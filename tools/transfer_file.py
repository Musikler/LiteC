#!/usr/bin/env python3
import sys
import struct
import os

# Must match kernel/litefs.c definitions
LITEFS_BASE = 192
INODE_LBAS = 8
DATA_START = LITEFS_BASE + 1 + INODE_LBAS
INODE_COUNT = 64
MAX_FILE_SIZE = 4096

class LiteFS:
    def __init__(self, path):
        self.path = path
        self.f = open(path, "r+b")
        self.f.seek(LITEFS_BASE * 512)
        self.sb_data = self.f.read(512)
        
        if len(self.sb_data) == 512 and self.sb_data[:8] == b"LITEFS01":
            self.version, self.inode_count, self.data_start = struct.unpack("<III", self.sb_data[8:20])
            if self.inode_count != INODE_COUNT or self.data_start != DATA_START:
                print("Warning: Filesystem layout mismatch, reformatting.")
                self.format()
        else:
            print("Unformatted or corrupt filesystem, formatting...")
            self.format()

    def format(self):
        self.version = 2
        self.inode_count = INODE_COUNT
        self.data_start = DATA_START
        self.sb_data = bytearray(512)
        self.sb_data[:8] = b"LITEFS01"
        struct.pack_into("<III", self.sb_data, 8, self.version, self.inode_count, self.data_start)
        self.f.seek(LITEFS_BASE * 512)
        self.f.write(self.sb_data)
        
        # Zero inode table
        empty = bytearray(512)
        for i in range(INODE_LBAS):
            self.f.seek((LITEFS_BASE + 1 + i) * 512)
            self.f.write(empty)

    def load_files(self):
        self.files = {}
        self.f.seek((LITEFS_BASE + 1) * 512)
        inode_data = self.f.read(INODE_LBAS * 512)
        if len(inode_data) < INODE_LBAS * 512:
            return
        
        for i in range(INODE_COUNT):
            off = i * 64
            inode = inode_data[off:off+64]
            name_bytes = inode[0:32]
            name = name_bytes.split(b'\x00')[0].decode('utf-8', errors='replace')
            if not name:
                continue
                
            size, first_lba, block_count, in_use = struct.unpack("<IIII", inode[32:48])
            
            if in_use and size > 0 and block_count > 0:
                self.f.seek(first_lba * 512)
                data = self.f.read(block_count * 512)[:size]
                self.files[name] = data

    def save_files(self):
        current_lba = DATA_START
        inode_data = bytearray(INODE_LBAS * 512)
        disk_size = os.path.getsize(self.path)
        
        idx = 0
        for name, data in self.files.items():
            if idx >= INODE_COUNT:
                print(f"Error: Too many files (max {INODE_COUNT}). Skipping {name}.")
                continue
                
            if len(data) > MAX_FILE_SIZE:
                print(f"Warning: {name} is too large ({len(data)} bytes), truncating to {MAX_FILE_SIZE}.")
                data = data[:MAX_FILE_SIZE]
                
            blocks = (len(data) + 511) // 512
            padded_data = data + b'\x00' * (blocks * 512 - len(data))
            
            if (current_lba + blocks) * 512 > disk_size:
                print(f"Error: Disk full, cannot write {name}.")
                break
                
            # Write data
            self.f.seek(current_lba * 512)
            self.f.write(padded_data)
            
            # Create inode
            inode = bytearray(64)
            name_bytes = name.encode('utf-8')[:31]
            inode[0:len(name_bytes)] = name_bytes
            struct.pack_into("<IIII", inode, 32, len(data), current_lba, blocks, 1)
            
            inode_data[idx*64:(idx+1)*64] = inode
            
            current_lba += blocks
            idx += 1
            
        # Write inode table
        self.f.seek((LITEFS_BASE + 1) * 512)
        self.f.write(inode_data)
        self.f.flush()

    def close(self):
        self.f.close()

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 transfer_file.py <disk.img> <file1.file> [file2.file ...]")
        sys.exit(1)
        
    disk_path = sys.argv[1]
    if not os.path.exists(disk_path):
        print(f"Disk image {disk_path} not found. Creating a blank 16MB disk.")
        with open(disk_path, "wb") as f:
            f.write(b'\x00' * (16 * 1024 * 1024))
            
    fs = LiteFS(disk_path)
    fs.load_files()
    
    changed = False
    for lcs_path in sys.argv[2:]:
        if not os.path.exists(lcs_path):
            print(f"File not found: {lcs_path}")
            continue
            
        with open(lcs_path, "rb") as f:
            data = f.read()
            
        # The kernel's `lcsm list` command strictly looks for "app_*.lcs"
        base = os.path.basename(lcs_path)
        if base.startswith("app_"):
            target_name = base
        else:
            target_name = "app_" + base
            
        print(f"Installing {lcs_path} as {target_name} ({len(data)} bytes)...")
        fs.files[target_name] = data
        changed = True
        
    if changed:
        fs.save_files()
        print("Saved changes to disk image.")
    else:
        print("No files changed.")
        
    fs.close()

if __name__ == "__main__":
    main()