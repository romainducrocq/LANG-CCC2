#include "semantic/id_resolve.hpp"
#include "util/error.hpp"
#include "util/names.hpp"
#include "semantic/loops.hpp"
#include "semantic/type_check.hpp"
#include "ast/ast.hpp"
#include "ast/c_ast.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
cdef dict[str, Py_ssize_t] external_linkage_scope_map = {}
*/
static std::unordered_map<TIdentifier, size_t> external_linkage_scope_map;

/**
cdef list[dict[str, str]] scoped_identifier_maps = [{}]
*/
static std::vector<std::unordered_map<TIdentifier, TIdentifier>> scoped_identifier_maps;

/**
cdef dict[str, str] goto_map = {}
*/
static std::unordered_map<TIdentifier, TIdentifier> goto_map;

/**
cdef set[str] label_set = set()
*/
static std::unordered_set<TIdentifier> label_set;

/**
cdef Py_ssize_t current_scope_depth():
    return len(scoped_identifier_maps)
*/
static size_t current_scope_depth() {
    return scoped_identifier_maps.size();
}

/**
cdef bint is_file_scope():
    return current_scope_depth() == 1
*/
static bool is_file_scope() {
    return current_scope_depth() == 1;
}

/**
cdef void enter_scope():
    scoped_identifier_maps.append({})
*/
static void enter_scope() {
    scoped_identifier_maps.emplace_back();
}

/**
cdef void exit_scope():
    cdef str identifier
    for identifier in scoped_identifier_maps[-1]:
        if identifier in external_linkage_scope_map and \
           external_linkage_scope_map[identifier] == current_scope_depth():
            del external_linkage_scope_map[identifier]
    del scoped_identifier_maps[-1]
*/
static void exit_scope() {
    for(const auto& identifier: scoped_identifier_maps.back()) {
        if(external_linkage_scope_map.find(identifier.first) != external_linkage_scope_map.end() &&
           external_linkage_scope_map[identifier.first] == current_scope_depth()) {
            external_linkage_scope_map.erase(identifier.first);
        }
    }
    scoped_identifier_maps.pop_back();
}

/**
cdef void resolve_label():
    cdef str target
    for target in goto_map:
        if not target in label_set:

            raise RuntimeError(
                f"An error occurred in variable resolution, goto \"{target}\" has no target label")
*/
static void resolve_label() {
    for(const auto& target: goto_map) {
        if(label_set.find(target.first) == label_set.end()) {
            raise_runtime_error("An error occurred in variable resolution, goto " + em(target.first) +
                                " has no target label");
        }
    }
}

static void resolve_expression(CExp* node);

/**
cdef void resolve_function_call_expression(CFunctionCall node):
    cdef Py_ssize_t i, scope
    cdef TIdentifier name
    for scope in range(current_scope_depth()):
        i = - (scope + 1)
        if node.name.str_t in scoped_identifier_maps[i]:
            name = TIdentifier(scoped_identifier_maps[i][node.name.str_t])
            node.name = name
            break
    else:

        raise RuntimeError(
            f"Function {node.name.str_t} was not declared in this scope")

    for i in range(len(node.args)):
        resolve_expression(node.args[i])
*/
static void resolve_function_call_expression(CFunctionCall* node) {
    for(size_t i = current_scope_depth(); i-- > 0;) {
        if(scoped_identifier_maps[i].find(node->name) != scoped_identifier_maps[i].end()) {
            node->name = scoped_identifier_maps[i][node->name];
            goto end;
        }
    }
    raise_runtime_error("Function " + em(node->name) +
                        " was not declared in this scope");
    end:

    for(size_t i = 0; i < node->args.size(); i++) {
        resolve_expression(node->args[i].get());
    }
}

