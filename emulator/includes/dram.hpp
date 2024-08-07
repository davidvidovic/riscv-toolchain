#pragma once

#include <iostream>
#include <fstream>
#include <cstdint>

#define DRAM_SIZE	256*256*1
#define DRAM_BASE	0x80000000

class DRAM
{
    private:
        int mem[DRAM_SIZE];

        // Private functions to be called internally after public function was called

        void dram_store_8(uint64_t, uint64_t);
        void dram_store_16(uint64_t, uint64_t);
        void dram_store_32(uint64_t, uint64_t);
        void dram_store_64(uint64_t, uint64_t);

        uint64_t dram_load_8(uint64_t);
        uint64_t dram_load_16(uint64_t);
        uint64_t dram_load_32(uint64_t);
        uint64_t dram_load_64(uint64_t);

    public:
        DRAM();
        uint64_t dram_load(uint64_t, uint64_t);
        void dram_store(uint64_t, uint64_t, uint64_t);
        void stack_dump(int);
};
