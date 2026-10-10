#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

struct UsbdTimeval {
    std::int64_t seconds;
    std::int64_t microseconds;
};

}

extern "C" {
std::int32_t APS5_VABI sceUsbdInit();
void APS5_VABI sceUsbdExit();
std::int64_t APS5_VABI sceUsbdGetDeviceList(void*** list);
void APS5_VABI sceUsbdFreeDeviceList(void** list, std::int32_t unrefDevices);
std::int32_t APS5_VABI sceUsbdHandleEventsTimeout(const UsbdTimeval* timeout);
int APS5_VABI sceUsbdOpen();
void* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets);
void APS5_VABI sceUsbdFreeTransfer(void* transfer);
void APS5_VABI sceUsbdFillInterruptTransfer(void* transfer, void* deviceHandle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length,
    void (APS5_VABI *callback)(void*), void* userData, std::uint32_t timeout);
std::int32_t APS5_VABI sceUsbdEventHandlingOk();
}

namespace {

constexpr std::int32_t invalidArgument = static_cast<std::int32_t>(0x80240002);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "USBD: %s\n", message);
        std::abort();
    }
}

}

int main() {
    Require(sceUsbdInit() == 0, "initialization failed");
    void** list = nullptr;
    Require(sceUsbdGetDeviceList(&list) == 0, "a device was listed");
    Require(list != nullptr && list[0] == nullptr, "device list is not an empty null-terminated array");
    sceUsbdFreeDeviceList(list, 1);
    Require(sceUsbdGetDeviceList(nullptr) == invalidArgument, "null list accepted");
    Require(sceUsbdHandleEventsTimeout(nullptr) == invalidArgument, "null timeout accepted");
    const UsbdTimeval invalid{0, 1000000};
    Require(sceUsbdHandleEventsTimeout(&invalid) == invalidArgument, "out-of-range microseconds accepted");
    const UsbdTimeval timeout{0, 50000};
    const auto start = std::chrono::steady_clock::now();
    Require(sceUsbdHandleEventsTimeout(&timeout) == 0, "event handling failed");
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(45), "event handling returned before the timeout");
    bool threw = false;
    try {
        sceUsbdOpen();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "opening a device did not throw");
    Require(sceUsbdEventHandlingOk() == 1, "event handling refused");
    Require(sceUsbdAllocTransfer(-1) == nullptr, "negative isochronous packet count accepted");
    auto* transfer = static_cast<unsigned char*>(sceUsbdAllocTransfer(3));
    Require(transfer != nullptr, "transfer allocation failed");
    std::int32_t isoPackets = 0;
    std::memcpy(&isoPackets, transfer + 56, sizeof(isoPackets));
    Require(isoPackets == 3, "isochronous packet count not recorded");
    for (std::size_t offset = 0; offset < 60 + 3 * 12; ++offset) {
        if (offset < 56 || offset >= 60) Require(transfer[offset] == 0, "transfer is not zeroed");
    }
    std::uint8_t buffer[8]{};
    int userData = 0;
    void* handle = &userData;
    const auto callback = reinterpret_cast<void (APS5_VABI *)(void*)>(&sceUsbdEventHandlingOk);
    sceUsbdFillInterruptTransfer(transfer, handle, 0x81, buffer, sizeof(buffer), callback, &userData, 250);
    void* storedHandle = nullptr;
    std::uint32_t timeout32 = 0;
    std::int32_t length = 0;
    void* storedCallback = nullptr;
    void* storedUserData = nullptr;
    void* storedBuffer = nullptr;
    std::memcpy(&storedHandle, transfer, 8);
    std::memcpy(&timeout32, transfer + 12, 4);
    std::memcpy(&length, transfer + 20, 4);
    std::memcpy(&storedCallback, transfer + 32, 8);
    std::memcpy(&storedUserData, transfer + 40, 8);
    std::memcpy(&storedBuffer, transfer + 48, 8);
    Require(storedHandle == handle && transfer[8] == 0 && transfer[9] == 0x81 && transfer[10] == 3, "interrupt transfer header not filled");
    Require(timeout32 == 250 && length == static_cast<std::int32_t>(sizeof(buffer)), "interrupt transfer sizes not filled");
    Require(storedCallback == reinterpret_cast<void*>(callback) && storedUserData == &userData && storedBuffer == buffer, "interrupt transfer pointers not filled");
    transfer[8] = 0x02;
    threw = false;
    try {
        sceUsbdFreeTransfer(transfer);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "freeing a guest buffer did not throw");
    transfer[8] = 0;
    sceUsbdFreeTransfer(transfer);
    sceUsbdFreeTransfer(nullptr);
    threw = false;
    try {
        sceUsbdFreeTransfer(buffer);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "freeing a foreign transfer did not throw");
    sceUsbdExit();
    std::puts("USBD tests passed");
    return 0;
}
