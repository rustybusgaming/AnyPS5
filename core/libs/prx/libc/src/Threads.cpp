#include <atomic>
#include <condition_variable>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>

#include "prx/libc/include/General.hpp"

namespace {

constexpr int ThrdSuccess = 0;
constexpr int ThrdNomem = 1;
constexpr int MtxPlain = 0x01;
constexpr int MtxTry = 0x02;
constexpr int MtxTimed = 0x04;
constexpr int MtxRecursive = 0x100;

struct GuestMtx {
    std::mutex mutex;
    std::atomic<std::thread::id> owner;
    unsigned count = 0;
    bool recursive = false;
};

struct GuestCnd {
    std::condition_variable condition;
};

template <typename T>
T* Require(T* handle, const char* caller) {
    if (handle == nullptr) throw std::invalid_argument(std::string(caller) + ": null handle");
    return handle;
}

void RequireOwned(const GuestMtx& mtx, const char* caller) {
    if (mtx.owner.load() != std::this_thread::get_id()) throw std::logic_error(std::string(caller) + ": mutex is not owned by the calling thread");
}

}

extern "C" {

int APS5_VABI _Mtx_init_nid_postfix(GuestMtx** mtx, int type) {
    Require(mtx, __func__);
    if ((type & ~(MtxPlain | MtxTry | MtxTimed | MtxRecursive)) != 0) throw std::invalid_argument("_Mtx_init: unknown mutex type " + std::to_string(type));
    auto* created = new (std::nothrow) GuestMtx;
    if (created == nullptr) return ThrdNomem;
    created->recursive = (type & MtxRecursive) != 0;
    *mtx = created;
    return ThrdSuccess;
}

void APS5_VABI _Mtx_destroy_nid_postfix(GuestMtx* mtx) {
    if (mtx == nullptr) return;
    if (mtx->owner.load() != std::thread::id{}) throw std::logic_error("_Mtx_destroy: mutex is still locked");
    delete mtx;
}

int APS5_VABI _Mtx_lock_nid_postfix(GuestMtx* mtx) {
    Require(mtx, __func__);
    if (mtx->owner.load() == std::this_thread::get_id()) {
        if (!mtx->recursive) throw std::logic_error("_Mtx_lock: non-recursive mutex locked again by its owner");
        ++mtx->count;
        return ThrdSuccess;
    }
    mtx->mutex.lock();
    mtx->owner.store(std::this_thread::get_id());
    mtx->count = 1;
    return ThrdSuccess;
}

int APS5_VABI _Mtx_unlock_nid_postfix(GuestMtx* mtx) {
    Require(mtx, __func__);
    RequireOwned(*mtx, __func__);
    if (--mtx->count == 0) {
        mtx->owner.store({});
        mtx->mutex.unlock();
    }
    return ThrdSuccess;
}

int APS5_VABI _Cnd_init_nid_postfix(GuestCnd** cnd) {
    Require(cnd, __func__);
    auto* created = new (std::nothrow) GuestCnd;
    if (created == nullptr) return ThrdNomem;
    *cnd = created;
    return ThrdSuccess;
}

void APS5_VABI _Cnd_destroy_nid_postfix(GuestCnd* cnd) {
    delete cnd;
}

int APS5_VABI _Cnd_wait_nid_postfix(GuestCnd* cnd, GuestMtx* mtx) {
    Require(cnd, __func__);
    Require(mtx, __func__);
    RequireOwned(*mtx, __func__);
    if (mtx->count != 1) throw std::logic_error("_Cnd_wait: recursive mutex is locked more than once");
    std::unique_lock lock(mtx->mutex, std::adopt_lock);
    mtx->owner.store({});
    mtx->count = 0;
    cnd->condition.wait(lock);
    mtx->owner.store(std::this_thread::get_id());
    mtx->count = 1;
    lock.release();
    return ThrdSuccess;
}

int APS5_VABI _Cnd_broadcast_nid_postfix(GuestCnd* cnd) {
    Require(cnd, __func__)->condition.notify_all();
    return ThrdSuccess;
}

}