/**
cdef void resolve_var_expression(CVar node):
    cdef Py_ssize_t i, scope
    cdef TIdentifier name
    for scope in range(current_scope_depth()):
        i = - (scope + 1)
        if node.name.str_t in scoped_identifier_maps[i]:
            name = TIdentifier(scoped_identifier_maps[i][node.name.str_t])
            node.name = name
            break
    else:

        raise RuntimeError(
            f"Variable {node.name.str_t} was not declared in this scope")
*/
static void resolve_var_expression(CVar* node) {
    for(size_t i = current_scope_depth(); i-- > 0;) {
        if(scoped_identifier_maps[i].find(node->name) != scoped_identifier_maps[i].end()) {
            node->name = scoped_identifier_maps[i][node->name];
            return;
        }
    }
    raise_runtime_error("Variable " + em(node->name) +
                        " was not declared in this scope");
}

/**
cdef void resolve_cast_expression(CCast node):
    resolve_expression(node.exp)
*/
static void resolve_cast_expression(CCast* node) {
    resolve_expression(node->exp.get());
}

/**
cdef void resolve_constant_expression(CConstant node):
    pass
*/
static void resolve_constant_expression(CConstant* /*node*/) {
    ;
}

/**
cdef void resolve_assignment_expression(CAssignment node):
    if not isinstance(node.exp_left, CVar):

        raise RuntimeError(
            f"Left expression {type(node.exp_left)} is an invalid lvalue")

    resolve_expression(node.exp_left)
    resolve_expression(node.exp_right)
*/
static void resolve_assignment_expression(CAssignment* node) {
    if(node->exp_left->type() != AST_T::CVar_t) {
        raise_runtime_error("Left expression is an invalid lvalue");
    }
    resolve_expression(node->exp_left.get());
    resolve_expression(node->exp_right.get());
}

/**
cdef void resolve_unary_expression(CUnary node):
    resolve_expression(node.exp)
*/
static void resolve_unary_expression(CUnary* node) {
    resolve_expression(node->exp.get());
}

/**
cdef void resolve_binary_expression(CBinary node):
    resolve_expression(node.exp_left)
    resolve_expression(node.exp_right)
*/
static void resolve_binary_expression(CBinary* node) {
    resolve_expression(node->exp_left.get());
    resolve_expression(node->exp_right.get());
}

/**
cdef void resolve_conditional_expression(CConditional node):
    resolve_expression(node.condition)
    resolve_expression(node.exp_middle)
    resolve_expression(node.exp_right)
*/
static void resolve_conditional_expression(CConditional* node) {
    resolve_expression(node->condition.get());
    resolve_expression(node->exp_middle.get());
    resolve_expression(node->exp_right.get());
}

