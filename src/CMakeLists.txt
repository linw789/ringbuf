cmake_minimum_required(VERSION 3.10)

project(RingBuffer)

add_executable(ringbuf_tests
    locked_ringbuf.h
    ringbuf.h
    ringbuf_tests.cpp
)

target_link_libraries(ringbuf_tests PRIVATE gtest)
