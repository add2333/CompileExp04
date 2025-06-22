///
/// @file GlobalVariable.h
/// @brief 全局变量描述类
///
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-29
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// </table>
///
#pragma once

#include "Common.h"
#include "GlobalValue.h"
#include "IRConstant.h"

///
/// @brief 全局变量，寻址时通过符号名或变量名来寻址
///
class GlobalVariable : public GlobalValue {

public:
    ///
    /// @brief 构建全局变量，默认对齐为4字节
    /// @param _type 类型
    /// @param _name 名字
    ///
    explicit GlobalVariable(Type * _type, std::string _name) : GlobalValue(_type, _name)
    {
        // 设置对齐大小
        setAlignment(4);
    }

    ///
    /// @brief  检查是否是函数
    /// @return true 是函数
    /// @return false 不是函数
    ///
    [[nodiscard]] bool isGlobalVarible() const override
    {
        return true;
    }

    ///
    /// @brief 是否属于BSS段的变量，即未初始化过的变量，或者初值都为0的变量
    /// @return true
    /// @return false
    ///
    [[nodiscard]] bool isInBSSSection() const
    {
        return this->inBSSSection;
    }

    ///
    /// @brief 取得变量所在的作用域层级
    /// @return int32_t 层级
    ///
    int32_t getScopeLevel() override
    {
        return 0;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    int32_t getLoadRegId() override
    {
        return this->loadRegNo;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    void setLoadRegId(int32_t regId) override
    {
        this->loadRegNo = regId;
    }

    /// @brief 设置初始化值
    /// @param val 初始化值
    void setInitValue(Value * val)
    {
        this->initVal = val;
        // 如果有初始化值，则不在BSS段
        if (val) {
            this->inBSSSection = false;
        }
    }

    /// @brief 设置是否初始化
    /// @param initialized 是否初始化
    void setIsInitialized(bool initialized)
    {
        this->isInitialized = initialized;
	}

    ///
    /// @brief Declare指令IR显示
    /// @param str
    ///
    void toDeclareString(std::string & str)
    {
        str = "declare " + getType()->toString() + " " + getIRName();

        // 如果是数组，添加维度信息
        if (isArray) {
            for (int32_t dim: arrayDimensions) {
                str += "[" + std::to_string(dim) + "]";
            }
        }
    }

    /// @brief 初始化指令 IR 显示
    void toInitString(std::string & str)
    {

        if (isInitialized) {
            minic_log(LOG_DEBUG, "输出全局变量初始化信息 = %s", initVal->getIRName().c_str());
            str += " = " + initVal->getIRName();
        }
    }

private:
    ///
    /// @brief 变量加载到寄存器中时对应的寄存器编号
    ///
    int32_t loadRegNo = -1;

    ///
    /// @brief 默认全局变量在BSS段，没有初始化，或者即使初始化过，但都值都为0
    ///
    bool inBSSSection = true;

    /// @brief 初始化值
    Value * initVal;

    /// @brief 是否初始化
    bool isInitialized = false;
};