/**
cdef void resolve_expression(CExp node):
    if isinstance(node, CFunctionCall):
        resolve_function_call_expression(node)
        checktype_function_call_expression(node)
    elif isinstance(node, CVar):
        resolve_var_expression(node)
        checktype_var_expression(node)
    elif isinstance(node, CCast):
        resolve_cast_expression(node)
        checktype_cast_expression(node)
    elif isinstance(node, CConstant):
        resolve_constant_expression(node)
        checktype_constant_expression(node)
    elif isinstance(node, CAssignment):
        resolve_assignment_expression(node)
        checktype_assignment_expression(node)
    elif isinstance(node, CAssignmentCompound):
        resolve_assignment_compound_expression(node)
        checktype_assignment_compound_expression(node)
    elif isinstance(node, CUnary):
        resolve_unary_expression(node)
        checktype_unary_expression(node)
    elif isinstance(node, CBinary):
        resolve_binary_expression(node)
        checktype_binary_expression(node)
    elif isinstance(node, CConditional):
        resolve_conditional_expression(node)
        checktype_conditional_expression(node)
    else:

        raise RuntimeError(
            "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_expression(CExp* node) {
    switch(node->type()) {
        case AST_T::CFunctionCall_t: {
            CFunctionCall* p_node = static_cast<CFunctionCall*>(node);
            resolve_function_call_expression(p_node);
            checktype_function_call_expression(p_node);
            break;
        }
        case AST_T::CVar_t: {
            CVar* p_node = static_cast<CVar*>(node);
            resolve_var_expression(p_node);
            checktype_var_expression(p_node);
            break;
        }
        case AST_T::CCast_t: {
            CCast* p_node = static_cast<CCast*>(node);
            resolve_cast_expression(p_node);
            checktype_cast_expression(p_node);
            break;
        }
        case AST_T::CConstant_t: {
            CConstant* p_node = static_cast<CConstant*>(node);
            resolve_constant_expression(p_node);
            checktype_constant_expression(p_node);
            break;
        }
        case AST_T::CAssignment_t: {
            CAssignment* p_node = static_cast<CAssignment*>(node);
            resolve_assignment_expression(p_node);
            checktype_assignment_expression(p_node);
            break;
        }
        case AST_T::CUnary_t: {
            CUnary* p_node = static_cast<CUnary*>(node);
            resolve_unary_expression(p_node);
            checktype_unary_expression(p_node);
            break;
        }
        case AST_T::CBinary_t: {
            CBinary* p_node = static_cast<CBinary*>(node);
            resolve_binary_expression(p_node);
            checktype_binary_expression(p_node);
            break;
        }
        case AST_T::CConditional_t: {
            CConditional* p_node = static_cast<CConditional*>(node);
            resolve_conditional_expression(p_node);
            checktype_conditional_expression(p_node);
            break;
        }
        default:
            RAISE_INTERNAL_ERROR;
    }
}

static void resolve_block(CBlock* node);
static void resolve_block_scope_variable_declaration(CVariableDeclaration* node);

static void resolve_statement(CStatement* node);

/**
cdef void resolve_for_block_scope_variable_declaration(CVariableDeclaration node):
    if node.storage_class:

        raise RuntimeError(
            f"Variable {node.name.str_t} was not declared with automatic linkage in for loop initializer")

    resolve_block_scope_variable_declaration(node)
*/
static void resolve_for_block_scope_variable_declaration(CVariableDeclaration* node) {
    if(node->storage_class) {
        raise_runtime_error("Variable " + em(node->name) +
                            " was not declared with automatic linkage in for loop initializer");
    }
    resolve_block_scope_variable_declaration(node);
}

/**
cdef void resolve_for_init(CForInit node):
    if isinstance(node, CInitDecl):
        resolve_for_block_scope_variable_declaration(node.init)
    elif isinstance(node, CInitExp):
        if node.init:
            resolve_expression(node.init)
    else:

        raise RuntimeError(
            "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_for_init(CForInit* node) {
    switch(node->type()) {
        case AST_T::CInitDecl_t:
            resolve_for_block_scope_variable_declaration(static_cast<CInitDecl*>(node)->init.get());
            break;
        case AST_T::CInitExp_t: {
            CInitExp* init_decl = static_cast<CInitExp*>(node);
            if(init_decl->init) {
                resolve_expression(init_decl->init.get());
            }
            break;
        }
        default:
            RAISE_INTERNAL_ERROR;
    }
}

/**
cdef void resolve_null_statement(CNull node):
    pass
*/
static void resolve_null_statement(CNull* /*node*/) {
    ;
}

/**
cdef void resolve_return_statement(CReturn node):
    resolve_expression(node.exp)
*/
static void resolve_return_statement(CReturn* node) {
    resolve_expression(node->exp.get());
}

/**
cdef void resolve_expression_statement(CExpression node):
    resolve_expression(node.exp)
*/
static void resolve_expression_statement(CExpression* node) {
    resolve_expression(node->exp.get());
}

/**
cdef void resolve_compound_statement(CCompound node):
    enter_scope()
    resolve_block(node.block)
    exit_scope()
*/
static void resolve_compound_statement(CCompound* node) {
    enter_scope();
    resolve_block(node->block.get());
    exit_scope();
}

/**
cdef void resolve_if_statement(CIf node):
    resolve_expression(node.condition)
    resolve_statement(node.then)
    if node.else_fi:
        resolve_statement(node.else_fi)
*/
static void resolve_if_statement(CIf* node) {
    resolve_expression(node->condition.get());
    resolve_statement(node->then.get());
    if(node->else_fi) {
        resolve_statement(node->else_fi.get());
    }
}

