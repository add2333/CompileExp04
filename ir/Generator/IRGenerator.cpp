///
/// @file IRGenerator.cpp
/// @brief AST遍历产生线性IR的源文件
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
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "ArgInstruction.h"
#include "AttrType.h"
#include "Common.h"
#include "FormalParam.h"
#include "Function.h"
#include "IRCode.h"
#include "IRGenerator.h"
#include "Instruction.h"
#include "IntegerType.h"
#include "LocalVariable.h"
#include "Module.h"
#include "EntryInstruction.h"
#include "LabelInstruction.h"
#include "ExitInstruction.h"
#include "FuncCallInstruction.h"
#include "BinaryInstruction.h"
#include "MoveInstruction.h"
#include "GotoInstruction.h"
#include "CondGotoInstruction.h"
#include "Type.h"
#include "UnaryInstruction.h"
#include "Value.h"

/// @brief 构造函数
/// @param _root AST的根
/// @param _module 符号表
IRGenerator::IRGenerator(ast_node * _root, Module * _module) : root(_root), module(_module)
{
    // TODO: 追加新运算符对应处理函数
    /* 叶子节点 */
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_LITERAL_UINT] = &IRGenerator::ir_leaf_node_uint;
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_VAR_ID] = &IRGenerator::ir_leaf_node_var_id;
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_TYPE] = &IRGenerator::ir_leaf_node_type;

    /* 表达式运算 */
    ast2ir_handlers[ast_operator_type::AST_OP_SUB] = &IRGenerator::ir_sub;
    ast2ir_handlers[ast_operator_type::AST_OP_ADD] = &IRGenerator::ir_add;
    ast2ir_handlers[ast_operator_type::AST_OP_NEG] = &IRGenerator::ir_neg;
    ast2ir_handlers[ast_operator_type::AST_OP_MUL] = &IRGenerator::ir_mul;
    ast2ir_handlers[ast_operator_type::AST_OP_DIV] = &IRGenerator::ir_div;
    ast2ir_handlers[ast_operator_type::AST_OP_MOD] = &IRGenerator::ir_mod;

    /* 控制流语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_IF] = &IRGenerator::ir_if_statement;
    ast2ir_handlers[ast_operator_type::AST_OP_WHILE] = &IRGenerator::ir_while_statement;
    ast2ir_handlers[ast_operator_type::AST_OP_BREAK] = &IRGenerator::ir_break_statement;
    ast2ir_handlers[ast_operator_type::AST_OP_CONTINUE] = &IRGenerator::ir_continue_statement;

    /* 语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_ASSIGN] = &IRGenerator::ir_assign;
    ast2ir_handlers[ast_operator_type::AST_OP_RETURN] = &IRGenerator::ir_return;

    /* 函数调用 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_CALL] = &IRGenerator::ir_function_call;

    /* 函数定义 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_DEF] = &IRGenerator::ir_function_define;
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS] = &IRGenerator::ir_function_formal_params;

    /* 变量定义语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_DECL_STMT] = &IRGenerator::ir_declare_statment;
    ast2ir_handlers[ast_operator_type::AST_OP_VAR_DECL] = &IRGenerator::ir_variable_declare;

    /* 语句块 */
    ast2ir_handlers[ast_operator_type::AST_OP_BLOCK] = &IRGenerator::ir_block;

    /* 编译单元 */
    ast2ir_handlers[ast_operator_type::AST_OP_COMPILE_UNIT] = &IRGenerator::ir_compile_unit;
}

/// @brief 遍历抽象语法树产生线性IR，保存到IRCode中
/// @param root 抽象语法树
/// @param IRCode 线性IR
/// @return true: 成功 false: 失败
bool IRGenerator::run()
{
    ast_node * node;

    // 从根节点进行遍历
    node = ir_visit_ast_node(root);

    return node != nullptr;
}

/// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
/// @param node AST节点
/// @return 成功返回node节点，否则返回nullptr
ast_node * IRGenerator::ir_visit_ast_node(ast_node * node)
{
    // 空节点
    if (nullptr == node) {
        return nullptr;
    }

    bool result;

    std::unordered_map<ast_operator_type, ast2ir_handler_t>::const_iterator pIter;
    pIter = ast2ir_handlers.find(node->node_type);
    if (pIter == ast2ir_handlers.end()) {
        // 没有找到，则说明当前不支持
        result = (this->ir_default)(node);
    } else {
        result = (this->*(pIter->second))(node);
    }

    if (!result) {
        // 语义解析错误，则出错返回
        node = nullptr;
    }

    return node;
}

