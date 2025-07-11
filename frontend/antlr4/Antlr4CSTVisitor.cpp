///
/// @file Antlr4CSTVisitor.cpp
/// @brief Antlr4的具体语法树的遍历产生AST
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

#include <cstddef>
#include <iostream>
#include <string>

#include "Antlr4CSTVisitor.h"
#include "AST.h"
#include "AttrType.h"
#include "Common.h"

#define Instanceof(res, type, var) auto res = dynamic_cast<type>(var)

/// @brief 构造函数
MiniCCSTVisitor::MiniCCSTVisitor()
{}

/// @brief 析构函数
MiniCCSTVisitor::~MiniCCSTVisitor()
{}

/// @brief 遍历CST产生AST
/// @param root CST语法树的根结点
/// @return AST的根节点
ast_node * MiniCCSTVisitor::run(MiniCParser::CompileUnitContext * root)
{
    return std::any_cast<ast_node *>(visitCompileUnit(root));
}

/// @brief 非终结运算符compileUnit的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitCompileUnit(MiniCParser::CompileUnitContext * ctx)
{
    // compileUnit: (funcDef | varDecl)* EOF

    // 请注意这里必须先遍历全局变量后遍历函数。肯定可以确保全局变量先声明后使用的规则，但有些情况却不能检查出。
    // 事实上可能函数A后全局变量B后函数C，这时在函数A中是不能使用变量B的，需要报语义错误，但目前的处理不会。
    // 因此在进行语义检查时，可能追加检查行号和列号，如果函数的行号/列号在全局变量的行号/列号的前面则需要报语义错误
    // TODO 请追加实现。

    ast_node * temp_node;
    ast_node * compileUnitNode = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT);

    // 可能多个变量，因此必须循环遍历
    for (auto varCtx: ctx->varDecl()) {

        // 变量函数定义
        temp_node = std::any_cast<ast_node *>(visitVarDecl(varCtx));
        (void) compileUnitNode->insert_son_node(temp_node);
    }

    // 可能有多个函数，因此必须循环遍历
    for (auto funcCtx: ctx->funcDef()) {

        // 变量函数定义
        temp_node = std::any_cast<ast_node *>(visitFuncDef(funcCtx));
        (void) compileUnitNode->insert_son_node(temp_node);
    }

    return compileUnitNode;
}

/// @brief 非终结运算符funcDef的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncDef(MiniCParser::FuncDefContext * ctx)
{
    // 识别的文法产生式：funcDef: funcType T_ID T_L_PAREN (funcParams | T_VOID)? T_R_PAREN block;

    // 函数返回类型，非终结符
    type_attr funcReturnType = std::any_cast<type_attr>(visitFuncType(ctx->funcType()));

    // 创建函数名的标识符终结符节点，终结符
    char * id = strdup(ctx->T_ID()->getText().c_str());
    var_id_attr funcId{id, (int64_t) ctx->T_ID()->getSymbol()->getLine()};

    // 形参结点处理
    ast_node * formalParamsNode = nullptr;
    if (ctx->funcParams()) {
        // 有参数列表
        formalParamsNode = std::any_cast<ast_node *>(visitFuncParams(ctx->funcParams()));
    } else if (ctx->T_VOID()) {
        // 显式void参数，formalParamsNode保持nullptr，create_func_def会处理
        formalParamsNode = nullptr;
    }
    // 如果既没有funcParams也没有T_VOID，formalParamsNode保持nullptr，create_func_def会处理

    // 遍历block结点创建函数体节点，非终结符
    auto blockNode = std::any_cast<ast_node *>(visitBlock(ctx->block()));

    // 创建函数定义的节点
    return create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
}

/// @brief 非终结运算符funcType的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncType(MiniCParser::FuncTypeContext * ctx)
{
    // 识别的文法产生式：funcType: T_INT | T_VOID;
    type_attr attr{BasicType::TYPE_VOID, -1};

    if (ctx->T_INT()) {
        attr.type = BasicType::TYPE_INT;
        attr.lineno = (int64_t) ctx->T_INT()->getSymbol()->getLine();
    } else if (ctx->T_VOID()) {
        attr.type = BasicType::TYPE_VOID;
        attr.lineno = (int64_t) ctx->T_VOID()->getSymbol()->getLine();
    }

    return attr;
}

