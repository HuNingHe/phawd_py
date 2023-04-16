/*!
 * PHAWD - Parameters Handler and Waveform Display
 * Licensed under the GNU GPLv3 license. See LICENSE for more details.
 * @author HuNing-He
 * @date 2022-2-24
 * @version 0.2
 * @email 2689112371@qq.com
 * @copyright (c) 2022 HuNing-He
 * @file SharedMemory.cpp
 */

#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include "SharedMemory.h"
#include "SharedParameter.h"
#if __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace phawd;

#if _WIN32

/*!
 * Allocate memory for the shared memory object and attach to it.
 * If allowOverwrite is true, and there's already an object with this name,
 * the old object is overwritten Note that if this happens, the object may be
 * initialized in a very weird state.
 */
template<class T>
void SharedMemory<T>::createNew(const std::string &name, size_t size, bool allowOverwrite) {
    // Size should be an integer multiple of 4096, this automatically done by system
    if (size <= 0) {
        throw std::runtime_error(
                "[Shared Memory] SharedMemory::createNew: invalid size!");
    }
    _size = size;
    if (name.length() == 0) {
        throw std::runtime_error(
                "[Shared Memory] SharedMemory::createNew: Shared memory name "
                "is NULL string!");
    }
    _name = name;

    if (_fileMapping != nullptr) {
        CloseHandle(_fileMapping);
    }

    if (_data != nullptr) {
        UnmapViewOfFile((void *)_data);
    }

    HANDLE fileHandle = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {  // Check that if the file has already existed
        if (!allowOverwrite) {
            throw std::runtime_error(
                    "[Shared Memory] SharedMemory::createNew on something that "
                    "wasn't new, file has already existed!");
        } else {
            int fh;

            if( _sopen_s( &fh, name.c_str(), _O_RDWR | _O_CREAT, _SH_DENYNO,
                          _S_IREAD | _S_IWRITE ) == 0 ){
                printf( "[Shared Memory] SharedMemory::createNew File size before change: %ld\n", _filelength( fh ) );
                if( _chsize( fh, size ) == 0 ) {
                    printf( "[Shared Memory] SharedMemory::createNew file named %s's size successfully changed\n", name.c_str());
                    printf( "File size changed to %ld\n", _filelength( fh ) );
                } else {
                    printf("[Error] SharedMemory::createNew change file: %s size error\n", name.c_str());
                    throw std::runtime_error("error in changed file size!");
                }
                _close(fh);
            }
        }
    }

    if (fileHandle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(
                "[ERROR] SharedMemory create file failed: "
                "INVALID_HANDLE_VALUE");
    }

    printf("[Shared Memory] Open new %s, size %zu bytes\n", name.c_str(), size);

    _fileMapping = OpenFileMapping(PAGE_READWRITE, false, name.c_str());

    if (_fileMapping != nullptr) {
        CloseHandle(_fileMapping);
        if(!allowOverwrite){
            throw std::runtime_error(
                    "[Shared Memory] SharedMemory::createNew(): fileMapping with "
                    "the same name has already existed, please close it and retry");
        }
    }

    _fileMapping = CreateFileMapping(fileHandle, nullptr, PAGE_READWRITE, 0, size,
                                         name.c_str());

    if (_fileMapping == nullptr) {
        CloseHandle(fileHandle);
        throw std::runtime_error(
                "[ERROR] SharedMemory::createNew() MapViewOfFile failed!");
    }

    void *shmBase = MapViewOfFile(_fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, size);

    if (GetLastError() == ERROR_ACCESS_DENIED) {
        std::cout << GetLastError() << std::endl;
    }
    memset(shmBase, 0, size);
    _data = (T *)shmBase;
    CloseHandle(fileHandle);
    printf("[Shared Memory] SharedMemory create success(%s), map file to memory "
            "of size(%zu)\n",
            name.c_str(), size);

    _closed = false;
}

/*!
 * Attach to an existing shared memory object.
 */