/// @brief 调用需要两个标签的节点处理
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 成功返回node节点，否则返回nullptr
ast_node * IRGenerator::ir_visit_ast_node_with_2_labels(ast_node * node,
                                                        LabelInstruction * trueLabel,
                                                        LabelInstruction * falseLabel)
{
    bool result = false;

    switch (node->node_type) {
        case ast_operator_type::AST_OP_AND:
            result = ir_logical_and(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_OR:
            result = ir_logical_or(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_NOT:
            result = ir_logical_not(node, trueLabel, falseLabel);
            break;

        // 添加对关系表达式的支持
        case ast_operator_type::AST_OP_EQ:
            result = ir_equal(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_NE:
            result = ir_not_equal(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_LT:
            result = ir_less_than(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_LE:
            result = ir_less_equal(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_GT:
            result = ir_greater_than(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_GE:
            result = ir_greater_equal(node, trueLabel, falseLabel);
            break;
        case ast_operator_type::AST_OP_NEG:
            result = ir_neg(node, trueLabel, falseLabel);
            break;
        default:
            // 如果不是逻辑或关系表达式，调用默认处理函数
            // result = ir_visit_ast_node(node);
            // 对于返回值再判断其是否不等于 0，从而判断是否为 true

            // 构造一个用于比较的，值为零的节点
            ast_node * zeroNode = ast_node::New(digit_int_attr{0, node->line_no});
            ast_node * notNode = ast_node::New(ast_operator_type::AST_OP_NE, node, zeroNode, nullptr);
            result = ir_not_equal(notNode, trueLabel, falseLabel);
            node = notNode;
            break;
    }

    if (!result) {
        // 语义解析错误，则出错返回
        node = nullptr;
    }

    return node;
}

/// @brief 未知节点类型的节点处理
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_default(ast_node * node)
{
    // 未知的节点
    printf("Unkown node(%d)\n", (int) node->node_type);
    return true;
}

/// @brief 编译单元AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_compile_unit(ast_node * node)
{
    module->setCurrentFunction(nullptr);

    for (auto son: node->sons) {

        // 遍历编译单元，要么是函数定义，要么是语句
        ast_node * son_node = ir_visit_ast_node(son);
        if (!son_node) {
            // TODO 自行追加语义错误处理
            return false;
        }
    }

    return true;
}

/// @brief 函数定义AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_define(ast_node * node)
{
    bool result = false;

    // 创建一个函数，用于当前函数处理
    if (module->getCurrentFunction()) {
        // 函数中嵌套定义函数，这是不允许的，错误退出
        // TODO 自行追加语义错误处理
        return false;
    }

    // 函数定义的AST包含四个孩子
    // 第一个孩子：函数返回类型
    // 第二个孩子：函数名字
    // 第三个孩子：形参列表
    // 第四个孩子：函数体即block
    ast_node * type_node = node->sons[0];
    ast_node * name_node = node->sons[1];
    ast_node * param_node = node->sons[2];
    ast_node * block_node = node->sons[3];

    // 创建一个新的函数定义
    Function * newFunc = module->newFunction(name_node->name, type_node->type);
    if (!newFunc) {
        // 新定义的函数已经存在，则失败返回。
        // TODO 自行追加语义错误处理
        return false;
    }

    // 当前函数设置有效，变更为当前的函数
    module->setCurrentFunction(newFunc);

    // 进入函数的作用域
    module->enterScope();

    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    InterCode & irCode = newFunc->getInterCode();

    // 这里也可增加一个函数入口Label指令，便于后续基本块划分

    // 创建并加入Entry入口指令
    irCode.addInst(new EntryInstruction(newFunc));

    // 创建出口指令但不加入出口指令，等函数内的指令处理完毕后加入出口指令
    LabelInstruction * exitLabelInst = new LabelInstruction(newFunc);

    // 函数出口指令保存到函数信息中，因为在语义分析函数体时return语句需要跳转到函数尾部，需要这个label指令
    newFunc->setExitLabel(exitLabelInst);

    // 遍历形参
    result = ir_function_formal_params(param_node);
    if (!result) {
        // 形参解析失败
        // TODO 自行追加语义错误处理
        return false;
    }
    node->blockInsts.addInst(param_node->blockInsts);

    // 新建一个Value，用于保存函数的返回值
    // 对于void类型函数，返回值变量会保持为nullptr
    LocalVariable * retValue = nullptr;
    if (!type_node->type->isVoidType()) {
        // 创建返回值变量
        retValue = static_cast<LocalVariable *>(module->newVarValue(type_node->type));

        // 对于 main 函数需要设置初始值为 0 避免返回值为进程返回值
        if (name_node->name == "main") {
            // main函数的返回值变量需要设置初始值为0
            // 这样在没有return语句时，默认返回0
            Value * zeroValue = nullptr;
            zeroValue = module->newConstInt(0);
            MoveInstruction * initInst = new MoveInstruction(newFunc, retValue, zeroValue);
            irCode.addInst(initInst);
        }
    }
    // 无论是否为void类型，都设置函数返回值变量（void为nullptr）
    newFunc->setReturnValue(retValue);

    // 函数内已经进入作用域，内部不再需要做变量的作用域管理
    block_node->needScope = false;

    // 遍历block
    result = ir_block(block_node);
    if (!result) {
        // block解析失败
        // TODO 自行追加语义错误处理
        return false;
    }

    // IR指令追加到当前的节点中
    node->blockInsts.addInst(block_node->blockInsts);

    // 此时，所有指令都加入到当前函数中，也就是node->blockInsts

    // node节点的指令移动到函数的IR指令列表中
    irCode.addInst(node->blockInsts);

    // 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
    irCode.addInst(exitLabelInst);

    // 函数出口指令
    // 对于void类型函数，retValue为nullptr，ExitInstruction将不带返回值
    irCode.addInst(new ExitInstruction(newFunc, retValue));

    // 恢复成外部函数
    module->setCurrentFunction(nullptr);

    // 退出函数的作用域
    module->leaveScope();

    return true;
}

/// @brief 形式参数列表AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_formal_params(ast_node * node)
{
    // 获取当前正在处理的函数
    Function * currentFunc = module->getCurrentFunction();
    if (!currentFunc) {
        // 错误：不在函数定义内部
        minic_log(LOG_ERROR, "形参翻译时当前没有活动的函数");
        return false;
    }

    // 遍历所有的形参，每个形参是AST_OP_FUNC_FORMAL_PARAM类型节点
    for (auto paramNode: node->sons) {
        if (paramNode->node_type != ast_operator_type::AST_OP_FUNC_FORMAL_PARAM) {
            // 错误：非形参节点
            minic_log(LOG_ERROR, "形参列表中包含非形参节点");
            continue;
        }

        ir_function_formal_param(paramNode);
        node->blockInsts.addInst(paramNode->blockInsts);
    }

    return true;
}

/// @brief 形式参数AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_formal_param(ast_node * node)
{
    // 形参节点的第一个子节点是类型，第二个是变量名
    ast_node * typeNode = node->sons[0];
    ast_node * nameNode = node->sons[1];

    // 检查类型节点是否有效
    if (!typeNode->type) {
        minic_log(LOG_ERROR, "形参类型无效");
        return false;
    }

    // 为形参创建临时变量，插入参数列表
    FormalParam * formalParam = new FormalParam(typeNode->type, nameNode->name);
    module->getCurrentFunction()->getParams().push_back(formalParam);

    // 为形参创建局部变量，用于存储实际值
    LocalVariable * paramVar = static_cast<LocalVariable *>(module->newVarValue(typeNode->type, nameNode->name));
    if (!paramVar) {
        // 错误：无法创建形参变量
        minic_log(LOG_ERROR, "无法创建形参变量 %s", nameNode->name.c_str());
        return false;
    }
    node->val = paramVar;

    // 创建赋值语句，把实参的值复制给变量
    MoveInstruction * moveInst = new MoveInstruction(module->getCurrentFunction(), paramVar, formalParam);
    node->blockInsts.addInst(moveInst);

    return true;
}

/// @brief 函数调用AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_call(ast_node * node)
{
    std::vector<Value *> realParams;

    // 获取当前正在处理的函数
    Function * currentFunc = module->getCurrentFunction();

    // 函数调用的节点包含两个节点：
    // 第一个节点：函数名节点
    // 第二个节点：实参列表节点

    std::string funcName = node->sons[0]->name;
    int64_t lineno = node->sons[0]->line_no;

    ast_node * paramsNode = node->sons[1];

    // 根据函数名查找函数，看是否存在。若不存在则出错
    // 这里约定函数必须先定义后使用
    auto calledFunction = module->findFunction(funcName);
    if (nullptr == calledFunction) {
        minic_log(LOG_ERROR, "函数(%s)未定义或声明", funcName.c_str());
        return false;
    }

    // 当前函数存在函数调用
    currentFunc->setExistFuncCall(true);

    // 处理函数调用参数
    int32_t argsCount = (int32_t) paramsNode->sons.size();

    // 当前函数中调用函数实参个数最大值统计，实际上是统计实参传参需在栈中分配的大小
    // 因为目前的语言支持的int和float都是四字节的，只统计个数即可
    if (argsCount > currentFunc->getMaxFuncCallArgCnt()) {
        currentFunc->setMaxFuncCallArgCnt(argsCount);
    }

    // 遍历参数列表，孩子是表达式
    // 这里自左往右计算表达式
    for (auto son: paramsNode->sons) {
        // 计算参数表达式的值
        ast_node * temp = ir_visit_ast_node(son);
        if (!temp) {
            return false;
        }

        // 生成函数参数计算指令
        node->blockInsts.addInst(temp->blockInsts);

        // 保存参数值用于函数调用
        realParams.push_back(temp->val);
    }

    // 进行参数相关的语义错误检查
    // 检查参数个数
    if (realParams.size() != calledFunction->getParams().size()) {
        // 函数参数的个数不一致，语义错误
        minic_log(LOG_ERROR,
                  "第%lld行的被调用函数(%s)参数个数不匹配，需要%zu个参数，提供了%zu个",
                  (long long) lineno,
                  funcName.c_str(),
                  calledFunction->getParams().size(),
                  realParams.size());
        return false;
    }

    // 检查每个参数的类型是否匹配
    for (size_t i = 0; i < realParams.size(); ++i) {
        FormalParam * formalParam = calledFunction->getParams()[i];
        Value * actualParam = realParams[i];

        // 类型兼容性检查
        if ((formalParam->getType()) != (actualParam->getType())) {
            minic_log(LOG_ERROR,
                      "第%lld行的被调用函数(%s)参数类型不匹配，第%zu个参数期望%s类型，提供了%s类型",
                      (long long) lineno,
                      funcName.c_str(),
                      i + 1,
                      formalParam->getType()->toString().c_str(),
                      actualParam->getType()->toString().c_str());
            return false;
        }
    }

    // 返回调用有返回值，则需要分配临时变量，用于保存函数调用的返回值
    Type * type = calledFunction->getReturnType();

    FuncCallInstruction * funcCallInst = new FuncCallInstruction(currentFunc, calledFunction, realParams, type);

    // 创建函数调用指令
    node->blockInsts.addInst(funcCallInst);

    // 函数调用结果Value保存到node中，可能为空，上层节点可利用这个值
    node->val = funcCallInst;

    return true;
}

/// @brief 语句块（含函数体）AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_block(ast_node * node)
{
    // 进入作用域
    if (node->needScope) {
        module->enterScope();
    }

    std::vector<ast_node *>::iterator pIter;
    for (pIter = node->sons.begin(); pIter != node->sons.end(); ++pIter) {

        // 遍历Block的每个语句，进行显示或者运算
        ast_node * temp = ir_visit_ast_node(*pIter);
        if (!temp) {
            return false;
        }

        node->blockInsts.addInst(temp->blockInsts);
    }

    // 离开作用域
    if (node->needScope) {
        module->leaveScope();
    }

    return true;
}

/// @brief 整数加法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_add(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 加法节点，左结合，先计算左节点，后计算右节点

    // 加法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 加法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    BinaryInstruction * addInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_ADD_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(addInst);

    node->val = addInst;

    return true;
}

/// @brief 整数减法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_sub(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 加法节点，左结合，先计算左节点，后计算右节点

    // 加法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 加法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    BinaryInstruction * subInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_SUB_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(subInst);

    node->val = subInst;

    return true;
}

/// @brief 整数单目运算符（求负）AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_neg(ast_node * node)
{
    ast_node * src1_node = node->sons[0];

    // 操作数
    ast_node * right = ir_visit_ast_node(src1_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    UnaryInstruction * negInst = new UnaryInstruction(module->getCurrentFunction(),
                                                      IRInstOperator::IRINST_OP_NEG_I,
                                                      right->val,
                                                      IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(negInst);

    node->val = negInst;

    return true;
}

/// @brief 布尔类型单目运算符（求负）AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_neg(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    ast_node * src1_node = node->sons[0];

    // 操作数
    ast_node * right = ir_visit_ast_node_with_2_labels(src1_node, trueLabel, falseLabel);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // // 处理布尔类数据
    // UnaryInstruction * negInst = new UnaryInstruction(module->getCurrentFunction(),
    //                                                   IRInstOperator::IRINST_OP_NEG_I,
    //                                                   right->val,
    //                                                   IntegerType::getTypeBool());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);

    // node->val = negInst;

    return true;
}

/// @brief 整数乘法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mul(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 乘法节点，左结合，先计算左节点，后计算右节点

    // 乘法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 乘法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    BinaryInstruction * mulInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_MUL_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(mulInst);

    node->val = mulInst;

    return true;
}

/// @brief 整数除法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_div(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 除法节点，左结合，先计算左节点，后计算右节点

    // 除法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 除法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    BinaryInstruction * divInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_DIV_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(divInst);

    node->val = divInst;

    return true;
}

/// @brief 整数取模AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mod(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 取模节点，左结合，先计算左节点，后计算右节点

    // 取模的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 取模的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    BinaryInstruction * modInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_MOD_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeInt());

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(modInst);

    node->val = modInst;

    return true;
}

/// @brief 赋值AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_assign(ast_node * node)
{
    ast_node * son1_node = node->sons[0];
    ast_node * son2_node = node->sons[1];

    // 赋值节点，自右往左运算

    // 赋值运算符的左侧操作数
    ast_node * left = ir_visit_ast_node(son1_node);
    if (!left) {
        // 某个变量没有定值
        // 这里缺省设置变量不存在则创建，因此这里不会错误
        return false;
    }

    // 赋值运算符的右侧操作数
    ast_node * right = ir_visit_ast_node(son2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    MoveInstruction * movInst = new MoveInstruction(module->getCurrentFunction(), left->val, right->val);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(movInst);

    // 这里假定赋值的类型是一致的
    node->val = movInst;

    return true;
}

/// @brief return节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_return(ast_node * node)
{
    ast_node * right = nullptr;

    // return语句可能没有没有表达式，也可能有，因此这里必须进行区分判断
    if (!node->sons.empty()) {

        ast_node * son_node = node->sons[0];

        // 返回的表达式的指令保存在right节点中
        right = ir_visit_ast_node(son_node);
        if (!right) {

            // 某个变量没有定值
            return false;
        }
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    Function * currentFunc = module->getCurrentFunction();
    LocalVariable * returnValue = currentFunc->getReturnValue();

    // 检查函数是否有返回值，如果有且提供了返回表达式
    if (returnValue && right) {
        // 非void函数有返回表达式 - 正常情况
        // 创建临时变量保存IR的值，以及线性IR指令
        node->blockInsts.addInst(right->blockInsts);

        // 检查返回值类型与函数返回类型是否兼容
        if ((returnValue->getType()) != (right->val->getType())) {
            minic_log(LOG_ERROR,
                      "函数返回值类型不匹配，期望%s类型，提供了%s类型",
                      returnValue->getType()->toString().c_str(),
                      right->val->getType()->toString().c_str());
            // 我们允许类型不完全匹配，但给出警告
        }

        // 返回值赋值到函数返回值变量上
        node->blockInsts.addInst(new MoveInstruction(currentFunc, returnValue, right->val));
        node->val = right->val;
    }
    // 如果函数有返回值但没有提供返回表达式
    else if (returnValue && !right) {
        minic_log(LOG_ERROR, "非void函数没有提供返回表达式");
        // 不需要额外指令，函数返回值已在函数开始处初始化为0
        node->val = nullptr;
    }
    // 如果函数没有返回值(void类型)但提供了返回表达式
    else if (!returnValue && right) {
        minic_log(LOG_ERROR, "void函数提供了返回表达式");
        // 虽然返回值会被忽略，但我们仍需要执行表达式，因为它可能有副作用
        node->blockInsts.addInst(right->blockInsts);
        node->val = nullptr;
    }
    // 函数没有返回值且没有提供返回表达式，符合void返回类型要求
    else {
        // void函数没有返回表达式 - 正常情况
        node->val = nullptr;
    }

    // 跳转到函数的尾部出口指令上
    node->blockInsts.addInst(new GotoInstruction(currentFunc, currentFunc->getExitLabel()));

    return true;
}

/// @brief 类型叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_type(ast_node * node)
{
    // 不需要做什么，直接从节点中获取即可。

    return true;
}

/// @brief 标识符叶子节点翻译成线性中间IR，变量声明的不走这个语句
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_var_id(ast_node * node)
{
    Value * val;

    // 查找ID型Value
    // 变量，则需要在符号表中查找对应的值

    val = module->findVarValue(node->name);

    node->val = val;

    return true;
}

/// @brief 无符号整数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_uint(ast_node * node)
{
    ConstInt * val;

    // 新建一个整数常量Value
    val = module->newConstInt((int32_t) node->integer_val);

    node->val = val;

    return true;
}

/// @brief 变量声明语句节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_declare_statment(ast_node * node)
{
    bool result = false;

    for (auto & child: node->sons) {

        // 遍历每个变量声明
        result = ir_variable_declare(child);
        if (!result) {
            break;
        }
        node->blockInsts.addInst(child->blockInsts);
    }

    return result;
}

/// @brief 变量定声明节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_variable_declare(ast_node * node)
{
    // 第一个孩子表示类型
    ast_node * typeNode = node->sons[0];

    // 第二个孩子表示变量名或赋值节点
    ast_node * varNode = node->sons[1];

    // 检查是否是赋值节点
    if (varNode->node_type == ast_operator_type::AST_OP_ASSIGN) {
        // 赋值节点，处理初值
        ast_node * idNode = varNode->sons[0];       // 左侧变量名节点
        ast_node * initExprNode = varNode->sons[1]; // 右侧表达式

        // 创建变量
        Value * varValue = module->newVarValue(typeNode->type, idNode->name);
        node->val = varValue;

        // 翻译初值表达式
        ast_node * exprResult = ir_visit_ast_node(initExprNode);
        if (!exprResult) {
            return false; // 初值表达式翻译失败
        }

        // 创建赋值指令，将初值赋给变量
        MoveInstruction * assignInst = new MoveInstruction(module->getCurrentFunction(), varValue, exprResult->val);
        node->blockInsts.addInst(exprResult->blockInsts); // 添加初值表达式的指令
        node->blockInsts.addInst(assignInst);             // 添加赋值指令
    } else {
        // 非赋值节点，直接创建变量
        Value * varValue = module->newVarValue(typeNode->type, varNode->name);
        node->val = varValue;
    }

    return true;
}

/// @brief 逻辑与AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_logical_and(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    Function * currentFunc = module->getCurrentFunction();

    // 计算左操作数
    // 条件分支: 如果左操作数为假(0)，直接跳转到假出口
    // 如果左操作数为真(非0)，继续计算右操作数
    LabelInstruction * rightOperandLabel = new LabelInstruction(currentFunc);
    ast_node * left = ir_visit_ast_node_with_2_labels(node->sons[0], rightOperandLabel, falseLabel);
    if (!left) {
        return false;
    }

    // 保存左操作数的指令
    node->blockInsts.addInst(left->blockInsts);

    // 右操作数标签
    node->blockInsts.addInst(rightOperandLabel);

    // 计算右操作数
    ast_node * right = ir_visit_ast_node_with_2_labels(node->sons[1], trueLabel, falseLabel);
    if (!right) {
        return false;
    }

    // 保存右操作数的指令
    node->blockInsts.addInst(right->blockInsts);

    return true;
}

/// @brief 逻辑或AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_logical_or(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    Function * currentFunc = module->getCurrentFunction();

    // 计算左操作数
    // 条件分支: 如果左操作数为真(非0)，直接跳转到真出口
    // 如果左操作数为假(0)，继续计算右操作数
    LabelInstruction * rightOperandLabel = new LabelInstruction(currentFunc);
    ast_node * left = ir_visit_ast_node_with_2_labels(node->sons[0], trueLabel, rightOperandLabel);
    if (!left) {
        return false;
    }

    // 保存左操作数的指令
    node->blockInsts.addInst(left->blockInsts);

    // 右操作数标签
    node->blockInsts.addInst(rightOperandLabel);

    // 计算右操作数
    ast_node * right = ir_visit_ast_node_with_2_labels(node->sons[1], trueLabel, falseLabel);
    if (!right) {
        return false;
    }

    // 保存右操作数的指令
    node->blockInsts.addInst(right->blockInsts);

    return true;
}

/// @brief 逻辑非AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_logical_not(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // Function * currentFunc = module->getCurrentFunction();

    // 如果子节点为常量或变量，则将变量转换为布尔值，并直接保存指令返回
    if (node->sons[0]->isLeafNode()) {
        // 计算左操作数
        ast_node * left = ir_visit_ast_node(node->sons[0]);
        if (!left) {
            return false;
        }
        // 保存左操作数的指令
        node->blockInsts.addInst(left->blockInsts);
        // 创建比较指令
        Value * boolZero = module->newConstInt(0);
        BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                            IRInstOperator::IRINST_OP_CMP_EQ_I,
                                                            left->val,
                                                            boolZero,
                                                            IntegerType::getTypeBool());
        // 保存比较指令
        node->blockInsts.addInst(cmpInst);

        // 创建条件跳转指令
        CondGotoInstruction * condGotoInst =
            new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);
        // 保存条件跳转指令
        node->blockInsts.addInst(condGotoInst);
        return true;
    }

    // 计算操作数
    ast_node * operand = ir_visit_ast_node_with_2_labels(node->sons[0], falseLabel, trueLabel);
    if (!operand) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(operand->blockInsts);

    return true;
}

