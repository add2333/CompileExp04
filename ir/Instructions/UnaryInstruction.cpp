///
/// @file UnaryInstruction.cpp
/// @brief 单目操作指令
///
/// @author add (yaomingrui@mail.nwpu.edu.cn)
/// @version 1.0
/// @date 2025-05-07
///
/// @copyright Copyright (c) 2025
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2025-05-07 <td>1.0     <td>add  <td>新建
/// <tr><td>2025-06-18 <td>1.1     <td>add  <td>添加解引用指令
/// </table>
///
#include "UnaryInstruction.h"
#include "Instruction.h"

/// @brief 构造函数
/// @param _op 操作符
/// @param _result 结果操作数
/// @param _srcVal1 源操作数1
UnaryInstruction::UnaryInstruction(Function * _func, IRInstOperator _op, Value * _srcVal1, Type * _type)
    : Instruction(_func, _op, _type)
{
    addOperand(_srcVal1);
}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void UnaryInstruction::toString(std::string & str)
{

    Value * src1 = getOperand(0);

    switch (op) {
        case IRInstOperator::IRINST_OP_NEG_I:

            // 求负指令，单目运算
            str = getIRName() + " = neg " + src1->getIRName();
            break;

        case IRInstOperator::IRINST_OP_DEREF:

            // 解引用指令，单目运算
            str = getIRName() + " = *" + src1->getIRName();
            break;

        default:
            // 未知指令
            Instruction::toString(str);
            break;
    }
}