/// @brief 非终结运算符funcParams的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncParams(MiniCParser::FuncParamsContext * ctx)
{
    // 识别的文法产生式：funcParams: funcParam (T_COMMA funcParam)*;

    auto paramsNode = new ast_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS);

    // 遍历所有参数
    for (auto paramCtx: ctx->funcParam()) {
        auto paramNode = std::any_cast<ast_node *>(visitFuncParam(paramCtx));
        paramsNode->insert_son_node(paramNode);
    }

    return paramsNode;
}

/// @brief 非终结符funcParam的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncParam(MiniCParser::FuncParamContext * ctx)
{
    // 识别的文法产生式：funcParam: basicType T_ID (T_L_BRACKET T_INT_CONST? T_R_BRACKET)*;

    // 获取参数类型
    type_attr paramType = std::any_cast<type_attr>(visitBasicType(ctx->basicType()));

    // 获取参数名
    char * id = strdup(ctx->T_ID()->getText().c_str());
    var_id_attr paramName{id, (int64_t) ctx->T_ID()->getSymbol()->getLine()};

    // 创建类型节点
    ast_node * type_node = create_type_node(paramType);

    // 创建参数名节点
    ast_node * name_node = ast_node::New(paramName);

    // 检查是否是数组参数
    if (!ctx->T_L_BRACKET().empty()) {
        // 数组参数，支持多维数组 如 int arr[] 或 int arr[10] 或 int arr[10][20]
        std::vector<ast_node *> dimension_exprs;

        // 获取所有的整数常量（维度大小）
        auto intConstList = ctx->T_INT_CONST();
        size_t intConstIndex = 0;

        // 遍历所有的 [ ] 对
        for (size_t i = 0; i < ctx->T_L_BRACKET().size(); i++) {
            if (intConstIndex < intConstList.size()) {
                // 有具体维度大小，如 arr[10]
                std::string dimText = intConstList[intConstIndex]->getText();
                uint32_t dimVal = (uint32_t) stoull(dimText, nullptr, 0);
                int64_t dimLineNo = (int64_t) intConstList[intConstIndex]->getSymbol()->getLine();
                dimension_exprs.push_back(ast_node::New(digit_int_attr{dimVal, dimLineNo}));
                intConstIndex++;
            } else {
                // 没有具体维度大小，如 arr[]，用空指针表示
                dimension_exprs.push_back(nullptr);
            }
        }

        // 创建数组访问节点来表示这是数组类型参数
        ast_node * array_access_node = create_array_access(name_node, dimension_exprs);

        // 创建形参节点，包含类型节点和数组访问节点
        return ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, array_access_node, nullptr);
    } else {
        // 普通参数，只包含类型和名称
        return ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, name_node, nullptr);
    }
}

/// @brief 非终结运算符block的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBlock(MiniCParser::BlockContext * ctx)
{
    // 识别的文法产生式：block : T_L_BRACE blockItemList? T_R_BRACE';
    if (!ctx->blockItemList()) {
        // 语句块没有语句

        // 为了方便创建一个空的Block节点
        return create_contain_node(ast_operator_type::AST_OP_BLOCK);
    }

    // 语句块含有语句

    // 内部创建Block节点，并把语句加入，这里不需要创建Block节点
    return visitBlockItemList(ctx->blockItemList());
}

/// @brief 非终结运算符blockItemList的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBlockItemList(MiniCParser::BlockItemListContext * ctx)
{
    // 识别的文法产生式：blockItemList : blockItem +;
    // 正闭包 循环 至少一个blockItem
    auto block_node = create_contain_node(ast_operator_type::AST_OP_BLOCK);

    for (auto blockItemCtx: ctx->blockItem()) {

        // 非终结符，需遍历
        auto blockItem = std::any_cast<ast_node *>(visitBlockItem(blockItemCtx));

        // 插入到块节点中
        (void) block_node->insert_son_node(blockItem);
    }

    return block_node;
}

///
/// @brief 非终结运算符blockItem的遍历
/// @param ctx CST上下文
///
std::any MiniCCSTVisitor::visitBlockItem(MiniCParser::BlockItemContext * ctx)
{
    // 识别的文法产生式：blockItem : statement | varDecl
    if (ctx->statement()) {
        // 语句识别

        return visitStatement(ctx->statement());
    } else if (ctx->varDecl()) {
        return visitVarDecl(ctx->varDecl());
    }

    return nullptr;
}

