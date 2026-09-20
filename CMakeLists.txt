cmake_minimum_required(VERSION 4.3)
project(heystack)

set(CMAKE_CXX_STANDARD 23)

add_executable(heystack main.cpp
        File/FileFlags.h
        File/FileRecord.cpp
        File/FileRecord.h
        Index/StringViewHash.h
        Index/SystemIndex.cpp
        Index/SystemIndex.h
        Scanner/FileScanner.cpp
        Scanner/FileScanner.h
        Debug/DebugTimer.h
        Serialization/IndexSerializer.cpp
        Serialization/IndexSerializer.h)
