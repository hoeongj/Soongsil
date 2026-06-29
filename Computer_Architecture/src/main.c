#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INSTRUCTIONS 10000
#define MAX_LINE_LEN 100

typedef struct {
    int regs[32];
    int pc;
} CPU;

void initCPU(CPU *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->regs[1] = 1;
    cpu->regs[2] = 2;
    cpu->regs[3] = 3;
    cpu->regs[4] = 4;
    cpu->regs[5] = 5;
    cpu->pc = 1000;
}

int parseBinary(const char *bin, int start, int length) {
    char temp[33];
    strncpy(temp, bin + start, length);
    temp[length] = '\0';
    return (int)strtol(temp, NULL, 2);
}

int lineLabels[MAX_INSTRUCTIONS];

int main() {
    char inputFileName[100];
    char outputFileName[120];
    char instructionBuffer[MAX_INSTRUCTIONS][33];
    int instCount = 0;

    setbuf(stdout, NULL);

    while (1) {
        printf(">> Enter Input File Name: ");
        if (scanf("%s", inputFileName) == EOF) break;

        if (strcmp(inputFileName, "terminate") == 0) {
            break;
        }

        FILE *fp = fopen(inputFileName, "r");
        if (fp == NULL) {
            printf("Input file does not exist!\n");
            continue;
        }

        char *dot = strrchr(inputFileName, '.');
        if (dot) *dot = '\0';
        sprintf(outputFileName, "%s.s", inputFileName);

        instCount = 0;
        int formatError = 0;
        char line[MAX_LINE_LEN];

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) continue;

            if (strlen(line) != 32) { formatError = 1; break; }
            for(int k=0; k<32; k++) {
                if(line[k] != '0' && line[k] != '1') { formatError = 1; break; }
            }
            if(formatError) break;

            strcpy(instructionBuffer[instCount], line);
            instCount++;
        }
        fclose(fp);

        if (formatError) {
            printf(">> Instruction Format Error!\n");
            remove(outputFileName); // [수정] 혹시 있을지 모를 파일 삭제
            continue;
        }

        //레이블 위치 계산 및 유효성 검사
        memset(lineLabels, 0, sizeof(lineLabels));
        int labelCounter = 1;
        int is_unknown = 0;

        for (int i = 0; i < instCount; i++) {
            char *inst = instructionBuffer[i];
            int opcode = parseBinary(inst, 25, 7);

            if (opcode == 51) { // R-Type
                int funct3 = parseBinary(inst, 17, 3);
                int funct7 = parseBinary(inst, 0, 7);
                if (funct3 == 0 && funct7 != 0 && funct7 != 32) is_unknown = 1;
                else if (funct3 == 5 && funct7 != 0 && funct7 != 32) is_unknown = 1;
                else if ((funct3 == 1 || funct3 == 4 || funct3 == 6 || funct3 == 7) && funct7 != 0) is_unknown = 1;
            }
            else if (opcode == 19) { // I-Type
                int funct3 = parseBinary(inst, 17, 3);
                int imm = parseBinary(inst, 0, 12);
                if (funct3 == 1 && ((imm >> 5) & 0x7F) != 0) is_unknown = 1;
                else if (funct3 == 5 && ((imm >> 5) & 0x7F) != 0 && ((imm >> 5) & 0x7F) != 32) is_unknown = 1;
            }
            else if (opcode == 99) { // Branch
                int imm_12 = inst[0] - '0';
                int imm_11 = inst[24] - '0';
                int imm_10_5 = parseBinary(inst, 1, 6);
                int imm_4_1 = parseBinary(inst, 20, 4);
                int imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
                if ((imm >> 12) & 1) imm |= 0xFFFFE000;

                if (imm % 4 != 0) { is_unknown = 1; }
                else {
                    int targetIndex = i + (imm / 4);
                    if (targetIndex < 0 || targetIndex >= instCount) is_unknown = 1;
                    else {
                        if (lineLabels[targetIndex] == 0) lineLabels[targetIndex] = labelCounter++;
                    }
                }
            }

            if (is_unknown) break;
        }

        if (is_unknown) {
            printf(">> Instruction Format Error!\n");
            remove(outputFileName);
            continue;
        }

        //파일 생성
        FILE *outFile = fopen(outputFileName, "w");

        for (int i = 0; i < instCount; i++) {
            char *inst = instructionBuffer[i];
            int opcode = parseBinary(inst, 25, 7);

            if (lineLabels[i] != 0) {
                fprintf(outFile, "label%d:\n", lineLabels[i]);
            }

            if (opcode == 51) { // R-Type
                int rd = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int rs2 = parseBinary(inst, 7, 5);
                int funct7 = parseBinary(inst, 0, 7);
                char *name = NULL;
                switch(funct3) {
                    case 0: if(funct7 == 0) name = "ADD"; else if(funct7 == 32) name = "SUB"; break;
                    case 1: if(funct7 == 0) name = "SLL"; break;
                    case 4: if(funct7 == 0) name = "XOR"; break;
                    case 5: if(funct7 == 0) name = "SRL"; else if(funct7 == 32) name = "SRA"; break;
                    case 6: if(funct7 == 0) name = "OR"; break;
                    case 7: if(funct7 == 0) name = "AND"; break;
                }
                if(name) fprintf(outFile, "%s x%d, x%d, x%d\n", name, rd, rs1, rs2);
                else is_unknown = 1;
            }
            else if (opcode == 19) { // I-Type
                int rd = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int imm = parseBinary(inst, 0, 12);
                if ((imm >> 11) & 1) imm |= 0xFFFFF000;
                char *name = NULL;
                switch(funct3) {
                    case 0: name = "ADDI"; break;
                    case 4: name = "XORI"; break;
                    case 6: name = "ORI"; break;
                    case 7: name = "ANDI"; break;
                    case 1: name = "SLLI"; imm &= 0x1F; break;
                    case 5:
                        if (((imm >> 5) & 0x7F) == 0) name = "SRLI";
                        else if (((imm >> 5) & 0x7F) == 32) name = "SRAI";
                        imm &= 0x1F;
                        break;
                }
                if(name) fprintf(outFile, "%s x%d, x%d, %d\n", name, rd, rs1, imm);
                else is_unknown = 1;
            }
            else if (opcode == 3) { // LW
                int rd = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int imm = parseBinary(inst, 0, 12);
                if ((imm >> 11) & 1) imm |= 0xFFFFF000;
                if (funct3 == 2) fprintf(outFile, "LW x%d, %d(x%d)\n", rd, imm, rs1);
                else is_unknown = 1;
            }
            else if (opcode == 35) { // SW
                int imm_4_0 = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int rs2 = parseBinary(inst, 7, 5);
                int imm_11_5 = parseBinary(inst, 0, 7);
                int imm = (imm_11_5 << 5) | imm_4_0;
                if ((imm >> 11) & 1) imm |= 0xFFFFF000;
                if (funct3 == 2) fprintf(outFile, "SW x%d, %d(x%d)\n", rs2, imm, rs1);
                else is_unknown = 1;
            }
            else if (opcode == 99) { // Branch
                int imm_12 = inst[0] - '0';
                int imm_11 = inst[24] - '0';
                int imm_10_5 = parseBinary(inst, 1, 6);
                int imm_4_1 = parseBinary(inst, 20, 4);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int rs2 = parseBinary(inst, 7, 5);

                int imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
                if ((imm >> 12) & 1) imm |= 0xFFFFE000;

                int targetIndex = i + (imm / 4);
                int targetLabel = 0;
                if(targetIndex >= 0 && targetIndex < instCount) targetLabel = lineLabels[targetIndex];

                char *name = NULL;
                switch(funct3) {
                    case 0: name = "BEQ"; break;
                    case 1: name = "BNE"; break;
                    case 4: name = "BLT"; break;
                    case 5: name = "BGE"; break;
                }
                if(name && targetLabel != 0) fprintf(outFile, "%s x%d, x%d, label%d\n", name, rs1, rs2, targetLabel);
                else is_unknown = 1;
            }
            else { is_unknown = 1; }

            if (is_unknown) break;
        }

        if (is_unknown) {
            printf(">> Instruction Format Error!\n");
            fclose(outFile);
            remove(outputFileName);
            continue;
        }
        fclose(outFile);

        //실행
        CPU cpu;
        initCPU(&cpu);

        while (1) {
            int pc_idx = (cpu.pc - 1000) / 4;
            if (pc_idx < 0 || pc_idx >= instCount) break;

            char *inst = instructionBuffer[pc_idx];
            int opcode = parseBinary(inst, 25, 7);
            int next_pc = cpu.pc + 4;

            if (opcode == 51) { // R-Type
                int rd = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int rs2 = parseBinary(inst, 7, 5);
                int funct7 = parseBinary(inst, 0, 7);
                if (rd != 0) {
                    switch(funct3) {
                        case 0: cpu.regs[rd] = (funct7==0) ? (cpu.regs[rs1] + cpu.regs[rs2]) : (cpu.regs[rs1] - cpu.regs[rs2]); break;
                        case 1: cpu.regs[rd] = cpu.regs[rs1] << cpu.regs[rs2]; break;
                        case 4: cpu.regs[rd] = cpu.regs[rs1] ^ cpu.regs[rs2]; break;
                        case 5: cpu.regs[rd] = (funct7==0) ? ((unsigned)cpu.regs[rs1] >> cpu.regs[rs2]) : (cpu.regs[rs1] >> cpu.regs[rs2]); break;
                        case 6: cpu.regs[rd] = cpu.regs[rs1] | cpu.regs[rs2]; break;
                        case 7: cpu.regs[rd] = cpu.regs[rs1] & cpu.regs[rs2]; break;
                    }
                }
            }
            else if (opcode == 19) { // I-Type
                int rd = parseBinary(inst, 20, 5);
                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int imm = parseBinary(inst, 0, 12);
                if ((imm >> 11) & 1) imm |= 0xFFFFF000;

                if (rd != 0) {
                    switch(funct3) {
                        case 0: cpu.regs[rd] = cpu.regs[rs1] + imm; break;
                        case 4: cpu.regs[rd] = cpu.regs[rs1] ^ imm; break;
                        case 6: cpu.regs[rd] = cpu.regs[rs1] | imm; break;
                        case 7: cpu.regs[rd] = cpu.regs[rs1] & imm; break;
                        case 1: cpu.regs[rd] = cpu.regs[rs1] << (imm & 0x1F); break;
                        case 5:
                            if (((imm>>5)&0x7F) == 0) cpu.regs[rd] = (unsigned)cpu.regs[rs1] >> (imm & 0x1F);
                            else cpu.regs[rd] = cpu.regs[rs1] >> (imm & 0x1F);
                            break;
                    }
                }
            }
            else if (opcode == 3) { // LW
                int rd = parseBinary(inst, 20, 5);
                if (rd != 0) cpu.regs[rd] = 0;
            }
            else if (opcode == 35) { // SW
                // [과제 명세서 구현 조건] Store 명령어로 메모리에 쓰여지는 값은 무시함
            }
            else if (opcode == 99) { // Branch
                int imm_12 = inst[0] - '0';
                int imm_11 = inst[24] - '0';
                int imm_10_5 = parseBinary(inst, 1, 6);
                int imm_4_1 = parseBinary(inst, 20, 4);

                int funct3 = parseBinary(inst, 17, 3);
                int rs1 = parseBinary(inst, 12, 5);
                int rs2 = parseBinary(inst, 7, 5);

                int imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
                if ((imm >> 12) & 1) imm |= 0xFFFFE000;

                int take_branch = 0;
                switch(funct3) {
                    case 0: if (cpu.regs[rs1] == cpu.regs[rs2]) take_branch = 1; break;
                    case 1: if (cpu.regs[rs1] != cpu.regs[rs2]) take_branch = 1; break;
                    case 4: if ((int)cpu.regs[rs1] < (int)cpu.regs[rs2]) take_branch = 1; break;
                    case 5: if ((int)cpu.regs[rs1] >= (int)cpu.regs[rs2]) take_branch = 1; break;
                }
                if (take_branch) next_pc = cpu.pc + imm;
            }
            cpu.pc = next_pc;
        }

        printf(">> Final values in PC, x1, x2, x3, x4, x5 are %d, %d, %d, %d, %d, %d\n",
       cpu.pc - 4, cpu.regs[1], cpu.regs[2], cpu.regs[3], cpu.regs[4], cpu.regs[5]);
    }
    return 0;
}