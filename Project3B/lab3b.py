#NAME: MICHAEL ZHOU,QIAN CAO
#EMAIL: mzjs96@gmail.com,talexcao@gmail.com
#ID: 804663317,404657587

import sys
import csv
import string

exit_status = 0

class Superblock:
    def __init__(self, row):
        self.total_block = int(row[1])
        self.total_inodes = int(row[2])
        self.block_size = int(row[3])
        self.inode_size = int(row[4])
        self.free_block_num = int(row[5])
        self.free_inode_num = int(row[6])
        self.first_non_reserved_inode = int(row[7])

class Inode:
    def __init__(self, row):
        self.inode_num = int(row[1])
        self.file_type = row[2]
        self.mode = row[3]
        self.owner = int(row[4])
        self.group = int(row[5])
        self.link_count = int(row[6])
        self.ctime = row[7]
        self.mtime = row[8]
        self.atime = row[9]
        self.file_size = int(row[10])
        self.num_blocks = int(row[11])
        if row[2] == 's':
            self.dirent = [int(row[12])]
        else:
            self.dirent = [int(row[i]) for i in range(12,24)]
            self.indirect = [int(row[i]) for i in range(24,27)]


class Dirent:
    def __init__(self, row):
        self.p_inode_num = int(row[1])
        self.byte_offset = int(row[2])
        self.inode = int(row[3])
        self.rec_len = int(row[4])
        self.name_len = int(row[5])
        self.name = row[6]

class Indirect:
    def __init__(self, row):
        self.p_inode_num = int(row[1])
        self.level = int(row[2])
        self.block_offset = int(row[3])
        self.block_num = int(row[4])
        self.ref_num = int(row[5])