/// @brief 非终结运算符statement中的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitStatement(MiniCParser::StatementContext * ctx)
{
    // 识别的文法产生式：statement: T_ID T_ASSIGN expr T_SEMICOLON  # assignStatement
    // | T_RETURN expr T_SEMICOLON # returnStatement
    // | block  # blockStatement
    // | T_IF T_L_PAREN expr T_R_PAREN statement (T_ELSE statement)? # ifStatement
    // | T_WHILE T_L_PAREN expr T_R_PAREN statement # whileStatement
    // | T_BREAK T_SEMICOLON # breakStatement
    // | T_CONTINUE T_SEMICOLON # continueStatement
    // | expr ? T_SEMICOLON #expressionStatement;
    if (Instanceof(assignCtx, MiniCParser::AssignStatementContext *, ctx)) {
        return visitAssignStatement(assignCtx);
    } else if (Instanceof(returnCtx, MiniCParser::ReturnStatementContext *, ctx)) {
        return visitReturnStatement(returnCtx);
    } else if (Instanceof(blockCtx, MiniCParser::BlockStatementContext *, ctx)) {
        return visitBlockStatement(blockCtx);
    } else if (Instanceof(ifCtx, MiniCParser::IfStatementContext *, ctx)) {
        return visitIfStatement(ifCtx);
    } else if (Instanceof(whileCtx, MiniCParser::WhileStatementContext *, ctx)) {
        return visitWhileStatement(whileCtx);
    } else if (Instanceof(breakCtx, MiniCParser::BreakStatementContext *, ctx)) {
        return visitBreakStatement(breakCtx);
    } else if (Instanceof(continueCtx, MiniCParser::ContinueStatementContext *, ctx)) {
        return visitContinueStatement(continueCtx);
    } else if (Instanceof(exprCtx, MiniCParser::ExpressionStatementContext *, ctx)) {
        return visitExpressionStatement(exprCtx);
    }

    return nullptr;
}

///
/// @brief 非终结运算符statement中的returnStatement的遍历
/// @param ctx CST上下文
///
std::any MiniCCSTVisitor::visitReturnStatement(MiniCParser::ReturnStatementContext * ctx)
{
    // 识别的文法产生式：returnStatement -> T_RETURN expr? T_SEMICOLON

    // 判断是否有返回表达式
    if (ctx->expr()) {
        // 非终结符，表达式expr遍历
        auto exprNode = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
        // 创建返回节点，其孩子为Expr
        return create_contain_node(ast_operator_type::AST_OP_RETURN, exprNode);
    } else {
        // 无返回值
        return create_contain_node(ast_operator_type::AST_OP_RETURN);
    }
}

/// @brief 非终结运算符expr的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitExpr(MiniCParser::ExprContext * ctx)
{
    // 识别文法产生式：expr: logicOrExp;
    return visitLogicOrExp(ctx->logicOrExp());
}

std::any MiniCCSTVisitor::visitAssignStatement(MiniCParser::AssignStatementContext * ctx)
{
    // 识别文法产生式：assignStatement: lVal T_ASSIGN expr T_SEMICOLON

    // 赋值左侧左值Lval遍历产生节点
    auto lvalNode = std::any_cast<ast_node *>(visitLVal(ctx->lVal()));

    // 赋值右侧expr遍历
    auto exprNode = std::any_cast<ast_node *>(visitExpr(ctx->expr()));

    // 创建一个AST_OP_ASSIGN类型的中间节点，孩子为Lval和Expr
    return ast_node::New(ast_operator_type::AST_OP_ASSIGN, lvalNode, exprNode, nullptr);
}

std::any MiniCCSTVisitor::visitBlockStatement(MiniCParser::BlockStatementContext * ctx)
{
    // 识别文法产生式 blockStatement: block

    return visitBlock(ctx->block());
}

