#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceG2PDialogInitialize(void);
int APS5_VABI sceG2PDialogTerminate(void);
int APS5_VABI sceG2PDialogOpen(const void* param);
int APS5_VABI sceG2PDialogUpdateStatus(void);
int APS5_VABI sceG2PDialogGetResult(void* result);
int APS5_VABI sceG2PDialogGetStatus(void);
}

namespace {

constexpr int COMMON_DIALOG_STATUS_NONE = 0;
constexpr int COMMON_DIALOG_STATUS_INITIALIZED = 1;
constexpr int COMMON_DIALOG_STATUS_FINISHED = 3;
constexpr int COMMON_DIALOG_RESULT_USER_CANCELED = 1;
constexpr int COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80B80004u);
constexpr int COMMON_DIALOG_ERROR_NOT_FINISHED = static_cast<int>(0x80B80005u);
constexpr int COMMON_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B8000Du);

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    std::uint8_t param[64] = {};
    std::int32_t result[8] = {};
    Require(sceG2PDialogGetStatus() == COMMON_DIALOG_STATUS_NONE);
    Require(sceG2PDialogOpen(param) == COMMON_DIALOG_ERROR_NOT_INITIALIZED);
    Require(sceG2PDialogGetResult(result) == COMMON_DIALOG_ERROR_NOT_INITIALIZED);
    Require(sceG2PDialogTerminate() == COMMON_DIALOG_ERROR_NOT_INITIALIZED);

    Require(sceG2PDialogInitialize() == 0);
    Require(sceG2PDialogInitialize() == COMMON_DIALOG_ERROR_ALREADY_INITIALIZED);
    Require(sceG2PDialogGetStatus() == COMMON_DIALOG_STATUS_INITIALIZED);
    Require(sceG2PDialogUpdateStatus() == COMMON_DIALOG_STATUS_INITIALIZED);
    Require(sceG2PDialogGetResult(result) == COMMON_DIALOG_ERROR_NOT_FINISHED);
    Require(sceG2PDialogOpen(nullptr) == COMMON_DIALOG_ERROR_ARG_NULL);
    Require(sceG2PDialogOpen(param) == 0);
    Require(sceG2PDialogGetStatus() == COMMON_DIALOG_STATUS_FINISHED);
    Require(sceG2PDialogGetResult(nullptr) == COMMON_DIALOG_ERROR_ARG_NULL);
    Require(sceG2PDialogGetResult(result) == 0 && result[0] == COMMON_DIALOG_RESULT_USER_CANCELED && result[1] == 0);
    Require(sceG2PDialogTerminate() == 0);
    Require(sceG2PDialogGetStatus() == COMMON_DIALOG_STATUS_NONE);
    Require(sceG2PDialogInitialize() == 0);
    Require(sceG2PDialogTerminate() == 0);
}