/// @brief 等于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_EQ_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief 不等于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_not_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_NE_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief 小于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_less_than(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_LT_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief 小于等于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_less_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_LE_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief 大于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_greater_than(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_GT_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief 大于等于比较AST节点翻译成线性中间IR
/// @param node AST节点
/// @param trueLabel 真出口标签
/// @param falseLabel 假出口标签
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_greater_equal(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel)
{
    // 计算左操作数
    ast_node * left = ir_visit_ast_node(node->sons[0]);
    if (!left) {
        return false;
    }

    // 计算右操作数
    ast_node * right = ir_visit_ast_node(node->sons[1]);
    if (!right) {
        return false;
    }

    // 保存操作数的指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);

    // 创建比较指令
    BinaryInstruction * cmpInst = new BinaryInstruction(module->getCurrentFunction(),
                                                        IRInstOperator::IRINST_OP_CMP_GE_I,
                                                        left->val,
                                                        right->val,
                                                        IntegerType::getTypeBool());

    // 保存比较指令
    node->blockInsts.addInst(cmpInst);

    // 创建条件跳转指令
    CondGotoInstruction * condGotoInst =
        new CondGotoInstruction(module->getCurrentFunction(), cmpInst, trueLabel, falseLabel);

    // 保存条件跳转指令
    node->blockInsts.addInst(condGotoInst);

    return true;
}

