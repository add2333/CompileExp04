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
/// </table>
///
#pragma once

#include "Instruction.h"

///
/// @brief 单目运算指令
///
class UnaryInstruction : public Instruction {

public:
    /// @brief 构造函数
    /// @param _op 操作符
    /// @param _result 结果操作数
    /// @param _srcVal1 源操作数1
    UnaryInstruction(Function * _func, IRInstOperator _op, Value * _srcVal1, Type * _type);

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};
