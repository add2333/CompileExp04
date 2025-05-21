///
/// @file IRGenerator.h
/// @brief AST遍历产生线性IR的头文件
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#pragma once

#include <unordered_map>

#include "AST.h"
#include "Module.h"
#include "LabelInstruction.h"

/// @brief AST遍历产生线性IR类
class IRGenerator {

public:
    /// @brief 构造函数
    /// @param root
    /// @param _module
    IRGenerator(ast_node * root, Module * _module);

    /// @brief 析构函数
    ~IRGenerator() = default;

    /// @brief 运行产生IR
    bool run();

protected:
    /// @brief 编译单元AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_compile_unit(ast_node * node);

    /// @brief 函数定义AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_define(ast_node * node);

    /// @brief 形式参数AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_formal_params(ast_node * node);

    /// @brief 函数调用AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_call(ast_node * node);

    /// @brief 语句块（含函数体）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_block(ast_node * node);

    /// @brief 整数加法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_add(ast_node * node);

    /// @brief 整数减法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_sub(ast_node * node);

    /// @brief 整数单目运算符（求负）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_neg(ast_node * node);

    /// @brief 布尔类型单目运算符（求负）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_neg(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

	/// @brief 整数乘法AST节点翻译成线性中间IR
	/// @param node AST节点
	/// @return 翻译是否成功，true：成功，false：失败
	bool ir_mul(ast_node * node);

    /// @brief 整数除法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_div(ast_node * node);

    /// @brief 整数取模AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_mod(ast_node * node);

    /// @brief 赋值AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_assign(ast_node * node);

    /// @brief return节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_return(ast_node * node);

    /// @brief 类型叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_type(ast_node * node);

    /// @brief 标识符叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_var_id(ast_node * node);

    /// @brief 无符号整数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_uint(ast_node * node);

    /// @brief float数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_float(ast_node * node);

    /// @brief 变量声明语句节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_declare_statment(ast_node * node);

    /// @brief 变量定声明节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_variable_declare(ast_node * node);

    /// @brief 未知节点类型的节点处理
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_default(ast_node * node);

    /// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
    /// @param node AST节点
    /// @return 成功返回node节点，否则返回nullptr
    ast_node * ir_visit_ast_node(ast_node * node);

    /// @brief 调用需要两个标签的节点处理
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 成功返回node节点，否则返回nullptr
    ast_node *
    ir_visit_ast_node_with_2_labels(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 逻辑与AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_logical_and(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 逻辑或AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_logical_or(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 逻辑非AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_logical_not(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /* Comparison operations */
    /// @brief 等于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 不等于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_not_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 小于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_less_than(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 小于等于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_less_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 大于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_greater_than(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief 大于等于比较AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_greater_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);

    /// @brief if语句AST节点翻译成线性中间IR (包含处理if-else)
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_if_statement(ast_node * node);

    /// @brief while循环AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_while_statement(ast_node * node);

    /// @brief break语句AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_break_statement(ast_node * node);

    /// @brief continue语句AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_continue_statement(ast_node * node);

    /// @brief AST的节点操作函数
    typedef bool (IRGenerator::*ast2ir_handler_t)(ast_node *);

    /// @brief AST节点运算符与动作函数关联的映射表
    std::unordered_map<ast_operator_type, ast2ir_handler_t> ast2ir_handlers;

private:
    /// @brief 抽象语法树的根
    ast_node * root;

    /// @brief 符号表:模块
    Module * module;
};
