#ifndef _AST_FRONT_SYMT_HPP
#define _AST_FRONT_SYMT_HPP

#include "ast/ast.hpp"

#include <memory>
#include <vector>
#include <unordered_map>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// type = Int
//      | Long
//      | Double
//      | UInt
//      | ULong
//      | FunType(type*, type)
//      | Pointer(type)
//      | Array(int, type)
struct Type : Ast {
    AST_T type() override;
};

struct Int : Type {
    AST_T type() override;
};

struct Long : Type {
    AST_T type() override;
};

struct Double : Type {
    AST_T type() override;
};

struct UInt : Type {
    AST_T type() override;
};

struct ULong : Type {
    AST_T type() override;
};

struct FunType : Type {
    AST_T type() override;
    FunType() = default;
    FunType(std::vector<std::shared_ptr<Type>> param_types, std::shared_ptr<Type> ret_type);

    std::vector<std::shared_ptr<Type>> param_types;
    std::shared_ptr<Type> ret_type;
};

struct Pointer : Type {
    AST_T type() override;
    Pointer() = default;
    Pointer(std::shared_ptr<Type> ref_type);

    std::shared_ptr<Type> ref_type;
};

struct Array : Type {
    AST_T type() override;
    Array() = default;
    Array(TULong size, std::shared_ptr<Type> elem_type);

    TULong size;
    std::shared_ptr<Type> elem_type;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// static_init = IntInit(int)
//             | LongInit(int)
//             | DoubleInit(double, int)
//             | UIntInit(int)
//             | ULongInit(int)
//             | ZeroInit(int)
struct StaticInit : Ast {
    AST_T type() override;
};

struct IntInit : StaticInit {
    AST_T type() override;
    IntInit() = default;
    IntInit(TInt value);

    TInt value;
};

struct LongInit : StaticInit {
    AST_T type() override;
    LongInit() = default;
    LongInit(TLong value);

    TLong value;
};

struct DoubleInit : StaticInit {
    AST_T type() override;
    DoubleInit() = default;
    DoubleInit(TDouble value, TULong binary);

    TDouble value;
    TULong binary;
};

struct UIntInit : StaticInit {
    AST_T type() override;
    UIntInit() = default;
    UIntInit(TUInt value);

    TUInt value;
};

struct ULongInit : StaticInit {
    AST_T type() override;
    ULongInit() = default;
    ULongInit(TULong value);

    TULong value;
};

struct ZeroInit : StaticInit {
    AST_T type() override;
    ZeroInit() = default;
    ZeroInit(TULong byte);

    TULong byte;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// initial_value = Tentative
//               | Initial(static_init*)
//               | NoInitializer
struct InitialValue : Ast {
    AST_T type() override;
};

struct Tentative : InitialValue {
    AST_T type() override;
};

struct Initial : InitialValue {
    AST_T type() override;
    Initial() = default;
    Initial(std::vector<std::shared_ptr<StaticInit>> static_inits);

    std::vector<std::shared_ptr<StaticInit>> static_inits;
};

struct NoInitializer : InitialValue {
    AST_T type() override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// identifier_attrs = FunAttr(bool, bool)
//                  | StaticAttr(initial_value, bool)
//                  | LocalAttr
struct IdentifierAttr : Ast {
    AST_T type() override;
};

struct FunAttr : IdentifierAttr {
    AST_T type() override;
    FunAttr() = default;
    FunAttr(bool is_defined, bool is_global);

    bool is_defined;
    bool is_global;
};

struct StaticAttr : IdentifierAttr {
    AST_T type() override;
    StaticAttr() = default;
    StaticAttr(bool is_global, std::shared_ptr<InitialValue> init);

    bool is_global;
    std::shared_ptr<InitialValue> init;
};

struct LocalAttr : IdentifierAttr {
    AST_T type() override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// symbol = Symbol(type, identifier_attrs)
struct Symbol : Ast {
    AST_T type() override;
    Symbol() = default;
    Symbol(std::shared_ptr<Type> type_t, std::unique_ptr<IdentifierAttr> attrs);

    std::shared_ptr<Type> type_t;
    std::unique_ptr<IdentifierAttr> attrs;
};

/*
struct Dummy : Ast {
};
*/

extern std::unordered_map<TIdentifier, std::unique_ptr<Symbol>> symbol_table;

#endif
