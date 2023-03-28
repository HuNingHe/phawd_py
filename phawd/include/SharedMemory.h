/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file SharedMemory.h
 * @brief shared memory creation and attach
 */

#pragma once
#include <cstring>
#include <iostream>
#include <string>
#if _WIN32
#include <windows.h>
#endif

/*!
 * A container class for an object which is stored in shared memory.  This
 * object can then be viewed in multiple processes or programs.  Note that there
 * is significant overhead when creating a shared memory object, so it is
 * recommended that two programs that communicate should have one single large
 * SharedMemoryObject instead of many small ones.
 *
 * A name string is used to identify shared objects across different programs
 *
 * Creating/deleting the memory can be done with createNew/closeNew.
 * Attention that when on linux platform, createNew and closeNew should be used
 * in pairs, otherwise the mapped file will not be deleted automatically, the
 * program believes that the shared memory has not been freed.
 *
 * Viewing an existing object allocated with createNew can be done with
 * attach/detach
 */

namespace phawd {

template<typename T>
class SharedMemory{
private:
    bool _closed = true;
    bool _attached = false;
    T *_data = nullptr;
#if _WIN32
    HANDLE _fileMapping = nullptr;
#elif __linux__
    int _fd = 0;
#endif
    size_t _size = 0;
    std::string _name{};

public:
    SharedMemory() = default;

    ~SharedMemory();

    void createNew(const std::string &name, size_t size, bool allowOverwrite = true);

    void attach(const std::string &name, size_t size);

    void closeNew();

    void detach();

    T *get();

    T &operator()();
};
}