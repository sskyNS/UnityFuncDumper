#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "dmntcht.h"
#include "service_guard.h"

static Service g_dmntchtSrv;

NX_GENERATE_SERVICE_GUARD(dmntcht);

Result _dmntchtInitialize(void) {
    return smGetService(&g_dmntchtSrv, "dmnt:cht");
}

void _dmntchtCleanup(void) {
    serviceClose(&g_dmntchtSrv);
}

Service *dmntchtGetServiceSession(void) {
    return &g_dmntchtSrv;
}

static Result _dmntchtSimpleCmd(Service *srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _dmntchtCmdInU32(Service *srv, u32 cmd_id, u32 in) {
    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _dmntchtCount(Service *srv, u64 *out_count, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out_count);
}

static Result _dmntchtEntries(Service *srv, void *buffer, u64 buffer_size, u64 offset, u64 *out_count, u32 cmd_id) {
    return serviceDispatchInOut(srv,
                                cmd_id,
                                offset,
                                *out_count,
                                .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                                .buffers = {{buffer, buffer_size}});
}

Result dmntchtHasCheatProcess(bool *out) {
    u8 tmp;
    Result rc = serviceDispatchOut(&g_dmntchtSrv, 65000, tmp);
    if (R_SUCCEEDED(rc) && out)
        *out = tmp & 1;
    return rc;
}

Result dmntchtGetCheatProcessEvent(Event *event) {
    Handle evt_handle;
    Result rc = serviceDispatch(&g_dmntchtSrv, 65001, .out_handle_attrs = {SfOutHandleAttr_HipcCopy}, .out_handles = &evt_handle);
    if (R_SUCCEEDED(rc))
        eventLoadRemote(event, evt_handle, true);
    return rc;
}

Result dmntchtGetCheatProcessMetadata(DmntCheatProcessMetadata *out_metadata) {
    return serviceDispatchOut(&g_dmntchtSrv, 65002, *out_metadata);
}

Result dmntchtForceOpenCheatProcess(void) {
    return _dmntchtSimpleCmd(&g_dmntchtSrv, 65003);
}

Result dmntchtPauseCheatProcess(void) {
    return _dmntchtSimpleCmd(&g_dmntchtSrv, 65004);
}

Result dmntchtResumeCheatProcess(void) {
    return _dmntchtSimpleCmd(&g_dmntchtSrv, 65005);
}

Result dmntchtGetCheatProcessMappingCount(u64 *out_count) {
    return _dmntchtCount(&g_dmntchtSrv, out_count, 65100);
}

Result dmntchtGetCheatProcessMappings(MemoryInfo *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtEntries(&g_dmntchtSrv, buffer, sizeof(*buffer) * max_count, offset, out_count, 65101);
}

Result dmntchtReadCheatProcessMemory(u64 address, void *buffer, size_t size) {
    struct { u64 address; u64 size; } in = {address, size};
    return serviceDispatchIn(&g_dmntchtSrv,
                             65102,
                             in,
                             .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                             .buffers = {{buffer, size}});
}

Result dmntchtWriteCheatProcessMemory(u64 address, const void *buffer, size_t size) {
    struct { u64 address; u64 size; } in = {address, size};
    return serviceDispatchIn(&g_dmntchtSrv,
                             65103,
                             in,
                             .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                             .buffers = {{buffer, size}});
}

Result dmntchtQueryCheatProcessMemory(MemoryInfo *mem_info, u64 address) {
    return serviceDispatchInOut(&g_dmntchtSrv, 65104, address, *mem_info);
}

Result dmntchtGetCheatCount(u64 *out_count) {
    return _dmntchtCount(&g_dmntchtSrv, out_count, 65200);
}

Result dmntchtGetCheats(DmntCheatEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtEntries(&g_dmntchtSrv, buffer, sizeof(*buffer) * max_count, offset, out_count, 65201);
}

Result dmntchtGetCheatById(DmntCheatEntry *out, u32 cheat_id) {
    return serviceDispatchIn(&g_dmntchtSrv,
                             65202,
                             cheat_id,
                             .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize},
                             .buffers = {{out, sizeof(*out)}});
}

Result dmntchtToggleCheat(u32 cheat_id) {
    return _dmntchtCmdInU32(&g_dmntchtSrv, 65203, cheat_id);
}

Result dmntchtAddCheat(DmntCheatDefinition *cheat_def, bool enabled, u32 *out_cheat_id) {
    u8 in = enabled != 0;
    return serviceDispatchInOut(&g_dmntchtSrv,
                                65204,
                                in,
                                *out_cheat_id,
                                .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize},
                                .buffers = {{cheat_def, sizeof(*cheat_def)}});
}

Result dmntchtRemoveCheat(u32 cheat_id) {
    return _dmntchtCmdInU32(&g_dmntchtSrv, 65205, cheat_id);
}

Result dmntchtReadStaticRegister(u64 *out, u8 which) {
    return serviceDispatchInOut(&g_dmntchtSrv, 65206, which, *out);
}

Result dmntchtWriteStaticRegister(u8 which, u64 value) {
    struct { u64 which; u64 value; } in = {which, value};
    return serviceDispatchIn(&g_dmntchtSrv, 65207, in);
}

Result dmntchtResetStaticRegisters(void) {
    return _dmntchtSimpleCmd(&g_dmntchtSrv, 65208);
}

Result dmntchtGetFrozenAddressCount(u64 *out_count) {
    return _dmntchtCount(&g_dmntchtSrv, out_count, 65300);
}

Result dmntchtGetFrozenAddresses(DmntFrozenAddressEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtEntries(&g_dmntchtSrv, buffer, sizeof(*buffer) * max_count, offset, out_count, 65301);
}

Result dmntchtGetFrozenAddress(DmntFrozenAddressEntry *out, u64 address) {
    return serviceDispatchInOut(&g_dmntchtSrv, 65302, address, *out);
}

Result dmntchtEnableFrozenAddress(u64 address, u64 width, u64 *out_value) {
    struct { u64 address; u64 width; } in = {address, width};
    return serviceDispatchInOut(&g_dmntchtSrv, 65303, in, *out_value);
}

Result dmntchtDisableFrozenAddress(u64 address) {
    return serviceDispatchIn(&g_dmntchtSrv, 65304, address);
}
