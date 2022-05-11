#pragma once

#include <stdlib.h>
#include <assert.h>
#include <mutex>

struct LockedRingBuffer {
    char* buf;
    int64_t buf_size;
    int64_t head;
    int64_t tail;
    std::mutex mutex;
};

int lrb_init(LockedRingBuffer* lrb, int64_t buf_size) {
    assert(buf_size > 0);

    lrb->buf_size = buf_size;
    lrb->head = 0;
    lrb->tail = 0;

    lrb->buf = (char*)malloc(buf_size);
    if (lrb->buf == 0) {
        return -1;
    } else {
        return 0;
    }
}

void lrb_destroy(LockedRingBuffer* lrb) {
    lrb->head = 0;
    lrb->tail = 0;

    free(lrb->buf);
    lrb->buf_size = 0;
}

int lrb_write(LockedRingBuffer* lrb, const char* msg, int64_t msg_size) {
    int64_t old_head = 0;
    int64_t split1 = 0;
    int64_t split2 = 0;
    {
        std::lock_guard<std::mutex> guard(lrb->mutex);
        int64_t tail = lrb->tail;

        // diff of head to tail
        int64_t diff = (tail + lrb->buf_size - lrb->head) % lrb->buf_size;
        if (diff < msg_size) {
            return -1;
        }

        int64_t new_head = lrb->head + msg_size;
        split1 = new_head >= lrb->buf_size ?
            (lrb->buf_size - lrb->head) : msg_size;
        split2 = msg_size - split1;

        old_head = lrb->head;
        lrb->head = split2 > 0 ? split2 : new_head;
    }

    memcpy(lrb->buf + old_head, msg, (size_t)split1);
    if (split2 > 0) {
        memcpy(lrb->buf, msg + split1, split2);
    }

    return 0;
}

int lrb_read(LockedRingBuffer* lrb, char* msg_buf, int64_t msg_buf_size) {
    assert(msg_buf_size >= lrb->buf_size);

    int64_t head = 0;
    int64_t buf_size = 0;
    {
        std::lock_guard<std::mutex> guard(lrb->mutex);
        head = lrb->head;
        buf_size = lrb->buf_size;
    }
    int64_t tail = lrb->tail;

    // diff of tail to head
    int64_t diff = (head + buf_size - tail) % buf_size;
    if (diff < (buf_size / 2)) {
        return 0;
    }

    int64_t split1 = tail > head ?  (buf_size - tail) : (head - tail);
    int64_t split2 = diff - split1;

    memcpy(msg_buf, lrb->buf + tail, split1);
    if (split2 > 0) {
        memcpy(msg_buf + split1, lrb->buf, split2);
    }

    lrb->tail = head;

    return 0;
}