/**
cdef void resolve_while_statement(CWhile node):
    annotate_while_loop(node)
    resolve_expression(node.condition)
    resolve_statement(node.body)
    deannotate_loop()
*/
static void resolve_while_statement(CWhile* node) {
    annotate_while_loop(node);
    resolve_expression(node->condition.get());
    resolve_statement(node->body.get());
    deannotate_loop();
}

/**
cdef void resolve_do_while_statement(CDoWhile node):
    annotate_do_while_loop(node)
    resolve_statement(node.body)
    resolve_expression(node.condition)
    deannotate_loop()
*/
static void resolve_do_while_statement(CDoWhile* node) {
    annotate_do_while_loop(node);
    resolve_statement(node->body.get());
    resolve_expression(node->condition.get());
    deannotate_loop();
}

/**
cdef void resolve_for_statement(CFor node):
    annotate_for_loop(node)
    enter_scope()
    resolve_for_init(node.init)
    if node.condition:
        resolve_expression(node.condition)
    if node.post:
        resolve_expression(node.post)
    resolve_statement(node.body)
    exit_scope()
    deannotate_loop()
*/
static void resolve_for_statement(CFor* node) {
    annotate_for_loop(node);
    enter_scope();
    resolve_for_init(node->init.get());
    if(node->condition) {
        resolve_expression(node->condition.get());
    }
    if(node->post) {
        resolve_expression(node->post.get());
    }
    resolve_statement(node->body.get());
    exit_scope();
    deannotate_loop();
}

/**
cdef void resolve_break_statement(CBreak node):
    annotate_break_loop(node)
*/
static void resolve_break_statement(CBreak* node) {
    annotate_break_loop(node);
}

/**
cdef void resolve_continue_statement(CContinue node):
    annotate_continue_loop(node)
*/
static void resolve_continue_statement(CContinue* node) {
    annotate_continue_loop(node);
}

/**
cdef void resolve_label_statement(CLabel node):
    if node.target.str_t in label_set:

        raise RuntimeError(
            f"Label {node.target.str_t} was already declared in this scope")

    label_set.add(node.target.str_t)

    cdef TIdentifier target
    if node.target.str_t in goto_map:
        target = TIdentifier(goto_map[node.target.str_t])
        node.target = target
    else:
        target = resolve_label_identifier(node.target)
        goto_map[node.target.str_t] = target.str_t
        node.target = target
    resolve_statement(node.jump_to)
*/
static void resolve_label_statement(CLabel* node) {
    if(label_set.find(node->target) != label_set.end()) {
        raise_runtime_error("Label " + em(node->target) +
                            " was already declared in this scope");
    }
    label_set.insert(node->target);

    if(goto_map.find(node->target) != goto_map.end()) {
        node->target = goto_map[node->target];
    }
    else {
        goto_map[node->target] = resolve_label_identifier(node->target);
        node->target = goto_map[node->target];
    }
    resolve_statement(node->jump_to.get());
}

/**
cdef void resolve_goto_statement(CGoto node):
    cdef TIdentifier target
    if node.target.str_t in goto_map:
        target = TIdentifier(goto_map[node.target.str_t])
        node.target = target
    else:
        target = resolve_label_identifier(node.target)
        goto_map[node.target.str_t] = target.str_t
        node.target = target
*/
static void resolve_goto_statement(CGoto* node) {
    if(goto_map.find(node->target) != goto_map.end()) {
        node->target = goto_map[node->target];
    }
    else {
        goto_map[node->target] = resolve_label_identifier(node->target);
        node->target = goto_map[node->target];
    }
}

