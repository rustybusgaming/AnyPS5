#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>

extern "C" {
int APS5_VABI _Mtx_init_nid_postfix(void** mtx, int type);
void APS5_VABI _Mtx_destroy_nid_postfix(void* mtx);
int APS5_VABI _Mtx_lock_nid_postfix(void* mtx);
int APS5_VABI _Mtx_unlock_nid_postfix(void* mtx);
int APS5_VABI _Cnd_init_nid_postfix(void** cnd);
void APS5_VABI _Cnd_destroy_nid_postfix(void* cnd);
int APS5_VABI _Cnd_wait_nid_postfix(void* cnd, void* mtx);
int APS5_VABI _Cnd_broadcast_nid_postfix(void* cnd);
unsigned long long APS5_VABI _WStoul_nid_postfix(const char16_t* str, char16_t** endptr, int base);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename F>
static bool Throws(F f) {
    try { f(); } catch (const std::exception&) { return true; }
    return false;
}

int main() {
    void* plain = nullptr;
    Require(_Mtx_init_nid_postfix(&plain, 0x01 | 0x02) == 0 && plain != nullptr);
    int counter = 0;
    std::vector<std::thread> workers;
    for (int index = 0; index < 8; ++index) workers.emplace_back([&] {
        for (int step = 0; step < 10000; ++step) {
            Require(_Mtx_lock_nid_postfix(plain) == 0);
            ++counter;
            Require(_Mtx_unlock_nid_postfix(plain) == 0);
        }
    });
    for (auto& worker : workers) worker.join();
    Require(counter == 80000);
    Require(_Mtx_lock_nid_postfix(plain) == 0);
    Require(Throws([&] { _Mtx_lock_nid_postfix(plain); }));
    Require(Throws([&] { _Mtx_destroy_nid_postfix(plain); }));
    std::thread([&] { Require(Throws([&] { _Mtx_unlock_nid_postfix(plain); })); }).join();
    Require(_Mtx_unlock_nid_postfix(plain) == 0);
    Require(Throws([&] { _Mtx_unlock_nid_postfix(plain); }));

    void* recursive = nullptr;
    Require(_Mtx_init_nid_postfix(&recursive, 0x01 | 0x100) == 0);
    Require(_Mtx_lock_nid_postfix(recursive) == 0 && _Mtx_lock_nid_postfix(recursive) == 0);
    void* condition = nullptr;
    Require(_Cnd_init_nid_postfix(&condition) == 0 && condition != nullptr);
    Require(Throws([&] { _Cnd_wait_nid_postfix(condition, recursive); }));
    Require(_Mtx_unlock_nid_postfix(recursive) == 0 && _Mtx_unlock_nid_postfix(recursive) == 0);
    _Mtx_destroy_nid_postfix(recursive);

    void* unknown = nullptr;
    Require(Throws([&] { _Mtx_init_nid_postfix(&unknown, 0x08); }) && unknown == nullptr);

    bool ready = false;
    int woken = 0;
    std::vector<std::thread> waiters;
    for (int index = 0; index < 4; ++index) waiters.emplace_back([&] {
        Require(_Mtx_lock_nid_postfix(plain) == 0);
        while (!ready) Require(_Cnd_wait_nid_postfix(condition, plain) == 0);
        ++woken;
        Require(_Mtx_unlock_nid_postfix(plain) == 0);
    });
    Require(_Mtx_lock_nid_postfix(plain) == 0);
    ready = true;
    Require(_Cnd_broadcast_nid_postfix(condition) == 0);
    Require(_Mtx_unlock_nid_postfix(plain) == 0);
    for (auto& waiter : waiters) waiter.join();
    Require(woken == 4);
    _Cnd_destroy_nid_postfix(condition);
    _Mtx_destroy_nid_postfix(plain);

    const char16_t text[] = u"  0x1Fz";
    char16_t* end = nullptr;
    Require(_WStoul_nid_postfix(text, &end, 16) == 0x1F && end == text + 6);
    const char16_t decimal[] = u"4294967296";
    Require(_WStoul_nid_postfix(decimal, nullptr, 10) == 4294967296ULL);
}