template<class T>
void SharedMemory<T>::attach(const std::string &name, size_t size) {
    if (size <= 0) {
        throw std::runtime_error(
                "[Shared Memory] SharedMemory::attach: invalid size!");
    }
    _size = size;
    if (name.length() == 0) {
        throw std::runtime_error(
                "[Shared Memory] SharedMemory::attach: name is NULL string!");
    }
    _name = name;
    // attention that when using OpenFileMapping function, we must use
    // FILE_MAP_ALL_ACCESS other than PAGE_READWRITE,otherwise it doesn't work
    _fileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, name.c_str());
    if (_fileMapping == nullptr) {
        throw std::runtime_error("[Shared Memory] SharedMemory attach failed");
    }
    // attention that MapViewOfFile treat FILE_MAP_ALL_ACCESS as FILE_MAP_WRITE
    void *shmBase =
            MapViewOfFile(_fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
    _data = (T *)shmBase;
    printf(
            "[Shared Memory] SharedMemory attach success(%s), map file to memory "
            "of size(%zu)\n",
            name.c_str(), size);
    _attached = true;
}
/*!
 * Free memory associated with the current open shared memory object.  The
 * object could have been opened with either attach or createNew.  After
 * calling this, no process can use this shared object
 */
template<class T>
void SharedMemory<T>::closeNew() {
    if (_data == nullptr) {
        throw std::runtime_error(
                "[ERROR] SharedMemory::closeNew(): the shared memory doesn't "
                "exist");
    }
    if (!UnmapViewOfFile((void *)_data)) {
        throw std::runtime_error(
                "[ERROR] SharedMemory::closeNew(): UnmapViewOfFile failed");
    }

    if (!CloseHandle(_fileMapping)) {
        throw std::runtime_error(
                "[ERROR] SharedMemory::closeNew() Close fileMapping error");
    }

    _data = nullptr;
    _fileMapping = nullptr;

    HANDLE fileHandle =
            CreateFile(_name.c_str(), DELETE, 0, nullptr, OPEN_EXISTING,
                       FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        printf(
                "[ERROR] SharedMemory::closeNew(%s) failed: delete file error "
                "when close shared memory, file may not exist or be occupied\n",
                _name.c_str());
    }
    CloseHandle(fileHandle);
    fileHandle = INVALID_HANDLE_VALUE;
    printf("[Shared Memory] SharedMemory::closeNew (%s) finished\n",
           _name.c_str());
    _closed = true;
}

/*!
 * Close this view of the currently opened shared memory object. The object
 * can be opened with either attach or createNew.  After calling this, this
 * process can no longer use this shared object, but other processes still
 * can.
 */
template<class T>
void SharedMemory<T>::detach() {
    if (_data == nullptr) {
        throw std::runtime_error(
                "[ERROR] SharedMemory::detach() failed, the shared memory "
                "doesn't exist");
    }

    if (!UnmapViewOfFile((void *)_data)) {
        throw std::runtime_error(
                "[ERROR] SharedMemory::detach(): UnmapViewOfFile failed");
    }
    printf("[Shared Memory] SharedMemory::detach (%s) success\n",
           _name.c_str());
    _data = nullptr;
    _attached = false;
}

#elif __linux__