/**
cdef void resolve_statement(CStatement node):
    if isinstance(node, CNull):
        resolve_null_statement(node)
    elif isinstance(node, CReturn):
        resolve_return_statement(node)
        checktype_return_statement(node)
    elif isinstance(node, CExpression):
        resolve_expression_statement(node)
    elif isinstance(node, CCompound):
        resolve_compound_statement(node)
    elif isinstance(node, CIf):
        resolve_if_statement(node)
    elif isinstance(node, CWhile):
        resolve_while_statement(node)
    elif isinstance(node, CDoWhile):
        resolve_do_while_statement(node)
    elif isinstance(node, CFor):
        resolve_for_statement(node)
    elif isinstance(node, CBreak):
        resolve_break_statement(node)
    elif isinstance(node, CContinue):
        resolve_continue_statement(node)
    elif isinstance(node, CLabel):
        resolve_label_statement(node)
    elif isinstance(node, CGoto):
        resolve_goto_statement(node)
    else:

        raise RuntimeError(
            "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_statement(CStatement* node) {
    switch(node->type()) {
        case AST_T::CNull_t:
            resolve_null_statement(static_cast<CNull*>(node));
            break;
        case AST_T::CReturn_t: {
            CReturn* p_node = static_cast<CReturn*>(node);
            resolve_return_statement(p_node);
            checktype_return_statement(p_node);
            break;
        }
        case AST_T::CExpression_t:
            resolve_expression_statement(static_cast<CExpression*>(node));
            break;
        case AST_T::CCompound_t:
            resolve_compound_statement(static_cast<CCompound*>(node));
            break;
        case AST_T::CIf_t:
            resolve_if_statement(static_cast<CIf*>(node));
            break;
        case AST_T::CWhile_t:
            resolve_while_statement(static_cast<CWhile*>(node));
            break;
        case AST_T::CDoWhile_t:
            resolve_do_while_statement(static_cast<CDoWhile*>(node));
            break;
        case AST_T::CFor_t:
            resolve_for_statement(static_cast<CFor*>(node));
            break;
        case AST_T::CBreak_t:
            resolve_break_statement(static_cast<CBreak*>(node));
            break;
        case AST_T::CContinue_t:
            resolve_continue_statement(static_cast<CContinue*>(node));
            break;
        case AST_T::CLabel_t:
            resolve_label_statement(static_cast<CLabel*>(node));
            break;
        case AST_T::CGoto_t:
            resolve_goto_statement(static_cast<CGoto*>(node));
            break;
        default:
            RAISE_INTERNAL_ERROR;
    }
}

static void resolve_declaration(CDeclaration* node);

/**
cdef void resolve_block_items(list[CBlockItem] list_node):

    cdef Py_ssize_t block_item
    for block_item in range(len(list_node)):
        if isinstance(list_node[block_item], CS):
            resolve_statement(list_node[block_item].statement)
        elif isinstance(list_node[block_item], CD):
            resolve_declaration(list_node[block_item].declaration)
        else:

            raise RuntimeError(
                "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_block_items(std::vector<std::unique_ptr<CBlockItem>>& list_node) {
    for(size_t block_item = 0; block_item < list_node.size(); block_item++) {
        switch(list_node[block_item]->type()) {
            case AST_T::CS_t:
                resolve_statement(static_cast<CS*>(list_node[block_item].get())->statement.get());
                break;
            case AST_T::CD_t:
                resolve_declaration(static_cast<CD*>(list_node[block_item].get())->declaration.get());
                break;
            default:
                RAISE_INTERNAL_ERROR;
        }
    }
}

/**
cdef void resolve_block(CBlock node):
    if isinstance(node, CB):
        resolve_block_items(node.block_items)
    else:

        raise RuntimeError(
            "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_block(CBlock* node) {
    switch(node->type()) {
        case AST_T::CB_t:
            resolve_block_items(static_cast<CB*>(node)->block_items);
            break;
        default:
            RAISE_INTERNAL_ERROR;
    }
}

/**
cdef void resolve_params(CFunctionDeclaration node):
    cdef Py_ssize_t param
    cdef TIdentifier name
    for param in range(len(node.params)):
        if node.params[param].str_t in scoped_identifier_maps[-1]:

            raise RuntimeError(
                f"Variable {node.params[param]} was already declared in this scope")

        name = resolve_variable_identifier(node.params[param])
        scoped_identifier_maps[-1][node.params[param].str_t] = name.str_t
        node.params[param] = name

    if node.body:
        checktype_params(node)
*/
static void resolve_params(CFunctionDeclaration* node) {
    for(size_t param = 0; param < node->params.size(); param++) {
        if(scoped_identifier_maps.back().find(node->params[param]) != scoped_identifier_maps.back().end()) {
            raise_runtime_error("Variable " + node->params[param] +
                                " was already declared in this scope");
        }
        scoped_identifier_maps.back()[node->params[param]] = resolve_variable_identifier(node->params[param]);
        node->params[param] = scoped_identifier_maps.back()[node->params[param]];
    }

    if(node->body) {
        checktype_params(node);
    }
}