/// @brief if语句AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_if_statement(ast_node * node)
{
    Function * currentFunc = module->getCurrentFunction();

    // if语句包含两个或三个子节点：
    // 第一个: 条件表达式
    // 第二个: then语句块
    // 可选的第三个: else语句块
    ast_node * condNode = node->sons[0];
    ast_node * thenNode = node->sons[1];

    // 判断是否有else部分
    bool hasElse = (node->sons.size() > 2);
    ast_node * elseNode = hasElse ? node->sons[2] : nullptr;

    // 创建需要的标签
    LabelInstruction * thenLabel = new LabelInstruction(currentFunc);
    LabelInstruction * elseLabel = hasElse ? new LabelInstruction(currentFunc) : nullptr;
    LabelInstruction * endIfLabel = new LabelInstruction(currentFunc);

    // 计算条件表达式
    ast_node * condition = ir_visit_ast_node_with_2_labels(condNode, thenLabel, hasElse ? elseLabel : endIfLabel);
    if (!condition) {
        return false;
    }

    // 保存条件表达式的指令
    node->blockInsts.addInst(condition->blockInsts);

    // then语句块
    node->blockInsts.addInst(thenLabel);
    ast_node * thenResult = ir_visit_ast_node(thenNode);
    if (!thenResult) {
        return false;
    }
    node->blockInsts.addInst(thenResult->blockInsts);

    if (hasElse) {
        // 无条件跳转到if语句结束
        node->blockInsts.addInst(new GotoInstruction(currentFunc, endIfLabel));

        // else语句块
        node->blockInsts.addInst(elseLabel);
        ast_node * elseResult = ir_visit_ast_node(elseNode);
        if (!elseResult) {
            return false;
        }
        node->blockInsts.addInst(elseResult->blockInsts);
    }

    // if语句结束标签
    node->blockInsts.addInst(endIfLabel);

    return true;
}

