#include "asmInterpreter.hpp"
#include "Console.hpp"
#include "UnityDumps.hpp"
#include <vector>
#include <string>
#include <algorithm>

struct pointerDump {
    UnityData *data;
    std::string toPrint;
    std::string register_print;
};

void wrongOperand(ad_insn *insn, const DmntCheatProcessMetadata &cheatMetadata, uint64_t address) {
    Console::Printf("不支持的指令！%s\n", insn->decoded);
    Console::Printf("偏移量：0x%lx\n", address - cheatMetadata.main_nso_extents.base);
    Console::Printf("操作数：\n");
}

void processInstruction(ad_insn *insn, MachineState &machineState, const DmntCheatProcessMetadata &cheatMetadata, uint64_t &start_address, std::vector<uint64_t> &returns, bool &cmp_flag) {
    auto processMemory = [&](uint64_t addr, void* dest, size_t size, bool updateState, int regIdx = 0) {
        if (dmntchtReadCheatProcessMemory(addr, dest, size) && updateState) {
            machineState.X[regIdx] = 0;
            wrongOperand(insn, cheatMetadata, start_address);
        }
    };

    switch (insn->instr_id) {
        case AD_INSTR_ADD:
            if (insn->operands[2].type == AD_OP_REG) {
                machineState.X[insn->operands[0].op_reg.rn] = machineState.X[insn->operands[1].op_reg.rn] + machineState.X[insn->operands[2].op_reg.rn];
            } else if (insn->operands[2].type == AD_OP_IMM) {
                machineState.X[insn->operands[0].op_reg.rn] = machineState.X[insn->operands[1].op_reg.rn] + insn->operands[2].op_imm.bits;
            } else {
                wrongOperand(insn, cheatMetadata, start_address);
            }
            break;
        case AD_INSTR_ADRP:
            machineState.X[insn->operands[0].op_reg.rn] = insn->operands[1].op_imm.bits;
            break;
        case AD_INSTR_B:
            start_address = insn->operands[0].op_imm.bits - 4;
            break;
        case AD_INSTR_BL:
            if (insn->num_operands == 1) {
                returns.push_back(start_address);
                start_address = insn->operands[0].op_imm.bits - 4;
            } else {
                wrongOperand(insn, cheatMetadata, start_address);
            }
            break;
        case AD_INSTR_BR:
            start_address = machineState.X[insn->operands[0].op_reg.rn] - 4;
            break;
        case AD_INSTR_CMP:
            cmp_flag = machineState.X[insn->operands[0].op_reg.rn] == machineState.X[insn->operands[1].op_reg.rn];
            break;
        case AD_INSTR_LDR: {
            uint64_t addr = machineState.X[insn->operands[1].op_reg.rn];
            if (insn->num_operands == 3) addr += insn->operands[2].op_imm.bits;
            processMemory(addr, &machineState.X[insn->operands[0].op_reg.rn], sizeof(uint64_t), true, insn->operands[0].op_reg.rn);
            break;
        }
        case AD_INSTR_LDP:
            // TODO: handle LDP if needed
            break;
        case AD_INSTR_RET:
            start_address = returns.back();
            returns.pop_back();
            break;
        default:
            wrongOperand(insn, cheatMetadata, start_address);
            break;
    }
    start_address += 4;
}

void dumpPointers(const std::vector<std::string>& UnityNames, const std::vector<uint32_t>& UnityOffsets, const DmntCheatProcessMetadata& cheatMetadata, const std::string& unity_sdk) {
    MachineState machineState = {0};
    std::vector<pointerDump> result;

    for (size_t i = 0; i < unityDataStruct.size(); ++i) {
        const auto& unityData = unityDataStruct[i];
        
        auto itr = std::find(UnityNames.begin(), UnityNames.end(), unityData.search_name);
        if (itr == UnityNames.end()) {
            Console::Printf(">未找到%s！>\n", unityData.search_name);
            continue;
        }

        uint64_t start_address = cheatMetadata.main_nso_extents.base + UnityOffsets[itr - UnityNames.begin()];
        uint32_t instruction = 0;
        std::vector<uint64_t> returns;
        std::vector<std::string> forPass;
        ad_insn* insn = nullptr;
        bool cmp_flag = false;

        while (true) {
            dmntchtReadCheatProcessMemory(start_address, (void*)&instruction, sizeof(uint32_t));
            if (returns.empty() && instruction == 0xD65F03C0) break;

            ArmadilloDisassemble(instruction, start_address, &insn);
            forPass.push_back(insn->decoded);
            processInstruction(insn, machineState, cheatMetadata, start_address, returns, cmp_flag);

            ArmadilloDone(&insn);
        }

        std::string toReturn;
        for (const auto& line : forPass) {
            toReturn += line + "\n";
        }

        char smallToPrint[64] = ""; 
        result.push_back({&unityDataStruct[i], toReturn, smallToPrint});
    }

    uint64_t BID = 0;
    char path[128] = "";
    memcpy(&BID, &(cheatMetadata.main_nso_build_id), 8);
    snprintf(path, sizeof(path), "sdmc:/switch/UnityFuncDumper/%016lX/%016lX.txt", cheatMetadata.title_id, __builtin_bswap64(BID));
    FILE* text_file = fopen(path, "w");

    if (!text_file) {
        Console::Printf(">无法创建txt文件!>\n");
    } else {
        fprintf(text_file, "{%s}\n{MAIN: 0x%lx}\n\n", unity_sdk.c_str(), cheatMetadata.main_nso_extents.base);
        for (const auto& dump : result) {
            if (dump.data->get) {
                fprintf(text_file, "{%s}\n", dump.register_print.c_str());
            }
            fprintf(text_file, "[%s]\n%s\n", dump.data->search_name, dump.toPrint.c_str());
        }
        fclose(text_file);
        Console::Printf("将指令转储到 txt 文件: *%s*\n", path);
    }
}
