/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#ifndef SOUNDIO_RING_BUFFER_H
#define SOUNDIO_RING_BUFFER_H

#include "os.h"
#include "atomics.h"

struct SoundIoRingBuffer
{
    std::shared_ptr<SoundIoOsMirroredMemory> mem;
    struct SoundIoAtomicULong write_offset;
    struct SoundIoAtomicULong read_offset;

    int init(int requested_capacity);

    int capacity() const
    {
        if (mem == nullptr)
        {
            return 0;
        }
        return mem->capacity;
    }

    int fill_count() const
    {
        unsigned long read_offset = SOUNDIO_ATOMIC_LOAD(this->read_offset);
        unsigned long write_offset = SOUNDIO_ATOMIC_LOAD(this->write_offset);
        int count = static_cast<int>(write_offset - read_offset);
        assert(count >= 0);
        assert(count <= capacity());
        return count;
    }

    char* write_ptr()
    {
        unsigned long write_offset = SOUNDIO_ATOMIC_LOAD(this->write_offset);
        return mem->address.get() + (write_offset % capacity());
    }

    char* read_ptr()
    {
        unsigned long read_offset = SOUNDIO_ATOMIC_LOAD(this->read_offset);
        return mem->address.get() + (read_offset % capacity());
    }


    void advance_write_ptr(int count)
    {
        SOUNDIO_ATOMIC_FETCH_ADD(this->write_offset, count);
        assert(fill_count() >= 0);
    }

    void advance_read_ptr(int count)
    {
        SOUNDIO_ATOMIC_FETCH_ADD(read_offset, count);
        assert(fill_count() >= 0);
    }

    int free_count() const
    {
        return capacity() - fill_count();
    }

    void clear()
    {
        unsigned long read_offset = SOUNDIO_ATOMIC_LOAD(this->read_offset);
        SOUNDIO_ATOMIC_STORE(write_offset, read_offset);
    }

private:
    int init_mirrored_memory(int requested_capacity);
};

int soundio_ring_buffer_init(std::shared_ptr<SoundIoRingBuffer> rb, int requested_capacity);

// void soundio_ring_buffer_deinit(std::shared_ptr<SoundIoRingBuffer> rb);

#endif