/**
cdef void resolve_function_declaration(CFunctionDeclaration node):
    global scoped_identifier_maps

    if not is_file_scope():
        if node.body:

            raise RuntimeError(
                f"Block scoped function definition {node.name.str_t} can not be nested")

        if isinstance(node.storage_class, CStatic):

            raise RuntimeError(
                f"Block scoped function definition {node.name.str_t} can not be static")

    if node.name.str_t not in external_linkage_scope_map:
        if node.name.str_t in scoped_identifier_maps[-1]:

            raise RuntimeError(
                f"Function {node.name.str_t} was already declared in this scope")

        external_linkage_scope_map[node.name.str_t] = current_scope_depth()

    scoped_identifier_maps[-1][node.name.str_t] = node.name.str_t
    checktype_function_declaration(node)

    enter_scope()
    if node.params:
        resolve_params(node)
    if node.body:
        resolve_block(node.body)
    exit_scope()
*/
static void resolve_function_declaration(CFunctionDeclaration* node) {
    if(!is_file_scope()) {
        if(node->body) {
            raise_runtime_error("Block scoped function definition " + em(node->name) +
                                " can not be nested");
        }
        if(node->storage_class && 
           node->storage_class->type() == AST_T::CStatic_t) {
            raise_runtime_error("Block scoped function definition " + em(node->name) +
                                " can not be static");
        }
    }

    if(external_linkage_scope_map.find(node->name) == external_linkage_scope_map.end()) {
        if(scoped_identifier_maps.back().find(node->name) != scoped_identifier_maps.back().end()) {
            raise_runtime_error("Function " + em(node->name) +
                                " was already declared in this scope");
        }
        external_linkage_scope_map[node->name] = current_scope_depth();
    }

    scoped_identifier_maps.back()[node->name] = node->name;
    checktype_function_declaration(node);

    enter_scope();
    if(!node->params.empty()) {
        resolve_params(node);
    }
    if(node->body) {
        resolve_block(node->body.get());
    }
    exit_scope();
}

/**
cdef void resolve_file_scope_variable_declaration(CVariableDeclaration node):
    global scoped_identifier_maps

    if node.name.str_t not in external_linkage_scope_map:
        external_linkage_scope_map[node.name.str_t] = current_scope_depth()

    scoped_identifier_maps[-1][node.name.str_t] = node.name.str_t
    if is_file_scope():
        checktype_file_scope_variable_declaration(node)
    else:
        checktype_block_scope_variable_declaration(node)
*/
static void resolve_file_scope_variable_declaration(CVariableDeclaration* node) {
    if(external_linkage_scope_map.find(node->name) == external_linkage_scope_map.end()) {
        external_linkage_scope_map[node->name] = current_scope_depth();
    }

    scoped_identifier_maps.back()[node->name] = node->name;
    if(is_file_scope()) {
        checktype_file_scope_variable_declaration(node);
    }
    else {
        checktype_block_scope_variable_declaration(node);
    }
}

