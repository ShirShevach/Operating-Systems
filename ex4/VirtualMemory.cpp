#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <algorithm>

void clear_frame(uint64_t p)
{
    for(uint64_t i=0;i<PAGE_SIZE;i++)
    {
        PMwrite(p*PAGE_SIZE + i,0);
    }
}

void VMinitialize()
{
    clear_frame(0);
}


void search_1(uint64_t original_frame, int cur_depth, uint64_t cur_frame, uint64_t parent, int index, uint64_t* found)
{
    if (cur_depth == TABLES_DEPTH){
        return;
    }
    word_t value;
    bool is_empty = true;
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        PMread(cur_frame * PAGE_SIZE + i, &value);
        if (value != 0)
        {
            is_empty = false;
            search_1(original_frame, cur_depth+1, value, cur_frame, i, found);
        }
    }
    if (is_empty && (cur_frame != original_frame))
    {
        *found = cur_frame;
        PMwrite(parent * PAGE_SIZE + index, 0);
        return;
    }
}

void search_2(uint64_t cur_frame, int cur_depth, int *maximum)
{
    if (cur_depth == TABLES_DEPTH){
        return;
    }
    word_t cur_row;
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        PMread(cur_frame * PAGE_SIZE + i, &cur_row);
        if (cur_row != 0)
        {
            search_2(cur_row, cur_depth + 1, maximum);
        }
        if (*maximum < cur_row){
            *maximum = cur_row;
        }
    }
    return;
}

int find_cyclic(uint64_t virtualAddress, uint64_t address_evict)
{
    uint16_t abs = 0;
    if (address_evict > virtualAddress)
    {
        abs = address_evict - virtualAddress;
    }
    if (virtualAddress > address_evict)
    {
        abs = virtualAddress - address_evict;
    }
    if ((NUM_PAGES - abs) < abs)
    {
        return (NUM_PAGES - abs);
    }
    return abs;
}

void search_3(uint64_t virtualAddress, uint64_t leaf_address,uint64_t* address_evict,uint64_t *value_evict,
              int cur_depth,int* max_evict,uint64_t* parent_evict, int index,uint64_t curr_frame, int * index_evict, uint64_t privious_frame)
{
    if (cur_depth == TABLES_DEPTH){
        int dis = find_cyclic(virtualAddress, leaf_address);
        if (dis > *max_evict)
        {
            *max_evict = dis;
            *address_evict = leaf_address;
            *value_evict = curr_frame;
            *parent_evict = privious_frame;
            *index_evict = index;
        }
        return;
    }
    word_t cur_row;
    for (int i = 0; i < PAGE_SIZE; i++ && (cur_depth < TABLES_DEPTH))
    {
        PMread(curr_frame * PAGE_SIZE + i, &cur_row);
        if (cur_row != 0)
        {
            search_3(virtualAddress, (leaf_address << OFFSET_WIDTH) + i,address_evict,value_evict,
                     cur_depth + 1,max_evict,parent_evict, i,cur_row, index_evict, curr_frame);
        }
    }
    return;
}

uint64_t find_frame(uint64_t virtualAddress, uint64_t original_frame)
{
  uint64_t found = 0;
  int max_evict = 0;
  uint64_t parent_evict = 0;
  uint64_t address_evict = 0;
  uint64_t value_evict = 0;
  int index_evict = 0;

  search_1(original_frame, 0, 0, 0, 0, &found);
  if (found != 0)
  {
      return found;
  }

    int maximum = 0;
    search_2(0, 0, &maximum);
    if (maximum + 1 < NUM_FRAMES)
    {
        return maximum + 1;
    }

    search_3(virtualAddress,0, &address_evict,&value_evict,0,
             &max_evict,&parent_evict, 0,0,&index_evict, 0);
    PMevict(value_evict,address_evict);
    clear_frame(value_evict);
    PMwrite(parent_evict * PAGE_SIZE + index_evict,0);
    return value_evict;
}

uint64_t read_and_write(uint64_t virtualAddress)
{
    uint64_t offset = virtualAddress % PAGE_SIZE;
    uint64_t rest_address = virtualAddress >> (OFFSET_WIDTH);
    uint64_t slice_address = virtualAddress >> (OFFSET_WIDTH);


    word_t addr1 = 0;
    word_t addr2 = 0;
    int SIZE_ADRRESS = CEIL((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH)/TABLES_DEPTH);

    for (int i=0; i < TABLES_DEPTH;i++)
    {
        uint64_t p = slice_address >> ((TABLES_DEPTH - i-1) * SIZE_ADRRESS);
        slice_address = slice_address & ((1 << ((TABLES_DEPTH-i-1)*SIZE_ADRRESS)) - 1);
        PMread(addr2 * PAGE_SIZE + p ,&addr1);

        if (addr1 == 0)
        {
            uint64_t f1 = find_frame(rest_address, addr2); //find frame and evict. addr2 is the original_frame.
            if (i + 1 < TABLES_DEPTH)
            {
                clear_frame(f1);
            }
            else
            {
                PMrestore(f1,rest_address);
            }
            PMwrite(addr2 * PAGE_SIZE + p, f1);
            addr2 = f1;
        }
        else
        {
            addr2 = addr1;
        }

    }
    uint64_t phsical_address = addr2 * PAGE_SIZE + offset;
    return phsical_address;
}

int VMread(uint64_t virtualAddress, word_t* value)
{
    if (virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t phsical_address = read_and_write(virtualAddress);
    PMread(phsical_address,value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (virtualAddress > VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    uint64_t phsical_address = read_and_write(virtualAddress);
    PMwrite(phsical_address,value);
    return 1;
}
