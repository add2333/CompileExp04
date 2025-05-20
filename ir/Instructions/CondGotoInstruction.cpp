///
/// @file CondGotoInstruction.cpp
/// @brief 条件跳转指令
///
/// @author add (yaomingrui@mail.nwpu.edu.cn)
/// @version 1.0
/// @date 2025-05-17
///
/// @copyright Copyright (c) 2025
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2025-05-17 <td>1.0     <td>add  <td>新建
/// </table>
///

#include "VoidType.h"
#include "CondGotoInstruction.h"

///
/// @brief 条件跳转指令的构造函数
/// @param _func 所属函数
/// @param _condVar 条件变量
/// @param _trueTarget 条件为真时跳转目标
/// @param _falseTarget 条件为假时跳转目标
///
CondGotoInstruction::CondGotoInstruction(Function * _func,
                                         Value * _condVar,
                                         Instruction * _trueTarget,
                                         Instruction * _falseTarget)
    : Instruction(_func, IRInstOperator::IRINST_OP_COND_GOTO, VoidType::getType()), condVar(_condVar)
{
    trueTarget = static_cast<LabelInstruction *>(_trueTarget);
    falseTarget = static_cast<LabelInstruction *>(_falseTarget);
}

/// @brief 转换成IR指令文本
void CondGotoInstruction::toString(std::string & str)
{
    str = "bc " + condVar->getIRName() + ", label " + trueTarget->getIRName() + ", label " + falseTarget->getIRName();
}

///
/// @brief 获取条件变量
/// @return Value* 条件变量
///
Value * CondGotoInstruction::getCondVar() const
{
    return condVar;
}

///
/// @brief 获取真跳转目标
/// @return LabelInstruction* 真跳转目标
///
LabelInstruction * CondGotoInstruction::getTrueTarget() const
{
    return trueTarget;
}

///
/// @brief 获取假跳转目标
/// @return LabelInstruction* 假跳转目标
///
LabelInstruction * CondGotoInstruction::getFalseTarget() const
{
    return falseTarget;
}