/**
cdef void resolve_block_scope_variable_declaration(CVariableDeclaration node):
    global scoped_identifier_maps

    if node.name.str_t in scoped_identifier_maps[-1] and \
       not (node.name.str_t in external_linkage_scope_map and
            isinstance(node.storage_class, CExtern)):

        raise RuntimeError(
            f"Variable {node.name.str_t} was already declared in this scope")

    if isinstance(node.storage_class, CExtern):
        resolve_file_scope_variable_declaration(node)
        return

    cdef TIdentifier name = resolve_variable_identifier(node.name)
    scoped_identifier_maps[-1][node.name.str_t] = name.str_t
    node.name = name
    checktype_block_scope_variable_declaration(node)

    if node.init and \
       not node.storage_class:
        resolve_expression(node.init)

    checktype_init_block_scope_variable_declaration(node)
*/
static void resolve_block_scope_variable_declaration(CVariableDeclaration* node) {
    if(scoped_identifier_maps.back().find(node->name) != scoped_identifier_maps.back().end() &&
       !(external_linkage_scope_map.find(node->name) != external_linkage_scope_map.end() &&
         (node->storage_class && 
          node->storage_class->type() == AST_T::CExtern_t))) {
       raise_runtime_error("Variable " + em(node->name) +
                           " was already declared in this scope");
    }
    if(node->storage_class && 
       node->storage_class->type() == AST_T::CExtern_t) {
        resolve_file_scope_variable_declaration(node);
        return;
    }

    scoped_identifier_maps.back()[node->name] = resolve_variable_identifier(node->name);
    node->name = scoped_identifier_maps.back()[node->name];
    checktype_block_scope_variable_declaration(node);

    if(node->init &&
       !node->storage_class) {
        resolve_expression(node->init.get());
    }
    checktype_init_block_scope_variable_declaration(node);
}

/**
cdef void init_resolve_labels():
    goto_map.clear()
    label_set.clear()
*/
static void clear_resolve_labels() {
    goto_map.clear();
    label_set.clear();
}

/**
cdef void resolve_fun_decl_declaration(CFunDecl node):
    if is_file_scope():
        init_resolve_labels()
        init_annotate_loops()
    resolve_function_declaration(node.function_decl)
    if is_file_scope():
        resolve_label()
*/
static void resolve_fun_decl_declaration(CFunDecl* node) {
    if(is_file_scope()) {
        clear_resolve_labels();
        clear_annotate_loops();
    }
    resolve_function_declaration(node->function_decl.get());
    if(is_file_scope()) {
        resolve_label();
    }
}

/**
cdef void resolve_var_decl_declaration(CVarDecl node):
    if is_file_scope():
        resolve_file_scope_variable_declaration(node.variable_decl)
    else:
        resolve_block_scope_variable_declaration(node.variable_decl)
*/
static void resolve_var_decl_declaration(CVarDecl* node) {
    if(is_file_scope()) {
        resolve_file_scope_variable_declaration(node->variable_decl.get());
    }
    else {
        resolve_block_scope_variable_declaration(node->variable_decl.get());
    }
}

/**
cdef void resolve_declaration(CDeclaration node):
    if isinstance(node, CFunDecl):
        resolve_fun_decl_declaration(node)
    elif isinstance(node, CVarDecl):
        resolve_var_decl_declaration(node)
    else:

        raise RuntimeError(
            "An error occurred in variable resolution, not all nodes were visited")
*/
static void resolve_declaration(CDeclaration* node) {
    switch(node->type()) {
        case AST_T::CFunDecl_t:
            resolve_fun_decl_declaration(static_cast<CFunDecl*>(node));
            break;
        case AST_T::CVarDecl_t:
            resolve_var_decl_declaration(static_cast<CVarDecl*>(node));
            break;
        default:
            RAISE_INTERNAL_ERROR;
    }
}

/**
cdef void init_resolve_identifiers():
    external_linkage_scope_map.clear()
    scoped_identifier_maps.clear()
    enter_scope()
*/

/**
cdef void resolve_identifiers(CProgram node):
    init_resolve_identifiers()
    init_check_types()

    cdef Py_ssize_t declaration
    for declaration in range(len(node.declarations)):
        resolve_declaration(node.declarations[declaration])
        resolve_label()
*/
static void resolve_identifiers(CProgram* node) {
    enter_scope();

    for(size_t declaration = 0; declaration < node->declarations.size(); declaration++) {
        resolve_declaration(node->declarations[declaration].get());
        resolve_label();
    }
}

/**
cdef void analyze_semantic(CProgram c_ast):

    resolve_identifiers(c_ast)
*/
void analyze_semantic(CProgram* node) {
    resolve_identifiers(node);
}