bool IRGenerator::ir_while_statement(ast_node * node)
{
    Function * currentFunc = module->getCurrentFunction();

    // while语句包含两个子节点：条件表达式和循环体
    ast_node * condNode = node->sons[0];
    ast_node * bodyNode = node->sons[1];

    // 创建三个标签：循环入口(L1)、循环体入口(L2)、循环出口(L3)
    LabelInstruction * loopEntryLabel = new LabelInstruction(currentFunc); // L1
    LabelInstruction * loopBodyLabel = new LabelInstruction(currentFunc);  // L2
    LabelInstruction * loopExitLabel = new LabelInstruction(currentFunc);  // L3

    // 将标签压入栈
    currentFunc->pushBreakLabel(loopExitLabel);
    currentFunc->pushContinueLabel(loopEntryLabel);

    // 插入循环入口标签 (L1)
    node->blockInsts.addInst(loopEntryLabel);

    // 遍历条件表达式，生成线性IR
    ast_node * condition = ir_visit_ast_node_with_2_labels(condNode, loopBodyLabel, loopExitLabel);
    if (!condition) {
        return false;
    }

    // 保存条件表达式的指令
    node->blockInsts.addInst(condition->blockInsts);

    // 插入循环体入口标签 (L2)
    node->blockInsts.addInst(loopBodyLabel);

    // 遍历循环体，生成线性IR
    // ast_node * bodyResult = ir_visit_ast_node_with_2_labels(bodyNode, loopEntryLabel, loopExitLabel);
    ast_node * bodyResult = ir_visit_ast_node(bodyNode);
    if (!bodyResult) {
        return false;
    }

    // 保存循环体的指令
    node->blockInsts.addInst(bodyResult->blockInsts);

    // 创建并插入无条件跳转指令，目标为循环入口 (L1)
    node->blockInsts.addInst(new GotoInstruction(currentFunc, loopEntryLabel));

    // 插入循环出口标签 (L3)
    node->blockInsts.addInst(loopExitLabel);

    // 弹出标签
    currentFunc->popBreakLabel();
    currentFunc->popContinueLabel();

    return true;
}

/// @brief break语句AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_break_statement(ast_node * node)
{
    Function * currentFunc = module->getCurrentFunction();

    // 获取当前的break跳转标签
    LabelInstruction * breakLabel = currentFunc->getBreakLabel();
    if (!breakLabel) {
        minic_log(LOG_ERROR, "break语句必须在循环中使用");
        return false;
    }

    // 创建无条件跳转指令到循环出口
    node->blockInsts.addInst(new GotoInstruction(currentFunc, breakLabel));

    return true;
}

/// @brief continue语句AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_continue_statement(ast_node * node)
{
    Function * currentFunc = module->getCurrentFunction();

    // 获取当前的continue跳转标签
    LabelInstruction * continueLabel = currentFunc->getContinueLabel();
    if (!continueLabel) {
        minic_log(LOG_ERROR, "continue语句必须在循环中使用");
        return false;
    }

    // 创建无条件跳转指令到循环入口
    node->blockInsts.addInst(new GotoInstruction(currentFunc, continueLabel));

    return true;
}