template<class T>
void SharedMemory<T>::createNew(const std::string &name, size_t size, bool allowOverwrite){
    if (size <= 0) {
        throw std::runtime_error(
            "[Shared Memory] SharedMemory::createNew: invalid size!");
    }
    _size = size;
    if (name.length() == 0) {
        throw std::runtime_error(
            "[Shared Memory] SharedMemory::createNew: Shared memory name is NULL string!");
    }
    _name = name;
    struct stat s{};  // restore file information

    _fd = shm_open(name.c_str(), O_RDWR | O_CREAT,
                   S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (_fd < 0) {
        throw std::runtime_error("[ERROR] SharedMemory create file failed");
    }

    printf("[Shared Memory] Open new %s, size %ld bytes\n", name.c_str(), _size);

    if (fstat(_fd, &s)) {
        throw std::runtime_error("[ERROR] SharedMemory file state error");
    }

    if (s.st_size) {
        if (!allowOverwrite) {
            throw std::runtime_error(
                "[Shared Memory] SharedMemory::createNew on something that "
                "wasn't new, file has already existed!");
        }
    }

    if (ftruncate(_fd, _size)) {
        throw std::runtime_error("[ERROR] SharedMemory::createNew(): ftruncate() error");
    }

    void *mem = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (mem == MAP_FAILED) {
        throw std::runtime_error("[ERROR] SharedMemory::createNew() mmap failed!");
    }
    memset(mem, 0, _size);
    _data = (T *) mem;
    printf("[Shared Memory] SharedMemory create success(%s), map file to memory "
           "of size (%ld)\n", name.c_str(), _size);
    _closed = false;
}

template<class T>
void SharedMemory<T>::attach(const std::string &name, size_t size){
    if (size <= 0) {
        throw std::runtime_error(
            "[Shared Memory] SharedMemory::attach: invalid size!");
    }
    _size = size;
    if (name.length() == 0) {
        throw std::runtime_error(
            "[Shared Memory] SharedMemory::createNew: Shared memory name "
            "is NULL string!");
    }
    _name = name;
    struct stat s{};

    _fd = shm_open(name.c_str(), O_RDWR,
                   S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (_fd < 0) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::attach(): open file failed");
    }

    printf("[Shared Memory] open existing %s size %ld bytes\n", name.c_str(), _size);

    if (fstat(_fd, &s)) {
        throw std::runtime_error("[ERROR] SharedMemory file state error");
    }

    if (s.st_size != _size) {
        throw std::runtime_error(
            "[Shared Memory] SharedMemory::attach on incorrect size!");
    }

    void *mem = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (mem == MAP_FAILED) {
        throw std::runtime_error("[ERROR] SharedMemory::attach(): mmap failed!");
    }
    _data = (T *) mem;

    printf(
        "[Shared Memory] SharedMemory attach success(%s), attached memory of "
        "size(%ld)\n",
        name.c_str(), _size);
    _attached = true;
}

template<class T>
void SharedMemory<T>::closeNew(){
    if (_data == nullptr) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::closeNew(): the shared memory doesn't "
            "exist");
    }

    if (munmap((void *) _data, _size)) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::closeNew(): munmap failed");
    }

    if (shm_unlink(_name.c_str())) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::closeNew() shm_unlink error");
    }

    // close fd and delete shared file
    if (close(_fd)) {
        printf("[ERROR] SharedMemory::closeNew (%s) close %s\n",
               _name.c_str(), strerror(errno));
    }

    _fd = 0;
    _data = nullptr;
    printf(
        "[Shared Memory] Shared memory %s closed. if you want to use it again, "
        "please create a new one.\n",
        _name.c_str());
    _closed = true;
}

template<class T>
void SharedMemory<T>::detach(){
    if (_data == nullptr) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::detach() failed, the shared memory "
            "doesn't exist");
    }

    if (munmap((void *) _data, _size)) {
        throw std::runtime_error(
            "[ERROR] SharedMemory::detach(): munmap failed");
    }

    _data = nullptr;

    if (close(_fd)) {
        printf("[ERROR] SharedMemory::closeNew (%s) close %s\n",
               _name.c_str(), strerror(errno));
    }
    _fd = 0;
    printf(
        "[Shared Memory] Shared memory %s detached. if you want to use it "
        "again, please attach again.\n",
        _name.c_str());
    _attached = false;
}

#endif

template<class T>
T* SharedMemory<T>::get(){
    if (_data == nullptr){
        throw std::runtime_error("create or attach shared memory first!");
    }
    return _data;
}

template<class T>
T& SharedMemory<T>::operator()(){
    if (_data == nullptr){
        throw std::runtime_error("create or attach shared memory first!");
    }
    return *_data;
}

template<class T>
SharedMemory<T>::~SharedMemory() {
    if (!_closed){
        closeNew();
    }
    if (_attached){
        detach();
    }
}

template class phawd::SharedMemory<phawd::SharedParameters>;