def block_consistency_audits(super_block, group, bfree, inodes, indirects):
    # first iterate through all the blocks to see if they are invalid
    block_dict = {}
    for inode in inodes:
        if inode.file_type != 's':
            for i in range(12):
                offset = 0
                block_num = inode.dirent[i]
                if block_num != 0:
                    if block_num > super_block.total_block or block_num < 0:
                        print("INVALID BLOCK {} IN INODE {} AT OFFSET {}".format(block_num, inode.inode_num, offset))
                        exit_status = 2
                    elif  block_num < 8:
                        print("RESERVED BLOCK {} IN INODE {} AT OFFSET {}".format(block_num, inode.inode_num, offset))
                        exit_status = 2
                    elif  block_num in block_dict:
                        block_dict[block_num].append(("BLOCK", inode.inode_num, offset))
                    else:
                        block_dict[block_num] = [("BLOCK", inode.inode_num, offset)]
                offset = offset +1

        block_type = None
        for i in range(3):
            if inode.file_type != 's':
                block_num = inode.indirect[i]
                if i == 0:
                    block_type = "INDIRECT BLOCK"
                    offset = 12
                elif i == 1:
                    block_type = "DOUBLE INDIRECT BLOCK"
                    offset = 268
                elif i == 2:
                    block_type = "TRIPLE INDIRECT BLOCK"
                    offset = 65804
                block_num = inode.indirect[i]
                if block_num != 0:
                    if block_num > super_block.total_block or block_num < 0:
                        print("INVALID {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode.inode_num, offset))
                        exit_status = 2
                    elif block_num < 8:
                        print("RESERVED {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode.inode_num, offset))
                        exit_status = 2
                    elif block_num in block_dict:
                        block_dict[block_num].append((block_type, inode.inode_num, offset))
                    else:
                        block_dict[block_num] = [(block_type, inode.inode_num, offset)]

    for indirect in indirects:
        #naming of the indirection block:
        if indirect.level == 1:
            block_type = "BLOCK"
        elif indirect.level == 2:
            block_type = "INDIRECT BLOCK"
        elif indirect.level == 3:
            block_type = "DOUBLE INDIRECT BLOCK"
        
        #check whether the block is valid or reserved:
        block_num = indirect.ref_num    #block number of the referenced block
        if block_num != 0:
            if block_num < 0 or block_num > super_block.total_block:
                print("INVALID {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode.inode_num, offset))
                exit_status = 2
            elif block_num < 8:
                print("RESERVED {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode.inode_num, offset))
                exit_status = 2
            elif block_num in block_dict:
                block_dict[block_num].append((block_type, inode.inode_num, offset))
            else:
                block_dict[block_num] = [(block_type, inode.inode_num, offset)]
        
    for block_num in range(8, super_block.total_block):
        if block_num not in bfree and block_num not in block_dict:
            print("UNREFERENCED BLOCK {}".format(block_num))
            exit_status = 2
        elif block_num in bfree and block_num in block_dict:
            print("ALLOCATED BLOCK {} ON FREELIST".format(block_num))
            exit_status = 2
        elif block_num in block_dict and len(block_dict[block_num]) > 1:
            for block_type, inode_num, offset in block_dict[block_num]:
                print("DUPLICATE {} {} IN INODE {} AT OFFSET {}".format(block_type, block_num, inode_num, offset))
                exit_status = 2
    return

def inode_allocation_audits(super_block, ifree, inodes):
    allocated_inodes = {inode.inode_num for inode in inodes}
    for inode in allocated_inodes:
        if inode in ifree:
            print("ALLOCATED INODE {} ON FREELIST".format(inode))
            exit_status = 2
    
    for inode in range(super_block.first_non_reserved_inode, super_block.total_inodes):
        if inode not in ifree and inode not in allocated_inodes:
            print("UNALLOCATED INODE {} NOT ON FREELIST".format(inode))
            exit_status = 2
    return allocated_inodes

def directory_consistency_audits(allocated_inodes, super_block, inodes, dirents):
    link_count = [0] * (super_block.total_inodes + 1)
    parent = [0] * (super_block.total_inodes + 1) 
    parent [2] = 2
    #inode number 2 is the root directory
    #the parent directory of the root is root itself

    for dirent in dirents:
        #check if the dirent is valid
        if dirent.inode > super_block.total_inodes or dirent.inode < 1:
            print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dirent.p_inode_num, dirent.name, dirent.inode))
            exit_status = 2
        #check whether the dirent is allocated
        elif dirent.inode not in allocated_inodes:
            print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dirent.p_inode_num, dirent.name, dirent.inode))
            exit_status = 2
        else:
            link_count[dirent.inode] += 1 
            if dirent.name != "'.'" and dirent.name != "'..'":
                parent[dirent.inode] = dirent.p_inode_num

    for inode in inodes:
        if inode.link_count != link_count[inode.inode_num]:
            print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(inode.inode_num, link_count[inode.inode_num], inode.link_count))
    
    for dirent in dirents:
        if dirent.name == "'.'" and dirent.p_inode_num != dirent.inode:
            print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(dirent.p_inode_num, dirent.inode, dirent.p_inode_num))
            exit_status = 2
        #if dirent.name == "'..'" and dirent.p_inode_num != parent[dirent.inode]:
        if dirent.name == "'..'" and parent[dirent.p_inode_num] != dirent.inode:
            print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(dirent.p_inode_num, dirent.inode, dirent.p_inode_num))
        #I have some serious doubts about this... I dont think this is correct...
		#What this is doing is comparing the root directory hard coded number '2' with the parent of the inode numbuer pointed by this directory
		#It should be: if the directory name is '..', and its inode number does match the parent of the inode it is being referred to, return error.
            exit_status = 2


def main():
    if len(sys.argv) != 2:
        print("Error: Invalid arguments. Correct usage: ./lab3b CSV_FILENAME", file=sys.stderr)
        sys.exit(1)

    super_block = None
    group = None
    bfree = []
    ifree = []
    inodes = []
    dirents = []
    indirects = []
    allocated_inodes = {}

    try:
        with open(sys.argv[1], 'r') as csvfile:
            reader = csv.reader(csvfile)
            for row in reader:
                if row[0] == 'SUPERBLOCK':
                    super_block = Superblock(row)
                elif row[0] == 'BFREE':
                    bfree.append(int(row[1]))
                elif row[0] == 'IFREE':
                    ifree.append(int(row[1]))
                elif row[0] == 'INODE':
                    inodes.append(Inode(row))
                elif row[0] == 'DIRENT':
                    dirents.append(Dirent(row))
                elif row[0] == 'INDIRECT':
                    indirects.append(Indirect(row))
    except (IOError, OSError):
        print("reading file failed!", file = sys.stderr)
        sys.exit(1)

    block_consistency_audits(super_block, group, bfree, inodes, indirects)
    allocated_inodes = inode_allocation_audits(super_block, ifree, inodes,)
    directory_consistency_audits(allocated_inodes, super_block, inodes, dirents)

    sys.exit(exit_status)

if __name__ == "__main__":
    main()