/// @brief 非终结运算符logicOrExp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLogicOrExp(MiniCParser::LogicOrExpContext * ctx)
{
    // 识别的文法产生式：logicOrExp: logicAndExp (T_OR logicAndExp)*;

    if (ctx->T_OR().empty()) {
        // 没有OR运算符，则说明闭包识别为0，只识别了第一个非终结符logicAndExp
        return visitLogicAndExp(ctx->logicAndExp()[0]);
    }

    ast_node *left, *right;

    // 存在OR运算符
    auto opsCtxVec = ctx->T_OR();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitLogicAndExp(ctx->logicAndExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitLogicAndExp(ctx->logicAndExp()[k + 1]));

        // 新建结点作为下一个运算符的左操作符
        left = ast_node::New(ast_operator_type::AST_OP_OR, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符logicAndExp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLogicAndExp(MiniCParser::LogicAndExpContext * ctx)
{
    // 识别的文法产生式：logicAndExp: equalityExp (T_AND equalityExp)*;

    if (ctx->T_AND().empty()) {
        // 没有AND运算符，则说明闭包识别为0，只识别了第一个非终结符equalityExp
        return visitEqualityExp(ctx->equalityExp()[0]);
    }

    ast_node *left, *right;

    // 存在AND运算符
    auto opsCtxVec = ctx->T_AND();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitEqualityExp(ctx->equalityExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitEqualityExp(ctx->equalityExp()[k + 1]));

        // 新建结点作为下一个运算符的左操作符
        left = ast_node::New(ast_operator_type::AST_OP_AND, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符equalityExp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitEqualityExp(MiniCParser::EqualityExpContext * ctx)
{
    // 识别的文法产生式：equalityExp: relationalExp (equalityOp relationalExp)*;

    if (ctx->equalityOp().empty()) {
        // 没有equalityOp运算符，则说明闭包识别为0，只识别了第一个非终结符relationalExp
        return visitRelationalExp(ctx->relationalExp()[0]);
    }

    ast_node *left, *right;

    // 存在equalityOp运算符
    auto opsCtxVec = ctx->equalityOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitEqualityOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitRelationalExp(ctx->relationalExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitRelationalExp(ctx->relationalExp()[k + 1]));

        // 新建结点作为下一个运算符的左操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符equalityOp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitEqualityOp(MiniCParser::EqualityOpContext * ctx)
{
    // 识别的文法产生式：equalityOp: T_EQ | T_NE;

    if (ctx->T_EQ()) {
        return ast_operator_type::AST_OP_EQ;
    } else {
        return ast_operator_type::AST_OP_NE;
    }
}

/// @brief 非终结运算符relationalExp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitRelationalExp(MiniCParser::RelationalExpContext * ctx)
{
    // 识别的文法产生式：relationalExp: addExp (relationalOp addExp)*;

    if (ctx->relationalOp().empty()) {
        // 没有relationalOp运算符，则说明闭包识别为0，只识别了第一个非终结符addExp
        return visitAddExp(ctx->addExp()[0]);
    }

    ast_node *left, *right;

    // 存在relationalOp运算符
    auto opsCtxVec = ctx->relationalOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitRelationalOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitAddExp(ctx->addExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitAddExp(ctx->addExp()[k + 1]));

        // 新建结点作为下一个运算符的左操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符relationalOp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitRelationalOp(MiniCParser::RelationalOpContext * ctx)
{
    // 识别的文法产生式：relationalOp: T_LT | T_GT | T_LE | T_GE;

    if (ctx->T_LT()) {
        return ast_operator_type::AST_OP_LT;
    } else if (ctx->T_GT()) {
        return ast_operator_type::AST_OP_GT;
    } else if (ctx->T_LE()) {
        return ast_operator_type::AST_OP_LE;
    } else {
        return ast_operator_type::AST_OP_GE;
    }
}

std::any MiniCCSTVisitor::visitAddExp(MiniCParser::AddExpContext * ctx)
{
    // 识别的文法产生式：addExp : mulExp (addOp mulExp)*;

    if (ctx->addOp().empty()) {
        // 没有addOp运算符，则说明闭包识别为0，只识别了第一个非终结符mulExp
        return visitMulExp(ctx->mulExp()[0]);
    }

    ast_node *left, *right;

    // 存在addOp运算符
    auto opsCtxVec = ctx->addOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitAddOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitMulExp(ctx->mulExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitMulExp(ctx->mulExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符addOp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitAddOp(MiniCParser::AddOpContext * ctx)
{
    // 识别的文法产生式：addOp : T_ADD | T_SUB

    if (ctx->T_ADD()) {
        return ast_operator_type::AST_OP_ADD;
    } else {
        return ast_operator_type::AST_OP_SUB;
    }
}

std::any MiniCCSTVisitor::visitMulExp(MiniCParser::MulExpContext * ctx)
{
    // 识别的文法产生式：mulExp : unaryExp (mulOp unaryExp)*;

    if (ctx->mulOp().empty()) {
        // 没有mulOp运算符，则说明闭包识别为0，只识别了第一个非终结符unaryExp
        return visitUnaryExp(ctx->unaryExp()[0]);
    }

    ast_node *left, *right;

    // 存在mulOp运算符
    auto opsCtxVec = ctx->mulOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {
        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitMulOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结运算符mulOp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitMulOp(MiniCParser::MulOpContext * ctx)
{
    // 识别的文法产生式：mulOp : T_MUL | T_DIV | T_MOD

    if (ctx->T_MUL()) {
        return ast_operator_type::AST_OP_MUL;
    } else if (ctx->T_DIV()) {
        return ast_operator_type::AST_OP_DIV;
    } else {
        return ast_operator_type::AST_OP_MOD;
    }
}

std::any MiniCCSTVisitor::visitUnaryExp(MiniCParser::UnaryExpContext * ctx)
{
    // 识别文法产生式：unaryExp: (T_SUB unaryExp) | (T_NOT unaryExp) | primaryExp | T_ID T_L_PAREN realParamList?
    // T_R_PAREN;
    if (ctx->primaryExp()) {
        // 普通表达式
        return visitPrimaryExp(ctx->primaryExp());
    } else if (ctx->T_ID()) {
        // 创建函数调用名终结符节点
        ast_node * funcname_node = ast_node::New(ctx->T_ID()->getText(), (int64_t) ctx->T_ID()->getSymbol()->getLine());

        // 实参列表
        ast_node * paramListNode = nullptr;

        // 函数调用
        if (ctx->realParamList()) {
            // 有参数
            paramListNode = std::any_cast<ast_node *>(visitRealParamList(ctx->realParamList()));
        }

        // 创建函数调用节点，其孩子为被调用函数名和实参，
        return create_func_call(funcname_node, paramListNode);
    } else if (ctx->T_SUB()) {
        // 处理单目负号
        ast_node * expr_node = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()));
        ast_node * negative_node = ast_node::New(ast_operator_type::AST_OP_NEG, expr_node, nullptr);
        return negative_node;
    } else if (ctx->T_NOT()) {
        // 处理逻辑非
        ast_node * expr_node = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()));
        ast_node * not_node = ast_node::New(ast_operator_type::AST_OP_NOT, expr_node, nullptr);
        return not_node;
    } else {
        return nullptr;
    }
}

std::any MiniCCSTVisitor::visitPrimaryExp(MiniCParser::PrimaryExpContext * ctx)
{
    ast_node * node = nullptr;

    if (ctx->T_INT_CONST()) {
        // 识别 primaryExp：T_INT_DIGIT
        // 无符号整型字面量
        // stoull 函数基数：0（自动）、8（八进制）、10（十进制）、16（十六进制）
        std::string intText = ctx->T_INT_CONST()->getText();
        uint32_t val = (uint32_t) stoull(intText, nullptr, 0);
        int64_t lineNo = (int64_t) ctx->T_INT_CONST()->getSymbol()->getLine();
        // 识别 primaryExp: T_DIGIT

        node = ast_node::New(digit_int_attr{val, lineNo});
    } else if (ctx->lVal()) {
        // 具有左值的表达式
        node = std::any_cast<ast_node *>(visitLVal(ctx->lVal()));
    } else if (ctx->expr()) {
        // 带有括号的表达式
        node = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
    }

    return node;
}

std::any MiniCCSTVisitor::visitLVal(MiniCParser::LValContext * ctx)
{
    // 识别文法产生式：lVal: T_ID (T_L_BRACKET expr T_R_BRACKET)*;

    // 获取变量名
    auto varId = ctx->T_ID()->getText();
    int64_t lineNo = (int64_t) ctx->T_ID()->getSymbol()->getLine();
    ast_node * var_node = ast_node::New(varId, lineNo);

    // 检查是否有数组下标
    if (!ctx->T_L_BRACKET().empty()) {
        // 数组访问
        std::vector<ast_node *> index_exprs;

        // 遍历所有下标表达式
        for (auto exprCtx: ctx->expr()) {
            ast_node * index_expr = std::any_cast<ast_node *>(visitExpr(exprCtx));
            index_exprs.push_back(index_expr);
        }

        return create_array_access(var_node, index_exprs);
    } else {
        // 普通变量
        return var_node;
    }
}

std::any MiniCCSTVisitor::visitExprList(MiniCParser::ExprListContext * ctx)
{
    // 识别的文法产生式：exprList: expr (T_COMMA expr)*;

    std::vector<ast_node *> init_exprs;

    // 遍历所有表达式
    for (auto exprCtx: ctx->expr()) {
        ast_node * expr_node = std::any_cast<ast_node *>(visitExpr(exprCtx));
        init_exprs.push_back(expr_node);
    }

    return create_array_init(init_exprs);
}

std::any MiniCCSTVisitor::visitVarDecl(MiniCParser::VarDeclContext * ctx)
{
    // varDecl: basicType varDef (T_COMMA varDef)* T_SEMICOLON;

    // 声明语句节点
    ast_node * stmt_node = create_contain_node(ast_operator_type::AST_OP_DECL_STMT);

    // 类型节点
    type_attr typeAttr = std::any_cast<type_attr>(visitBasicType(ctx->basicType()));

    for (auto & varCtx: ctx->varDef()) {
        // 获取变量定义节点（可能包含初值）
        ast_node * var_def_node = std::any_cast<ast_node *>(visitVarDef(varCtx));

        // 创建类型节点
        ast_node * type_node = create_type_node(typeAttr);

        // 创建变量声明节点
        ast_node * decl_node = ast_node::New(ast_operator_type::AST_OP_VAR_DECL, type_node, var_def_node, nullptr);

        // 插入到变量声明语句
        (void) stmt_node->insert_son_node(decl_node);
    }

    return stmt_node;
}

std::any MiniCCSTVisitor::visitVarDef(MiniCParser::VarDefContext * ctx)
{
    // varDef: T_ID (T_L_BRACKET T_INT_CONST T_R_BRACKET)* (T_ASSIGN (expr | T_L_BRACE exprList T_R_BRACE))?;

    auto varId = ctx->T_ID()->getText();
    int64_t lineNo = (int64_t) ctx->T_ID()->getSymbol()->getLine();

    // 检查是否有数组维度
    if (!ctx->T_L_BRACKET().empty()) {
        // 数组声明
        char * id = strdup(varId.c_str());
        var_id_attr arrayName{id, lineNo};

        // 获取数组维度
        std::vector<ast_node *> dimensions;
        for (auto intConst: ctx->T_INT_CONST()) {
            std::string dimText = intConst->getText();
            uint32_t dimVal = (uint32_t) stoull(dimText, nullptr, 0);
            int64_t dimLineNo = (int64_t) intConst->getSymbol()->getLine();
            dimensions.push_back(ast_node::New(digit_int_attr{dimVal, dimLineNo}));
        }

        // 创建基本类型属性（需要从父上下文获取）
        ast_node * array_node = create_array_decl(arrayName, dimensions);

        // 检查是否有初始化
        if (ctx->T_ASSIGN()) {
            if (ctx->exprList()) {
                // 数组初始化列表
                ast_node * init_list = std::any_cast<ast_node *>(visitExprList(ctx->exprList()));
                return ast_node::New(ast_operator_type::AST_OP_ASSIGN, array_node, init_list, nullptr);
            } else if (ctx->expr()) {
                // 单个表达式初始化
                ast_node * init_expr = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
                return ast_node::New(ast_operator_type::AST_OP_ASSIGN, array_node, init_expr, nullptr);
            }
        }

        return array_node;
    } else {
        // 普通变量定义
        ast_node * id_node = ast_node::New(varId, lineNo);

        // 检查是否有初值
        if (ctx->T_ASSIGN()) {
            if (ctx->expr()) {
                // 表达式初始化
                ast_node * init_expr = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
                return ast_node::New(ast_operator_type::AST_OP_ASSIGN, id_node, init_expr, nullptr);
            }
        }

        return id_node;
    }
}

std::any MiniCCSTVisitor::visitBasicType(MiniCParser::BasicTypeContext * ctx)
{
    // basicType: T_INT;
    type_attr attr{BasicType::TYPE_VOID, -1};
    if (ctx->T_INT()) {
        attr.type = BasicType::TYPE_INT;
        attr.lineno = (int64_t) ctx->T_INT()->getSymbol()->getLine();
    }

    return attr;
}

std::any MiniCCSTVisitor::visitRealParamList(MiniCParser::RealParamListContext * ctx)
{
    // 识别的文法产生式：realParamList : expr (T_COMMA expr)*;

    auto paramListNode = create_contain_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS);

    for (auto paramCtx: ctx->expr()) {

        auto paramNode = std::any_cast<ast_node *>(visitExpr(paramCtx));

        paramListNode->insert_son_node(paramNode);
    }

    return paramListNode;
}

std::any MiniCCSTVisitor::visitExpressionStatement(MiniCParser::ExpressionStatementContext * ctx)
{
    // 识别文法产生式  expr ? T_SEMICOLON #expressionStatement;
    if (ctx->expr()) {
        // 表达式语句

        // 遍历expr非终结符，创建表达式节点后返回
        return visitExpr(ctx->expr());
    } else {
        // 空语句，创建一个空块
        return create_contain_node(ast_operator_type::AST_OP_BLOCK);
    }
}

std::any MiniCCSTVisitor::visitIfStatement(MiniCParser::IfStatementContext * ctx)
{
    // 获取条件表达式
    ast_node * condExpr = std::any_cast<ast_node *>(visitExpr(ctx->expr()));

    // std::cout << "Visiting statement: " << ctx->getText() << "\n";

    // 获取真分支语句块
    std::any trueResult = visitStatement(ctx->statement(0));
    ast_node * trueBlock;

    // 检查是否为空语句
    if (trueResult.type() == typeid(std::nullptr_t)) {
        // 如果是空语句，创建一个空块
        trueBlock = create_contain_node(ast_operator_type::AST_OP_BLOCK);
    } else {
        trueBlock = std::any_cast<ast_node *>(trueResult);
    }

    // 创建IF节点
    ast_node * ifNode;

    if (ctx->T_ELSE()) {
        // 有else分支
        std::any falseResult = visitStatement(ctx->statement(1));
        ast_node * falseBlock;

        // 检查是否为空语句
        if (falseResult.type() == typeid(std::nullptr_t)) {
            // 如果是空语句，创建一个空块
            falseBlock = create_contain_node(ast_operator_type::AST_OP_BLOCK);
        } else {
            falseBlock = std::any_cast<ast_node *>(falseResult);
        }

        // IF节点有三个子节点：条件、真分支、假分支
        ifNode = ast_node::New(ast_operator_type::AST_OP_IF, condExpr, trueBlock, falseBlock, nullptr);
    } else {
        // 没有else分支

        // IF节点有两个子节点：条件、真分支
        ifNode = ast_node::New(ast_operator_type::AST_OP_IF, condExpr, trueBlock, nullptr);
    }

    return ifNode;
}

std::any MiniCCSTVisitor::visitWhileStatement(MiniCParser::WhileStatementContext * ctx)
{
    // 识别文法产生式：T_WHILE T_L_PAREN expr T_R_PAREN statement

    // 获取循环条件表达式
    ast_node * condExpr = std::any_cast<ast_node *>(visitExpr(ctx->expr()));

    // 获取循环体
    ast_node * bodyBlock = std::any_cast<ast_node *>(visitStatement(ctx->statement()));

    // 创建WHILE节点，有两个子节点：条件、循环体
    ast_node * whileNode = ast_node::New(ast_operator_type::AST_OP_WHILE, condExpr, bodyBlock, nullptr);

    return whileNode;
}

std::any MiniCCSTVisitor::visitBreakStatement(MiniCParser::BreakStatementContext * ctx)
{
    // 识别文法产生式：breakStatement -> T_BREAK T_SEMICOLON

    // 创建BREAK节点
    return create_contain_node(ast_operator_type::AST_OP_BREAK);
}

std::any MiniCCSTVisitor::visitContinueStatement(MiniCParser::ContinueStatementContext * ctx)
{
    // 识别文法产生式：continueStatement -> T_CONTINUE T_SEMICOLON

    // 创建CONTINUE节点
    return create_contain_node(ast_operator_type::AST_OP_CONTINUE);
}
