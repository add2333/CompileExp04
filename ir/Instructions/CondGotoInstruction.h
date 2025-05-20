///
/// @file CondGotoInstruction.h
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
#pragma once

#include <string>

#include "Instruction.h"
#include "LabelInstruction.h"
#include "Function.h"

///
/// @brief 条件跳转指令
///
class CondGotoInstruction final : public Instruction {

public:
    ///
    /// @brief 条件跳转指令的构造函数
    /// @param _func 所属函数
    /// @param _condVar 条件变量
    /// @param _trueTarget 条件为真时跳转目标
    /// @param _falseTarget 条件为假时跳转目标
    ///
    CondGotoInstruction(Function * _func, Value * _condVar, Instruction * _trueTarget, Instruction * _falseTarget);

    /// @brief 转换成字符串
    void toString(std::string & str) override;

    ///
    /// @brief 获取条件变量
    /// @return Value* 条件变量
    ///
    [[nodiscard]] Value * getCondVar() const;

    ///
    /// @brief 获取真跳转目标
    /// @return LabelInstruction*
    ///
    [[nodiscard]] LabelInstruction * getTrueTarget() const;

    ///
    /// @brief 获取假跳转目标
    /// @return LabelInstruction*
    ///
    [[nodiscard]] LabelInstruction * getFalseTarget() const;

private:
    ///
    /// @brief 条件变量
    ///
    Value * condVar;

    ///
    /// @brief 条件为真时跳转目标
    ///
    LabelInstruction * trueTarget;

    ///
    /// @brief 条件为假时跳转目标
    ///
    LabelInstruction * falseTarget;
};