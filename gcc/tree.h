/* Definitions for the ubiquitous 'tree' type for GNU compilers.
   Copyright (C) 1989-2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_TREE_H
#define GCC_TREE_H

#include "tree-core.h"
#include "options.h"
#include "vec.h"

/* Convert a target-independent built-in function code to a combined_fn.  */

inline combined_fn
as_combined_fn (built_in_function fn)
{
  return combined_fn (int (fn));
}

/* Convert an internal function code to a combined_fn.  */

inline combined_fn
as_combined_fn (internal_fn fn)
{
  return combined_fn (int (fn) + int (END_BUILTINS));
}

/* Return true if CODE is a target-independent built-in function.  */

inline bool
builtin_fn_p (combined_fn code)
{
  return int (code) < int (END_BUILTINS);
}

/* Return the target-independent built-in function represented by CODE.
   Only valid if builtin_fn_p (CODE).  */

inline built_in_function
as_builtin_fn (combined_fn code)
{
  gcc_checking_assert (builtin_fn_p (code));
  return built_in_function (int (code));
}

/* Return true if CODE is an internal function.  */

inline bool
internal_fn_p (combined_fn code)
{
  return int (code) >= int (END_BUILTINS);
}

/* Return the internal function represented by CODE.  Only valid if
   internal_fn_p (CODE).  */

inline internal_fn
as_internal_fn (combined_fn code)
{
  gcc_checking_assert (internal_fn_p (code));
  return internal_fn (int (code) - int (END_BUILTINS));
}

/* Helper to transparently allow tree codes and builtin function codes
   exist in one storage entity.  */
class code_helper
{
public:
  code_helper () {}
  code_helper (tree_code code) : rep ((int) code) {}
  code_helper (combined_fn fn) : rep (-(int) fn) {}
  code_helper (internal_fn fn) : rep (-(int) as_combined_fn (fn)) {}
  explicit operator tree_code () const { return (tree_code) rep; }
  explicit operator combined_fn () const { return (combined_fn) -rep; }
  explicit operator internal_fn () const;
  explicit operator built_in_function () const;
  bool is_tree_code () const { return rep > 0; }
  bool is_fn_code () const { return rep < 0; }
  bool is_internal_fn () const;
  bool is_builtin_fn () const;
  int get_rep () const { return rep; }
  tree_code safe_as_tree_code () const;
  combined_fn safe_as_fn_code () const;
  bool operator== (const code_helper &other) { return rep == other.rep; }
  bool operator!= (const code_helper &other) { return rep != other.rep; }
  bool operator== (tree_code c) { return rep == code_helper (c).rep; }
  bool operator!= (tree_code c) { return rep != code_helper (c).rep; }

private:
  int rep;
};

/* Helper function that returns the tree_code representation of THIS
   code_helper if it is a tree_code and MAX_TREE_CODES otherwise.  This is
   useful when passing a code_helper to a tree_code only check.  */

inline tree_code
code_helper::safe_as_tree_code () const
{
  return is_tree_code () ? (tree_code) *this : MAX_TREE_CODES;
}

/* Helper function that returns the combined_fn representation of THIS
   code_helper if it is a fn_code and CFN_LAST otherwise.  This is useful when
   passing a code_helper to a combined_fn only check.  */

inline combined_fn
code_helper::safe_as_fn_code () const {
  return is_fn_code () ? (combined_fn) *this : CFN_LAST;
}

inline code_helper::operator internal_fn () const
{
  return as_internal_fn (combined_fn (*this));
}

inline code_helper::operator built_in_function () const
{
  return as_builtin_fn (combined_fn (*this));
}

inline bool
code_helper::is_internal_fn () const
{
  return is_fn_code () && internal_fn_p (combined_fn (*this));
}

inline bool
code_helper::is_builtin_fn () const
{
  return is_fn_code () && builtin_fn_p (combined_fn (*this));
}

/* Macros for initializing `tree_contains_struct'.  */
#define MARK_TS_BASE(C)					\
  (tree_contains_struct[C][TS_BASE] = true)

#define MARK_TS_TYPED(C)				\
  (MARK_TS_BASE (C),					\
   tree_contains_struct[C][TS_TYPED] = true)

#define MARK_TS_COMMON(C)				\
  (MARK_TS_TYPED (C),					\
   tree_contains_struct[C][TS_COMMON] = true)

#define MARK_TS_TYPE_COMMON(C)				\
  (MARK_TS_COMMON (C),					\
   tree_contains_struct[C][TS_TYPE_COMMON] = true)

#define MARK_TS_TYPE_WITH_LANG_SPECIFIC(C)		\
  (MARK_TS_TYPE_COMMON (C),				\
   tree_contains_struct[C][TS_TYPE_WITH_LANG_SPECIFIC] = true)

#define MARK_TS_TYPE_NON_COMMON(C)			\
  (MARK_TS_TYPE_WITH_LANG_SPECIFIC (C),			\
   tree_contains_struct[C][TS_TYPE_NON_COMMON] = true)	\

#define MARK_TS_DECL_MINIMAL(C)				\
  (MARK_TS_COMMON (C),					\
   tree_contains_struct[C][TS_DECL_MINIMAL] = true)

#define MARK_TS_DECL_COMMON(C)				\
  (MARK_TS_DECL_MINIMAL (C),				\
   tree_contains_struct[C][TS_DECL_COMMON] = true)

#define MARK_TS_DECL_WRTL(C)				\
  (MARK_TS_DECL_COMMON (C),				\
   tree_contains_struct[C][TS_DECL_WRTL] = true)

#define MARK_TS_DECL_WITH_VIS(C)			\
  (MARK_TS_DECL_WRTL (C),				\
   tree_contains_struct[C][TS_DECL_WITH_VIS] = true)

#define MARK_TS_DECL_NON_COMMON(C)			\
  (MARK_TS_DECL_WITH_VIS (C),				\
   tree_contains_struct[C][TS_DECL_NON_COMMON] = true)

#define MARK_TS_EXP(C)					\
  (MARK_TS_TYPED (C),					\
   tree_contains_struct[C][TS_EXP] = true)

/* Returns the string representing CLASS.  */

#define TREE_CODE_CLASS_STRING(CLASS)\
        tree_code_class_strings[(int) (CLASS)]

#if __cpp_inline_variables < 201606L
#define TREE_CODE_CLASS(CODE)	\
  tree_code_type_tmpl <0>::tree_code_type[(int) (CODE)]
#else
#define TREE_CODE_CLASS(CODE)	tree_code_type[(int) (CODE)]
#endif

/* Nonzero if NODE represents an exceptional code.  */

#define EXCEPTIONAL_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_exceptional)

/* Nonzero if NODE represents a constant.  */

#define CONSTANT_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_constant)

/* Nonzero if NODE represents a constant, or is a location wrapper
   around such a node.  */

#define CONSTANT_CLASS_OR_WRAPPER_P(NODE)\
	(CONSTANT_CLASS_P (tree_strip_any_location_wrapper (NODE)))

/* Nonzero if NODE represents a type.  */

#define TYPE_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_type)

/* Nonzero if NODE represents a declaration.  */

#define DECL_P(NODE)\
        (TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_declaration)

/* True if NODE designates a variable declaration.  */
#define VAR_P(NODE) \
  (TREE_CODE (NODE) == VAR_DECL)

/* Nonzero if DECL represents a VAR_DECL or FUNCTION_DECL.  */

#define VAR_OR_FUNCTION_DECL_P(DECL)\
  (TREE_CODE (DECL) == VAR_DECL || TREE_CODE (DECL) == FUNCTION_DECL)

/* Nonzero if NODE represents a INDIRECT_REF.  Keep these checks in
   ascending code order.  */

#define INDIRECT_REF_P(NODE)\
  (TREE_CODE (NODE) == INDIRECT_REF)

/* Nonzero if NODE represents a reference.  */

#define REFERENCE_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_reference)

/* Nonzero if NODE represents a comparison.  */

#define COMPARISON_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_comparison)

/* Nonzero if NODE represents a unary arithmetic expression.  */

#define UNARY_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_unary)

/* Nonzero if NODE represents a binary arithmetic expression.  */

#define BINARY_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_binary)

/* Nonzero if NODE represents a statement expression.  */

#define STATEMENT_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_statement)

/* Nonzero if NODE represents a function call-like expression with a
   variable-length operand vector.  */

#define VL_EXP_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_vl_exp)

/* Nonzero if NODE represents any other expression.  */

#define EXPRESSION_CLASS_P(NODE)\
	(TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_expression)

/* Returns nonzero iff NODE represents a type or declaration.  */

#define IS_TYPE_OR_DECL_P(NODE)\
	(TYPE_P (NODE) || DECL_P (NODE))

/* Returns nonzero iff CLASS is the tree-code class of an
   expression.  */

#define IS_EXPR_CODE_CLASS(CLASS)\
	((CLASS) >= tcc_reference && (CLASS) <= tcc_expression)

/* Returns nonzero iff NODE is an expression of some kind.  */

#define EXPR_P(NODE) IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (TREE_CODE (NODE)))

#if __cpp_inline_variables < 201606L
#define TREE_CODE_LENGTH(CODE)	\
  tree_code_length_tmpl <0>::tree_code_length[(int) (CODE)]
#else
#define TREE_CODE_LENGTH(CODE)	tree_code_length[(int) (CODE)]
#endif


/* Helper macros for math builtins.  */

#define CASE_FLT_FN(FN) case FN: case FN##F: case FN##L
#define CASE_FLT_FN_FLOATN_NX(FN)			   \
  case FN##F16: case FN##F32: case FN##F64: case FN##F128: \
  case FN##F32X: case FN##F64X: case FN##F128X
#define CASE_FLT_FN_REENT(FN) case FN##_R: case FN##F_R: case FN##L_R
#define CASE_INT_FN(FN) case FN: case FN##L: case FN##LL: case FN##IMAX

#define NULL_TREE (tree) NULL

/* Define accessors for the fields that all tree nodes have
   (though some fields are not used for all kinds of nodes).  */

/* The tree-code says what kind of node it is.
   Codes are defined in tree.def.  */
#define TREE_CODE(NODE) ((enum tree_code) (NODE)->base.code)
#define TREE_SET_CODE(NODE, VALUE) ((NODE)->base.code = (VALUE))

/* When checking is enabled, errors will be generated if a tree node
   is accessed incorrectly. The macros die with a fatal error.  */
#if defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 2007)

#define TREE_CHECK(T, CODE) \
(tree_check ((T), __FILE__, __LINE__, __FUNCTION__, (CODE)))

#define TREE_NOT_CHECK(T, CODE) \
(tree_not_check ((T), __FILE__, __LINE__, __FUNCTION__, (CODE)))

#define TREE_CHECK2(T, CODE1, CODE2) \
(tree_check2 ((T), __FILE__, __LINE__, __FUNCTION__, (CODE1), (CODE2)))

#define TREE_NOT_CHECK2(T, CODE1, CODE2) \
(tree_not_check2 ((T), __FILE__, __LINE__, __FUNCTION__, (CODE1), (CODE2)))

#define TREE_CHECK3(T, CODE1, CODE2, CODE3) \
(tree_check3 ((T), __FILE__, __LINE__, __FUNCTION__, (CODE1), (CODE2), (CODE3)))

#define TREE_NOT_CHECK3(T, CODE1, CODE2, CODE3) \
(tree_not_check3 ((T), __FILE__, __LINE__, __FUNCTION__, \
                               (CODE1), (CODE2), (CODE3)))

#define TREE_CHECK4(T, CODE1, CODE2, CODE3, CODE4) \
(tree_check4 ((T), __FILE__, __LINE__, __FUNCTION__, \
                           (CODE1), (CODE2), (CODE3), (CODE4)))

#define TREE_NOT_CHECK4(T, CODE1, CODE2, CODE3, CODE4) \
(tree_not_check4 ((T), __FILE__, __LINE__, __FUNCTION__, \
                               (CODE1), (CODE2), (CODE3), (CODE4)))

#define TREE_CHECK5(T, CODE1, CODE2, CODE3, CODE4, CODE5) \
(tree_check5 ((T), __FILE__, __LINE__, __FUNCTION__, \
                           (CODE1), (CODE2), (CODE3), (CODE4), (CODE5)))

#define TREE_NOT_CHECK5(T, CODE1, CODE2, CODE3, CODE4, CODE5) \
(tree_not_check5 ((T), __FILE__, __LINE__, __FUNCTION__, \
                               (CODE1), (CODE2), (CODE3), (CODE4), (CODE5)))

#define TREE_CHECK6(T, CODE1, CODE2, CODE3, CODE4, CODE5, CODE6) \
(tree_check6 ((T), __FILE__, __LINE__, __FUNCTION__, \
			(CODE1), (CODE2), (CODE3), (CODE4), (CODE5), (CODE6)))

#define TREE_NOT_CHECK6(T, CODE1, CODE2, CODE3, CODE4, CODE5, CODE6) \
(tree_not_check6 ((T), __FILE__, __LINE__, __FUNCTION__, \
			(CODE1), (CODE2), (CODE3), (CODE4), (CODE5), (CODE6)))

#define CONTAINS_STRUCT_CHECK(T, STRUCT) \
(contains_struct_check ((T), (STRUCT), __FILE__, __LINE__, __FUNCTION__))

#define TREE_CLASS_CHECK(T, CLASS) \
(tree_class_check ((T), (CLASS), __FILE__, __LINE__, __FUNCTION__))

#define TREE_RANGE_CHECK(T, CODE1, CODE2) \
(tree_range_check ((T), (CODE1), (CODE2), __FILE__, __LINE__, __FUNCTION__))

#define OMP_CLAUSE_SUBCODE_CHECK(T, CODE) \
(omp_clause_subcode_check ((T), (CODE), __FILE__, __LINE__, __FUNCTION__))

#define OMP_CLAUSE_RANGE_CHECK(T, CODE1, CODE2) \
(omp_clause_range_check ((T), (CODE1), (CODE2), \
                                      __FILE__, __LINE__, __FUNCTION__))

/* These checks have to be special cased.  */
#define EXPR_CHECK(T) \
(expr_check ((T), __FILE__, __LINE__, __FUNCTION__))

/* These checks have to be special cased.  */
#define NON_TYPE_CHECK(T) \
(non_type_check ((T), __FILE__, __LINE__, __FUNCTION__))

/* These checks have to be special cased.  */
#define ANY_INTEGRAL_TYPE_CHECK(T) \
(any_integral_type_check ((T), __FILE__, __LINE__, __FUNCTION__))

#define TREE_INT_CST_ELT_CHECK(T, I) \
(*tree_int_cst_elt_check ((T), (I), __FILE__, __LINE__, __FUNCTION__))

#define TREE_VEC_ELT_CHECK(T, I) \
(*(CONST_CAST2 (tree *, typeof (T)*, \
     tree_vec_elt_check ((T), (I), __FILE__, __LINE__, __FUNCTION__))))

#define OMP_CLAUSE_ELT_CHECK(T, I) \
(*(omp_clause_elt_check ((T), (I), __FILE__, __LINE__, __FUNCTION__)))

/* Special checks for TREE_OPERANDs.  */
#define TREE_OPERAND_CHECK(T, I) \
(*(CONST_CAST2 (tree*, typeof (T)*, \
     tree_operand_check ((T), (I), __FILE__, __LINE__, __FUNCTION__))))

#define TREE_OPERAND_CHECK_CODE(T, CODE, I) \
(*(tree_operand_check_code ((T), (CODE), (I), \
                                         __FILE__, __LINE__, __FUNCTION__)))

/* Nodes are chained together for many purposes.
   Types are chained together to record them for being output to the debugger
   (see the function `chain_type').
   Decls in the same scope are chained together to record the contents
   of the scope.
   Statement nodes for successive statements used to be chained together.
   Often lists of things are represented by TREE_LIST nodes that
   are chained together.  */

#define TREE_CHAIN(NODE) \
(CONTAINS_STRUCT_CHECK (NODE, TS_COMMON)->common.chain)

/* In all nodes that are expressions, this is the data type of the expression.
   In POINTER_TYPE nodes, this is the type that the pointer points to.
   In ARRAY_TYPE nodes, this is the type of the elements.
   In VECTOR_TYPE nodes, this is the type of the elements.  */
#define TREE_TYPE(NODE) \
(CONTAINS_STRUCT_CHECK (NODE, TS_TYPED)->typed.type)

extern void tree_contains_struct_check_failed (const_tree,
					       const enum tree_node_structure_enum,
					       const char *, int, const char *)
  ATTRIBUTE_NORETURN ATTRIBUTE_COLD;

extern void tree_check_failed (const_tree, const char *, int, const char *,
			       ...) ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_not_check_failed (const_tree, const char *, int, const char *,
				   ...) ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_class_check_failed (const_tree, const enum tree_code_class,
				     const char *, int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_range_check_failed (const_tree, const char *, int,
				     const char *, enum tree_code,
				     enum tree_code)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_not_class_check_failed (const_tree,
					 const enum tree_code_class,
					 const char *, int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_int_cst_elt_check_failed (int, int, const char *,
					   int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_vec_elt_check_failed (int, int, const char *,
				       int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void phi_node_elt_check_failed (int, int, const char *,
				       int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void tree_operand_check_failed (int, const_tree,
				       const char *, int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void omp_clause_check_failed (const_tree, const char *, int,
				     const char *, enum omp_clause_code)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void omp_clause_operand_check_failed (int, const_tree, const char *,
				             int, const char *)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;
extern void omp_clause_range_check_failed (const_tree, const char *, int,
			       const char *, enum omp_clause_code,
			       enum omp_clause_code)
    ATTRIBUTE_NORETURN ATTRIBUTE_COLD;

#else /* not ENABLE_TREE_CHECKING, or not gcc */

#define CONTAINS_STRUCT_CHECK(T, ENUM)          (T)
#define TREE_CHECK(T, CODE)			(T)
#define TREE_NOT_CHECK(T, CODE)			(T)
#define TREE_CHECK2(T, CODE1, CODE2)		(T)
#define TREE_NOT_CHECK2(T, CODE1, CODE2)	(T)
#define TREE_CHECK3(T, CODE1, CODE2, CODE3)	(T)
#define TREE_NOT_CHECK3(T, CODE1, CODE2, CODE3)	(T)
#define TREE_CHECK4(T, CODE1, CODE2, CODE3, CODE4) (T)
#define TREE_NOT_CHECK4(T, CODE1, CODE2, CODE3, CODE4) (T)
#define TREE_CHECK5(T, CODE1, CODE2, CODE3, CODE4, CODE5) (T)
#define TREE_NOT_CHECK5(T, CODE1, CODE2, CODE3, CODE4, CODE5) (T)
#define TREE_CHECK6(T, CODE1, CODE2, CODE3, CODE4, CODE5, CODE6) (T)
#define TREE_NOT_CHECK6(T, CODE1, CODE2, CODE3, CODE4, CODE5, CODE6) (T)
#define TREE_CLASS_CHECK(T, CODE)		(T)
#define TREE_RANGE_CHECK(T, CODE1, CODE2)	(T)
#define EXPR_CHECK(T)				(T)
#define NON_TYPE_CHECK(T)			(T)
#define TREE_INT_CST_ELT_CHECK(T, I)		((T)->int_cst.val[I])
#define TREE_VEC_ELT_CHECK(T, I)		((T)->vec.a[I])
#define TREE_OPERAND_CHECK(T, I)		((T)->exp.operands[I])
#define TREE_OPERAND_CHECK_CODE(T, CODE, I)	((T)->exp.operands[I])
#define OMP_CLAUSE_ELT_CHECK(T, i)	        ((T)->omp_clause.ops[i])
#define OMP_CLAUSE_RANGE_CHECK(T, CODE1, CODE2)	(T)
#define OMP_CLAUSE_SUBCODE_CHECK(T, CODE)	(T)
#define ANY_INTEGRAL_TYPE_CHECK(T)		(T)

#define TREE_CHAIN(NODE) ((NODE)->common.chain)
#define TREE_TYPE(NODE) ((NODE)->typed.type)

#endif

#define TREE_BLOCK(NODE)		(tree_block (NODE))
#define TREE_SET_BLOCK(T, B)		(tree_set_block ((T), (B)))

#include "tree-check.h"

#define TYPE_CHECK(T)		TREE_CLASS_CHECK (T, tcc_type)
#define DECL_MINIMAL_CHECK(T)   CONTAINS_STRUCT_CHECK (T, TS_DECL_MINIMAL)
#define DECL_COMMON_CHECK(T)    CONTAINS_STRUCT_CHECK (T, TS_DECL_COMMON)
#define DECL_WRTL_CHECK(T)      CONTAINS_STRUCT_CHECK (T, TS_DECL_WRTL)
#define DECL_WITH_VIS_CHECK(T)  CONTAINS_STRUCT_CHECK (T, TS_DECL_WITH_VIS)
#define DECL_NON_COMMON_CHECK(T) CONTAINS_STRUCT_CHECK (T, TS_DECL_NON_COMMON)
#define CST_CHECK(T)		TREE_CLASS_CHECK (T, tcc_constant)
#define STMT_CHECK(T)		TREE_CLASS_CHECK (T, tcc_statement)
#define VL_EXP_CHECK(T)		TREE_CLASS_CHECK (T, tcc_vl_exp)
#define FUNC_OR_METHOD_CHECK(T)	TREE_CHECK2 (T, FUNCTION_TYPE, METHOD_TYPE)
#define PTR_OR_REF_CHECK(T)	TREE_CHECK2 (T, POINTER_TYPE, REFERENCE_TYPE)

#define RECORD_OR_UNION_CHECK(T)	\
  TREE_CHECK3 (T, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE)
#define NOT_RECORD_OR_UNION_CHECK(T) \
  TREE_NOT_CHECK3 (T, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE)
#define ARRAY_OR_INTEGER_TYPE_CHECK(T)	\
  TREE_CHECK2 (T, ARRAY_TYPE, INTEGER_TYPE)

#define NUMERICAL_TYPE_CHECK(T)					\
  TREE_CHECK6 (T, INTEGER_TYPE, ENUMERAL_TYPE, BOOLEAN_TYPE, REAL_TYPE,	\
	       FIXED_POINT_TYPE, BITINT_TYPE)

/* Here is how primitive or already-canonicalized types' hash codes
   are made.  */
#define TYPE_HASH(TYPE) (TYPE_UID (TYPE))

/* A simple hash function for an arbitrary tree node.  This must not be
   used in hash tables which are saved to a PCH.  */
#define TREE_HASH(NODE) ((size_t) (NODE) & 0777777)

/* Tests if CODE is a conversion expr (NOP_EXPR or CONVERT_EXPR).  */
#define CONVERT_EXPR_CODE_P(CODE)				\
  ((CODE) == NOP_EXPR || (CODE) == CONVERT_EXPR)

/* Similarly, but accept an expression instead of a tree code.  */
#define CONVERT_EXPR_P(EXP)	CONVERT_EXPR_CODE_P (TREE_CODE (EXP))

/* Generate case for NOP_EXPR, CONVERT_EXPR.  */

#define CASE_CONVERT						\
  case NOP_EXPR:						\
  case CONVERT_EXPR

/* Given an expression as a tree, strip any conversion that generates
   no instruction.  Accepts both tree and const_tree arguments since
   we are not modifying the tree itself.  */

#define STRIP_NOPS(EXP) \
  (EXP) = tree_strip_nop_conversions (CONST_CAST_TREE (EXP))

/* Like STRIP_NOPS, but don't let the signedness change either.  */

#define STRIP_SIGN_NOPS(EXP) \
  (EXP) = tree_strip_sign_nop_conversions (CONST_CAST_TREE (EXP))

/* Like STRIP_NOPS, but don't alter the TREE_TYPE either.  */

#define STRIP_TYPE_NOPS(EXP) \
  while ((CONVERT_EXPR_P (EXP)					\
	  || TREE_CODE (EXP) == NON_LVALUE_EXPR)		\
	 && TREE_OPERAND (EXP, 0) != error_mark_node		\
	 && (TREE_TYPE (EXP)					\
	     == TREE_TYPE (TREE_OPERAND (EXP, 0))))		\
    (EXP) = TREE_OPERAND (EXP, 0)

/* Remove unnecessary type conversions according to
   tree_ssa_useless_type_conversion.  */

#define STRIP_USELESS_TYPE_CONVERSION(EXP) \
  (EXP) = tree_ssa_strip_useless_type_conversions (EXP)

/* Remove any VIEW_CONVERT_EXPR or NON_LVALUE_EXPR that's purely
   in use to provide a location_t.  */

#define STRIP_ANY_LOCATION_WRAPPER(EXP) \
  (EXP) = tree_strip_any_location_wrapper (CONST_CAST_TREE (EXP))

/* Nonzero if TYPE represents a vector type.  */

#define VECTOR_TYPE_P(TYPE) (TREE_CODE (TYPE) == VECTOR_TYPE)

/* Nonzero if TYPE represents a vector of booleans.  */

#define VECTOR_BOOLEAN_TYPE_P(TYPE)				\
  (TREE_CODE (TYPE) == VECTOR_TYPE			\
   && TREE_CODE (TREE_TYPE (TYPE)) == BOOLEAN_TYPE)

/* Nonzero if TYPE represents an integral type.  Note that we do not
   include COMPLEX types here.  Keep these checks in ascending code
   order.  */

#define INTEGRAL_TYPE_P(TYPE)  \
  (TREE_CODE (TYPE) == ENUMERAL_TYPE  \
   || TREE_CODE (TYPE) == BOOLEAN_TYPE \
   || TREE_CODE (TYPE) == INTEGER_TYPE \
   || TREE_CODE (TYPE) == BITINT_TYPE)

/* Nonzero if TYPE represents an integral type, including complex
   and vector integer types.  */

#define ANY_INTEGRAL_TYPE_P(TYPE)		\
  (INTEGRAL_TYPE_P (TYPE)			\
   || ((TREE_CODE (TYPE) == COMPLEX_TYPE 	\
        || VECTOR_TYPE_P (TYPE))		\
       && INTEGRAL_TYPE_P (TREE_TYPE (TYPE))))

/* Nonzero if TYPE is bit-precise integer type.  */

#define BITINT_TYPE_P(TYPE) (TREE_CODE (TYPE) == BITINT_TYPE)

/* Nonzero if TYPE represents a non-saturating fixed-point type.  */

#define NON_SAT_FIXED_POINT_TYPE_P(TYPE) \
  (TREE_CODE (TYPE) == FIXED_POINT_TYPE && !TYPE_SATURATING (TYPE))

/* Nonzero if TYPE represents a saturating fixed-point type.  */

#define SAT_FIXED_POINT_TYPE_P(TYPE) \
  (TREE_CODE (TYPE) == FIXED_POINT_TYPE && TYPE_SATURATING (TYPE))

/* Nonzero if TYPE represents a fixed-point type.  */

#define FIXED_POINT_TYPE_P(TYPE)	(TREE_CODE (TYPE) == FIXED_POINT_TYPE)

/* Nonzero if TYPE represents a scalar floating-point type.  */

#define SCALAR_FLOAT_TYPE_P(TYPE) (TREE_CODE (TYPE) == REAL_TYPE)

/* Nonzero if TYPE represents a complex floating-point type.  */

#define COMPLEX_FLOAT_TYPE_P(TYPE)	\
  (TREE_CODE (TYPE) == COMPLEX_TYPE	\
   && TREE_CODE (TREE_TYPE (TYPE)) == REAL_TYPE)

/* Nonzero if TYPE represents a vector integer type.  */

#define VECTOR_INTEGER_TYPE_P(TYPE)			\
  (VECTOR_TYPE_P (TYPE)					\
   && TREE_CODE (TREE_TYPE (TYPE)) == INTEGER_TYPE)


/* Nonzero if TYPE represents a vector floating-point type.  */

#define VECTOR_FLOAT_TYPE_P(TYPE)	\
  (VECTOR_TYPE_P (TYPE)			\
   && TREE_CODE (TREE_TYPE (TYPE)) == REAL_TYPE)

/* Nonzero if TYPE represents a floating-point type, including complex
   and vector floating-point types.  The vector and complex check does
   not use the previous two macros to enable early folding.  */

#define FLOAT_TYPE_P(TYPE)			\
  (SCALAR_FLOAT_TYPE_P (TYPE)			\
   || ((TREE_CODE (TYPE) == COMPLEX_TYPE 	\
        || VECTOR_TYPE_P (TYPE))		\
       && SCALAR_FLOAT_TYPE_P (TREE_TYPE (TYPE))))

/* Nonzero if TYPE represents a decimal floating-point type.  */
#define DECIMAL_FLOAT_TYPE_P(TYPE)		\
  (SCALAR_FLOAT_TYPE_P (TYPE)			\
   && DECIMAL_FLOAT_MODE_P (TYPE_MODE (TYPE)))

/* Nonzero if TYPE is a record or union type.  */
#define RECORD_OR_UNION_TYPE_P(TYPE)		\
  (TREE_CODE (TYPE) == RECORD_TYPE		\
   || TREE_CODE (TYPE) == UNION_TYPE		\
   || TREE_CODE (TYPE) == QUAL_UNION_TYPE)

/* Nonzero if TYPE represents an aggregate (multi-component) type.
   Keep these checks in ascending code order.  */

#define AGGREGATE_TYPE_P(TYPE) \
  (TREE_CODE (TYPE) == ARRAY_TYPE || RECORD_OR_UNION_TYPE_P (TYPE))

/* Nonzero if TYPE represents a pointer or reference type.
   (It should be renamed to INDIRECT_TYPE_P.)  Keep these checks in
   ascending code order.  */

#define POINTER_TYPE_P(TYPE) \
  (TREE_CODE (TYPE) == POINTER_TYPE || TREE_CODE (TYPE) == REFERENCE_TYPE)

/* Nonzero if TYPE represents a pointer to function.  */
#define FUNCTION_POINTER_TYPE_P(TYPE) \
  (POINTER_TYPE_P (TYPE) && TREE_CODE (TREE_TYPE (TYPE)) == FUNCTION_TYPE)

/* Nonzero if this type is a complete type.  */
#define COMPLETE_TYPE_P(NODE) (TYPE_SIZE (NODE) != NULL_TREE)

/* Nonzero if this type is the (possibly qualified) void type.  */
#define VOID_TYPE_P(NODE) (TREE_CODE (NODE) == VOID_TYPE)

/* Nonzero if this type is complete or is cv void.  */
#define COMPLETE_OR_VOID_TYPE_P(NODE) \
  (COMPLETE_TYPE_P (NODE) || VOID_TYPE_P (NODE))

/* Nonzero if this type is complete or is an array with unspecified bound.  */
#define COMPLETE_OR_UNBOUND_ARRAY_TYPE_P(NODE) \
  (COMPLETE_TYPE_P (TREE_CODE (NODE) == ARRAY_TYPE ? TREE_TYPE (NODE) : (NODE)))

#define FUNC_OR_METHOD_TYPE_P(NODE) \
  (TREE_CODE (NODE) == FUNCTION_TYPE || TREE_CODE (NODE) == METHOD_TYPE)

#define OPAQUE_TYPE_P(NODE) \
    (TREE_CODE (NODE) == OPAQUE_TYPE)

/* Define many boolean fields that all tree nodes have.  */

/* In VAR_DECL, PARM_DECL and RESULT_DECL nodes, nonzero means address
   of this is needed.  So it cannot be in a register.
   In a FUNCTION_DECL it has no meaning.
   In LABEL_DECL nodes, it means a goto for this label has been seen
   from a place outside all binding contours that restore stack levels.
   In an artificial SSA_NAME that points to a stack partition with at least
   two variables, it means that at least one variable has TREE_ADDRESSABLE.
   In ..._TYPE nodes, it means that objects of this type must be fully
   addressable.  This means that pieces of this object cannot go into
   register parameters, for example.  If this a function type, this
   means that the value must be returned in memory.
   In CONSTRUCTOR nodes, it means object constructed must be in memory.
   In IDENTIFIER_NODEs, this means that some extern decl for this name
   had its address taken.  That matters for inline functions.
   In a STMT_EXPR, it means we want the result of the enclosed expression.  */
#define TREE_ADDRESSABLE(NODE) ((NODE)->base.addressable_flag)

/* Set on a CALL_EXPR if the call is in a tail position, ie. just before the
   exit of a function.  Calls for which this is true are candidates for tail
   call optimizations.  */
#define CALL_EXPR_TAILCALL(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.addressable_flag)

/* Set on a CALL_EXPR if the call has been marked as requiring tail call
   optimization for correctness.  */
#define CALL_EXPR_MUST_TAIL_CALL(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.static_flag)

/* Used as a temporary field on a CASE_LABEL_EXPR to indicate that the
   CASE_LOW operand has been processed.  */
#define CASE_LOW_SEEN(NODE) \
  (CASE_LABEL_EXPR_CHECK (NODE)->base.addressable_flag)

#define PREDICT_EXPR_OUTCOME(NODE) \
  ((enum prediction) (PREDICT_EXPR_CHECK (NODE)->base.addressable_flag))
#define SET_PREDICT_EXPR_OUTCOME(NODE, OUTCOME) \
  (PREDICT_EXPR_CHECK (NODE)->base.addressable_flag = (int) OUTCOME)
#define PREDICT_EXPR_PREDICTOR(NODE) \
  ((enum br_predictor)tree_to_shwi (TREE_OPERAND (PREDICT_EXPR_CHECK (NODE), 0)))

/* In a VAR_DECL, nonzero means allocate static storage.
   In a FUNCTION_DECL, nonzero if function has been defined.
   In a CONSTRUCTOR, nonzero means allocate static storage.  */
#define TREE_STATIC(NODE) ((NODE)->base.static_flag)

/* In an ADDR_EXPR, nonzero means do not use a trampoline.  */
#define TREE_NO_TRAMPOLINE(NODE) (ADDR_EXPR_CHECK (NODE)->base.static_flag)

/* In a TARGET_EXPR or WITH_CLEANUP_EXPR, means that the pertinent cleanup
   should only be executed if an exception is thrown, not on normal exit
   of its scope.  */
#define CLEANUP_EH_ONLY(NODE) ((NODE)->base.static_flag)

/* In a TRY_CATCH_EXPR, means that the handler should be considered a
   separate cleanup in honor_protect_cleanup_actions.  */
#define TRY_CATCH_IS_CLEANUP(NODE) \
  (TRY_CATCH_EXPR_CHECK (NODE)->base.static_flag)

/* Used as a temporary field on a CASE_LABEL_EXPR to indicate that the
   CASE_HIGH operand has been processed.  */
#define CASE_HIGH_SEEN(NODE) \
  (CASE_LABEL_EXPR_CHECK (NODE)->base.static_flag)

/* Used to mark scoped enums.  */
#define ENUM_IS_SCOPED(NODE) (ENUMERAL_TYPE_CHECK (NODE)->base.static_flag)

/* Determines whether an ENUMERAL_TYPE has defined the list of constants. */
#define ENUM_IS_OPAQUE(NODE) (ENUMERAL_TYPE_CHECK (NODE)->base.private_flag)

/* In an expr node (usually a conversion) this means the node was made
   implicitly and should not lead to any sort of warning.  In a decl node,
   warnings concerning the decl should be suppressed.  This is used at
   least for used-before-set warnings, and it set after one warning is
   emitted.  */
#define TREE_NO_WARNING(NODE) ((NODE)->base.nowarning_flag)

/* Nonzero if we should warn about the change in empty class parameter
   passing ABI in this TU.  */
#define TRANSLATION_UNIT_WARN_EMPTY_P(NODE) \
  (TRANSLATION_UNIT_DECL_CHECK (NODE)->decl_common.decl_flag_0)

/* Nonzero if this type is "empty" according to the particular psABI.  */
#define TYPE_EMPTY_P(NODE) (TYPE_CHECK (NODE)->type_common.empty_flag)

/* Used to indicate that this TYPE represents a compiler-generated entity.  */
#define TYPE_ARTIFICIAL(NODE) (TYPE_CHECK (NODE)->base.nowarning_flag)

/* True if the type is indivisible at the source level, i.e. if its
   component parts cannot be accessed directly.  This is used to suppress
   normal GNU extensions for target-specific vector types.  */
#define TYPE_INDIVISIBLE_P(NODE) (TYPE_CHECK (NODE)->type_common.indivisible_p)

/* True if this is a stdarg function with no named arguments (C23
   (...) prototype, where arguments can be accessed with va_start and
   va_arg), as opposed to an unprototyped function.  */
#define TYPE_NO_NAMED_ARGS_STDARG_P(NODE) \
  (FUNC_OR_METHOD_CHECK (NODE)->type_common.no_named_args_stdarg_p)

/* True if this RECORD_TYPE or UNION_TYPE includes a flexible array member
   as the last field recursively.  */
#define TYPE_INCLUDES_FLEXARRAY(NODE) \
  (RECORD_OR_UNION_CHECK (NODE)->type_common.no_named_args_stdarg_p)

/* In an IDENTIFIER_NODE, this means that assemble_name was called with
   this string as an argument.  */
#define TREE_SYMBOL_REFERENCED(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->base.static_flag)

/* Nonzero in a pointer or reference type means the data pointed to
   by this type can alias anything.  */
#define TYPE_REF_CAN_ALIAS_ALL(NODE) \
  (PTR_OR_REF_CHECK (NODE)->base.static_flag)

/* In an INTEGER_CST, REAL_CST, or COMPLEX_CST, this means
   there was an overflow in folding.  */

#define TREE_OVERFLOW(NODE) (CST_CHECK (NODE)->base.public_flag)

/* TREE_OVERFLOW can only be true for EXPR of CONSTANT_CLASS_P.  */

#define TREE_OVERFLOW_P(EXPR) \
 (CONSTANT_CLASS_P (EXPR) && TREE_OVERFLOW (EXPR))

/* In a VAR_DECL, FUNCTION_DECL, NAMESPACE_DECL or TYPE_DECL,
   nonzero means name is to be accessible from outside this translation unit.
   In an IDENTIFIER_NODE, nonzero means an external declaration
   accessible from outside this translation unit was previously seen
   for this name in an inner scope.  */
#define TREE_PUBLIC(NODE) ((NODE)->base.public_flag)

/* In a _TYPE, indicates whether TYPE_CACHED_VALUES contains a vector
   of cached values, or is something else.  */
#define TYPE_CACHED_VALUES_P(NODE) (TYPE_CHECK (NODE)->base.public_flag)

/* In a SAVE_EXPR, indicates that the original expression has already
   been substituted with a VAR_DECL that contains the value.  */
#define SAVE_EXPR_RESOLVED_P(NODE) \
  (SAVE_EXPR_CHECK (NODE)->base.public_flag)

/* Set on a CALL_EXPR if this stdarg call should be passed the argument
   pack.  */
#define CALL_EXPR_VA_ARG_PACK(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.public_flag)

/* In any expression, decl, or constant, nonzero means it has side effects or
   reevaluation of the whole expression could produce a different value.
   This is set if any subexpression is a function call, a side effect or a
   reference to a volatile variable.  In a ..._DECL, this is set only if the
   declaration said `volatile'.  This will never be set for a constant.  */
#define TREE_SIDE_EFFECTS(NODE) \
  (NON_TYPE_CHECK (NODE)->base.side_effects_flag)

/* In a LABEL_DECL, nonzero means this label had its address taken
   and therefore can never be deleted and is a jump target for
   computed gotos.  */
#define FORCED_LABEL(NODE) (LABEL_DECL_CHECK (NODE)->base.side_effects_flag)

/* Whether a case or a user-defined label is allowed to fall through to.
   This is used to implement -Wimplicit-fallthrough.  */
#define FALLTHROUGH_LABEL_P(NODE) \
  (LABEL_DECL_CHECK (NODE)->base.private_flag)

/* Set on the artificial label created for break; stmt from a switch.
   This is used to implement -Wimplicit-fallthrough.  */
#define SWITCH_BREAK_LABEL_P(NODE) \
  (LABEL_DECL_CHECK (NODE)->base.protected_flag)

/* Set on label that is known not to be jumped to, it can be only
   reached by falling through from previous statements.
   This is used to implement -Wimplicit-fallthrough.  */
#define UNUSED_LABEL_P(NODE) \
  (LABEL_DECL_CHECK (NODE)->base.default_def_flag)

/* Nonzero means this expression is volatile in the C sense:
   its address should be of type `volatile WHATEVER *'.
   In other words, the declared item is volatile qualified.
   This is used in _DECL nodes and _REF nodes.
   On a FUNCTION_DECL node, this means the function does not
   return normally.  This is the same effect as setting
   the attribute noreturn on the function in C.

   In a ..._TYPE node, means this type is volatile-qualified.
   But use TYPE_VOLATILE instead of this macro when the node is a type,
   because eventually we may make that a different bit.

   If this bit is set in an expression, so is TREE_SIDE_EFFECTS.  */
#define TREE_THIS_VOLATILE(NODE) ((NODE)->base.volatile_flag)

/* Nonzero means this node will not trap.  In an INDIRECT_REF, means
   accessing the memory pointed to won't generate a trap.  However,
   this only applies to an object when used appropriately: it doesn't
   mean that writing a READONLY mem won't trap.

   In ARRAY_REF and ARRAY_RANGE_REF means that we know that the index
   (or slice of the array) always belongs to the range of the array.
   I.e. that the access will not trap, provided that the access to
   the base to the array will not trap.  */
#define TREE_THIS_NOTRAP(NODE) \
  (TREE_CHECK5 (NODE, INDIRECT_REF, MEM_REF, TARGET_MEM_REF, ARRAY_REF,	\
		ARRAY_RANGE_REF)->base.nothrow_flag)

/* In a VAR_DECL, PARM_DECL or FIELD_DECL, or any kind of ..._REF node,
   nonzero means it may not be the lhs of an assignment.
   Nonzero in a FUNCTION_DECL means this function should be treated
   as "const" function (can only read its arguments).  */
#define TREE_READONLY(NODE) (NON_TYPE_CHECK (NODE)->base.readonly_flag)

/* Value of expression is constant.  Always on in all ..._CST nodes.  May
   also appear in an expression or decl where the value is constant.  */
#define TREE_CONSTANT(NODE) (NON_TYPE_CHECK (NODE)->base.constant_flag)

/* Nonzero if NODE, a type, has had its sizes gimplified.  */
#define TYPE_SIZES_GIMPLIFIED(NODE) \
  (TYPE_CHECK (NODE)->base.constant_flag)

/* In a decl (most significantly a FIELD_DECL), means an unsigned field.  */
#define DECL_UNSIGNED(NODE) \
  (DECL_COMMON_CHECK (NODE)->base.u.bits.unsigned_flag)

/* In integral and pointer types, means an unsigned type.  */
#define TYPE_UNSIGNED(NODE) (TYPE_CHECK (NODE)->base.u.bits.unsigned_flag)

/* Same as TYPE_UNSIGNED but converted to SIGNOP.  */
#define TYPE_SIGN(NODE) ((signop) TYPE_UNSIGNED (NODE))

/* True if overflow wraps around for the given integral or pointer type.  That
   is, TYPE_MAX + 1 == TYPE_MIN.  */
#define TYPE_OVERFLOW_WRAPS(TYPE) \
  (POINTER_TYPE_P (TYPE)					\
   ? flag_wrapv_pointer						\
   : (ANY_INTEGRAL_TYPE_CHECK(TYPE)->base.u.bits.unsigned_flag	\
      || flag_wrapv))

/* True if overflow is undefined for the given integral or pointer type.
   We may optimize on the assumption that values in the type never overflow.

   IMPORTANT NOTE: Any optimization based on TYPE_OVERFLOW_UNDEFINED
   must issue a warning based on warn_strict_overflow.  In some cases
   it will be appropriate to issue the warning immediately, and in
   other cases it will be appropriate to simply set a flag and let the
   caller decide whether a warning is appropriate or not.  */
#define TYPE_OVERFLOW_UNDEFINED(TYPE)				\
  (POINTER_TYPE_P (TYPE)					\
   ? !flag_wrapv_pointer					\
   : (!ANY_INTEGRAL_TYPE_CHECK(TYPE)->base.u.bits.unsigned_flag	\
      && !flag_wrapv && !flag_trapv))

/* True if overflow for the given integral type should issue a
   trap.  */
#define TYPE_OVERFLOW_TRAPS(TYPE) \
  (!ANY_INTEGRAL_TYPE_CHECK(TYPE)->base.u.bits.unsigned_flag && flag_trapv)

/* True if an overflow is to be preserved for sanitization.  */
#define TYPE_OVERFLOW_SANITIZED(TYPE)			\
  (INTEGRAL_TYPE_P (TYPE)				\
   && !TYPE_OVERFLOW_WRAPS (TYPE)			\
   && (flag_sanitize & SANITIZE_SI_OVERFLOW))

/* Nonzero in a VAR_DECL or STRING_CST means assembler code has been written.
   Nonzero in a FUNCTION_DECL means that the function has been compiled.
   This is interesting in an inline function, since it might not need
   to be compiled separately.
   Nonzero in a RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE, ENUMERAL_TYPE
   or TYPE_DECL if the debugging info for the type has been written.
   In a BLOCK node, nonzero if reorder_blocks has already seen this block.
   In an SSA_NAME node, nonzero if the SSA_NAME occurs in an abnormal
   PHI node.  */
#define TREE_ASM_WRITTEN(NODE) ((NODE)->base.asm_written_flag)

/* Nonzero in a _DECL if the name is used in its scope.
   Nonzero in an expr node means inhibit warning if value is unused.
   In IDENTIFIER_NODEs, this means that some extern decl for this name
   was used.
   In a BLOCK, this means that the block contains variables that are used.  */
#define TREE_USED(NODE) ((NODE)->base.used_flag)

/* In a FUNCTION_DECL, nonzero means a call to the function cannot
   throw an exception.  In a CALL_EXPR, nonzero means the call cannot
   throw.  We can't easily check the node type here as the C++
   frontend also uses this flag (for AGGR_INIT_EXPR).  */
#define TREE_NOTHROW(NODE) ((NODE)->base.nothrow_flag)

/* In a CALL_EXPR, means that it's safe to use the target of the call
   expansion as the return slot for a call that returns in memory.  */
#define CALL_EXPR_RETURN_SLOT_OPT(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.private_flag)

/* In a RESULT_DECL, PARM_DECL and VAR_DECL, means that it is
   passed by invisible reference (and the TREE_TYPE is a pointer to the true
   type).  */
#define DECL_BY_REFERENCE(NODE) \
  (TREE_CHECK3 (NODE, VAR_DECL, PARM_DECL, \
		RESULT_DECL)->decl_common.decl_by_reference_flag)

/* In VAR_DECL and PARM_DECL, set when the decl has been used except for
   being set.  */
#define DECL_READ_P(NODE) \
  (TREE_CHECK2 (NODE, VAR_DECL, PARM_DECL)->decl_common.decl_read_flag)

/* In VAR_DECL or RESULT_DECL, set when significant code movement precludes
   attempting to share the stack slot with some other variable.  */
#define DECL_NONSHAREABLE(NODE) \
  (TREE_CHECK2 (NODE, VAR_DECL, \
		RESULT_DECL)->decl_common.decl_nonshareable_flag)

/* In a PARM_DECL, set for Fortran hidden string length arguments that some
   buggy callers don't pass to the callee.  */
#define DECL_HIDDEN_STRING_LENGTH(NODE) \
  (TREE_CHECK (NODE, PARM_DECL)->decl_common.decl_nonshareable_flag)

/* In a CALL_EXPR, means that the call is the jump from a thunk to the
   thunked-to function.  Be careful to avoid using this macro when one of the
   next two applies instead.  */
#define CALL_FROM_THUNK_P(NODE) (CALL_EXPR_CHECK (NODE)->base.protected_flag)

/* In a CALL_EXPR, if the function being called is BUILT_IN_ALLOCA, means that
   it has been built for the declaration of a variable-sized object and, if the
   function being called is BUILT_IN_MEMCPY, means that it has been built for
   the assignment of a variable-sized object.  */
#define CALL_ALLOCA_FOR_VAR_P(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.protected_flag)

/* In a CALL_EXPR, if the function being called is DECL_IS_OPERATOR_NEW_P or
   DECL_IS_OPERATOR_DELETE_P, true for allocator calls from C++ new or delete
   expressions.  Not set for C++20 destroying delete operators.  */
#define CALL_FROM_NEW_OR_DELETE_P(NODE) \
  (CALL_EXPR_CHECK (NODE)->base.protected_flag)

/* Used in classes in C++.  */
#define TREE_PRIVATE(NODE) ((NODE)->base.private_flag)
/* Used in classes in C++. */
#define TREE_PROTECTED(NODE) ((NODE)->base.protected_flag)

/* True if reference type NODE is a C++ rvalue reference.  */
#define TYPE_REF_IS_RVALUE(NODE) \
  (REFERENCE_TYPE_CHECK (NODE)->base.private_flag)

/* Nonzero in a _DECL if the use of the name is defined as a
   deprecated feature by __attribute__((deprecated)).  */
#define TREE_DEPRECATED(NODE) \
  ((NODE)->base.deprecated_flag)

/* Nonzero in a _DECL if the use of the name is defined as an
   unavailable feature by __attribute__((unavailable)).  */
#define TREE_UNAVAILABLE(NODE) \
  ((NODE)->base.u.bits.unavailable_flag)

/* Nonzero indicates an IDENTIFIER_NODE that names an anonymous
   aggregate, (as created by anon_aggr_name_format).  */
#define IDENTIFIER_ANON_P(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->base.private_flag)

/* Nonzero indicates an IDENTIFIER_NODE that names an internal label.
   The prefix used to generate the label can be found on the TREE_CHAIN.  */
#define IDENTIFIER_INTERNAL_P(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->base.volatile_flag)

/* Nonzero in an IDENTIFIER_NODE if the name is a local alias, whose
   uses are to be substituted for uses of the TREE_CHAINed identifier.  */
#define IDENTIFIER_TRANSPARENT_ALIAS(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->base.deprecated_flag)

/* In an aggregate type, indicates that the scalar fields of the type are
   stored in reverse order from the target order.  This effectively
   toggles BYTES_BIG_ENDIAN and WORDS_BIG_ENDIAN within the type.  */
#define TYPE_REVERSE_STORAGE_ORDER(NODE) \
  (TREE_CHECK4 (NODE, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE, ARRAY_TYPE)->base.u.bits.saturating_flag)

/* In a non-aggregate type, indicates a saturating type.  */
#define TYPE_SATURATING(NODE) \
  (TREE_NOT_CHECK4 (NODE, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE, ARRAY_TYPE)->base.u.bits.saturating_flag)

/* In a BIT_FIELD_REF and MEM_REF, indicates that the reference is to a group
   of bits stored in reverse order from the target order.  This effectively
   toggles both BYTES_BIG_ENDIAN and WORDS_BIG_ENDIAN for the reference.

   The overall strategy is to preserve the invariant that every scalar in
   memory is associated with a single storage order, i.e. all accesses to
   this scalar are done with the same storage order.  This invariant makes
   it possible to factor out the storage order in most transformations, as
   only the address and/or the value (in target order) matter for them.
   But, of course, the storage order must be preserved when the accesses
   themselves are rewritten or transformed.  */
#define REF_REVERSE_STORAGE_ORDER(NODE) \
  (TREE_CHECK2 (NODE, BIT_FIELD_REF, MEM_REF)->base.default_def_flag)

  /* In an ADDR_EXPR, indicates that this is a pointer to nested function
   represented by a descriptor instead of a trampoline.  */
#define FUNC_ADDR_BY_DESCRIPTOR(NODE) \
  (TREE_CHECK (NODE, ADDR_EXPR)->base.default_def_flag)

/* In a CALL_EXPR, indicates that this is an indirect call for which
   pointers to nested function are descriptors instead of trampolines.  */
#define CALL_EXPR_BY_DESCRIPTOR(NODE) \
  (TREE_CHECK (NODE, CALL_EXPR)->base.default_def_flag)

/* These flags are available for each language front end to use internally.  */
#define TREE_LANG_FLAG_0(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_0)
#define TREE_LANG_FLAG_1(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_1)
#define TREE_LANG_FLAG_2(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_2)
#define TREE_LANG_FLAG_3(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_3)
#define TREE_LANG_FLAG_4(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_4)
#define TREE_LANG_FLAG_5(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_5)
#define TREE_LANG_FLAG_6(NODE) \
  (TREE_NOT_CHECK2 (NODE, TREE_VEC, SSA_NAME)->base.u.bits.lang_flag_6)

/* Define additional fields and accessors for nodes representing constants.  */

#define TREE_INT_CST_NUNITS(NODE) \
  (INTEGER_CST_CHECK (NODE)->base.u.int_length.unextended)
#define TREE_INT_CST_EXT_NUNITS(NODE) \
  (INTEGER_CST_CHECK (NODE)->base.u.int_length.extended)
#define TREE_INT_CST_ELT(NODE, I) TREE_INT_CST_ELT_CHECK (NODE, I)
#define TREE_INT_CST_LOW(NODE) \
  ((unsigned HOST_WIDE_INT) TREE_INT_CST_ELT (NODE, 0))

/* Return true if NODE is a POLY_INT_CST.  This is only ever true on
   targets with variable-sized modes.  */
#define POLY_INT_CST_P(NODE) \
  (NUM_POLY_INT_COEFFS > 1 && TREE_CODE (NODE) == POLY_INT_CST)

/* In a POLY_INT_CST node.  */
#define POLY_INT_CST_COEFF(NODE, I) \
  (POLY_INT_CST_CHECK (NODE)->poly_int_cst.coeffs[I])

#define TREE_REAL_CST_PTR(NODE) (&REAL_CST_CHECK (NODE)->real_cst.value)
#define TREE_REAL_CST(NODE) (*TREE_REAL_CST_PTR (NODE))

#define TREE_FIXED_CST_PTR(NODE) \
  (FIXED_CST_CHECK (NODE)->fixed_cst.fixed_cst_ptr)
#define TREE_FIXED_CST(NODE) (*TREE_FIXED_CST_PTR (NODE))

/* In a STRING_CST */
/* In C terms, this is sizeof, not strlen.  */
#define TREE_STRING_LENGTH(NODE) (STRING_CST_CHECK (NODE)->string.length)
#define TREE_STRING_POINTER(NODE) \
  ((const char *)(STRING_CST_CHECK (NODE)->string.str))

/* In a RAW_DATA_CST */
#define RAW_DATA_LENGTH(NODE) \
  (RAW_DATA_CST_CHECK (NODE)->raw_data_cst.length)
#define RAW_DATA_POINTER(NODE) \
  (RAW_DATA_CST_CHECK (NODE)->raw_data_cst.str)
#define RAW_DATA_OWNER(NODE) \
  (RAW_DATA_CST_CHECK (NODE)->raw_data_cst.owner)
#define RAW_DATA_UCHAR_ELT(NODE, I) \
  (((const unsigned char *) RAW_DATA_POINTER (NODE))[I])
#define RAW_DATA_SCHAR_ELT(NODE, I) \
  (((const signed char *) RAW_DATA_POINTER (NODE))[I])

/* In a COMPLEX_CST node.  */
#define TREE_REALPART(NODE) (COMPLEX_CST_CHECK (NODE)->complex.real)
#define TREE_IMAGPART(NODE) (COMPLEX_CST_CHECK (NODE)->complex.imag)

/* In a VECTOR_CST node.  See generic.texi for details.  */
#define VECTOR_CST_NELTS(NODE) (TYPE_VECTOR_SUBPARTS (TREE_TYPE (NODE)))
#define VECTOR_CST_ELT(NODE,IDX) vector_cst_elt (NODE, IDX)

#define VECTOR_CST_LOG2_NPATTERNS(NODE) \
  (VECTOR_CST_CHECK (NODE)->base.u.vector_cst.log2_npatterns)
#define VECTOR_CST_NPATTERNS(NODE) \
  (1U << VECTOR_CST_LOG2_NPATTERNS (NODE))
#define VECTOR_CST_NELTS_PER_PATTERN(NODE) \
  (VECTOR_CST_CHECK (NODE)->base.u.vector_cst.nelts_per_pattern)
#define VECTOR_CST_DUPLICATE_P(NODE) \
  (VECTOR_CST_NELTS_PER_PATTERN (NODE) == 1)
#define VECTOR_CST_STEPPED_P(NODE) \
  (VECTOR_CST_NELTS_PER_PATTERN (NODE) == 3)
#define VECTOR_CST_ENCODED_ELTS(NODE) \
  (VECTOR_CST_CHECK (NODE)->vector.elts)
#define VECTOR_CST_ENCODED_ELT(NODE, ELT) \
  (VECTOR_CST_CHECK (NODE)->vector.elts[ELT])

/* Define fields and accessors for some special-purpose tree nodes.  */

/* Unlike STRING_CST, in C terms this is strlen, not sizeof.  */
#define IDENTIFIER_LENGTH(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->identifier.id.len)
#define IDENTIFIER_POINTER(NODE) \
  ((const char *) IDENTIFIER_NODE_CHECK (NODE)->identifier.id.str)
#define IDENTIFIER_HASH_VALUE(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE)->identifier.id.hash_value)

/* Translate a hash table identifier pointer to a tree_identifier
   pointer, and vice versa.  */

#define HT_IDENT_TO_GCC_IDENT(NODE) \
  ((tree) ((char *) (NODE) - sizeof (struct tree_common)))
#define GCC_IDENT_TO_HT_IDENT(NODE) (&((struct tree_identifier *) (NODE))->id)

/* In a TREE_LIST node.  */
#define TREE_PURPOSE(NODE) (TREE_LIST_CHECK (NODE)->list.purpose)
#define TREE_VALUE(NODE) (TREE_LIST_CHECK (NODE)->list.value)

/* In a TREE_VEC node.  */
#define TREE_VEC_LENGTH(NODE) (TREE_VEC_CHECK (NODE)->base.u.length)
#define TREE_VEC_BEGIN(NODE) (&TREE_VEC_CHECK (NODE)->vec.a[0])
#define TREE_VEC_END(NODE) \
  ((void) TREE_VEC_CHECK (NODE), &((NODE)->vec.a[(NODE)->base.u.length]))

#define TREE_VEC_ELT(NODE,I) TREE_VEC_ELT_CHECK (NODE, I)

/* In a CONSTRUCTOR node.  */
#define CONSTRUCTOR_ELTS(NODE) (CONSTRUCTOR_CHECK (NODE)->constructor.elts)
#define CONSTRUCTOR_ELT(NODE,IDX) \
  (&(*CONSTRUCTOR_ELTS (NODE))[IDX])
#define CONSTRUCTOR_NELTS(NODE) \
  (vec_safe_length (CONSTRUCTOR_ELTS (NODE)))
#define CONSTRUCTOR_NO_CLEARING(NODE) \
  (CONSTRUCTOR_CHECK (NODE)->base.public_flag)
/* True if even padding bits should be zeroed during initialization.  */
#define CONSTRUCTOR_ZERO_PADDING_BITS(NODE) \
  (CONSTRUCTOR_CHECK (NODE)->base.default_def_flag)

/* Iterate through the vector V of CONSTRUCTOR_ELT elements, yielding the
   value of each element (stored within VAL). IX must be a scratch variable
   of unsigned integer type.  */
#define FOR_EACH_CONSTRUCTOR_VALUE(V, IX, VAL) \
  for (IX = 0; (IX >= vec_safe_length (V)) \
	       ? false \
	       : ((VAL = (*(V))[IX].value), \
	       true); \
       (IX)++)

/* Iterate through the vector V of CONSTRUCTOR_ELT elements, yielding both
   the value of each element (stored within VAL) and its index (stored
   within INDEX). IX must be a scratch variable of unsigned integer type.  */
#define FOR_EACH_CONSTRUCTOR_ELT(V, IX, INDEX, VAL) \
  for (IX = 0; (IX >= vec_safe_length (V)) \
	       ? false \
	       : (((void) (VAL = (*V)[IX].value)), \
		  (INDEX = (*V)[IX].index), \
		  true); \
       (IX)++)

/* Append a new constructor element to V, with the specified INDEX and VAL.  */
#define CONSTRUCTOR_APPEND_ELT(V, INDEX, VALUE) \
  do { \
    constructor_elt _ce___ = {INDEX, VALUE}; \
    vec_safe_push ((V), _ce___); \
  } while (0)

/* True if NODE, a FIELD_DECL, is to be processed as a bitfield for
   constructor output purposes.  */
#define CONSTRUCTOR_BITFIELD_P(NODE) \
  (DECL_BIT_FIELD (FIELD_DECL_CHECK (NODE)) \
   && (DECL_MODE (NODE) != BLKmode \
       || TREE_CODE (TREE_TYPE (NODE)) == BITINT_TYPE))

/* True if NODE is a clobber right hand side, an expression of indeterminate
   value that clobbers the LHS in a copy instruction.  We use a volatile
   empty CONSTRUCTOR for this, as it matches most of the necessary semantic.
   In particular the volatile flag causes us to not prematurely remove
   such clobber instructions.  */
#define TREE_CLOBBER_P(NODE) \
  (TREE_CODE (NODE) == CONSTRUCTOR && TREE_THIS_VOLATILE (NODE))

/* Return the clobber_kind of a CLOBBER CONSTRUCTOR.  */
#define CLOBBER_KIND(NODE) \
  (CONSTRUCTOR_CHECK (NODE)->base.u.bits.address_space)

/* Define fields and accessors for some nodes that represent expressions.  */

/* Nonzero if NODE is an empty statement (NOP_EXPR <0>).  */
#define IS_EMPTY_STMT(NODE)	(TREE_CODE (NODE) == NOP_EXPR \
				 && VOID_TYPE_P (TREE_TYPE (NODE)) \
				 && integer_zerop (TREE_OPERAND (NODE, 0)))

/* In ordinary expression nodes.  */
#define TREE_OPERAND_LENGTH(NODE) tree_operand_length (NODE)
#define TREE_OPERAND(NODE, I) TREE_OPERAND_CHECK (NODE, I)

/* In a tcc_vl_exp node, operand 0 is an INT_CST node holding the operand
   length.  Its value includes the length operand itself; that is,
   the minimum valid length is 1.
   Note that we have to bypass the use of TREE_OPERAND to access
   that field to avoid infinite recursion in expanding the macros.  */
#define VL_EXP_OPERAND_LENGTH(NODE) \
  ((int)TREE_INT_CST_LOW (VL_EXP_CHECK (NODE)->exp.operands[0]))

/* Nonzero if gimple_debug_nonbind_marker_p() may possibly hold.  */
#define MAY_HAVE_DEBUG_MARKER_STMTS debug_nonbind_markers_p
/* Nonzero if gimple_debug_bind_p() (and thus
   gimple_debug_source_bind_p()) may possibly hold.  */
#define MAY_HAVE_DEBUG_BIND_STMTS flag_var_tracking_assignments
/* Nonzero if is_gimple_debug() may possibly hold.  */
#define MAY_HAVE_DEBUG_STMTS					\
  (MAY_HAVE_DEBUG_MARKER_STMTS || MAY_HAVE_DEBUG_BIND_STMTS)

/* In a LOOP_EXPR node.  */
#define LOOP_EXPR_BODY(NODE) TREE_OPERAND_CHECK_CODE (NODE, LOOP_EXPR, 0)

/* The source location of this expression.  Non-tree_exp nodes such as
   decls and constants can be shared among multiple locations, so
   return nothing.  */
#define EXPR_LOCATION(NODE) \
  (CAN_HAVE_LOCATION_P ((NODE)) ? (NODE)->exp.locus : UNKNOWN_LOCATION)
#define SET_EXPR_LOCATION(NODE, LOCUS) EXPR_CHECK ((NODE))->exp.locus = (LOCUS)
#define EXPR_HAS_LOCATION(NODE) (LOCATION_LOCUS (EXPR_LOCATION (NODE))	\
  != UNKNOWN_LOCATION)
/* The location to be used in a diagnostic about this expression.  Do not
   use this macro if the location will be assigned to other expressions.  */
#define EXPR_LOC_OR_LOC(NODE, LOCUS) (EXPR_HAS_LOCATION (NODE) \
				      ? (NODE)->exp.locus : (LOCUS))
#define EXPR_FILENAME(NODE) LOCATION_FILE (EXPR_CHECK ((NODE))->exp.locus)
#define EXPR_LINENO(NODE) LOCATION_LINE (EXPR_CHECK (NODE)->exp.locus)

#define CAN_HAVE_RANGE_P(NODE) (CAN_HAVE_LOCATION_P (NODE))
#define EXPR_LOCATION_RANGE(NODE) (get_expr_source_range (EXPR_CHECK ((NODE))))

#define EXPR_HAS_RANGE(NODE) \
    (CAN_HAVE_RANGE_P (NODE) \
     ? EXPR_LOCATION_RANGE (NODE).m_start != UNKNOWN_LOCATION \
     : false)

/* True if a tree is an expression or statement that can have a
   location.  */
#define CAN_HAVE_LOCATION_P(NODE) ((NODE) && EXPR_P (NODE))

inline source_range
get_expr_source_range (tree expr)
{
  location_t loc = EXPR_LOCATION (expr);
  return get_range_from_loc (line_table, loc);
}

extern void protected_set_expr_location (tree, location_t);
extern void protected_set_expr_location_if_unset (tree, location_t);
ATTRIBUTE_WARN_UNUSED_RESULT
extern tree protected_set_expr_location_unshare (tree, location_t);

WARN_UNUSED_RESULT extern tree maybe_wrap_with_location (tree, location_t);

extern int suppress_location_wrappers;

/* A class for suppressing the creation of location wrappers.
   Location wrappers will not be created during the lifetime
   of an instance of this class.  */

class auto_suppress_location_wrappers
{
 public:
  auto_suppress_location_wrappers () { ++suppress_location_wrappers; }
  ~auto_suppress_location_wrappers () { --suppress_location_wrappers; }
};

/* In a TARGET_EXPR node.  */
#define TARGET_EXPR_SLOT(NODE) TREE_OPERAND_CHECK_CODE (NODE, TARGET_EXPR, 0)
#define TARGET_EXPR_INITIAL(NODE) TREE_OPERAND_CHECK_CODE (NODE, TARGET_EXPR, 1)
#define TARGET_EXPR_CLEANUP(NODE) TREE_OPERAND_CHECK_CODE (NODE, TARGET_EXPR, 2)
/* Don't elide the initialization of TARGET_EXPR_SLOT for this TARGET_EXPR
   on rhs of MODIFY_EXPR.  */
#define TARGET_EXPR_NO_ELIDE(NODE) (TARGET_EXPR_CHECK (NODE)->base.private_flag)

/* DECL_EXPR accessor. This gives access to the DECL associated with
   the given declaration statement.  */
#define DECL_EXPR_DECL(NODE)    TREE_OPERAND (DECL_EXPR_CHECK (NODE), 0)

#define EXIT_EXPR_COND(NODE)	     TREE_OPERAND (EXIT_EXPR_CHECK (NODE), 0)

/* COMPOUND_LITERAL_EXPR accessors.  */
#define COMPOUND_LITERAL_EXPR_DECL_EXPR(NODE)		\
  TREE_OPERAND (COMPOUND_LITERAL_EXPR_CHECK (NODE), 0)
#define COMPOUND_LITERAL_EXPR_DECL(NODE)			\
  DECL_EXPR_DECL (COMPOUND_LITERAL_EXPR_DECL_EXPR (NODE))

/* SWITCH_EXPR accessors. These give access to the condition and body.  */
#define SWITCH_COND(NODE)       TREE_OPERAND (SWITCH_EXPR_CHECK (NODE), 0)
#define SWITCH_BODY(NODE)       TREE_OPERAND (SWITCH_EXPR_CHECK (NODE), 1)
/* True if there are case labels for all possible values of SWITCH_COND, either
   because there is a default: case label or because the case label ranges cover
   all values.  */
#define SWITCH_ALL_CASES_P(NODE) (SWITCH_EXPR_CHECK (NODE)->base.private_flag)

/* CASE_LABEL_EXPR accessors. These give access to the high and low values
   of a case label, respectively.  */
#define CASE_LOW(NODE)          	TREE_OPERAND (CASE_LABEL_EXPR_CHECK (NODE), 0)
#define CASE_HIGH(NODE)         	TREE_OPERAND (CASE_LABEL_EXPR_CHECK (NODE), 1)
#define CASE_LABEL(NODE)		TREE_OPERAND (CASE_LABEL_EXPR_CHECK (NODE), 2)
#define CASE_CHAIN(NODE)		TREE_OPERAND (CASE_LABEL_EXPR_CHECK (NODE), 3)

/* The operands of a TARGET_MEM_REF.  Operands 0 and 1 have to match
   corresponding MEM_REF operands.  */
#define TMR_BASE(NODE) (TREE_OPERAND (TARGET_MEM_REF_CHECK (NODE), 0))
#define TMR_OFFSET(NODE) (TREE_OPERAND (TARGET_MEM_REF_CHECK (NODE), 1))
#define TMR_INDEX(NODE) (TREE_OPERAND (TARGET_MEM_REF_CHECK (NODE), 2))
#define TMR_STEP(NODE) (TREE_OPERAND (TARGET_MEM_REF_CHECK (NODE), 3))
#define TMR_INDEX2(NODE) (TREE_OPERAND (TARGET_MEM_REF_CHECK (NODE), 4))

#define MR_DEPENDENCE_CLIQUE(NODE) \
  (TREE_CHECK2 (NODE, MEM_REF, TARGET_MEM_REF)->base.u.dependence_info.clique)
#define MR_DEPENDENCE_BASE(NODE) \
  (TREE_CHECK2 (NODE, MEM_REF, TARGET_MEM_REF)->base.u.dependence_info.base)

/* The operands of a BIND_EXPR.  */
#define BIND_EXPR_VARS(NODE) (TREE_OPERAND (BIND_EXPR_CHECK (NODE), 0))
#define BIND_EXPR_BODY(NODE) (TREE_OPERAND (BIND_EXPR_CHECK (NODE), 1))
#define BIND_EXPR_BLOCK(NODE) (TREE_OPERAND (BIND_EXPR_CHECK (NODE), 2))

/* GOTO_EXPR accessor. This gives access to the label associated with
   a goto statement.  */
#define GOTO_DESTINATION(NODE)  TREE_OPERAND (GOTO_EXPR_CHECK (NODE), 0)

/* ASM_EXPR accessors. ASM_STRING returns a STRING_CST for the
   instruction (e.g., "mov x, y"). ASM_OUTPUTS, ASM_INPUTS, and
   ASM_CLOBBERS represent the outputs, inputs, and clobbers for the
   statement.  */
#define ASM_STRING(NODE)        TREE_OPERAND (ASM_EXPR_CHECK (NODE), 0)
#define ASM_OUTPUTS(NODE)       TREE_OPERAND (ASM_EXPR_CHECK (NODE), 1)
#define ASM_INPUTS(NODE)        TREE_OPERAND (ASM_EXPR_CHECK (NODE), 2)
#define ASM_CLOBBERS(NODE)      TREE_OPERAND (ASM_EXPR_CHECK (NODE), 3)
#define ASM_LABELS(NODE)	TREE_OPERAND (ASM_EXPR_CHECK (NODE), 4)
/* Nonzero if the asm is a basic asm, zero if it is an extended asm.
   Basic asms use a plain ASM_INPUT insn pattern whereas extended asms
   use an ASM_OPERANDS insn pattern.  */
#define ASM_BASIC_P(NODE)	(ASM_EXPR_CHECK (NODE)->base.static_flag)
#define ASM_VOLATILE_P(NODE)	(ASM_EXPR_CHECK (NODE)->base.public_flag)
/* Nonzero if we want to consider this asm as minimum length and cost
   for inlining decisions.  */
#define ASM_INLINE_P(NODE)	(ASM_EXPR_CHECK (NODE)->base.protected_flag)

/* COND_EXPR accessors.  */
#define COND_EXPR_COND(NODE)	(TREE_OPERAND (COND_EXPR_CHECK (NODE), 0))
#define COND_EXPR_THEN(NODE)	(TREE_OPERAND (COND_EXPR_CHECK (NODE), 1))
#define COND_EXPR_ELSE(NODE)	(TREE_OPERAND (COND_EXPR_CHECK (NODE), 2))

/* Accessors for the chains of recurrences.  */
#define CHREC_LEFT(NODE)        TREE_OPERAND (POLYNOMIAL_CHREC_CHECK (NODE), 0)
#define CHREC_RIGHT(NODE)       TREE_OPERAND (POLYNOMIAL_CHREC_CHECK (NODE), 1)
#define CHREC_VARIABLE(NODE)    POLYNOMIAL_CHREC_CHECK (NODE)->base.u.chrec_var
/* Nonzero if this chrec doesn't overflow (i.e., nonwrapping).  */
#define CHREC_NOWRAP(NODE)      POLYNOMIAL_CHREC_CHECK (NODE)->base.nothrow_flag

/* LABEL_EXPR accessor. This gives access to the label associated with
   the given label expression.  */
#define LABEL_EXPR_LABEL(NODE)  TREE_OPERAND (LABEL_EXPR_CHECK (NODE), 0)

/* CATCH_EXPR accessors.  */
#define CATCH_TYPES(NODE)	TREE_OPERAND (CATCH_EXPR_CHECK (NODE), 0)
#define CATCH_BODY(NODE)	TREE_OPERAND (CATCH_EXPR_CHECK (NODE), 1)

/* EH_FILTER_EXPR accessors.  */
#define EH_FILTER_TYPES(NODE)	TREE_OPERAND (EH_FILTER_EXPR_CHECK (NODE), 0)
#define EH_FILTER_FAILURE(NODE)	TREE_OPERAND (EH_FILTER_EXPR_CHECK (NODE), 1)

/* OBJ_TYPE_REF accessors.  */
#define OBJ_TYPE_REF_EXPR(NODE)	  TREE_OPERAND (OBJ_TYPE_REF_CHECK (NODE), 0)
#define OBJ_TYPE_REF_OBJECT(NODE) TREE_OPERAND (OBJ_TYPE_REF_CHECK (NODE), 1)
#define OBJ_TYPE_REF_TOKEN(NODE)  TREE_OPERAND (OBJ_TYPE_REF_CHECK (NODE), 2)

/* CALL_EXPR accessors.  */
#define CALL_EXPR_FN(NODE) TREE_OPERAND (CALL_EXPR_CHECK (NODE), 1)
#define CALL_EXPR_STATIC_CHAIN(NODE) TREE_OPERAND (CALL_EXPR_CHECK (NODE), 2)
#define CALL_EXPR_ARG(NODE, I) TREE_OPERAND (CALL_EXPR_CHECK (NODE), (I) + 3)
#define call_expr_nargs(NODE) (VL_EXP_OPERAND_LENGTH (NODE) - 3)
#define CALL_EXPR_IFN(NODE) (CALL_EXPR_CHECK (NODE)->base.u.ifn)

/* CALL_EXPR_ARGP returns a pointer to the argument vector for NODE.
   We can't use &CALL_EXPR_ARG (NODE, 0) because that will complain if
   the argument count is zero when checking is enabled.  Instead, do
   the pointer arithmetic to advance past the 3 fixed operands in a
   CALL_EXPR.  That produces a valid pointer to just past the end of the
   operand array, even if it's not valid to dereference it.  */
#define CALL_EXPR_ARGP(NODE) \
  (&(TREE_OPERAND (CALL_EXPR_CHECK (NODE), 0)) + 3)

/* TM directives and accessors.  */
#define TRANSACTION_EXPR_BODY(NODE) \
  TREE_OPERAND (TRANSACTION_EXPR_CHECK (NODE), 0)
#define TRANSACTION_EXPR_OUTER(NODE) \
  (TRANSACTION_EXPR_CHECK (NODE)->base.static_flag)
#define TRANSACTION_EXPR_RELAXED(NODE) \
  (TRANSACTION_EXPR_CHECK (NODE)->base.public_flag)

/* OpenMP and OpenACC directive and clause accessors.  */

/* Generic accessors for OMP nodes that keep the body as operand 0, and clauses
   as operand 1.  */
#define OMP_BODY(NODE) \
  TREE_OPERAND (TREE_RANGE_CHECK (NODE, OACC_PARALLEL, OMP_MASTER), 0)
#define OMP_CLAUSES(NODE) \
  TREE_OPERAND (TREE_RANGE_CHECK (NODE, OACC_PARALLEL, OMP_SCAN), 1)

/* Generic accessors for OMP nodes that keep clauses as operand 0.  */
#define OMP_STANDALONE_CLAUSES(NODE) \
  TREE_OPERAND (TREE_RANGE_CHECK (NODE, OACC_CACHE, OMP_TARGET_EXIT_DATA), 0)

#define OACC_DATA_BODY(NODE) \
  TREE_OPERAND (OACC_DATA_CHECK (NODE), 0)
#define OACC_DATA_CLAUSES(NODE) \
  TREE_OPERAND (OACC_DATA_CHECK (NODE), 1)

#define OACC_HOST_DATA_BODY(NODE) \
  TREE_OPERAND (OACC_HOST_DATA_CHECK (NODE), 0)
#define OACC_HOST_DATA_CLAUSES(NODE) \
  TREE_OPERAND (OACC_HOST_DATA_CHECK (NODE), 1)

#define OACC_CACHE_CLAUSES(NODE) \
  TREE_OPERAND (OACC_CACHE_CHECK (NODE), 0)

#define OACC_DECLARE_CLAUSES(NODE) \
  TREE_OPERAND (OACC_DECLARE_CHECK (NODE), 0)

#define OACC_ENTER_DATA_CLAUSES(NODE) \
  TREE_OPERAND (OACC_ENTER_DATA_CHECK (NODE), 0)

#define OACC_EXIT_DATA_CLAUSES(NODE) \
  TREE_OPERAND (OACC_EXIT_DATA_CHECK (NODE), 0)

#define OACC_UPDATE_CLAUSES(NODE) \
  TREE_OPERAND (OACC_UPDATE_CHECK (NODE), 0)

#define OMP_PARALLEL_BODY(NODE)    TREE_OPERAND (OMP_PARALLEL_CHECK (NODE), 0)
#define OMP_PARALLEL_CLAUSES(NODE) TREE_OPERAND (OMP_PARALLEL_CHECK (NODE), 1)

#define OMP_TASK_BODY(NODE)	   TREE_OPERAND (OMP_TASK_CHECK (NODE), 0)
#define OMP_TASK_CLAUSES(NODE)	   TREE_OPERAND (OMP_TASK_CHECK (NODE), 1)

#define OMP_TASKREG_CHECK(NODE)	  TREE_RANGE_CHECK (NODE, OMP_PARALLEL, OMP_TASK)
#define OMP_TASKREG_BODY(NODE)    TREE_OPERAND (OMP_TASKREG_CHECK (NODE), 0)
#define OMP_TASKREG_CLAUSES(NODE) TREE_OPERAND (OMP_TASKREG_CHECK (NODE), 1)

#define OMP_LOOPING_CHECK(NODE) TREE_RANGE_CHECK (NODE, OMP_FOR, OACC_LOOP)
#define OMP_FOR_BODY(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 0)
#define OMP_FOR_CLAUSES(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 1)
#define OMP_FOR_INIT(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 2)
#define OMP_FOR_COND(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 3)
#define OMP_FOR_INCR(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 4)
#define OMP_FOR_PRE_BODY(NODE)	   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 5)
#define OMP_FOR_ORIG_DECLS(NODE)   TREE_OPERAND (OMP_LOOPING_CHECK (NODE), 6)

#define OMP_INTEROP_CLAUSES(NODE)\
  TREE_OPERAND (OMP_INTEROP_CHECK (NODE), 0)

#define OMP_LOOPXFORM_CHECK(NODE) TREE_RANGE_CHECK (NODE, OMP_TILE, OMP_UNROLL)
#define OMP_LOOPXFORM_LOWERED(NODE) \
  (OMP_LOOPXFORM_CHECK (NODE)->base.public_flag)

#define OMP_SECTIONS_BODY(NODE)    TREE_OPERAND (OMP_SECTIONS_CHECK (NODE), 0)
#define OMP_SECTIONS_CLAUSES(NODE) TREE_OPERAND (OMP_SECTIONS_CHECK (NODE), 1)

#define OMP_SECTION_BODY(NODE)	   TREE_OPERAND (OMP_SECTION_CHECK (NODE), 0)

#define OMP_STRUCTURED_BLOCK_BODY(NODE) \
  TREE_OPERAND (OMP_STRUCTURED_BLOCK_CHECK (NODE), 0)

#define OMP_SINGLE_BODY(NODE)	   TREE_OPERAND (OMP_SINGLE_CHECK (NODE), 0)
#define OMP_SINGLE_CLAUSES(NODE)   TREE_OPERAND (OMP_SINGLE_CHECK (NODE), 1)

#define OMP_SCOPE_BODY(NODE)	   TREE_OPERAND (OMP_SCOPE_CHECK (NODE), 0)
#define OMP_SCOPE_CLAUSES(NODE)	   TREE_OPERAND (OMP_SCOPE_CHECK (NODE), 1)

#define OMP_MASTER_BODY(NODE)	   TREE_OPERAND (OMP_MASTER_CHECK (NODE), 0)

#define OMP_MASKED_BODY(NODE)	   TREE_OPERAND (OMP_MASKED_CHECK (NODE), 0)
#define OMP_MASKED_CLAUSES(NODE)   TREE_OPERAND (OMP_MASKED_CHECK (NODE), 1)

#define OMP_TASKGROUP_BODY(NODE)   TREE_OPERAND (OMP_TASKGROUP_CHECK (NODE), 0)
#define OMP_TASKGROUP_CLAUSES(NODE) \
  TREE_OPERAND (OMP_TASKGROUP_CHECK (NODE), 1)

#define OMP_ORDERED_BODY(NODE)	   TREE_OPERAND (OMP_ORDERED_CHECK (NODE), 0)
#define OMP_ORDERED_CLAUSES(NODE)  TREE_OPERAND (OMP_ORDERED_CHECK (NODE), 1)

#define OMP_CRITICAL_BODY(NODE)    TREE_OPERAND (OMP_CRITICAL_CHECK (NODE), 0)
#define OMP_CRITICAL_CLAUSES(NODE) TREE_OPERAND (OMP_CRITICAL_CHECK (NODE), 1)
#define OMP_CRITICAL_NAME(NODE)    TREE_OPERAND (OMP_CRITICAL_CHECK (NODE), 2)

#define OMP_TEAMS_BODY(NODE)	   TREE_OPERAND (OMP_TEAMS_CHECK (NODE), 0)
#define OMP_TEAMS_CLAUSES(NODE)	   TREE_OPERAND (OMP_TEAMS_CHECK (NODE), 1)

#define OMP_TARGET_DATA_BODY(NODE) \
  TREE_OPERAND (OMP_TARGET_DATA_CHECK (NODE), 0)
#define OMP_TARGET_DATA_CLAUSES(NODE)\
  TREE_OPERAND (OMP_TARGET_DATA_CHECK (NODE), 1)

#define OMP_TARGET_BODY(NODE)	   TREE_OPERAND (OMP_TARGET_CHECK (NODE), 0)
#define OMP_TARGET_CLAUSES(NODE)   TREE_OPERAND (OMP_TARGET_CHECK (NODE), 1)

#define OMP_TARGET_UPDATE_CLAUSES(NODE)\
  TREE_OPERAND (OMP_TARGET_UPDATE_CHECK (NODE), 0)

#define OMP_TARGET_ENTER_DATA_CLAUSES(NODE)\
  TREE_OPERAND (OMP_TARGET_ENTER_DATA_CHECK (NODE), 0)

#define OMP_TARGET_EXIT_DATA_CLAUSES(NODE)\
  TREE_OPERAND (OMP_TARGET_EXIT_DATA_CHECK (NODE), 0)

#define OMP_METADIRECTIVE_VARIANTS(NODE) \
  TREE_OPERAND (OMP_METADIRECTIVE_CHECK (NODE), 0)

#define OMP_METADIRECTIVE_VARIANT_SELECTOR(v) \
  TREE_PURPOSE (v)
#define OMP_METADIRECTIVE_VARIANT_DIRECTIVE(v) \
  TREE_PURPOSE (TREE_VALUE (v))
#define OMP_METADIRECTIVE_VARIANT_BODY(v) \
  TREE_VALUE (TREE_VALUE (v))

#define OMP_DECLARE_MAPPER_ID(NODE) \
  TREE_OPERAND (OMP_DECLARE_MAPPER_CHECK (NODE), 0)
#define OMP_DECLARE_MAPPER_DECL(NODE) \
  TREE_OPERAND (OMP_DECLARE_MAPPER_CHECK (NODE), 1)
#define OMP_DECLARE_MAPPER_CLAUSES(NODE) \
  TREE_OPERAND (OMP_DECLARE_MAPPER_CHECK (NODE), 2)

#define OMP_SCAN_BODY(NODE)	TREE_OPERAND (OMP_SCAN_CHECK (NODE), 0)
#define OMP_SCAN_CLAUSES(NODE)	TREE_OPERAND (OMP_SCAN_CHECK (NODE), 1)

#define OMP_DISPATCH_BODY(NODE) TREE_OPERAND (OMP_DISPATCH_CHECK (NODE), 0)
#define OMP_DISPATCH_CLAUSES(NODE) TREE_OPERAND (OMP_DISPATCH_CHECK (NODE), 1)

#define OMP_CLAUSE_SIZE(NODE)						\
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (OMP_CLAUSE_CHECK (NODE),	\
					      OMP_CLAUSE_FROM,		\
					      OMP_CLAUSE__CACHE_), 1)

#define OMP_CLAUSE_CHAIN(NODE)     TREE_CHAIN (OMP_CLAUSE_CHECK (NODE))
#define OMP_CLAUSE_DECL(NODE)      					\
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (OMP_CLAUSE_CHECK (NODE),	\
					      OMP_CLAUSE_PRIVATE,	\
					      OMP_CLAUSE__SCANTEMP_), 0)
#define OMP_CLAUSE_HAS_LOCATION(NODE) \
  (LOCATION_LOCUS ((OMP_CLAUSE_CHECK (NODE))->omp_clause.locus)		\
  != UNKNOWN_LOCATION)
#define OMP_CLAUSE_LOCATION(NODE)  (OMP_CLAUSE_CHECK (NODE))->omp_clause.locus

/* True on OMP_FOR and other OpenMP/OpenACC looping constructs if the loop nest
   is non-rectangular.  */
#define OMP_FOR_NON_RECTANGULAR(NODE) \
  (OMP_LOOPING_CHECK (NODE)->base.private_flag)

/* True on an OMP_SECTION statement that was the last lexical member.
   This status is meaningful in the implementation of lastprivate.  */
#define OMP_SECTION_LAST(NODE) \
  (OMP_SECTION_CHECK (NODE)->base.private_flag)

/* True on an OMP_PARALLEL statement if it represents an explicit
   combined parallel work-sharing constructs.  */
#define OMP_PARALLEL_COMBINED(NODE) \
  (OMP_PARALLEL_CHECK (NODE)->base.private_flag)

/* True on an OMP_TEAMS statement if it represents an explicit
   combined teams distribute constructs.  */
#define OMP_TEAMS_COMBINED(NODE) \
  (OMP_TEAMS_CHECK (NODE)->base.private_flag)

/* True on an OMP_TARGET statement if it represents explicit
   combined target teams, target parallel or target simd constructs.  */
#define OMP_TARGET_COMBINED(NODE) \
  (OMP_TARGET_CHECK (NODE)->base.private_flag)

/* True on an OMP_MASTER statement if it represents an explicit
   combined master constructs.  */
#define OMP_MASTER_COMBINED(NODE) \
  (OMP_MASTER_CHECK (NODE)->base.private_flag)

/* True on an OMP_MASKED statement if it represents an explicit
   combined masked constructs.  */
#define OMP_MASKED_COMBINED(NODE) \
  (OMP_MASKED_CHECK (NODE)->base.private_flag)

/* Memory order for OMP_ATOMIC*.  */
#define OMP_ATOMIC_MEMORY_ORDER(NODE) \
  (TREE_RANGE_CHECK (NODE, OMP_ATOMIC, \
		     OMP_ATOMIC_CAPTURE_NEW)->base.u.omp_atomic_memory_order)

/* Weak clause on OMP_ATOMIC*.  */
#define OMP_ATOMIC_WEAK(NODE) \
  (TREE_RANGE_CHECK (NODE, OMP_ATOMIC, \
		     OMP_ATOMIC_CAPTURE_NEW)->base.public_flag)

/* True on a PRIVATE clause if its decl is kept around for debugging
   information only and its DECL_VALUE_EXPR is supposed to point
   to what it has been remapped to.  */
#define OMP_CLAUSE_PRIVATE_DEBUG(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PRIVATE)->base.public_flag)

/* True on a PRIVATE clause if ctor needs access to outer region's
   variable.  */
#define OMP_CLAUSE_PRIVATE_OUTER_REF(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PRIVATE))

/* True if a PRIVATE clause is for a C++ class IV on taskloop construct
   (thus should be private on the outer taskloop and firstprivate on
   task).  */
#define OMP_CLAUSE_PRIVATE_TASKLOOP_IV(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PRIVATE))

/* True on a FIRSTPRIVATE clause if it has been added implicitly.  */
#define OMP_CLAUSE_FIRSTPRIVATE_IMPLICIT(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_FIRSTPRIVATE)->base.public_flag)

/* True on a FIRSTPRIVATE clause if only the reference and not what it refers
   to should be firstprivatized.  */
#define OMP_CLAUSE_FIRSTPRIVATE_NO_REFERENCE(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_FIRSTPRIVATE))

/* True on a FIRSTPRIVATE clause with OMP_CLAUSE_FIRSTPRIVATE_IMPLICIT also
   set if target construct is the only one that accepts the clause.  */
#define OMP_CLAUSE_FIRSTPRIVATE_IMPLICIT_TARGET(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_FIRSTPRIVATE))

/* True on a LASTPRIVATE clause if a FIRSTPRIVATE clause for the same
   decl is present in the chain.  */
#define OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LASTPRIVATE)->base.public_flag)
#define OMP_CLAUSE_LASTPRIVATE_STMT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE,			\
						OMP_CLAUSE_LASTPRIVATE),\
		      1)
#define OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ(NODE) \
  (OMP_CLAUSE_CHECK (NODE))->omp_clause.gimple_reduction_init

/* True if a LASTPRIVATE clause is for a C++ class IV on taskloop or
   loop construct (thus should be lastprivate on the outer taskloop and
   firstprivate on task for the taskloop construct and carefully handled
   for loop construct).  */
#define OMP_CLAUSE_LASTPRIVATE_LOOP_IV(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LASTPRIVATE))

/* True if a LASTPRIVATE clause has CONDITIONAL: modifier.  */
#define OMP_CLAUSE_LASTPRIVATE_CONDITIONAL(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LASTPRIVATE))

/* True on a SHARED clause if a FIRSTPRIVATE clause for the same
   decl is present in the chain (this can happen only for taskloop
   with FIRSTPRIVATE/LASTPRIVATE on it originally.  */
#define OMP_CLAUSE_SHARED_FIRSTPRIVATE(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SHARED)->base.public_flag)

/* True on a SHARED clause if a scalar is not modified in the body and
   thus could be optimized as firstprivate.  */
#define OMP_CLAUSE_SHARED_READONLY(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SHARED))

#define OMP_CLAUSE_IF_MODIFIER(NODE)	\
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_IF)->omp_clause.subcode.if_modifier)

#define OMP_CLAUSE_FINAL_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_FINAL), 0)
#define OMP_CLAUSE_IF_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_IF), 0)
#define OMP_CLAUSE_SELF_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SELF), 0)
#define OMP_CLAUSE_NUM_THREADS_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_THREADS),0)
#define OMP_CLAUSE_SCHEDULE_CHUNK_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SCHEDULE), 0)
#define OMP_CLAUSE_NUM_TASKS_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_TASKS), 0)
#define OMP_CLAUSE_HINT_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_HINT), 0)
#define OMP_CLAUSE_FILTER_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_FILTER), 0)
#define OMP_CLAUSE_PARTIAL_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PARTIAL), 0)
#define OMP_CLAUSE_SIZES_LIST(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SIZES), 0)
#define OMP_CLAUSE_NOVARIANTS_EXPR(NODE)                                       \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NOVARIANTS), 0)
#define OMP_CLAUSE_NOCONTEXT_EXPR(NODE)                                        \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NOCONTEXT), 0)

#define OMP_CLAUSE_GRAINSIZE_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_GRAINSIZE),0)

#define OMP_CLAUSE_PRIORITY_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PRIORITY),0)

#define OMP_CLAUSE_GRAINSIZE_STRICT(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_GRAINSIZE))
#define OMP_CLAUSE_NUM_TASKS_STRICT(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_TASKS))

/* OpenACC clause expressions  */
#define OMP_CLAUSE_EXPR(NODE, CLAUSE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, CLAUSE), 0)
#define OMP_CLAUSE_GANG_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_GANG), 0)
#define OMP_CLAUSE_GANG_STATIC_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_GANG), 1)
#define OMP_CLAUSE_ASYNC_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ASYNC), 0)
#define OMP_CLAUSE_WAIT_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_WAIT), 0)
#define OMP_CLAUSE_VECTOR_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_VECTOR), 0)
#define OMP_CLAUSE_WORKER_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_WORKER), 0)
#define OMP_CLAUSE_NUM_GANGS_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_GANGS), 0)
#define OMP_CLAUSE_NUM_WORKERS_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_WORKERS), 0)
#define OMP_CLAUSE_VECTOR_LENGTH_EXPR(NODE) \
  OMP_CLAUSE_OPERAND ( \
    OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_VECTOR_LENGTH), 0)

#define OMP_CLAUSE_DEPEND_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEPEND)->omp_clause.subcode.depend_kind)

#define OMP_CLAUSE_DOACROSS_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DOACROSS)->omp_clause.subcode.doacross_kind)

#define OMP_CLAUSE_DOACROSS_SINK_NEGATIVE(NODE) \
  TREE_PUBLIC (TREE_LIST_CHECK (NODE))

/* True if DOACROSS clause is spelled as DEPEND.  */
#define OMP_CLAUSE_DOACROSS_DEPEND(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DOACROSS))

#define OMP_CLAUSE_MAP_KIND(NODE) \
  ((enum gomp_map_kind) OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->omp_clause.subcode.map_kind)
#define OMP_CLAUSE_SET_MAP_KIND(NODE, MAP_KIND) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->omp_clause.subcode.map_kind \
   = (unsigned int) (MAP_KIND))

#define OMP_CLAUSE_MOTION_PRESENT(NODE) \
  (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_FROM, OMP_CLAUSE_TO)->base.deprecated_flag)

#define OMP_CLAUSE_INIT_TARGET(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_INIT)->base.public_flag)
#define OMP_CLAUSE_INIT_TARGETSYNC(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_INIT)->base.deprecated_flag)
#define OMP_CLAUSE_INIT_PREFER_TYPE(NODE)				\
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (OMP_CLAUSE_CHECK (NODE),	\
					      OMP_CLAUSE_INIT,		\
					      OMP_CLAUSE_INIT), 1)

/* Nonzero if this map clause is for array (rather than pointer) based array
   section with zero bias.  Both the non-decl OMP_CLAUSE_MAP and corresponding
   OMP_CLAUSE_MAP with GOMP_MAP_POINTER are marked with this flag.  */
#define OMP_CLAUSE_MAP_ZERO_BIAS_ARRAY_SECTION(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->base.public_flag)
/* Nonzero if this is a mapped array section, that might need special
   treatment if OMP_CLAUSE_SIZE is zero.  */
#define OMP_CLAUSE_MAP_MAYBE_ZERO_LENGTH_ARRAY_SECTION(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP))
/* Nonzero if this map clause is for an OpenACC compute construct's reduction
   variable or OpenMP map clause mentioned also in in_reduction clause on the
   same construct.  */
#define OMP_CLAUSE_MAP_IN_REDUCTION(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP))
/* Nonzero on map clauses added implicitly for reduction clauses on combined
   or composite constructs.  They shall be removed if there is an explicit
   map clause.  */
#define OMP_CLAUSE_MAP_IMPLICIT(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->base.default_def_flag)
/* Nonzero if this map clause is to be indicated to the runtime as 'implicit',
   due to being created through implicit data-mapping rules in the middle-end.
   NOTE: this is different than OMP_CLAUSE_MAP_IMPLICIT.  */
#define OMP_CLAUSE_MAP_RUNTIME_IMPLICIT_P(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->base.deprecated_flag)
/* Nonzero for an attach/detach node whose decl was explicitly mapped on the
   same directive.  */
#define OMP_CLAUSE_ATTACHMENT_MAPPING_ERASED(NODE) \
  TREE_STATIC (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP))
/* Nonzero if this is a release/delete node which refers to a (Fortran) array
   descriptor.  */
#define OMP_CLAUSE_RELEASE_DESCRIPTOR(NODE) \
  TREE_NOTHROW (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP))

/* Flag that 'OMP_CLAUSE_DECL (NODE)' is to be made addressable during OMP
   lowering.  */
#define OMP_CLAUSE_MAP_DECL_MAKE_ADDRESSABLE(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP)->base.addressable_flag)

/* Nonzero if OpenACC 'readonly' modifier set, used for 'copyin'.  */
#define OMP_CLAUSE_MAP_READONLY(NODE) \
  TREE_READONLY (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_MAP))

/* Same as above, for use in OpenACC cache directives.  */
#define OMP_CLAUSE__CACHE__READONLY(NODE) \
  TREE_READONLY (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE__CACHE_))

/* True on an OMP_CLAUSE_USE_DEVICE_PTR with an OpenACC 'if_present'
   clause.  */
#define OMP_CLAUSE_USE_DEVICE_PTR_IF_PRESENT(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_USE_DEVICE_PTR)->base.public_flag)

#define OMP_CLAUSE_PROC_BIND_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_PROC_BIND)->omp_clause.subcode.proc_bind_kind)

#define OMP_CLAUSE_DEVICE_TYPE_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEVICE_TYPE)->omp_clause.subcode.device_type_kind)

#define OMP_CLAUSE_INDIRECT_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_INDIRECT), 0)


/* True if there is a device clause with a device-modifier 'ancestor'.  */
#define OMP_CLAUSE_DEVICE_ANCESTOR(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEVICE)->base.public_flag)

#define OMP_CLAUSE_COLLAPSE_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_COLLAPSE), 0)
#define OMP_CLAUSE_COLLAPSE_ITERVAR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_COLLAPSE), 1)
#define OMP_CLAUSE_COLLAPSE_COUNT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_COLLAPSE), 2)

#define OMP_CLAUSE_ORDERED_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ORDERED), 0)

/* True on an OMP_CLAUSE_ORDERED if stand-alone ordered construct is nested
   inside of work-sharing loop the clause is on.  */
#define OMP_CLAUSE_ORDERED_DOACROSS(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ORDERED)->base.public_flag)

/* True for unconstrained modifier on order(concurrent) clause.  */
#define OMP_CLAUSE_ORDER_UNCONSTRAINED(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ORDER)->base.public_flag)
/* True for reproducible modifier on order(concurrent) clause.  */
#define OMP_CLAUSE_ORDER_REPRODUCIBLE(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ORDER))

#define OMP_CLAUSE_REDUCTION_CODE(NODE)	\
  (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
     OMP_CLAUSE_IN_REDUCTION)->omp_clause.subcode.reduction_code)
#define OMP_CLAUSE_REDUCTION_INIT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
					      OMP_CLAUSE_IN_REDUCTION), 1)
#define OMP_CLAUSE_REDUCTION_MERGE(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
					      OMP_CLAUSE_IN_REDUCTION), 2)
#define OMP_CLAUSE_REDUCTION_GIMPLE_INIT(NODE) \
  (OMP_CLAUSE_CHECK (NODE))->omp_clause.gimple_reduction_init
#define OMP_CLAUSE_REDUCTION_GIMPLE_MERGE(NODE) \
  (OMP_CLAUSE_CHECK (NODE))->omp_clause.gimple_reduction_merge
#define OMP_CLAUSE_REDUCTION_PLACEHOLDER(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
					      OMP_CLAUSE_IN_REDUCTION), 3)
#define OMP_CLAUSE_REDUCTION_DECL_PLACEHOLDER(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
					      OMP_CLAUSE_IN_REDUCTION), 4)

/* True if a REDUCTION clause may reference the original list item (omp_orig)
   in its OMP_CLAUSE_REDUCTION_{,GIMPLE_}INIT.  */
#define OMP_CLAUSE_REDUCTION_OMP_ORIG_REF(NODE) \
  (OMP_CLAUSE_RANGE_CHECK (NODE, OMP_CLAUSE_REDUCTION, \
			   OMP_CLAUSE_IN_REDUCTION)->base.public_flag)

/* True if a REDUCTION clause has task reduction-modifier.  */
#define OMP_CLAUSE_REDUCTION_TASK(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_REDUCTION))

/* True if a REDUCTION clause has inscan reduction-modifier.  */
#define OMP_CLAUSE_REDUCTION_INSCAN(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_REDUCTION))

/* True if a LINEAR clause doesn't need copy in.  True for iterator vars which
   are always initialized inside of the loop construct, false otherwise.  */
#define OMP_CLAUSE_LINEAR_NO_COPYIN(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR)->base.public_flag)

/* True if a LINEAR clause doesn't need copy out.  True for iterator vars which
   are declared inside of the simd construct.  */
#define OMP_CLAUSE_LINEAR_NO_COPYOUT(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR))

/* True if a LINEAR clause has a stride that is variable.  */
#define OMP_CLAUSE_LINEAR_VARIABLE_STRIDE(NODE) \
  TREE_PROTECTED (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR))

/* True for a LINEAR clause with old style modifier syntax
   linear(modifier(list)) or linear(modifier(list):step).  */
#define OMP_CLAUSE_LINEAR_OLD_LINEAR_MODIFIER(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR)->base.addressable_flag)

/* True if a LINEAR clause is for an array or allocatable variable that
   needs special handling by the frontend.  */
#define OMP_CLAUSE_LINEAR_ARRAY(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR)->base.deprecated_flag)

#define OMP_CLAUSE_LINEAR_STEP(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR), 1)

#define OMP_CLAUSE_LINEAR_STMT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR), 2)

#define OMP_CLAUSE_LINEAR_GIMPLE_SEQ(NODE) \
  (OMP_CLAUSE_CHECK (NODE))->omp_clause.gimple_reduction_init

#define OMP_CLAUSE_LINEAR_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_LINEAR)->omp_clause.subcode.linear_kind)

#define OMP_CLAUSE_ALIGNED_ALIGNMENT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ALIGNED), 1)

#define OMP_CLAUSE_ALLOCATE_ALLOCATOR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ALLOCATE), 1)

#define OMP_CLAUSE_ALLOCATE_ALIGN(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ALLOCATE), 2)

/* True if an ALLOCATE clause was present on a combined or composite
   construct and the code for splitting the clauses has already performed
   checking if the listed variable has explicit privatization on the
   construct.  */
#define OMP_CLAUSE_ALLOCATE_COMBINED(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ALLOCATE)->base.public_flag)

#define OMP_CLAUSE_NUM_TEAMS_UPPER_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_TEAMS), 0)

#define OMP_CLAUSE_NUM_TEAMS_LOWER_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_NUM_TEAMS), 1)

#define OMP_CLAUSE_THREAD_LIMIT_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, \
						OMP_CLAUSE_THREAD_LIMIT), 0)

#define OMP_CLAUSE_DEVICE_ID(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEVICE), 0)

#define OMP_CLAUSE_DIST_SCHEDULE_CHUNK_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, \
						OMP_CLAUSE_DIST_SCHEDULE), 0)

#define OMP_CLAUSE_SAFELEN_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SAFELEN), 0)

#define OMP_CLAUSE_SIMDLEN_EXPR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SIMDLEN), 0)

#define OMP_CLAUSE__SIMDUID__DECL(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE__SIMDUID_), 0)

#define OMP_CLAUSE_SCHEDULE_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SCHEDULE)->omp_clause.subcode.schedule_kind)

/* True if a SCHEDULE clause has the simd modifier on it.  */
#define OMP_CLAUSE_SCHEDULE_SIMD(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_SCHEDULE)->base.public_flag)

#define OMP_CLAUSE_DEFAULT_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEFAULT)->omp_clause.subcode.default_kind)

#define OMP_CLAUSE_DEFAULTMAP_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_DEFAULTMAP)->omp_clause.subcode.defaultmap_kind)
#define OMP_CLAUSE_DEFAULTMAP_CATEGORY(NODE) \
  ((enum omp_clause_defaultmap_kind) \
   (OMP_CLAUSE_DEFAULTMAP_KIND (NODE) & OMP_CLAUSE_DEFAULTMAP_CATEGORY_MASK))
#define OMP_CLAUSE_DEFAULTMAP_BEHAVIOR(NODE) \
  ((enum omp_clause_defaultmap_kind) \
   (OMP_CLAUSE_DEFAULTMAP_KIND (NODE) & OMP_CLAUSE_DEFAULTMAP_MASK))
#define OMP_CLAUSE_DEFAULTMAP_SET_KIND(NODE, BEHAVIOR, CATEGORY) \
  (OMP_CLAUSE_DEFAULTMAP_KIND (NODE) \
   = (enum omp_clause_defaultmap_kind) (CATEGORY | BEHAVIOR))

#define OMP_CLAUSE_BIND_KIND(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_BIND)->omp_clause.subcode.bind_kind)

/* True if ENTER clause is spelled as TO.  */
#define OMP_CLAUSE_ENTER_TO(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_ENTER)->base.public_flag)

#define OMP_CLAUSE_TILE_LIST(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_TILE), 0)
#define OMP_CLAUSE_TILE_ITERVAR(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_TILE), 1)
#define OMP_CLAUSE_TILE_COUNT(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE_TILE), 2)

/* _CONDTEMP_ holding temporary with iteration count.  */
#define OMP_CLAUSE__CONDTEMP__ITER(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE__CONDTEMP_)->base.public_flag)

/* _SCANTEMP_ holding temporary with pointer to thread's local array;
   allocation.  */
#define OMP_CLAUSE__SCANTEMP__ALLOC(NODE) \
  (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE__SCANTEMP_)->base.public_flag)

/* _SCANTEMP_ holding temporary with a control variable for deallocation;
   one boolean_type_node for test whether alloca was used, another one
   to pass to __builtin_stack_restore or free.  */
#define OMP_CLAUSE__SCANTEMP__CONTROL(NODE) \
  TREE_PRIVATE (OMP_CLAUSE_SUBCODE_CHECK (NODE, OMP_CLAUSE__SCANTEMP_))

/* OpenMP OMP_NEXT_VARIANT accessors.  */
#define OMP_NEXT_VARIANT_INDEX(NODE)			\
  TREE_OPERAND (OMP_NEXT_VARIANT_CHECK (NODE), 0)
#define OMP_NEXT_VARIANT_STATE(NODE)			\
  TREE_OPERAND (OMP_NEXT_VARIANT_CHECK (NODE), 1)

/* OpenMP OMP_TARGET_DEVICE_MATCHES accessors.  */
#define OMP_TARGET_DEVICE_MATCHES_SELECTOR(NODE)	\
  TREE_OPERAND (OMP_TARGET_DEVICE_MATCHES_CHECK (NODE), 0)
#define OMP_TARGET_DEVICE_MATCHES_PROPERTIES(NODE)	\
  TREE_OPERAND (OMP_TARGET_DEVICE_MATCHES_CHECK (NODE), 1)

#define OMP_CLAUSE__MAPPER_BINDING__ID(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, \
			OMP_CLAUSE__MAPPER_BINDING_), 0)

#define OMP_CLAUSE__MAPPER_BINDING__DECL(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, \
			OMP_CLAUSE__MAPPER_BINDING_), 1)

#define OMP_CLAUSE__MAPPER_BINDING__MAPPER(NODE) \
  OMP_CLAUSE_OPERAND (OMP_CLAUSE_SUBCODE_CHECK (NODE, \
			OMP_CLAUSE__MAPPER_BINDING_), 2)

/* SSA_NAME accessors.  */

/* Whether SSA_NAME NODE is a virtual operand.  This simply caches the
   information in the underlying SSA_NAME_VAR for efficiency.  */
#define SSA_NAME_IS_VIRTUAL_OPERAND(NODE) \
  SSA_NAME_CHECK (NODE)->base.public_flag

/* Returns the IDENTIFIER_NODE giving the SSA name a name or NULL_TREE
   if there is no name associated with it.  */
#define SSA_NAME_IDENTIFIER(NODE)				\
  (SSA_NAME_CHECK (NODE)->ssa_name.var != NULL_TREE		\
   ? (TREE_CODE ((NODE)->ssa_name.var) == IDENTIFIER_NODE	\
      ? (NODE)->ssa_name.var					\
      : DECL_NAME ((NODE)->ssa_name.var))			\
   : NULL_TREE)

/* Returns the variable being referenced.  This can be NULL_TREE for
   temporaries not associated with any user variable.
   Once released, this is the only field that can be relied upon.  */
#define SSA_NAME_VAR(NODE)					\
  (SSA_NAME_CHECK (NODE)->ssa_name.var == NULL_TREE		\
   || TREE_CODE ((NODE)->ssa_name.var) == IDENTIFIER_NODE	\
   ? NULL_TREE : (NODE)->ssa_name.var)

#define SET_SSA_NAME_VAR_OR_IDENTIFIER(NODE,VAR) \
  do \
    { \
      tree var_ = (VAR); \
      SSA_NAME_CHECK (NODE)->ssa_name.var = var_; \
      SSA_NAME_IS_VIRTUAL_OPERAND (NODE) \
	= (var_ \
	   && TREE_CODE (var_) == VAR_DECL \
	   && VAR_DECL_IS_VIRTUAL_OPERAND (var_)); \
    } \
  while (0)

/* Returns the statement which defines this SSA name.  */
#define SSA_NAME_DEF_STMT(NODE)	SSA_NAME_CHECK (NODE)->ssa_name.def_stmt

/* Returns the SSA version number of this SSA name.  Note that in
   tree SSA, version numbers are not per variable and may be recycled.  */
#define SSA_NAME_VERSION(NODE)	SSA_NAME_CHECK (NODE)->base.u.version

/* Nonzero if this SSA name occurs in an abnormal PHI.  SSA_NAMES are
   never output, so we can safely use the ASM_WRITTEN_FLAG for this
   status bit.  */
#define SSA_NAME_OCCURS_IN_ABNORMAL_PHI(NODE) \
    SSA_NAME_CHECK (NODE)->base.asm_written_flag

/* Nonzero if this SSA_NAME expression is currently on the free list of
   SSA_NAMES.  Using NOTHROW_FLAG seems reasonably safe since throwing
   has no meaning for an SSA_NAME.  */
#define SSA_NAME_IN_FREE_LIST(NODE) \
    SSA_NAME_CHECK (NODE)->base.nothrow_flag

/* Nonzero if this SSA_NAME is the default definition for the
   underlying symbol.  A default SSA name is created for symbol S if
   the very first reference to S in the function is a read operation.
   Default definitions are always created by an empty statement and
   belong to no basic block.  */
#define SSA_NAME_IS_DEFAULT_DEF(NODE) \
    SSA_NAME_CHECK (NODE)->base.default_def_flag

/* Nonzero if this SSA_NAME is known to point to memory that may not
   be written to.  This is set for default defs of function parameters
   that have a corresponding r or R specification in the functions
   fn spec attribute.  This is used by alias analysis.  */
#define SSA_NAME_POINTS_TO_READONLY_MEMORY(NODE) \
    SSA_NAME_CHECK (NODE)->base.deprecated_flag

/* Attributes for SSA_NAMEs for pointer-type variables.  */
#define SSA_NAME_PTR_INFO(N) \
   SSA_NAME_CHECK (N)->ssa_name.info.ptr_info

/* Value range info attributes for SSA_NAMEs of non pointer-type variables.  */
#define SSA_NAME_RANGE_INFO(N) \
    SSA_NAME_CHECK (N)->ssa_name.info.range_info

/* Return the immediate_use information for an SSA_NAME. */
#define SSA_NAME_IMM_USE_NODE(NODE) SSA_NAME_CHECK (NODE)->ssa_name.imm_uses

#define OMP_CLAUSE_CODE(NODE)					\
	(OMP_CLAUSE_CHECK (NODE))->omp_clause.code

#define OMP_CLAUSE_SET_CODE(NODE, CODE)				\
	((OMP_CLAUSE_CHECK (NODE))->omp_clause.code = (CODE))

#define OMP_CLAUSE_OPERAND(NODE, I)				\
	OMP_CLAUSE_ELT_CHECK (NODE, I)

/* True if the clause decl NODE contains an OpenMP iterator.  */
#define OMP_ITERATOR_DECL_P(NODE) \
	(TREE_CODE (NODE) == TREE_LIST				\
	 && TREE_PURPOSE (NODE)					\
	 && TREE_CODE (TREE_PURPOSE (NODE)) == TREE_VEC)

/* In a BLOCK (scope) node:
   Variables declared in the scope NODE.  */
#define BLOCK_VARS(NODE) (BLOCK_CHECK (NODE)->block.vars)
#define BLOCK_NONLOCALIZED_VARS(NODE) \
  (BLOCK_CHECK (NODE)->block.nonlocalized_vars)
#define BLOCK_NUM_NONLOCALIZED_VARS(NODE) \
  vec_safe_length (BLOCK_NONLOCALIZED_VARS (NODE))
#define BLOCK_NONLOCALIZED_VAR(NODE,N) (*BLOCK_NONLOCALIZED_VARS (NODE))[N]
/* A chain of BLOCKs (scopes) nested within the scope NODE.  */
#define BLOCK_SUBBLOCKS(NODE) (BLOCK_CHECK (NODE)->block.subblocks)
/* The scope enclosing the scope NODE, or FUNCTION_DECL for the "outermost"
   function scope.  Inlined functions are chained by this so that given
   expression E and its TREE_BLOCK(E) B, BLOCK_SUPERCONTEXT(B) is the scope
   in which E has been made or into which E has been inlined.   */
#define BLOCK_SUPERCONTEXT(NODE) (BLOCK_CHECK (NODE)->block.supercontext)
/* Points to the next scope at the same level of nesting as scope NODE.  */
#define BLOCK_CHAIN(NODE) (BLOCK_CHECK (NODE)->block.chain)
/* A BLOCK, or FUNCTION_DECL of the function from which a block has been
   inlined.  In a scope immediately enclosing an inlined leaf expression,
   points to the outermost scope into which it has been inlined (thus
   bypassing all intermediate BLOCK_SUPERCONTEXTs). */
#define BLOCK_ABSTRACT_ORIGIN(NODE) (BLOCK_CHECK (NODE)->block.abstract_origin)
#define BLOCK_ORIGIN(NODE) \
  (BLOCK_ABSTRACT_ORIGIN(NODE) ? BLOCK_ABSTRACT_ORIGIN(NODE) : (NODE))
#define BLOCK_DIE(NODE) (BLOCK_CHECK (NODE)->block.die)

/* True if BLOCK has the same ranges as its BLOCK_SUPERCONTEXT.  */
#define BLOCK_SAME_RANGE(NODE) (BLOCK_CHECK (NODE)->base.u.bits.nameless_flag)

/* True if BLOCK appears in cold section.  */
#define BLOCK_IN_COLD_SECTION_P(NODE) \
  (BLOCK_CHECK (NODE)->base.u.bits.atomic_flag)

/* An index number for this block.  These values are not guaranteed to
   be unique across functions -- whether or not they are depends on
   the debugging output format in use.  */
#define BLOCK_NUMBER(NODE) (BLOCK_CHECK (NODE)->block.block_num)

/* If block reordering splits a lexical block into discontiguous
   address ranges, we'll make a copy of the original block.

   Note that this is logically distinct from BLOCK_ABSTRACT_ORIGIN.
   In that case, we have one source block that has been replicated
   (through inlining or unrolling) into many logical blocks, and that
   these logical blocks have different physical variables in them.

   In this case, we have one logical block split into several
   non-contiguous address ranges.  Most debug formats can't actually
   represent this idea directly, so we fake it by creating multiple
   logical blocks with the same variables in them.  However, for those
   that do support non-contiguous regions, these allow the original
   logical block to be reconstructed, along with the set of address
   ranges.

   One of the logical block fragments is arbitrarily chosen to be
   the ORIGIN.  The other fragments will point to the origin via
   BLOCK_FRAGMENT_ORIGIN; the origin itself will have this pointer
   be null.  The list of fragments will be chained through
   BLOCK_FRAGMENT_CHAIN from the origin.  */

#define BLOCK_FRAGMENT_ORIGIN(NODE) (BLOCK_CHECK (NODE)->block.fragment_origin)
#define BLOCK_FRAGMENT_CHAIN(NODE) (BLOCK_CHECK (NODE)->block.fragment_chain)

/* For an inlined function, this gives the location where it was called
   from.  This is only set in the top level block, which corresponds to the
   inlined function scope.  This is used in the debug output routines.  */

#define BLOCK_SOURCE_LOCATION(NODE) (BLOCK_CHECK (NODE)->block.locus)

/* This gives the location of the end of the block, useful to attach
   code implicitly generated for outgoing paths.  */

#define BLOCK_SOURCE_END_LOCATION(NODE) (BLOCK_CHECK (NODE)->block.end_locus)

/* Define fields and accessors for nodes representing data types.  */

/* See tree.def for documentation of the use of these fields.
   Look at the documentation of the various ..._TYPE tree codes.

   Note that the type.values, type.minval, and type.maxval fields are
   overloaded and used for different macros in different kinds of types.
   Each macro must check to ensure the tree node is of the proper kind of
   type.  Note also that some of the front-ends also overload these fields,
   so they must be checked as well.  */

#define TYPE_UID(NODE) (TYPE_CHECK (NODE)->type_common.uid)
/* Type size in bits as a tree expression.  Need not be constant and may
   be greater than TYPE_SIZE for a C++ FIELD_DECL representing a base
   class subobject with its own virtual base classes (which are laid out
   separately).  */
#define TYPE_SIZE(NODE) (TYPE_CHECK (NODE)->type_common.size)
/* Likewise, type size in bytes.  */
#define TYPE_SIZE_UNIT(NODE) (TYPE_CHECK (NODE)->type_common.size_unit)
#define TYPE_POINTER_TO(NODE) (TYPE_CHECK (NODE)->type_common.pointer_to)
#define TYPE_REFERENCE_TO(NODE) (TYPE_CHECK (NODE)->type_common.reference_to)
#define TYPE_PRECISION(NODE) \
  (TREE_NOT_CHECK (TYPE_CHECK (NODE), VECTOR_TYPE)->type_common.precision)
#define TYPE_PRECISION_RAW(NODE) (TYPE_CHECK (NODE)->type_common.precision)
#define TYPE_NAME(NODE) (TYPE_CHECK (NODE)->type_common.name)
#define TYPE_NEXT_VARIANT(NODE) (TYPE_CHECK (NODE)->type_common.next_variant)
#define TYPE_MAIN_VARIANT(NODE) (TYPE_CHECK (NODE)->type_common.main_variant)
#define TYPE_CONTEXT(NODE) (TYPE_CHECK (NODE)->type_common.context)

#define TYPE_MODE_RAW(NODE) (TYPE_CHECK (NODE)->type_common.mode)
#define TYPE_MODE(NODE) \
  (VECTOR_TYPE_P (TYPE_CHECK (NODE)) \
   ? vector_type_mode (NODE) : (NODE)->type_common.mode)
#define SCALAR_TYPE_MODE(NODE) \
  (as_a <scalar_mode> (TYPE_CHECK (NODE)->type_common.mode))
#define SCALAR_INT_TYPE_MODE(NODE) \
  (as_a <scalar_int_mode> (TYPE_CHECK (NODE)->type_common.mode))
#define SCALAR_FLOAT_TYPE_MODE(NODE) \
  (as_a <scalar_float_mode> (TYPE_CHECK (NODE)->type_common.mode))
#define SET_TYPE_MODE(NODE, MODE) \
  (TYPE_CHECK (NODE)->type_common.mode = (MODE))

extern unsigned int element_precision (const_tree);
extern machine_mode element_mode (const_tree);
extern machine_mode vector_type_mode (const_tree);
extern unsigned int vector_element_bits (const_tree);
extern tree vector_element_bits_tree (const_tree);

/* The "canonical" type for this type node, which is used by frontends to
   compare the type for equality with another type.  If two types are
   equal (based on the semantics of the language), then they will have
   equivalent TYPE_CANONICAL entries.

   As a special case, if TYPE_CANONICAL is NULL_TREE, and thus
   TYPE_STRUCTURAL_EQUALITY_P is true, then it cannot
   be used for comparison against other types.  Instead, the type is
   said to require structural equality checks, described in
   TYPE_STRUCTURAL_EQUALITY_P.

   For unqualified aggregate and function types the middle-end relies on
   TYPE_CANONICAL to tell whether two variables can be assigned
   to each other without a conversion.  The middle-end also makes sure
   to assign the same alias-sets to the type partition with equal
   TYPE_CANONICAL of their unqualified variants.  */
#define TYPE_CANONICAL(NODE) (TYPE_CHECK (NODE)->type_common.canonical)
/* Indicates that the type node requires structural equality
   checks.  The compiler will need to look at the composition of the
   type to determine whether it is equal to another type, rather than
   just comparing canonical type pointers.  For instance, we would need
   to look at the return and parameter types of a FUNCTION_TYPE
   node.  */
#define TYPE_STRUCTURAL_EQUALITY_P(NODE) (TYPE_CANONICAL (NODE) == NULL_TREE)
/* Sets the TYPE_CANONICAL field to NULL_TREE, indicating that the
   type node requires structural equality.  */
#define SET_TYPE_STRUCTURAL_EQUALITY(NODE) (TYPE_CANONICAL (NODE) = NULL_TREE)

#define TYPE_IBIT(NODE) (GET_MODE_IBIT (TYPE_MODE (NODE)))
#define TYPE_FBIT(NODE) (GET_MODE_FBIT (TYPE_MODE (NODE)))

/* The (language-specific) typed-based alias set for this type.
   Objects whose TYPE_ALIAS_SETs are different cannot alias each
   other.  If the TYPE_ALIAS_SET is -1, no alias set has yet been
   assigned to this type.  If the TYPE_ALIAS_SET is 0, objects of this
   type can alias objects of any type.  */
#define TYPE_ALIAS_SET(NODE) (TYPE_CHECK (NODE)->type_common.alias_set)

/* Nonzero iff the typed-based alias set for this type has been
   calculated.  */
#define TYPE_ALIAS_SET_KNOWN_P(NODE) \
  (TYPE_CHECK (NODE)->type_common.alias_set != -1)

/* A TREE_LIST of IDENTIFIER nodes of the attributes that apply
   to this type.  */
#define TYPE_ATTRIBUTES(NODE) (TYPE_CHECK (NODE)->type_common.attributes)

/* Raw access to the alignment field.  */
#define TYPE_ALIGN_RAW(NODE)			\
  (TYPE_CHECK (NODE)->type_common.align)

/* The alignment necessary for objects of this type.
   The value is an int, measured in bits and must be a power of two.
   We support also an "alignment" of zero.  */
#define TYPE_ALIGN(NODE)					\
  (TYPE_ALIGN_RAW (NODE)					\
   ? ((unsigned)1) << (TYPE_ALIGN_RAW(NODE) - 1) : 0)

/* Specify that TYPE_ALIGN(NODE) is X.  */
#define SET_TYPE_ALIGN(NODE, X) \
    (TYPE_CHECK (NODE)->type_common.align = ffs_hwi (X))

/* 1 if the alignment for this type was requested by "aligned" attribute,
   0 if it is the default for this type.  */
#define TYPE_USER_ALIGN(NODE) (TYPE_CHECK (NODE)->base.u.bits.user_align)

/* The alignment for NODE, in bytes.  */
#define TYPE_ALIGN_UNIT(NODE) (TYPE_ALIGN (NODE) / BITS_PER_UNIT)

/* The minimum alignment necessary for objects of this type without
   warning.  The value is an int, measured in bits.  */
#define TYPE_WARN_IF_NOT_ALIGN_RAW(NODE) \
    (TYPE_CHECK (NODE)->type_common.warn_if_not_align)
#define TYPE_WARN_IF_NOT_ALIGN(NODE) \
    (TYPE_WARN_IF_NOT_ALIGN_RAW (NODE) \
     ? ((unsigned)1) << (TYPE_WARN_IF_NOT_ALIGN_RAW (NODE) - 1) : 0)

/* Specify that TYPE_WARN_IF_NOT_ALIGN(NODE) is X.  */
#define SET_TYPE_WARN_IF_NOT_ALIGN(NODE, X) \
    (TYPE_WARN_IF_NOT_ALIGN_RAW (NODE) = ffs_hwi (X))

/* If your language allows you to declare types, and you want debug info
   for them, then you need to generate corresponding TYPE_DECL nodes.
   These "stub" TYPE_DECL nodes have no name, and simply point at the
   type node.  You then set the TYPE_STUB_DECL field of the type node
   to point back at the TYPE_DECL node.  This allows the debug routines
   to know that the two nodes represent the same type, so that we only
   get one debug info record for them.  */
#define TYPE_STUB_DECL(NODE) (TREE_CHAIN (TYPE_CHECK (NODE)))

/* In a RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE or ARRAY_TYPE, it means
   the type has BLKmode only because it lacks the alignment required for
   its size.  */
#define TYPE_NO_FORCE_BLK(NODE) \
  (TYPE_CHECK (NODE)->type_common.no_force_blk_flag)

/* Nonzero in a type considered volatile as a whole.  */
#define TYPE_VOLATILE(NODE) (TYPE_CHECK (NODE)->base.volatile_flag)

/* Nonzero in a type considered atomic as a whole.  */
#define TYPE_ATOMIC(NODE) (TYPE_CHECK (NODE)->base.u.bits.atomic_flag)

/* Means this type is const-qualified.  */
#define TYPE_READONLY(NODE) (TYPE_CHECK (NODE)->base.readonly_flag)

/* If nonzero, this type is `restrict'-qualified, in the C sense of
   the term.  */
#define TYPE_RESTRICT(NODE) (TYPE_CHECK (NODE)->type_common.restrict_flag)

/* If nonzero, type's name shouldn't be emitted into debug info.  */
#define TYPE_NAMELESS(NODE) (TYPE_CHECK (NODE)->base.u.bits.nameless_flag)

/* The address space the type is in.  */
#define TYPE_ADDR_SPACE(NODE) (TYPE_CHECK (NODE)->base.u.bits.address_space)

/* Encode/decode the named memory support as part of the qualifier.  If more
   than 8 qualifiers are added, these macros need to be adjusted.  */
#define ENCODE_QUAL_ADDR_SPACE(NUM) ((NUM & 0xFF) << 8)
#define DECODE_QUAL_ADDR_SPACE(X) (((X) >> 8) & 0xFF)

/* Return all qualifiers except for the address space qualifiers.  */
#define CLEAR_QUAL_ADDR_SPACE(X) ((X) & ~0xFF00)

/* Only keep the address space out of the qualifiers and discard the other
   qualifiers.  */
#define KEEP_QUAL_ADDR_SPACE(X) ((X) & 0xFF00)

/* The set of type qualifiers for this type.  */
#define TYPE_QUALS(NODE)					\
  ((int) ((TYPE_READONLY (NODE) * TYPE_QUAL_CONST)		\
	  | (TYPE_VOLATILE (NODE) * TYPE_QUAL_VOLATILE)		\
	  | (TYPE_ATOMIC (NODE) * TYPE_QUAL_ATOMIC)		\
	  | (TYPE_RESTRICT (NODE) * TYPE_QUAL_RESTRICT)		\
	  | (ENCODE_QUAL_ADDR_SPACE (TYPE_ADDR_SPACE (NODE)))))

/* The same as TYPE_QUALS without the address space qualifications.  */
#define TYPE_QUALS_NO_ADDR_SPACE(NODE)				\
  ((int) ((TYPE_READONLY (NODE) * TYPE_QUAL_CONST)		\
	  | (TYPE_VOLATILE (NODE) * TYPE_QUAL_VOLATILE)		\
	  | (TYPE_ATOMIC (NODE) * TYPE_QUAL_ATOMIC)		\
	  | (TYPE_RESTRICT (NODE) * TYPE_QUAL_RESTRICT)))

/* The same as TYPE_QUALS without the address space and atomic
   qualifications.  */
#define TYPE_QUALS_NO_ADDR_SPACE_NO_ATOMIC(NODE)		\
  ((int) ((TYPE_READONLY (NODE) * TYPE_QUAL_CONST)		\
	  | (TYPE_VOLATILE (NODE) * TYPE_QUAL_VOLATILE)		\
	  | (TYPE_RESTRICT (NODE) * TYPE_QUAL_RESTRICT)))

/* These flags are available for each language front end to use internally.  */
#define TYPE_LANG_FLAG_0(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_0)
#define TYPE_LANG_FLAG_1(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_1)
#define TYPE_LANG_FLAG_2(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_2)
#define TYPE_LANG_FLAG_3(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_3)
#define TYPE_LANG_FLAG_4(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_4)
#define TYPE_LANG_FLAG_5(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_5)
#define TYPE_LANG_FLAG_6(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_6)
#define TYPE_LANG_FLAG_7(NODE) (TYPE_CHECK (NODE)->type_common.lang_flag_7)

/* Used to keep track of visited nodes in tree traversals.  This is set to
   0 by copy_node and make_node.  */
#define TREE_VISITED(NODE) ((NODE)->base.visited)

/* If set in an ARRAY_TYPE, indicates a string type (for languages
   that distinguish string from array of char).
   If set in a INTEGER_TYPE, indicates a character type.  */
#define TYPE_STRING_FLAG(NODE) \
	(ARRAY_OR_INTEGER_TYPE_CHECK (NODE)->type_common.string_flag)

/* If set for RECORD_TYPE or UNION_TYPE it indicates that the type conforms
   to the C++ one definition rule.  This is used for LTO canonical type
   computation.  */
#define TYPE_CXX_ODR_P(NODE) \
	(RECORD_OR_UNION_CHECK (NODE)->type_common.string_flag)

/* Nonzero in a VECTOR_TYPE if the frontends should not emit warnings
   about missing conversions to other vector types of the same size.  */
#define TYPE_VECTOR_OPAQUE(NODE) \
  (VECTOR_TYPE_CHECK (NODE)->base.default_def_flag)

/* Indicates that objects of this type must be initialized by calling a
   function when they are created.  */
#define TYPE_NEEDS_CONSTRUCTING(NODE) \
  (TYPE_CHECK (NODE)->type_common.needs_constructing_flag)

/* Indicates that a UNION_TYPE object should be passed the same way that
   the first union alternative would be passed, or that a RECORD_TYPE
   object should be passed the same way that the first (and only) member
   would be passed.  */
#define TYPE_TRANSPARENT_AGGR(NODE) \
  (RECORD_OR_UNION_CHECK (NODE)->type_common.transparent_aggr_flag)

/* For an ARRAY_TYPE, indicates that it is not permitted to take the
   address of a component of the type.  This is the counterpart of
   DECL_NONADDRESSABLE_P for arrays, see the definition of this flag.  */
#define TYPE_NONALIASED_COMPONENT(NODE) \
  (ARRAY_TYPE_CHECK (NODE)->type_common.transparent_aggr_flag)

/* For an ARRAY_TYPE, a RECORD_TYPE, a UNION_TYPE or a QUAL_UNION_TYPE
   whether the array is typeless storage or the type contains a member
   with this flag set.  Such types are exempt from type-based alias
   analysis.  For ARRAY_TYPEs with AGGREGATE_TYPE_P element types
   the flag should be inherited from the element type, can change
   when type is finalized and because of that should not be used in
   type hashing.  For ARRAY_TYPEs with non-AGGREGATE_TYPE_P element types
   the flag should not be changed after the array is created and should
   be used in type hashing.  */
#define TYPE_TYPELESS_STORAGE(NODE) \
  (TREE_CHECK4 (NODE, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE, \
		ARRAY_TYPE)->type_common.typeless_storage)

/* Indicated that objects of this type should be laid out in as
   compact a way as possible.  */
#define TYPE_PACKED(NODE) (TYPE_CHECK (NODE)->base.u.bits.packed_flag)

/* Used by type_contains_placeholder_p to avoid recomputation.
   Values are: 0 (unknown), 1 (false), 2 (true).  Never access
   this field directly.  */
#define TYPE_CONTAINS_PLACEHOLDER_INTERNAL(NODE) \
  (TYPE_CHECK (NODE)->type_common.contains_placeholder_bits)

/* Nonzero if RECORD_TYPE represents a final derivation of class.  */
#define TYPE_FINAL_P(NODE) \
  (RECORD_OR_UNION_CHECK (NODE)->base.default_def_flag)

/* The debug output functions use the symtab union field to store
   information specific to the debugging format.  The different debug
   output hooks store different types in the union field.  These three
   macros are used to access different fields in the union.  The debug
   hooks are responsible for consistently using only a specific
   macro.  */

/* Symtab field as an integer.  Used by stabs generator in dbxout.cc to
   hold the type's number in the generated stabs.  */
#define TYPE_SYMTAB_ADDRESS(NODE) \
  (TYPE_CHECK (NODE)->type_common.symtab.address)

/* Symtab field as a pointer to a DWARF DIE.  Used by DWARF generator
   in dwarf2out.cc to point to the DIE generated for the type.  */
#define TYPE_SYMTAB_DIE(NODE) \
  (TYPE_CHECK (NODE)->type_common.symtab.die)

/* The garbage collector needs to know the interpretation of the
   symtab field.  These constants represent the different types in the
   union.  */

#define TYPE_SYMTAB_IS_ADDRESS (0)
#define TYPE_SYMTAB_IS_DIE (1)

#define TYPE_LANG_SPECIFIC(NODE) \
  (TYPE_CHECK (NODE)->type_with_lang_specific.lang_specific)

#define TYPE_VALUES(NODE) (ENUMERAL_TYPE_CHECK (NODE)->type_non_common.values)
#define TYPE_DOMAIN(NODE) (ARRAY_TYPE_CHECK (NODE)->type_non_common.values)
#define TYPE_FIELDS(NODE)				\
  (RECORD_OR_UNION_CHECK (NODE)->type_non_common.values)
#define TYPE_CACHED_VALUES(NODE) (TYPE_CHECK (NODE)->type_non_common.values)
#define TYPE_ARG_TYPES(NODE)				\
  (FUNC_OR_METHOD_CHECK (NODE)->type_non_common.values)
#define TYPE_VALUES_RAW(NODE) (TYPE_CHECK (NODE)->type_non_common.values)

#define TYPE_MIN_VALUE(NODE)				\
  (NUMERICAL_TYPE_CHECK (NODE)->type_non_common.minval)
#define TYPE_NEXT_PTR_TO(NODE)				\
  (POINTER_TYPE_CHECK (NODE)->type_non_common.minval)
#define TYPE_NEXT_REF_TO(NODE)				\
  (REFERENCE_TYPE_CHECK (NODE)->type_non_common.minval)
#define TYPE_VFIELD(NODE)				\
  (RECORD_OR_UNION_CHECK (NODE)->type_non_common.minval)
#define TYPE_MIN_VALUE_RAW(NODE) (TYPE_CHECK (NODE)->type_non_common.minval)

#define TYPE_MAX_VALUE(NODE) \
  (NUMERICAL_TYPE_CHECK (NODE)->type_non_common.maxval)
#define TYPE_METHOD_BASETYPE(NODE)			\
  (FUNC_OR_METHOD_CHECK (NODE)->type_non_common.maxval)
#define TYPE_OFFSET_BASETYPE(NODE)			\
  (OFFSET_TYPE_CHECK (NODE)->type_non_common.maxval)
/* If non-NULL, this is an upper bound of the size (in bytes) of an
   object of the given ARRAY_TYPE_NON_COMMON.  This allows temporaries to be
   allocated.  */
#define TYPE_ARRAY_MAX_SIZE(ARRAY_TYPE) \
  (ARRAY_TYPE_CHECK (ARRAY_TYPE)->type_non_common.maxval)
#define TYPE_MAX_VALUE_RAW(NODE) (TYPE_CHECK (NODE)->type_non_common.maxval)
/* For record and union types, information about this type, as a base type
   for itself.  */
#define TYPE_BINFO(NODE) (RECORD_OR_UNION_CHECK (NODE)->type_non_common.maxval)

/* For types, used in a language-dependent way.  */
#define TYPE_LANG_SLOT_1(NODE) \
  (TYPE_CHECK (NODE)->type_non_common.lang_1)

/* Define accessor macros for information about type inheritance
   and basetypes.

   A "basetype" means a particular usage of a data type for inheritance
   in another type.  Each such basetype usage has its own "binfo"
   object to describe it.  The binfo object is a TREE_VEC node.

   Inheritance is represented by the binfo nodes allocated for a
   given type.  For example, given types C and D, such that D is
   inherited by C, 3 binfo nodes will be allocated: one for describing
   the binfo properties of C, similarly one for D, and one for
   describing the binfo properties of D as a base type for C.
   Thus, given a pointer to class C, one can get a pointer to the binfo
   of D acting as a basetype for C by looking at C's binfo's basetypes.  */

/* BINFO specific flags.  */

/* Nonzero means that the derivation chain is via a `virtual' declaration.  */
#define BINFO_VIRTUAL_P(NODE) (TREE_BINFO_CHECK (NODE)->base.static_flag)

/* Flags for language dependent use.  */
#define BINFO_FLAG_0(NODE) TREE_LANG_FLAG_0 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_1(NODE) TREE_LANG_FLAG_1 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_2(NODE) TREE_LANG_FLAG_2 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_3(NODE) TREE_LANG_FLAG_3 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_4(NODE) TREE_LANG_FLAG_4 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_5(NODE) TREE_LANG_FLAG_5 (TREE_BINFO_CHECK (NODE))
#define BINFO_FLAG_6(NODE) TREE_LANG_FLAG_6 (TREE_BINFO_CHECK (NODE))

/* The actual data type node being inherited in this basetype.  */
#define BINFO_TYPE(NODE) TREE_TYPE (TREE_BINFO_CHECK (NODE))

/* The offset where this basetype appears in its containing type.
   BINFO_OFFSET slot holds the offset (in bytes)
   from the base of the complete object to the base of the part of the
   object that is allocated on behalf of this `type'.
   This is always 0 except when there is multiple inheritance.  */

#define BINFO_OFFSET(NODE) (TREE_BINFO_CHECK (NODE)->binfo.offset)
#define BINFO_OFFSET_ZEROP(NODE) (integer_zerop (BINFO_OFFSET (NODE)))

/* The virtual function table belonging to this basetype.  Virtual
   function tables provide a mechanism for run-time method dispatching.
   The entries of a virtual function table are language-dependent.  */

#define BINFO_VTABLE(NODE) (TREE_BINFO_CHECK (NODE)->binfo.vtable)

/* The virtual functions in the virtual function table.  This is
   a TREE_LIST that is used as an initial approximation for building
   a virtual function table for this basetype.  */
#define BINFO_VIRTUALS(NODE) (TREE_BINFO_CHECK (NODE)->binfo.virtuals)

/* A vector of binfos for the direct basetypes inherited by this
   basetype.

   If this basetype describes type D as inherited in C, and if the
   basetypes of D are E and F, then this vector contains binfos for
   inheritance of E and F by C.  */
#define BINFO_BASE_BINFOS(NODE) (&TREE_BINFO_CHECK (NODE)->binfo.base_binfos)

/* The number of basetypes for NODE.  */
#define BINFO_N_BASE_BINFOS(NODE) (BINFO_BASE_BINFOS (NODE)->length ())

/* Accessor macro to get to the Nth base binfo of this binfo.  */
#define BINFO_BASE_BINFO(NODE,N) \
 ((*BINFO_BASE_BINFOS (NODE))[(N)])
#define BINFO_BASE_ITERATE(NODE,N,B) \
 (BINFO_BASE_BINFOS (NODE)->iterate ((N), &(B)))
#define BINFO_BASE_APPEND(NODE,T) \
 (BINFO_BASE_BINFOS (NODE)->quick_push ((T)))

/* For a BINFO record describing a virtual base class, i.e., one where
   TREE_VIA_VIRTUAL is set, this field assists in locating the virtual
   base.  The actual contents are language-dependent.  In the C++
   front-end this field is an INTEGER_CST giving an offset into the
   vtable where the offset to the virtual base can be found.  */
#define BINFO_VPTR_FIELD(NODE) (TREE_BINFO_CHECK (NODE)->binfo.vptr_field)

/* Indicates the accesses this binfo has to its bases. The values are
   access_public_node, access_protected_node or access_private_node.
   If this array is not present, public access is implied.  */
#define BINFO_BASE_ACCESSES(NODE) \
  (TREE_BINFO_CHECK (NODE)->binfo.base_accesses)

#define BINFO_BASE_ACCESS(NODE,N) \
  (*BINFO_BASE_ACCESSES (NODE))[(N)]
#define BINFO_BASE_ACCESS_APPEND(NODE,T) \
  BINFO_BASE_ACCESSES (NODE)->quick_push ((T))

/* The index in the VTT where this subobject's sub-VTT can be found.
   NULL_TREE if there is no sub-VTT.  */
#define BINFO_SUBVTT_INDEX(NODE) (TREE_BINFO_CHECK (NODE)->binfo.vtt_subvtt)

/* The index in the VTT where the vptr for this subobject can be
   found.  NULL_TREE if there is no secondary vptr in the VTT.  */
#define BINFO_VPTR_INDEX(NODE) (TREE_BINFO_CHECK (NODE)->binfo.vtt_vptr)

/* The BINFO_INHERITANCE_CHAIN points at the binfo for the base
   inheriting this base for non-virtual bases. For virtual bases it
   points either to the binfo for which this is a primary binfo, or to
   the binfo of the most derived type.  */
#define BINFO_INHERITANCE_CHAIN(NODE) \
	(TREE_BINFO_CHECK (NODE)->binfo.inheritance)


/* Define fields and accessors for nodes representing declared names.  */

/* Nonzero if DECL represents an SSA name or a variable that can possibly
   have an associated SSA name.  */
#define SSA_VAR_P(DECL)							\
	(TREE_CODE (DECL) == VAR_DECL					\
	 || TREE_CODE (DECL) == PARM_DECL				\
	 || TREE_CODE (DECL) == RESULT_DECL				\
	 || TREE_CODE (DECL) == SSA_NAME)


#define DECL_CHAIN(NODE) (TREE_CHAIN (DECL_MINIMAL_CHECK (NODE)))

/* This is the name of the object as written by the user.
   It is an IDENTIFIER_NODE.  */
#define DECL_NAME(NODE) (DECL_MINIMAL_CHECK (NODE)->decl_minimal.name)

/* The IDENTIFIER_NODE associated with the TYPE_NAME field.  */
#define TYPE_IDENTIFIER(NODE) \
  (TYPE_NAME (NODE) && DECL_P (TYPE_NAME (NODE)) \
   ? DECL_NAME (TYPE_NAME (NODE)) : TYPE_NAME (NODE))

/* Every ..._DECL node gets a unique number.  */
#define DECL_UID(NODE) (DECL_MINIMAL_CHECK (NODE)->decl_minimal.uid)

/* DEBUG_EXPR_DECLs get negative UID numbers, to catch erroneous
   uses.  */
#define DEBUG_TEMP_UID(NODE) (-DECL_UID (TREE_CHECK ((NODE), DEBUG_EXPR_DECL)))

/* Every ..._DECL node gets a unique number that stays the same even
   when the decl is copied by the inliner once it is set.  */
#define DECL_PT_UID(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.pt_uid == -1u \
   ? (NODE)->decl_minimal.uid : (NODE)->decl_common.pt_uid)
/* Initialize the ..._DECL node pt-uid to the decls uid.  */
#define SET_DECL_PT_UID(NODE, UID) \
  (DECL_COMMON_CHECK (NODE)->decl_common.pt_uid = (UID))
/* Whether the ..._DECL node pt-uid has been initialized and thus needs to
   be preserved when copyin the decl.  */
#define DECL_PT_UID_SET_P(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.pt_uid != -1u)

/* These two fields describe where in the source code the declaration
   was.  If the declaration appears in several places (as for a C
   function that is declared first and then defined later), this
   information should refer to the definition.  */
#define DECL_SOURCE_LOCATION(NODE) \
  (DECL_MINIMAL_CHECK (NODE)->decl_minimal.locus)
#define DECL_SOURCE_FILE(NODE) LOCATION_FILE (DECL_SOURCE_LOCATION (NODE))
#define DECL_SOURCE_LINE(NODE) LOCATION_LINE (DECL_SOURCE_LOCATION (NODE))
#define DECL_SOURCE_COLUMN(NODE) LOCATION_COLUMN (DECL_SOURCE_LOCATION (NODE))
/* This decl was created by a front-end or back-end rather than by
   user code, and has not been explicitly declared by the user -- when
   that happens the source location is updated to the user's
   source.  This includes decls with no location (!).  */
#define DECL_IS_UNDECLARED_BUILTIN(DECL) \
  (DECL_SOURCE_LOCATION (DECL) <= BUILTINS_LOCATION)

/*  For FIELD_DECLs, this is the RECORD_TYPE, UNION_TYPE, or
    QUAL_UNION_TYPE node that the field is a member of.  For VAR_DECL,
    PARM_DECL, FUNCTION_DECL, LABEL_DECL, RESULT_DECL, and CONST_DECL
    nodes, this points to either the FUNCTION_DECL for the containing
    function, the RECORD_TYPE or UNION_TYPE for the containing type, or
    NULL_TREE or a TRANSLATION_UNIT_DECL if the given decl has "file
    scope".  In particular, for VAR_DECLs which are virtual table pointers
    (they have DECL_VIRTUAL set), we use DECL_CONTEXT to determine the type
    they belong to.  */
#define DECL_CONTEXT(NODE) (DECL_MINIMAL_CHECK (NODE)->decl_minimal.context)
#define DECL_FIELD_CONTEXT(NODE) \
  (FIELD_DECL_CHECK (NODE)->decl_minimal.context)

/* If nonzero, decl's name shouldn't be emitted into debug info.  */
#define DECL_NAMELESS(NODE) (DECL_MINIMAL_CHECK (NODE)->base.u.bits.nameless_flag)

/* For any sort of a ..._DECL node, this points to the original (abstract)
   decl node which this decl is an inlined/cloned instance of, or else it
   is NULL indicating that this decl is not an instance of some other decl.

   The C front-end also uses this in a nested declaration of an inline
   function, to point back to the definition.  */
#define DECL_ABSTRACT_ORIGIN(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.abstract_origin)

/* Like DECL_ABSTRACT_ORIGIN, but returns NODE if there's no abstract
   origin.  This is useful when setting the DECL_ABSTRACT_ORIGIN.  */
#define DECL_ORIGIN(NODE) \
  (DECL_ABSTRACT_ORIGIN (NODE) ? DECL_ABSTRACT_ORIGIN (NODE) : (NODE))

/* Nonzero for any sort of ..._DECL node means this decl node represents an
   inline instance of some original (abstract) decl from an inline function;
   suppress any warnings about shadowing some other variable.  FUNCTION_DECL
   nodes can also have their abstract origin set to themselves.  */
#define DECL_FROM_INLINE(NODE) \
  (DECL_ABSTRACT_ORIGIN (NODE) != NULL_TREE \
   && DECL_ABSTRACT_ORIGIN (NODE) != (NODE))

/* In a DECL this is the field where attributes are stored.  */
#define DECL_ATTRIBUTES(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.attributes)

/* For a FUNCTION_DECL, holds the tree of BINDINGs.
   For a TRANSLATION_UNIT_DECL, holds the namespace's BLOCK.
   For a VAR_DECL, holds the initial value.
   For a PARM_DECL, used for DECL_ARG_TYPE--default
   values for parameters are encoded in the type of the function,
   not in the PARM_DECL slot.
   For a FIELD_DECL, this is used for enumeration values and the C
   frontend uses it for temporarily storing bitwidth of bitfields.

   ??? Need to figure out some way to check this isn't a PARM_DECL.  */
#define DECL_INITIAL(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.initial)

/* Holds the size of the datum, in bits, as a tree expression.
   Need not be constant and may be null.  May be less than TYPE_SIZE
   for a C++ FIELD_DECL representing a base class subobject with its
   own virtual base classes (which are laid out separately).  */
#define DECL_SIZE(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.size)
/* Likewise for the size in bytes.  */
#define DECL_SIZE_UNIT(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.size_unit)
#define DECL_ALIGN_RAW(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.align)
/* Returns the alignment required for the datum, in bits.  It must
   be a power of two, but an "alignment" of zero is supported
   (e.g. as "uninitialized" sentinel).  */
#define DECL_ALIGN(NODE)					\
  (DECL_ALIGN_RAW (NODE)					\
   ? ((unsigned)1) << (DECL_ALIGN_RAW (NODE) - 1) : 0)
/* Specify that DECL_ALIGN(NODE) is X.  */
#define SET_DECL_ALIGN(NODE, X) \
  (DECL_ALIGN_RAW (NODE) = ffs_hwi (X))

/* The minimum alignment necessary for the datum, in bits, without
   warning.  */
#define DECL_WARN_IF_NOT_ALIGN_RAW(NODE)			\
  (DECL_COMMON_CHECK (NODE)->decl_common.warn_if_not_align)
#define DECL_WARN_IF_NOT_ALIGN(NODE)					\
  (DECL_WARN_IF_NOT_ALIGN_RAW (NODE)					\
   ? ((unsigned)1) << (DECL_WARN_IF_NOT_ALIGN_RAW (NODE) - 1) : 0)

/* Specify that DECL_WARN_IF_NOT_ALIGN(NODE) is X.  */
#define SET_DECL_WARN_IF_NOT_ALIGN(NODE, X)		\
  (DECL_WARN_IF_NOT_ALIGN_RAW (NODE) = ffs_hwi (X))

/* The alignment of NODE, in bytes.  */
#define DECL_ALIGN_UNIT(NODE) (DECL_ALIGN (NODE) / BITS_PER_UNIT)
/* Set if the alignment of this DECL has been set by the user, for
   example with an 'aligned' attribute.  */
#define DECL_USER_ALIGN(NODE) \
  (DECL_COMMON_CHECK (NODE)->base.u.bits.user_align)
/* Holds the machine mode corresponding to the declaration of a variable or
   field.  Always equal to TYPE_MODE (TREE_TYPE (decl)) except for a
   FIELD_DECL.  */
#define DECL_MODE(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.mode)
#define SET_DECL_MODE(NODE, MODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.mode = (MODE))

/* For FUNCTION_DECL, if it is built-in, this identifies which built-in
   operation it is.  This is only intended for low-level accesses;
   normally DECL_FUNCTION_CODE, DECL_FE_FUNCTION_CODE or DECL_MD_FUNCTION
   should be used instead.  */
#define DECL_UNCHECKED_FUNCTION_CODE(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.function_code)

/* Test if FCODE is a function code for an alloca operation.  */
#define ALLOCA_FUNCTION_CODE_P(FCODE)				\
  ((FCODE) == BUILT_IN_ALLOCA					\
   || (FCODE) == BUILT_IN_ALLOCA_WITH_ALIGN			\
   || (FCODE) == BUILT_IN_ALLOCA_WITH_ALIGN_AND_MAX)

/* Generate case for an alloca operation.  */
#define CASE_BUILT_IN_ALLOCA			\
  case BUILT_IN_ALLOCA:				\
  case BUILT_IN_ALLOCA_WITH_ALIGN:		\
  case BUILT_IN_ALLOCA_WITH_ALIGN_AND_MAX

#define DECL_FUNCTION_PERSONALITY(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.personality)

/* Nonzero for a given ..._DECL node means that the name of this node should
   be ignored for symbolic debug purposes.  For a TYPE_DECL, this means that
   the associated type should be ignored.  For a FUNCTION_DECL, the body of
   the function should also be ignored.  */
#define DECL_IGNORED_P(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.ignored_flag)

/* Nonzero for a given ..._DECL node means that this node represents an
   "abstract instance" of the given declaration (e.g. in the original
   declaration of an inline function).  When generating symbolic debugging
   information, we mustn't try to generate any address information for nodes
   marked as "abstract instances" because we don't actually generate
   any code or allocate any data space for such instances.  */
#define DECL_ABSTRACT_P(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.abstract_flag)

/* Language-specific decl information.  */
#define DECL_LANG_SPECIFIC(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_specific)

/* In a VAR_DECL or FUNCTION_DECL, nonzero means external reference:
   do not allocate storage, and refer to a definition elsewhere.  Note that
   this does not necessarily imply the entity represented by NODE
   has no program source-level definition in this translation unit.  For
   example, for a FUNCTION_DECL, DECL_SAVED_TREE may be non-NULL and
   DECL_EXTERNAL may be true simultaneously; that can be the case for
   a C99 "extern inline" function.  */
#define DECL_EXTERNAL(NODE) (DECL_COMMON_CHECK (NODE)->decl_common.decl_flag_1)

/* Nonzero in a ..._DECL means this variable is ref'd from a nested function.
   For VAR_DECL nodes, PARM_DECL nodes, and FUNCTION_DECL nodes.

   For LABEL_DECL nodes, nonzero if nonlocal gotos to the label are permitted.

   Also set in some languages for variables, etc., outside the normal
   lexical scope, such as class instance variables.  */
#define DECL_NONLOCAL(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.nonlocal_flag)

/* Used in VAR_DECLs to indicate that the variable is a vtable.
   Used in FIELD_DECLs for vtable pointers.
   Used in FUNCTION_DECLs to indicate that the function is virtual.  */
#define DECL_VIRTUAL_P(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.virtual_flag)

/* Used to indicate that this DECL represents a compiler-generated entity.  */
#define DECL_ARTIFICIAL(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.artificial_flag)

/* Additional flags for language-specific uses.  */
#define DECL_LANG_FLAG_0(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_0)
#define DECL_LANG_FLAG_1(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_1)
#define DECL_LANG_FLAG_2(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_2)
#define DECL_LANG_FLAG_3(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_3)
#define DECL_LANG_FLAG_4(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_4)
#define DECL_LANG_FLAG_5(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_5)
#define DECL_LANG_FLAG_6(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_6)
#define DECL_LANG_FLAG_7(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_7)
#define DECL_LANG_FLAG_8(NODE) \
  (DECL_COMMON_CHECK (NODE)->decl_common.lang_flag_8)

/* Nonzero for a scope which is equal to file scope.  */
#define SCOPE_FILE_SCOPE_P(EXP)	\
  (! (EXP) || TREE_CODE (EXP) == TRANSLATION_UNIT_DECL)
/* Nonzero for a decl which is at file scope.  */
#define DECL_FILE_SCOPE_P(EXP) SCOPE_FILE_SCOPE_P (DECL_CONTEXT (EXP))
/* Nonzero for a type which is at file scope.  */
#define TYPE_FILE_SCOPE_P(EXP) SCOPE_FILE_SCOPE_P (TYPE_CONTEXT (EXP))

/* Nonzero for a decl that is decorated using attribute used.
   This indicates to compiler tools that this decl needs to be preserved.  */
#define DECL_PRESERVE_P(DECL) \
  DECL_COMMON_CHECK (DECL)->decl_common.preserve_flag

/* Nonzero for a decl that is decorated with the "noinit" attribute.
   decls with this attribute are placed into the ".noinit" section, so they are
   not initialized by the target's startup code.  */
#define DECL_NOINIT_P(DECL)	\
  (DECL_P (DECL)		\
   && (lookup_attribute ("noinit", DECL_ATTRIBUTES (DECL)) != NULL_TREE))

/* Nonzero for a decl that is decorated with the "persistent" attribute.
   decls with this attribute are placed into the ".persistent" section, so they
   are not initialized by the target's startup code.  */
#define DECL_PERSISTENT_P(DECL)	\
  (DECL_P (DECL)		\
   && (lookup_attribute ("persistent", DECL_ATTRIBUTES (DECL)) != NULL_TREE))

/* For function local variables indicates that the variable
   should not be treated as a GIMPLE register.  In particular
   this means that partial definitions can appear and the
   variable cannot be written into SSA form and instead uses
   virtual operands to represent the use-def dataflow.  */
#define DECL_NOT_GIMPLE_REG_P(DECL) \
  DECL_COMMON_CHECK (DECL)->decl_common.not_gimple_reg_flag

extern tree decl_value_expr_lookup (tree);
extern void decl_value_expr_insert (tree, tree);

/* In a VAR_DECL or PARM_DECL, the location at which the value may be found,
   if transformations have made this more complicated than evaluating the
   decl itself.  */
#define DECL_HAS_VALUE_EXPR_P(NODE) \
  (TREE_CHECK3 (NODE, VAR_DECL, PARM_DECL, RESULT_DECL) \
   ->decl_common.decl_flag_2)
#define DECL_VALUE_EXPR(NODE) \
  (decl_value_expr_lookup (DECL_WRTL_CHECK (NODE)))
#define SET_DECL_VALUE_EXPR(NODE, VAL) \
  (decl_value_expr_insert (DECL_WRTL_CHECK (NODE), VAL))

/* Holds the RTL expression for the value of a variable or function.
   This value can be evaluated lazily for functions, variables with
   static storage duration, and labels.  */
#define DECL_RTL(NODE)					\
  (DECL_WRTL_CHECK (NODE)->decl_with_rtl.rtl		\
   ? (NODE)->decl_with_rtl.rtl					\
   : (make_decl_rtl (NODE), (NODE)->decl_with_rtl.rtl))

/* Set the DECL_RTL for NODE to RTL.  */
#define SET_DECL_RTL(NODE, RTL) set_decl_rtl (NODE, RTL)

/* Returns nonzero if NODE is a tree node that can contain RTL.  */
#define HAS_RTL_P(NODE) (CODE_CONTAINS_STRUCT (TREE_CODE (NODE), TS_DECL_WRTL))

/* Returns nonzero if the DECL_RTL for NODE has already been set.  */
#define DECL_RTL_SET_P(NODE) \
  (HAS_RTL_P (NODE) && DECL_WRTL_CHECK (NODE)->decl_with_rtl.rtl != NULL)

/* Copy the RTL from SRC_DECL to DST_DECL.  If the RTL was not set for
   SRC_DECL, it will not be set for DST_DECL; this is a lazy copy.  */
#define COPY_DECL_RTL(SRC_DECL, DST_DECL) \
  (DECL_WRTL_CHECK (DST_DECL)->decl_with_rtl.rtl \
   = DECL_WRTL_CHECK (SRC_DECL)->decl_with_rtl.rtl)

/* The DECL_RTL for NODE, if it is set, or NULL, if it is not set.  */
#define DECL_RTL_IF_SET(NODE) (DECL_RTL_SET_P (NODE) ? DECL_RTL (NODE) : NULL)

#if (GCC_VERSION >= 2007)
#define DECL_RTL_KNOWN_SET(decl) __extension__				\
({  tree const __d = (decl);						\
    gcc_checking_assert (DECL_RTL_SET_P (__d));				\
    /* Dereference it so the compiler knows it can't be NULL even	\
       without assertion checking.  */					\
    &*DECL_RTL_IF_SET (__d); })
#else
#define DECL_RTL_KNOWN_SET(decl) (&*DECL_RTL_IF_SET (decl))
#endif

/* In VAR_DECL and PARM_DECL nodes, nonzero means declared `register'.  */
#define DECL_REGISTER(NODE) (DECL_WRTL_CHECK (NODE)->decl_common.decl_flag_0)

/* In a FIELD_DECL, this is the field position, counting in bytes, of the
   DECL_OFFSET_ALIGN-bit-sized word containing the bit closest to the beginning
   of the structure.  */
#define DECL_FIELD_OFFSET(NODE) (FIELD_DECL_CHECK (NODE)->field_decl.offset)

/* In a FIELD_DECL, this is the offset, in bits, of the first bit of the
   field from DECL_FIELD_OFFSET.  This field may be nonzero even for fields
   that are not bit fields (since DECL_OFFSET_ALIGN may be larger than the
   natural alignment of the field's type).  */
#define DECL_FIELD_BIT_OFFSET(NODE) \
  (FIELD_DECL_CHECK (NODE)->field_decl.bit_offset)

/* In a FIELD_DECL, this indicates whether the field was a bit-field and
   if so, the type that was originally specified for it.
   TREE_TYPE may have been modified (in finish_struct).  */
#define DECL_BIT_FIELD_TYPE(NODE) \
  (FIELD_DECL_CHECK (NODE)->field_decl.bit_field_type)

/* In a FIELD_DECL of a RECORD_TYPE, this is a pointer to the storage
   representative FIELD_DECL.  */
#define DECL_BIT_FIELD_REPRESENTATIVE(NODE) \
  (FIELD_DECL_CHECK (NODE)->field_decl.qualifier)

/* For a FIELD_DECL in a QUAL_UNION_TYPE, records the expression, which
   if nonzero, indicates that the field occupies the type.  */
#define DECL_QUALIFIER(NODE) (FIELD_DECL_CHECK (NODE)->field_decl.qualifier)

/* For FIELD_DECLs, off_align holds the number of low-order bits of
   DECL_FIELD_OFFSET which are known to be always zero.
   DECL_OFFSET_ALIGN thus returns the alignment that DECL_FIELD_OFFSET
   has.  */
#define DECL_OFFSET_ALIGN(NODE) \
  (HOST_WIDE_INT_1U << FIELD_DECL_CHECK (NODE)->decl_common.off_align)

/* Specify that DECL_OFFSET_ALIGN(NODE) is X.  */
#define SET_DECL_OFFSET_ALIGN(NODE, X) \
  (FIELD_DECL_CHECK (NODE)->decl_common.off_align = ffs_hwi (X) - 1)

/* For FIELD_DECLS, DECL_FCONTEXT is the *first* baseclass in
   which this FIELD_DECL is defined.  This information is needed when
   writing debugging information about vfield and vbase decls for C++.  */
#define DECL_FCONTEXT(NODE) (FIELD_DECL_CHECK (NODE)->field_decl.fcontext)

/* In a FIELD_DECL, indicates this field should be bit-packed.  */
#define DECL_PACKED(NODE) (FIELD_DECL_CHECK (NODE)->base.u.bits.packed_flag)

/* Nonzero in a FIELD_DECL means it is a bit field, and must be accessed
   specially.  */
#define DECL_BIT_FIELD(NODE) (FIELD_DECL_CHECK (NODE)->decl_common.decl_flag_1)

/* In a FIELD_DECL, indicates this field should be ignored for ABI decisions
   like passing/returning containing struct by value.
   Set for C++17 empty base artificial FIELD_DECLs as well as
   empty [[no_unique_address]] non-static data members.  */
#define DECL_FIELD_ABI_IGNORED(NODE) \
  (!DECL_BIT_FIELD (NODE) && (NODE)->decl_common.decl_flag_0)
#define SET_DECL_FIELD_ABI_IGNORED(NODE, VAL) \
  do {									\
    gcc_checking_assert (!DECL_BIT_FIELD (NODE));			\
    FIELD_DECL_CHECK (NODE)->decl_common.decl_flag_0 = (VAL);		\
  } while (0)

/* In a FIELD_DECL, indicates C++ zero-width bitfield that used to be
   removed from the IL since PR42217 until PR101539 and by that changed
   the ABI on several targets.  This flag is provided so that the backends
   can decide on the ABI with zero-width bitfields and emit -Wpsabi
   warnings.  */
#define DECL_FIELD_CXX_ZERO_WIDTH_BIT_FIELD(NODE) \
  (DECL_BIT_FIELD (NODE) && (NODE)->decl_common.decl_flag_0)
#define SET_DECL_FIELD_CXX_ZERO_WIDTH_BIT_FIELD(NODE, VAL) \
  do {									\
    gcc_checking_assert (DECL_BIT_FIELD (NODE));			\
    FIELD_DECL_CHECK (NODE)->decl_common.decl_flag_0 = (VAL);		\
  } while (0)

/* Used in a FIELD_DECL to indicate that we cannot form the address of
   this component.  This makes it possible for Type-Based Alias Analysis
   to disambiguate accesses to this field with indirect accesses using
   the field's type:

     struct S { int i; } s;
     int *p;

   If the flag is set on 'i', TBAA computes that s.i and *p never conflict.

   From the implementation's viewpoint, the alias set of the type of the
   field 'i' (int) will not be recorded as a subset of that of the type of
   's' (struct S) in record_component_aliases.  The counterpart is that
   accesses to s.i must not be given the alias set of the type of 'i'
   (int) but instead directly that of the type of 's' (struct S).  */
#define DECL_NONADDRESSABLE_P(NODE) \
  (FIELD_DECL_CHECK (NODE)->decl_common.decl_flag_2)

/* Used in a FIELD_DECL to indicate that this field is padding.  */
#define DECL_PADDING_P(NODE) \
  (FIELD_DECL_CHECK (NODE)->decl_common.decl_flag_3)

/* Used in a FIELD_DECL to indicate whether this field is not a flexible
   array member. This is only valid for the last array type field of a
   structure.  */
#define DECL_NOT_FLEXARRAY(NODE) \
  (FIELD_DECL_CHECK (NODE)->decl_common.decl_not_flexarray)

/* A numeric unique identifier for a LABEL_DECL.  The UID allocation is
   dense, unique within any one function, and may be used to index arrays.
   If the value is -1, then no UID has been assigned.  */
#define LABEL_DECL_UID(NODE) \
  (LABEL_DECL_CHECK (NODE)->label_decl.label_decl_uid)

/* In a LABEL_DECL, the EH region number for which the label is the
   post_landing_pad.  */
#define EH_LANDING_PAD_NR(NODE) \
  (LABEL_DECL_CHECK (NODE)->label_decl.eh_landing_pad_nr)

/* For a PARM_DECL, records the data type used to pass the argument,
   which may be different from the type seen in the program.  */
#define DECL_ARG_TYPE(NODE) (PARM_DECL_CHECK (NODE)->decl_common.initial)

/* For PARM_DECL, holds an RTL for the stack slot or register
   where the data was actually passed.  */
#define DECL_INCOMING_RTL(NODE) \
  (PARM_DECL_CHECK (NODE)->parm_decl.incoming_rtl)

/* Nonzero for a given ..._DECL node means that no warnings should be
   generated just because this node is unused.  */
#define DECL_IN_SYSTEM_HEADER(NODE) \
  (in_system_header_at (DECL_SOURCE_LOCATION (NODE)))

/* Used to indicate that the linkage status of this DECL is not yet known,
   so it should not be output now.  */
#define DECL_DEFER_OUTPUT(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.defer_output)

/* In a VAR_DECL that's static,
   nonzero if the space is in the text section.  */
#define DECL_IN_TEXT_SECTION(NODE) \
  (VAR_DECL_CHECK (NODE)->decl_with_vis.in_text_section)

/* In a VAR_DECL that's static,
   nonzero if it belongs to the global constant pool.  */
#define DECL_IN_CONSTANT_POOL(NODE) \
  (VAR_DECL_CHECK (NODE)->decl_with_vis.in_constant_pool)

/* Nonzero for a given ..._DECL node means that this node should be
   put in .common, if possible.  If a DECL_INITIAL is given, and it
   is not error_mark_node, then the decl cannot be put in .common.  */
#define DECL_COMMON(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.common_flag)

/* In a VAR_DECL, nonzero if the decl is a register variable with
   an explicit asm specification.  */
#define DECL_HARD_REGISTER(NODE)  \
  (VAR_DECL_CHECK (NODE)->decl_with_vis.hard_register)

  /* Used to indicate that this DECL has weak linkage.  */
#define DECL_WEAK(NODE) (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.weak_flag)

/* Used to indicate that the DECL is a dllimport.  */
#define DECL_DLLIMPORT_P(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.dllimport_flag)

/* Used in a DECL to indicate that, even if it TREE_PUBLIC, it need
   not be put out unless it is needed in this translation unit.
   Entities like this are shared across translation units (like weak
   entities), but are guaranteed to be generated by any translation
   unit that needs them, and therefore need not be put out anywhere
   where they are not needed.  DECL_COMDAT is just a hint to the
   back-end; it is up to front-ends which set this flag to ensure
   that there will never be any harm, other than bloat, in putting out
   something which is DECL_COMDAT.  */
#define DECL_COMDAT(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.comdat_flag)

#define DECL_COMDAT_GROUP(NODE) \
  decl_comdat_group (NODE)

/* Used in TREE_PUBLIC decls to indicate that copies of this DECL in
   multiple translation units should be merged.  */
#define DECL_ONE_ONLY(NODE) (DECL_COMDAT_GROUP (NODE) != NULL_TREE \
			     && (TREE_PUBLIC (NODE) || DECL_EXTERNAL (NODE)))

/* The name of the object as the assembler will see it (but before any
   translations made by ASM_OUTPUT_LABELREF).  Often this is the same
   as DECL_NAME.  It is an IDENTIFIER_NODE.

   ASSEMBLER_NAME of TYPE_DECLS may store global name of type used for
   One Definition Rule based type merging at LTO.  It is computed only for
   LTO compilation and C++.  */
#define DECL_ASSEMBLER_NAME(NODE) decl_assembler_name (NODE)

/* Raw accessor for DECL_ASSEMBLE_NAME.  */
#define DECL_ASSEMBLER_NAME_RAW(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.assembler_name)

/* Return true if NODE is a NODE that can contain a DECL_ASSEMBLER_NAME.
   This is true of all DECL nodes except FIELD_DECL.  */
#define HAS_DECL_ASSEMBLER_NAME_P(NODE) \
  (CODE_CONTAINS_STRUCT (TREE_CODE (NODE), TS_DECL_WITH_VIS))

/* Returns nonzero if the DECL_ASSEMBLER_NAME for NODE has been set.  If zero,
   the NODE might still have a DECL_ASSEMBLER_NAME -- it just hasn't been set
   yet.  */
#define DECL_ASSEMBLER_NAME_SET_P(NODE) \
  (DECL_ASSEMBLER_NAME_RAW (NODE) != NULL_TREE)

/* Set the DECL_ASSEMBLER_NAME for NODE to NAME.  */
#define SET_DECL_ASSEMBLER_NAME(NODE, NAME) \
  overwrite_decl_assembler_name (NODE, NAME)

/* Copy the DECL_ASSEMBLER_NAME from SRC_DECL to DST_DECL.  Note that
   if SRC_DECL's DECL_ASSEMBLER_NAME has not yet been set, using this
   macro will not cause the DECL_ASSEMBLER_NAME to be set, but will
   clear DECL_ASSEMBLER_NAME of DST_DECL, if it was already set.  In
   other words, the semantics of using this macro, are different than
   saying:

     SET_DECL_ASSEMBLER_NAME(DST_DECL, DECL_ASSEMBLER_NAME (SRC_DECL))

   which will try to set the DECL_ASSEMBLER_NAME for SRC_DECL.  */

#define COPY_DECL_ASSEMBLER_NAME(SRC_DECL, DST_DECL)			\
  SET_DECL_ASSEMBLER_NAME (DST_DECL, DECL_ASSEMBLER_NAME_RAW (SRC_DECL))

/* Records the section name in a section attribute.  Used to pass
   the name from decl_attributes to make_function_rtl and make_decl_rtl.  */
#define DECL_SECTION_NAME(NODE) decl_section_name (NODE)

/* Nonzero in a decl means that the gimplifier has seen (or placed)
   this variable in a BIND_EXPR.  */
#define DECL_SEEN_IN_BIND_EXPR_P(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.seen_in_bind_expr)

/* Value of the decls's visibility attribute */
#define DECL_VISIBILITY(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.visibility)

/* Nonzero means that the decl (or an enclosing scope) had its
   visibility specified rather than being inferred.  */
#define DECL_VISIBILITY_SPECIFIED(NODE) \
  (DECL_WITH_VIS_CHECK (NODE)->decl_with_vis.visibility_specified)

/* In a VAR_DECL, the model to use if the data should be allocated from
   thread-local storage.  */
#define DECL_TLS_MODEL(NODE) decl_tls_model (NODE)

/* In a VAR_DECL, nonzero if the data should be allocated from
   thread-local storage.  */
#define DECL_THREAD_LOCAL_P(NODE) \
  ((TREE_STATIC (NODE) || DECL_EXTERNAL (NODE)) && decl_tls_model (NODE) >= TLS_MODEL_REAL)

/* In a non-local VAR_DECL with static storage duration, true if the
   variable has an initialization priority.  If false, the variable
   will be initialized at the DEFAULT_INIT_PRIORITY.  */
#define DECL_HAS_INIT_PRIORITY_P(NODE) \
  (VAR_DECL_CHECK (NODE)->decl_with_vis.init_priority_p)

extern tree decl_debug_expr_lookup (tree);
extern void decl_debug_expr_insert (tree, tree);

/* For VAR_DECL, this is set to an expression that it was split from.  */
#define DECL_HAS_DEBUG_EXPR_P(NODE) \
  (VAR_DECL_CHECK (NODE)->decl_common.debug_expr_is_from)
#define DECL_DEBUG_EXPR(NODE) \
  (decl_debug_expr_lookup (VAR_DECL_CHECK (NODE)))

#define SET_DECL_DEBUG_EXPR(NODE, VAL) \
  (decl_debug_expr_insert (VAR_DECL_CHECK (NODE), VAL))

extern priority_type decl_init_priority_lookup (tree);
extern priority_type decl_fini_priority_lookup (tree);
extern void decl_init_priority_insert (tree, priority_type);
extern void decl_fini_priority_insert (tree, priority_type);

/* For a VAR_DECL or FUNCTION_DECL the initialization priority of
   NODE.  */
#define DECL_INIT_PRIORITY(NODE) \
  (decl_init_priority_lookup (NODE))
/* Set the initialization priority for NODE to VAL.  */
#define SET_DECL_INIT_PRIORITY(NODE, VAL) \
  (decl_init_priority_insert (NODE, VAL))

/* For a FUNCTION_DECL the finalization priority of NODE.  */
#define DECL_FINI_PRIORITY(NODE) \
  (decl_fini_priority_lookup (NODE))
/* Set the finalization priority for NODE to VAL.  */
#define SET_DECL_FINI_PRIORITY(NODE, VAL) \
  (decl_fini_priority_insert (NODE, VAL))

/* The initialization priority for entities for which no explicit
   initialization priority has been specified.  */
#define DEFAULT_INIT_PRIORITY 65535

/* The maximum allowed initialization priority.  */
#define MAX_INIT_PRIORITY 65535

/* The largest priority value reserved for use by system runtime
   libraries.  */
#define MAX_RESERVED_INIT_PRIORITY 100

/* In a VAR_DECL, nonzero if this is a global variable for VOPs.  */
#define VAR_DECL_IS_VIRTUAL_OPERAND(NODE) \
  (VAR_DECL_CHECK (NODE)->base.u.bits.saturating_flag)

/* In a VAR_DECL, nonzero if this is a non-local frame structure.  */
#define DECL_NONLOCAL_FRAME(NODE)  \
  (VAR_DECL_CHECK (NODE)->base.default_def_flag)

/* In a VAR_DECL, nonzero if this variable is not aliased by any pointer.  */
#define DECL_NONALIASED(NODE) \
  (VAR_DECL_CHECK (NODE)->base.nothrow_flag)

/* In a VAR_DECL, nonzero if this variable is not required to have a distinct
   address from other variables with the same constant value.  In other words,
   consider -fmerge-all-constants to be on for this VAR_DECL.  */
#define DECL_MERGEABLE(NODE) \
  (VAR_DECL_CHECK (NODE)->decl_common.decl_flag_3)

/* This field is used to reference anything in decl.result and is meant only
   for use by the garbage collector.  */
#define DECL_RESULT_FLD(NODE) \
  (DECL_NON_COMMON_CHECK (NODE)->decl_non_common.result)

/* The DECL_VINDEX is used for FUNCTION_DECLS in two different ways.
   Before the struct containing the FUNCTION_DECL is laid out,
   DECL_VINDEX may point to a FUNCTION_DECL in a base class which
   is the FUNCTION_DECL which this FUNCTION_DECL will replace as a virtual
   function.  When the class is laid out, this pointer is changed
   to an INTEGER_CST node which is suitable for use as an index
   into the virtual function table. */
#define DECL_VINDEX(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.vindex)

/* In FUNCTION_DECL, holds the decl for the return value.  */
#define DECL_RESULT(NODE) (FUNCTION_DECL_CHECK (NODE)->decl_non_common.result)

/* In a FUNCTION_DECL, nonzero if the function cannot be inlined.  */
#define DECL_UNINLINABLE(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.uninlinable)

/* In a FUNCTION_DECL, the saved representation of the body of the
   entire function.  */
#define DECL_SAVED_TREE(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.saved_tree)

/* Nonzero in a FUNCTION_DECL means this function should be treated
   as if it were a malloc, meaning it returns a pointer that is
   not an alias.  */
#define DECL_IS_MALLOC(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.malloc_flag)

/* Macro for direct set and get of function_decl.decl_type.  */
#define FUNCTION_DECL_DECL_TYPE(NODE) \
  (NODE->function_decl.decl_type)

/* Set decl_type of a DECL.  Set it to T when SET is true, or reset
   it to NONE.  */

inline void
set_function_decl_type (tree decl, function_decl_type t, bool set)
{
  if (set)
    {
      gcc_assert (FUNCTION_DECL_DECL_TYPE (decl) == function_decl_type::NONE
		  || FUNCTION_DECL_DECL_TYPE (decl) == t);
      FUNCTION_DECL_DECL_TYPE (decl) = t;
    }
  else if (FUNCTION_DECL_DECL_TYPE (decl) == t)
    FUNCTION_DECL_DECL_TYPE (decl) = function_decl_type::NONE;
}

/* Nonzero in a FUNCTION_DECL means this function is a replaceable
   function (like replaceable operators new or delete).  */
#define DECL_IS_REPLACEABLE_OPERATOR(NODE)\
   (FUNCTION_DECL_CHECK (NODE)->function_decl.replaceable_operator)

/* Nonzero in a FUNCTION_DECL means this function should be treated as
   C++ operator new, meaning that it returns a pointer for which we
   should not use type based aliasing.  */
#define DECL_IS_OPERATOR_NEW_P(NODE) \
  (FUNCTION_DECL_DECL_TYPE (FUNCTION_DECL_CHECK (NODE)) \
   == function_decl_type::OPERATOR_NEW)

#define DECL_IS_REPLACEABLE_OPERATOR_NEW_P(NODE) \
  (DECL_IS_OPERATOR_NEW_P (NODE) && DECL_IS_REPLACEABLE_OPERATOR (NODE))

#define DECL_SET_IS_OPERATOR_NEW(NODE, VAL) \
  set_function_decl_type (FUNCTION_DECL_CHECK (NODE), \
			  function_decl_type::OPERATOR_NEW, VAL)

/* Nonzero in a FUNCTION_DECL means this function should be treated as
   C++ operator delete.  */
#define DECL_IS_OPERATOR_DELETE_P(NODE) \
  (FUNCTION_DECL_DECL_TYPE (FUNCTION_DECL_CHECK (NODE)) \
   == function_decl_type::OPERATOR_DELETE)

#define DECL_SET_IS_OPERATOR_DELETE(NODE, VAL) \
  set_function_decl_type (FUNCTION_DECL_CHECK (NODE), \
			  function_decl_type::OPERATOR_DELETE, VAL)

/* Nonzero in a FUNCTION_DECL means this function may return more
   than once.  */
#define DECL_IS_RETURNS_TWICE(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.returns_twice_flag)

/* Nonzero in a FUNCTION_DECL means this function should be treated
   as "pure" function (like const function, but may read global memory).
   Note that being pure or const for a function is orthogonal to being
   nothrow, i.e. it is valid to have DECL_PURE_P set and TREE_NOTHROW
   cleared.  */
#define DECL_PURE_P(NODE) (FUNCTION_DECL_CHECK (NODE)->function_decl.pure_flag)

/* Nonzero only if one of TREE_READONLY or DECL_PURE_P is nonzero AND
   the const or pure function may not terminate.  When this is nonzero
   for a const or pure function, it can be dealt with by cse passes
   but cannot be removed by dce passes since you are not allowed to
   change an infinite looping program into one that terminates without
   error.  */
#define DECL_LOOPING_CONST_OR_PURE_P(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.looping_const_or_pure_flag)

/* Nonzero in a FUNCTION_DECL means this function should be treated
   as "novops" function (function that does not read global memory,
   but may have arbitrary side effects).  */
#define DECL_IS_NOVOPS(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.novops_flag)

/* Used in FUNCTION_DECLs to indicate that they should be run automatically
   at the beginning or end of execution.  */
#define DECL_STATIC_CONSTRUCTOR(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.static_ctor_flag)

#define DECL_STATIC_DESTRUCTOR(NODE) \
(FUNCTION_DECL_CHECK (NODE)->function_decl.static_dtor_flag)

/* Used in FUNCTION_DECLs to indicate that function entry and exit should
   be instrumented with calls to support routines.  */
#define DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.no_instrument_function_entry_exit)

/* Used in FUNCTION_DECLs to indicate that limit-stack-* should be
   disabled in this function.  */
#define DECL_NO_LIMIT_STACK(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.no_limit_stack)

/* In a FUNCTION_DECL indicates that a static chain is needed.  */
#define DECL_STATIC_CHAIN(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->decl_with_vis.regdecl_flag)

/* Nonzero for a decl that cgraph has decided should be inlined into
   at least one call site.  It is not meaningful to look at this
   directly; always use cgraph_function_possibly_inlined_p.  */
#define DECL_POSSIBLY_INLINED(DECL) \
  FUNCTION_DECL_CHECK (DECL)->function_decl.possibly_inlined

/* Nonzero in a FUNCTION_DECL means that this function was declared inline,
   such as via the `inline' keyword in C/C++.  This flag controls the linkage
   semantics of 'inline'  */
#define DECL_DECLARED_INLINE_P(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.declared_inline_flag)

/* Nonzero in a FUNCTION_DECL means this function should not get
   -Winline warnings.  */
#define DECL_NO_INLINE_WARNING_P(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.no_inline_warning_flag)

/* Nonzero if a FUNCTION_CODE is a TM load/store.  */
#define BUILTIN_TM_LOAD_STORE_P(FN) \
  ((FN) >= BUILT_IN_TM_STORE_1 && (FN) <= BUILT_IN_TM_LOAD_RFW_LDOUBLE)

/* Nonzero if a FUNCTION_CODE is a TM load.  */
#define BUILTIN_TM_LOAD_P(FN) \
  ((FN) >= BUILT_IN_TM_LOAD_1 && (FN) <= BUILT_IN_TM_LOAD_RFW_LDOUBLE)

/* Nonzero if a FUNCTION_CODE is a TM store.  */
#define BUILTIN_TM_STORE_P(FN) \
  ((FN) >= BUILT_IN_TM_STORE_1 && (FN) <= BUILT_IN_TM_STORE_WAW_LDOUBLE)

#define CASE_BUILT_IN_TM_LOAD(FN)	\
  case BUILT_IN_TM_LOAD_##FN:		\
  case BUILT_IN_TM_LOAD_RAR_##FN:	\
  case BUILT_IN_TM_LOAD_RAW_##FN:	\
  case BUILT_IN_TM_LOAD_RFW_##FN

#define CASE_BUILT_IN_TM_STORE(FN)	\
  case BUILT_IN_TM_STORE_##FN:		\
  case BUILT_IN_TM_STORE_WAR_##FN:	\
  case BUILT_IN_TM_STORE_WAW_##FN

/* Nonzero in a FUNCTION_DECL that should be always inlined by the inliner
   disregarding size and cost heuristics.  This is equivalent to using
   the always_inline attribute without the required diagnostics if the
   function cannot be inlined.  */
#define DECL_DISREGARD_INLINE_LIMITS(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.disregard_inline_limits)

extern vec<tree, va_gc> **decl_debug_args_lookup (tree);
extern vec<tree, va_gc> **decl_debug_args_insert (tree);

/* Nonzero if a FUNCTION_DECL has DEBUG arguments attached to it.  */
#define DECL_HAS_DEBUG_ARGS_P(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.has_debug_args_flag)

/* For FUNCTION_DECL, this holds a pointer to a structure ("struct function")
   that describes the status of this function.  */
#define DECL_STRUCT_FUNCTION(NODE) \
  (FUNCTION_DECL_CHECK (NODE)->function_decl.f)

/* For a builtin function, identify which part of the compiler defined it.  */
#define DECL_BUILT_IN_CLASS(NODE) \
   ((built_in_class) FUNCTION_DECL_CHECK (NODE)->function_decl.built_in_class)

/* In FUNCTION_DECL, a chain of ..._DECL nodes.  */
#define DECL_ARGUMENTS(NODE) \
   (FUNCTION_DECL_CHECK (NODE)->function_decl.arguments)

/* In FUNCTION_DECL, the function specific target options to use when compiling
   this function.  */
#define DECL_FUNCTION_SPECIFIC_TARGET(NODE) \
   (FUNCTION_DECL_CHECK (NODE)->function_decl.function_specific_target)

/* In FUNCTION_DECL, the function specific optimization options to use when
   compiling this function.  */
#define DECL_FUNCTION_SPECIFIC_OPTIMIZATION(NODE) \
   (FUNCTION_DECL_CHECK (NODE)->function_decl.function_specific_optimization)

/* In FUNCTION_DECL, this is set if this function has other versions generated
   to support different architecture feature sets, e.g. using "target" or
   "target_version" attributes.  */
#define DECL_FUNCTION_VERSIONED(NODE)\
   (FUNCTION_DECL_CHECK (NODE)->function_decl.versioned_function)

/* In FUNCTION_DECL, this is set if this function is a C++ constructor.
   Devirtualization machinery uses this knowledge for determing type of the
   object constructed.  Also we assume that constructor address is not
   important.  */
#define DECL_CXX_CONSTRUCTOR_P(NODE)\
   (FUNCTION_DECL_CHECK (NODE)->decl_with_vis.cxx_constructor)

/* In FUNCTION_DECL, this is set if this function is a C++ destructor.
   Devirtualization machinery uses this to track types in destruction.  */
#define DECL_CXX_DESTRUCTOR_P(NODE)\
   (FUNCTION_DECL_CHECK (NODE)->decl_with_vis.cxx_destructor)

/* In FUNCTION_DECL, this is set if this function is a lambda function.  */
#define DECL_LAMBDA_FUNCTION_P(NODE) \
  (FUNCTION_DECL_DECL_TYPE (FUNCTION_DECL_CHECK (NODE)) \
   == function_decl_type::LAMBDA_FUNCTION)

#define DECL_SET_LAMBDA_FUNCTION(NODE, VAL) \
  set_function_decl_type (FUNCTION_DECL_CHECK (NODE), \
			  function_decl_type::LAMBDA_FUNCTION, VAL)

/* In FUNCTION_DECL that represent an virtual method this is set when
   the method is final.  */
#define DECL_FINAL_P(NODE)\
   (FUNCTION_DECL_CHECK (NODE)->decl_with_vis.final)

/* The source language of the translation-unit.  */
#define TRANSLATION_UNIT_LANGUAGE(NODE) \
  (TRANSLATION_UNIT_DECL_CHECK (NODE)->translation_unit_decl.language)

/* TRANSLATION_UNIT_DECL inherits from DECL_MINIMAL.  */

/* For a TYPE_DECL, holds the "original" type.  (TREE_TYPE has the copy.) */
#define DECL_ORIGINAL_TYPE(NODE) \
  (TYPE_DECL_CHECK (NODE)->decl_non_common.result)

/* In a TYPE_DECL nonzero means the detail info about this type is not dumped
   into stabs.  Instead it will generate cross reference ('x') of names.
   This uses the same flag as DECL_EXTERNAL.  */
#define TYPE_DECL_SUPPRESS_DEBUG(NODE) \
  (TYPE_DECL_CHECK (NODE)->decl_common.decl_flag_1)

/* Getter of the imported declaration associated to the
   IMPORTED_DECL node.  */
#define IMPORTED_DECL_ASSOCIATED_DECL(NODE) \
(DECL_INITIAL (IMPORTED_DECL_CHECK (NODE)))

/* Getter of the symbol declaration associated with the
   NAMELIST_DECL node.  */
#define NAMELIST_DECL_ASSOCIATED_DECL(NODE) \
  (DECL_INITIAL (NODE))

/* A STATEMENT_LIST chains statements together in GENERIC and GIMPLE.
   To reduce overhead, the nodes containing the statements are not trees.
   This avoids the overhead of tree_common on all linked list elements.

   Use the interface in tree-iterator.h to access this node.  */

#define STATEMENT_LIST_HEAD(NODE) \
  (STATEMENT_LIST_CHECK (NODE)->stmt_list.head)
#define STATEMENT_LIST_TAIL(NODE) \
  (STATEMENT_LIST_CHECK (NODE)->stmt_list.tail)

#define TREE_OPTIMIZATION(NODE) \
  (OPTIMIZATION_NODE_CHECK (NODE)->optimization.opts)

#define TREE_OPTIMIZATION_OPTABS(NODE) \
  (OPTIMIZATION_NODE_CHECK (NODE)->optimization.optabs)

#define TREE_OPTIMIZATION_BASE_OPTABS(NODE) \
  (OPTIMIZATION_NODE_CHECK (NODE)->optimization.base_optabs)

/* Return a tree node that encapsulates the optimization options in OPTS
   and OPTS_SET.  */
extern tree build_optimization_node (struct gcc_options *opts,
				     struct gcc_options *opts_set);

#define TREE_TARGET_OPTION(NODE) \
  (TARGET_OPTION_NODE_CHECK (NODE)->target_option.opts)

#define TREE_TARGET_GLOBALS(NODE) \
  (TARGET_OPTION_NODE_CHECK (NODE)->target_option.globals)

/* Return a tree node that encapsulates the target options in OPTS and
   OPTS_SET.  */
extern tree build_target_option_node (struct gcc_options *opts,
				      struct gcc_options *opts_set);

extern void prepare_target_option_nodes_for_pch (void);

#if defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 2007)

inline tree
tree_check (tree __t, const char *__f, int __l, const char *__g, tree_code __c)
{
  if (TREE_CODE (__t) != __c)
    tree_check_failed (__t, __f, __l, __g, __c, 0);
  return __t;
}

inline tree
tree_not_check (tree __t, const char *__f, int __l, const char *__g,
                enum tree_code __c)
{
  if (TREE_CODE (__t) == __c)
    tree_not_check_failed (__t, __f, __l, __g, __c, 0);
  return __t;
}

inline tree
tree_check2 (tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, 0);
  return __t;
}

inline tree
tree_not_check2 (tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, 0);
  return __t;
}

inline tree
tree_check3 (tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, 0);
  return __t;
}

inline tree
tree_not_check3 (tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, 0);
  return __t;
}

inline tree
tree_check4 (tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
             enum tree_code __c4)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, 0);
  return __t;
}

inline tree
tree_not_check4 (tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
                 enum tree_code __c4)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, 0);
  return __t;
}

inline tree
tree_check5 (tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
             enum tree_code __c4, enum tree_code __c5)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4
      && TREE_CODE (__t) != __c5)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, 0);
  return __t;
}

inline tree
tree_not_check5 (tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
                 enum tree_code __c4, enum tree_code __c5)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4
      || TREE_CODE (__t) == __c5)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, 0);
  return __t;
}

inline tree
tree_check6 (tree __t, const char *__f, int __l, const char *__g,
	     enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
	     enum tree_code __c4, enum tree_code __c5, enum tree_code __c6)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4
      && TREE_CODE (__t) != __c5
      && TREE_CODE (__t) != __c6)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, __c6,
		       0);
  return __t;
}

inline tree
tree_not_check6 (tree __t, const char *__f, int __l, const char *__g,
		 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
		 enum tree_code __c4, enum tree_code __c5, enum tree_code __c6)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4
      || TREE_CODE (__t) == __c5
      || TREE_CODE (__t) == __c6)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5,
			   __c6, 0);
  return __t;
}

inline tree
contains_struct_check (tree __t, const enum tree_node_structure_enum __s,
                       const char *__f, int __l, const char *__g)
{
  if (tree_contains_struct[TREE_CODE (__t)][__s] != 1)
      tree_contains_struct_check_failed (__t, __s, __f, __l, __g);
  return __t;
}

inline tree
tree_class_check (tree __t, const enum tree_code_class __class,
                  const char *__f, int __l, const char *__g)
{
  if (TREE_CODE_CLASS (TREE_CODE (__t)) != __class)
    tree_class_check_failed (__t, __class, __f, __l, __g);
  return __t;
}

inline tree
tree_range_check (tree __t,
                  enum tree_code __code1, enum tree_code __code2,
                  const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) < __code1 || TREE_CODE (__t) > __code2)
    tree_range_check_failed (__t, __f, __l, __g, __code1, __code2);
  return __t;
}

inline tree
omp_clause_subcode_check (tree __t, enum omp_clause_code __code,
                          const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if (__t->omp_clause.code != __code)
    omp_clause_check_failed (__t, __f, __l, __g, __code);
  return __t;
}

inline tree
omp_clause_range_check (tree __t,
                        enum omp_clause_code __code1,
                        enum omp_clause_code __code2,
                        const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if ((int) __t->omp_clause.code < (int) __code1
      || (int) __t->omp_clause.code > (int) __code2)
    omp_clause_range_check_failed (__t, __f, __l, __g, __code1, __code2);
  return __t;
}

/* These checks have to be special cased.  */

inline tree
expr_check (tree __t, const char *__f, int __l, const char *__g)
{
  char const __c = TREE_CODE_CLASS (TREE_CODE (__t));
  if (!IS_EXPR_CODE_CLASS (__c))
    tree_class_check_failed (__t, tcc_expression, __f, __l, __g);
  return __t;
}

/* These checks have to be special cased.  */

inline tree
non_type_check (tree __t, const char *__f, int __l, const char *__g)
{
  if (TYPE_P (__t))
    tree_not_class_check_failed (__t, tcc_type, __f, __l, __g);
  return __t;
}

inline const HOST_WIDE_INT *
tree_int_cst_elt_check (const_tree __t, int __i,
			const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != INTEGER_CST)
    tree_check_failed (__t, __f, __l, __g, INTEGER_CST, 0);
  if (__i < 0 || __i >= __t->base.u.int_length.extended)
    tree_int_cst_elt_check_failed (__i, __t->base.u.int_length.extended,
				   __f, __l, __g);
  return &CONST_CAST_TREE (__t)->int_cst.val[__i];
}

inline HOST_WIDE_INT *
tree_int_cst_elt_check (tree __t, int __i,
			const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != INTEGER_CST)
    tree_check_failed (__t, __f, __l, __g, INTEGER_CST, 0);
  if (__i < 0 || __i >= __t->base.u.int_length.extended)
    tree_int_cst_elt_check_failed (__i, __t->base.u.int_length.extended,
				   __f, __l, __g);
  return &CONST_CAST_TREE (__t)->int_cst.val[__i];
}

/* Workaround -Wstrict-overflow false positive during profiledbootstrap.  */

# if GCC_VERSION >= 4006
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

inline tree *
tree_vec_elt_check (tree __t, int __i,
                    const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != TREE_VEC)
    tree_check_failed (__t, __f, __l, __g, TREE_VEC, 0);
  if (__i < 0 || __i >= __t->base.u.length)
    tree_vec_elt_check_failed (__i, __t->base.u.length, __f, __l, __g);
  return &CONST_CAST_TREE (__t)->vec.a[__i];
}

# if GCC_VERSION >= 4006
#pragma GCC diagnostic pop
#endif

inline tree *
omp_clause_elt_check (tree __t, int __i,
                      const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if (__i < 0 || __i >= omp_clause_num_ops [__t->omp_clause.code])
    omp_clause_operand_check_failed (__i, __t, __f, __l, __g);
  return &__t->omp_clause.ops[__i];
}

/* These checks have to be special cased.  */

inline tree
any_integral_type_check (tree __t, const char *__f, int __l, const char *__g)
{
  if (!ANY_INTEGRAL_TYPE_P (__t))
    tree_check_failed (__t, __f, __l, __g, BOOLEAN_TYPE, ENUMERAL_TYPE,
		       INTEGER_TYPE, BITINT_TYPE, 0);
  return __t;
}

inline const_tree
tree_check (const_tree __t, const char *__f, int __l, const char *__g,
	    tree_code __c)
{
  if (TREE_CODE (__t) != __c)
    tree_check_failed (__t, __f, __l, __g, __c, 0);
  return __t;
}

inline const_tree
tree_not_check (const_tree __t, const char *__f, int __l, const char *__g,
                enum tree_code __c)
{
  if (TREE_CODE (__t) == __c)
    tree_not_check_failed (__t, __f, __l, __g, __c, 0);
  return __t;
}

inline const_tree
tree_check2 (const_tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, 0);
  return __t;
}

inline const_tree
tree_not_check2 (const_tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, 0);
  return __t;
}

inline const_tree
tree_check3 (const_tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, 0);
  return __t;
}

inline const_tree
tree_not_check3 (const_tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, 0);
  return __t;
}

inline const_tree
tree_check4 (const_tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
             enum tree_code __c4)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, 0);
  return __t;
}

inline const_tree
tree_not_check4 (const_tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
                 enum tree_code __c4)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, 0);
  return __t;
}

inline const_tree
tree_check5 (const_tree __t, const char *__f, int __l, const char *__g,
             enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
             enum tree_code __c4, enum tree_code __c5)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4
      && TREE_CODE (__t) != __c5)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, 0);
  return __t;
}

inline const_tree
tree_not_check5 (const_tree __t, const char *__f, int __l, const char *__g,
                 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
                 enum tree_code __c4, enum tree_code __c5)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4
      || TREE_CODE (__t) == __c5)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, 0);
  return __t;
}

inline const_tree
tree_check6 (const_tree __t, const char *__f, int __l, const char *__g,
	     enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
	     enum tree_code __c4, enum tree_code __c5, enum tree_code __c6)
{
  if (TREE_CODE (__t) != __c1
      && TREE_CODE (__t) != __c2
      && TREE_CODE (__t) != __c3
      && TREE_CODE (__t) != __c4
      && TREE_CODE (__t) != __c5
      && TREE_CODE (__t) != __c6)
    tree_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5, __c6,
		       0);
  return __t;
}

inline const_tree
tree_not_check6 (const_tree __t, const char *__f, int __l, const char *__g,
		 enum tree_code __c1, enum tree_code __c2, enum tree_code __c3,
		 enum tree_code __c4, enum tree_code __c5, enum tree_code __c6)
{
  if (TREE_CODE (__t) == __c1
      || TREE_CODE (__t) == __c2
      || TREE_CODE (__t) == __c3
      || TREE_CODE (__t) == __c4
      || TREE_CODE (__t) == __c5
      || TREE_CODE (__t) == __c6)
    tree_not_check_failed (__t, __f, __l, __g, __c1, __c2, __c3, __c4, __c5,
			   __c6, 0);
  return __t;
}

inline const_tree
contains_struct_check (const_tree __t, const enum tree_node_structure_enum __s,
                       const char *__f, int __l, const char *__g)
{
  if (tree_contains_struct[TREE_CODE (__t)][__s] != 1)
      tree_contains_struct_check_failed (__t, __s, __f, __l, __g);
  return __t;
}

inline const_tree
tree_class_check (const_tree __t, const enum tree_code_class __class,
                  const char *__f, int __l, const char *__g)
{
  if (TREE_CODE_CLASS (TREE_CODE (__t)) != __class)
    tree_class_check_failed (__t, __class, __f, __l, __g);
  return __t;
}

inline const_tree
tree_range_check (const_tree __t,
                  enum tree_code __code1, enum tree_code __code2,
                  const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) < __code1 || TREE_CODE (__t) > __code2)
    tree_range_check_failed (__t, __f, __l, __g, __code1, __code2);
  return __t;
}

inline const_tree
omp_clause_subcode_check (const_tree __t, enum omp_clause_code __code,
                          const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if (__t->omp_clause.code != __code)
    omp_clause_check_failed (__t, __f, __l, __g, __code);
  return __t;
}

inline const_tree
omp_clause_range_check (const_tree __t,
                        enum omp_clause_code __code1,
                        enum omp_clause_code __code2,
                        const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if ((int) __t->omp_clause.code < (int) __code1
      || (int) __t->omp_clause.code > (int) __code2)
    omp_clause_range_check_failed (__t, __f, __l, __g, __code1, __code2);
  return __t;
}

inline const_tree
expr_check (const_tree __t, const char *__f, int __l, const char *__g)
{
  char const __c = TREE_CODE_CLASS (TREE_CODE (__t));
  if (!IS_EXPR_CODE_CLASS (__c))
    tree_class_check_failed (__t, tcc_expression, __f, __l, __g);
  return __t;
}

inline const_tree
non_type_check (const_tree __t, const char *__f, int __l, const char *__g)
{
  if (TYPE_P (__t))
    tree_not_class_check_failed (__t, tcc_type, __f, __l, __g);
  return __t;
}

# if GCC_VERSION >= 4006
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

inline const_tree *
tree_vec_elt_check (const_tree __t, int __i,
                    const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != TREE_VEC)
    tree_check_failed (__t, __f, __l, __g, TREE_VEC, 0);
  if (__i < 0 || __i >= __t->base.u.length)
    tree_vec_elt_check_failed (__i, __t->base.u.length, __f, __l, __g);
  return CONST_CAST (const_tree *, &__t->vec.a[__i]);
  //return &__t->vec.a[__i];
}

# if GCC_VERSION >= 4006
#pragma GCC diagnostic pop
#endif

inline const_tree *
omp_clause_elt_check (const_tree __t, int __i,
                      const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != OMP_CLAUSE)
    tree_check_failed (__t, __f, __l, __g, OMP_CLAUSE, 0);
  if (__i < 0 || __i >= omp_clause_num_ops [__t->omp_clause.code])
    omp_clause_operand_check_failed (__i, __t, __f, __l, __g);
  return CONST_CAST (const_tree *, &__t->omp_clause.ops[__i]);
}

inline const_tree
any_integral_type_check (const_tree __t, const char *__f, int __l,
			 const char *__g)
{
  if (!ANY_INTEGRAL_TYPE_P (__t))
    tree_check_failed (__t, __f, __l, __g, BOOLEAN_TYPE, ENUMERAL_TYPE,
		       INTEGER_TYPE, BITINT_TYPE, 0);
  return __t;
}

#endif

/* Compute the number of operands in an expression node NODE.  For
   tcc_vl_exp nodes like CALL_EXPRs, this is stored in the node itself,
   otherwise it is looked up from the node's code.  */
inline int
tree_operand_length (const_tree node)
{
  if (VL_EXP_CLASS_P (node))
    return VL_EXP_OPERAND_LENGTH (node);
  else
    return TREE_CODE_LENGTH (TREE_CODE (node));
}

#if defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 2007)

/* Special checks for TREE_OPERANDs.  */
inline tree *
tree_operand_check (tree __t, int __i,
                    const char *__f, int __l, const char *__g)
{
  const_tree __u = EXPR_CHECK (__t);
  if (__i < 0 || __i >= TREE_OPERAND_LENGTH (__u))
    tree_operand_check_failed (__i, __u, __f, __l, __g);
  return &CONST_CAST_TREE (__u)->exp.operands[__i];
}

inline tree *
tree_operand_check_code (tree __t, enum tree_code __code, int __i,
                         const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != __code)
    tree_check_failed (__t, __f, __l, __g, __code, 0);
  if (__i < 0 || __i >= TREE_OPERAND_LENGTH (__t))
    tree_operand_check_failed (__i, __t, __f, __l, __g);
  return &__t->exp.operands[__i];
}

inline const_tree *
tree_operand_check (const_tree __t, int __i,
                    const char *__f, int __l, const char *__g)
{
  const_tree __u = EXPR_CHECK (__t);
  if (__i < 0 || __i >= TREE_OPERAND_LENGTH (__u))
    tree_operand_check_failed (__i, __u, __f, __l, __g);
  return CONST_CAST (const_tree *, &__u->exp.operands[__i]);
}

inline const_tree *
tree_operand_check_code (const_tree __t, enum tree_code __code, int __i,
                         const char *__f, int __l, const char *__g)
{
  if (TREE_CODE (__t) != __code)
    tree_check_failed (__t, __f, __l, __g, __code, 0);
  if (__i < 0 || __i >= TREE_OPERAND_LENGTH (__t))
    tree_operand_check_failed (__i, __t, __f, __l, __g);
  return CONST_CAST (const_tree *, &__t->exp.operands[__i]);
}

#endif

/* True iff an identifier matches a C string.  */

inline bool
id_equal (const_tree id, const char *str)
{
  return !strcmp (IDENTIFIER_POINTER (id), str);
}

inline bool
id_equal (const char *str, const_tree id)
{
  return id_equal (id, str);
}

/* Return the number of elements in the VECTOR_TYPE given by NODE.  */

inline poly_uint64
TYPE_VECTOR_SUBPARTS (const_tree node)
{
  STATIC_ASSERT (NUM_POLY_INT_COEFFS <= 2);
  unsigned int precision = VECTOR_TYPE_CHECK (node)->type_common.precision;
  if (NUM_POLY_INT_COEFFS == 2)
    {
      /* See the corresponding code in SET_TYPE_VECTOR_SUBPARTS for a
	 description of the encoding.  */
      poly_uint64 res = 0;
      res.coeffs[0] = HOST_WIDE_INT_1U << (precision & 0xff);
      if (precision & 0x100)
	res.coeffs[1] = HOST_WIDE_INT_1U << (precision & 0xff);
      return res;
    }
  else
    return HOST_WIDE_INT_1U << precision;
}

/* Set the number of elements in VECTOR_TYPE NODE to SUBPARTS, which must
   satisfy valid_vector_subparts_p.  */

inline void
SET_TYPE_VECTOR_SUBPARTS (tree node, poly_uint64 subparts)
{
  STATIC_ASSERT (NUM_POLY_INT_COEFFS <= 2);
  unsigned HOST_WIDE_INT coeff0 = subparts.coeffs[0];
  int index = exact_log2 (coeff0);
  gcc_assert (index >= 0);
  if (NUM_POLY_INT_COEFFS == 2)
    {
      /* We have two coefficients that are each in the range 1 << [0, 63],
	 so supporting all combinations would require 6 bits per coefficient
	 and 12 bits in total.  Since the precision field is only 10 bits
	 in size, we need to be more restrictive than that.

	 At present, coeff[1] is always either 0 (meaning that the number
	 of units is constant) or equal to coeff[0] (meaning that the number
	 of units is N + X * N for some target-dependent zero-based runtime
	 parameter X).  We can therefore encode coeff[1] in a single bit.

	 The most compact encoding would be to use mask 0x3f for coeff[0]
	 and 0x40 for coeff[1], leaving 0x380 unused.  It's possible to
	 get slightly more efficient code on some hosts if we instead
	 treat the shift amount as an independent byte, so here we use
	 0xff for coeff[0] and 0x100 for coeff[1].  */
      unsigned HOST_WIDE_INT coeff1 = subparts.coeffs[1];
      gcc_assert (coeff1 == 0 || coeff1 == coeff0);
      VECTOR_TYPE_CHECK (node)->type_common.precision
	= index + (coeff1 != 0 ? 0x100 : 0);
    }
  else
    VECTOR_TYPE_CHECK (node)->type_common.precision = index;
}

/* Return true if we can construct vector types with the given number
   of subparts.  */

inline bool
valid_vector_subparts_p (poly_uint64 subparts)
{
  unsigned HOST_WIDE_INT coeff0 = subparts.coeffs[0];
  if (!pow2p_hwi (coeff0))
    return false;
  if (NUM_POLY_INT_COEFFS == 2)
    {
      unsigned HOST_WIDE_INT coeff1 = subparts.coeffs[1];
      if (coeff1 != 0 && coeff1 != coeff0)
	return false;
    }
  return true;
}

/* Return the built-in function that DECL represents, given that it is known
   to be a FUNCTION_DECL with built-in class BUILT_IN_NORMAL.  */
inline built_in_function
DECL_FUNCTION_CODE (const_tree decl)
{
  const tree_function_decl &fndecl = FUNCTION_DECL_CHECK (decl)->function_decl;
  gcc_checking_assert (fndecl.built_in_class == BUILT_IN_NORMAL);
  return (built_in_function) fndecl.function_code;
}

/* Return the target-specific built-in function that DECL represents,
   given that it is known to be a FUNCTION_DECL with built-in class
   BUILT_IN_MD.  */
inline int
DECL_MD_FUNCTION_CODE (const_tree decl)
{
  const tree_function_decl &fndecl = FUNCTION_DECL_CHECK (decl)->function_decl;
  gcc_checking_assert (fndecl.built_in_class == BUILT_IN_MD);
  return fndecl.function_code;
}

/* Return the frontend-specific built-in function that DECL represents,
   given that it is known to be a FUNCTION_DECL with built-in class
   BUILT_IN_FRONTEND.  */
inline int
DECL_FE_FUNCTION_CODE (const_tree decl)
{
  const tree_function_decl &fndecl = FUNCTION_DECL_CHECK (decl)->function_decl;
  gcc_checking_assert (fndecl.built_in_class == BUILT_IN_FRONTEND);
  return fndecl.function_code;
}

/* Record that FUNCTION_DECL DECL represents built-in function FCODE of
   class FCLASS.  */
inline void
set_decl_built_in_function (tree decl, built_in_class fclass,
			    unsigned int fcode)
{
  tree_function_decl &fndecl = FUNCTION_DECL_CHECK (decl)->function_decl;
  fndecl.built_in_class = fclass;
  fndecl.function_code = fcode;
}

/* Record that FUNCTION_DECL NEWDECL represents the same built-in function
   as OLDDECL (or none, if OLDDECL doesn't represent a built-in function).  */
inline void
copy_decl_built_in_function (tree newdecl, const_tree olddecl)
{
  tree_function_decl &newfndecl = FUNCTION_DECL_CHECK (newdecl)->function_decl;
  const tree_function_decl &oldfndecl
    = FUNCTION_DECL_CHECK (olddecl)->function_decl;
  newfndecl.built_in_class = oldfndecl.built_in_class;
  newfndecl.function_code = oldfndecl.function_code;
}

/* In NON_LVALUE_EXPR and VIEW_CONVERT_EXPR, set when this node is merely a
   wrapper added to express a location_t on behalf of the node's child
   (e.g. by maybe_wrap_with_location).  */

#define EXPR_LOCATION_WRAPPER_P(NODE) \
  (TREE_CHECK2(NODE, NON_LVALUE_EXPR, VIEW_CONVERT_EXPR)->base.public_flag)

/* Test if EXP is merely a wrapper node, added to express a location_t
   on behalf of the node's child (e.g. by maybe_wrap_with_location).  */

inline bool
location_wrapper_p (const_tree exp)
{
  /* A wrapper node has code NON_LVALUE_EXPR or VIEW_CONVERT_EXPR, and
     the flag EXPR_LOCATION_WRAPPER_P is set.
     It normally has the same type as its operand, but it can have a
     different one if the type of the operand has changed (e.g. when
     merging duplicate decls).

     NON_LVALUE_EXPR is used for wrapping constants, apart from STRING_CST.
     VIEW_CONVERT_EXPR is used for wrapping non-constants and STRING_CST.  */
  if ((TREE_CODE (exp) == NON_LVALUE_EXPR
       || TREE_CODE (exp) == VIEW_CONVERT_EXPR)
      && EXPR_LOCATION_WRAPPER_P (exp))
    return true;
  return false;
}

/* Implementation of STRIP_ANY_LOCATION_WRAPPER.  */

inline tree
tree_strip_any_location_wrapper (tree exp)
{
  if (location_wrapper_p (exp))
    return TREE_OPERAND (exp, 0);
  else
    return exp;
}

#define error_mark_node			global_trees[TI_ERROR_MARK]

#define intQI_type_node			global_trees[TI_INTQI_TYPE]
#define intHI_type_node			global_trees[TI_INTHI_TYPE]
#define intSI_type_node			global_trees[TI_INTSI_TYPE]
#define intDI_type_node			global_trees[TI_INTDI_TYPE]
#define intTI_type_node			global_trees[TI_INTTI_TYPE]

#define unsigned_intQI_type_node	global_trees[TI_UINTQI_TYPE]
#define unsigned_intHI_type_node	global_trees[TI_UINTHI_TYPE]
#define unsigned_intSI_type_node	global_trees[TI_UINTSI_TYPE]
#define unsigned_intDI_type_node	global_trees[TI_UINTDI_TYPE]
#define unsigned_intTI_type_node	global_trees[TI_UINTTI_TYPE]

#define atomicQI_type_node	global_trees[TI_ATOMICQI_TYPE]
#define atomicHI_type_node	global_trees[TI_ATOMICHI_TYPE]
#define atomicSI_type_node	global_trees[TI_ATOMICSI_TYPE]
#define atomicDI_type_node	global_trees[TI_ATOMICDI_TYPE]
#define atomicTI_type_node	global_trees[TI_ATOMICTI_TYPE]

#define uint16_type_node		global_trees[TI_UINT16_TYPE]
#define uint32_type_node		global_trees[TI_UINT32_TYPE]
#define uint64_type_node		global_trees[TI_UINT64_TYPE]
#define uint128_type_node		global_trees[TI_UINT128_TYPE]

#define void_node			global_trees[TI_VOID]

#define integer_zero_node		global_trees[TI_INTEGER_ZERO]
#define integer_one_node		global_trees[TI_INTEGER_ONE]
#define integer_minus_one_node		global_trees[TI_INTEGER_MINUS_ONE]
#define size_zero_node			global_trees[TI_SIZE_ZERO]
#define size_one_node			global_trees[TI_SIZE_ONE]
#define bitsize_zero_node		global_trees[TI_BITSIZE_ZERO]
#define bitsize_one_node		global_trees[TI_BITSIZE_ONE]
#define bitsize_unit_node		global_trees[TI_BITSIZE_UNIT]

/* Base access nodes.  */
#define access_public_node		global_trees[TI_PUBLIC]
#define access_protected_node	        global_trees[TI_PROTECTED]
#define access_private_node		global_trees[TI_PRIVATE]

#define null_pointer_node		global_trees[TI_NULL_POINTER]

#define float_type_node			global_trees[TI_FLOAT_TYPE]
#define double_type_node		global_trees[TI_DOUBLE_TYPE]
#define long_double_type_node		global_trees[TI_LONG_DOUBLE_TYPE]
#define bfloat16_type_node		global_trees[TI_BFLOAT16_TYPE]

/* Nodes for particular _FloatN and _FloatNx types in sequence.  */
#define FLOATN_TYPE_NODE(IDX)		global_trees[TI_FLOATN_TYPE_FIRST + (IDX)]
#define FLOATN_NX_TYPE_NODE(IDX)	global_trees[TI_FLOATN_NX_TYPE_FIRST + (IDX)]
#define FLOATNX_TYPE_NODE(IDX)		global_trees[TI_FLOATNX_TYPE_FIRST + (IDX)]

/* Names for individual types (code should normally iterate over all
   such types; these are only for back-end use, or in contexts such as
   *.def where iteration is not possible).  */
#define float16_type_node		global_trees[TI_FLOAT16_TYPE]
#define float32_type_node		global_trees[TI_FLOAT32_TYPE]
#define float64_type_node		global_trees[TI_FLOAT64_TYPE]
#define float128_type_node		global_trees[TI_FLOAT128_TYPE]
#define float32x_type_node		global_trees[TI_FLOAT32X_TYPE]
#define float64x_type_node		global_trees[TI_FLOAT64X_TYPE]
#define float128x_type_node		global_trees[TI_FLOAT128X_TYPE]

/* Type used by certain backends for __float128, which in C++ should be
   distinct type from _Float128 for backwards compatibility reasons.  */
#define float128t_type_node		global_trees[TI_FLOAT128T_TYPE]

#define float_ptr_type_node		global_trees[TI_FLOAT_PTR_TYPE]
#define double_ptr_type_node		global_trees[TI_DOUBLE_PTR_TYPE]
#define long_double_ptr_type_node	global_trees[TI_LONG_DOUBLE_PTR_TYPE]
#define integer_ptr_type_node		global_trees[TI_INTEGER_PTR_TYPE]

#define complex_integer_type_node	global_trees[TI_COMPLEX_INTEGER_TYPE]
#define complex_float_type_node		global_trees[TI_COMPLEX_FLOAT_TYPE]
#define complex_double_type_node	global_trees[TI_COMPLEX_DOUBLE_TYPE]
#define complex_long_double_type_node	global_trees[TI_COMPLEX_LONG_DOUBLE_TYPE]

#define COMPLEX_FLOATN_NX_TYPE_NODE(IDX)	global_trees[TI_COMPLEX_FLOATN_NX_TYPE_FIRST + (IDX)]

#define void_type_node			global_trees[TI_VOID_TYPE]
/* The C type `void *'.  */
#define ptr_type_node			global_trees[TI_PTR_TYPE]
/* The C type `const void *'.  */
#define const_ptr_type_node		global_trees[TI_CONST_PTR_TYPE]
/* The C type `size_t'.  */
#define size_type_node                  global_trees[TI_SIZE_TYPE]
#define pid_type_node                   global_trees[TI_PID_TYPE]
#define ptrdiff_type_node		global_trees[TI_PTRDIFF_TYPE]
#define va_list_type_node		global_trees[TI_VA_LIST_TYPE]
#define va_list_gpr_counter_field	global_trees[TI_VA_LIST_GPR_COUNTER_FIELD]
#define va_list_fpr_counter_field	global_trees[TI_VA_LIST_FPR_COUNTER_FIELD]
/* The C type `FILE *'.  */
#define fileptr_type_node		global_trees[TI_FILEPTR_TYPE]
/* The C type `const struct tm *'.  */
#define const_tm_ptr_type_node		global_trees[TI_CONST_TM_PTR_TYPE]
/* The C type `fenv_t *'.  */
#define fenv_t_ptr_type_node		global_trees[TI_FENV_T_PTR_TYPE]
#define const_fenv_t_ptr_type_node	global_trees[TI_CONST_FENV_T_PTR_TYPE]
/* The C type `fexcept_t *'.  */
#define fexcept_t_ptr_type_node		global_trees[TI_FEXCEPT_T_PTR_TYPE]
#define const_fexcept_t_ptr_type_node	global_trees[TI_CONST_FEXCEPT_T_PTR_TYPE]
#define pointer_sized_int_node		global_trees[TI_POINTER_SIZED_TYPE]

#define boolean_type_node		global_trees[TI_BOOLEAN_TYPE]
#define boolean_false_node		global_trees[TI_BOOLEAN_FALSE]
#define boolean_true_node		global_trees[TI_BOOLEAN_TRUE]

/* The decimal floating point types. */
#define dfloat32_type_node              global_trees[TI_DFLOAT32_TYPE]
#define dfloat64_type_node              global_trees[TI_DFLOAT64_TYPE]
#define dfloat128_type_node             global_trees[TI_DFLOAT128_TYPE]
#define dfloat64x_type_node		global_trees[TI_DFLOAT64X_TYPE]

/* The fixed-point types.  */
#define sat_short_fract_type_node       global_trees[TI_SAT_SFRACT_TYPE]
#define sat_fract_type_node             global_trees[TI_SAT_FRACT_TYPE]
#define sat_long_fract_type_node        global_trees[TI_SAT_LFRACT_TYPE]
#define sat_long_long_fract_type_node   global_trees[TI_SAT_LLFRACT_TYPE]
#define sat_unsigned_short_fract_type_node \
					global_trees[TI_SAT_USFRACT_TYPE]
#define sat_unsigned_fract_type_node    global_trees[TI_SAT_UFRACT_TYPE]
#define sat_unsigned_long_fract_type_node \
					global_trees[TI_SAT_ULFRACT_TYPE]
#define sat_unsigned_long_long_fract_type_node \
					global_trees[TI_SAT_ULLFRACT_TYPE]
#define short_fract_type_node           global_trees[TI_SFRACT_TYPE]
#define fract_type_node                 global_trees[TI_FRACT_TYPE]
#define long_fract_type_node            global_trees[TI_LFRACT_TYPE]
#define long_long_fract_type_node       global_trees[TI_LLFRACT_TYPE]
#define unsigned_short_fract_type_node  global_trees[TI_USFRACT_TYPE]
#define unsigned_fract_type_node        global_trees[TI_UFRACT_TYPE]
#define unsigned_long_fract_type_node   global_trees[TI_ULFRACT_TYPE]
#define unsigned_long_long_fract_type_node \
					global_trees[TI_ULLFRACT_TYPE]
#define sat_short_accum_type_node       global_trees[TI_SAT_SACCUM_TYPE]
#define sat_accum_type_node             global_trees[TI_SAT_ACCUM_TYPE]
#define sat_long_accum_type_node        global_trees[TI_SAT_LACCUM_TYPE]
#define sat_long_long_accum_type_node   global_trees[TI_SAT_LLACCUM_TYPE]
#define sat_unsigned_short_accum_type_node \
					global_trees[TI_SAT_USACCUM_TYPE]
#define sat_unsigned_accum_type_node    global_trees[TI_SAT_UACCUM_TYPE]
#define sat_unsigned_long_accum_type_node \
					global_trees[TI_SAT_ULACCUM_TYPE]
#define sat_unsigned_long_long_accum_type_node \
					global_trees[TI_SAT_ULLACCUM_TYPE]
#define short_accum_type_node           global_trees[TI_SACCUM_TYPE]
#define accum_type_node                 global_trees[TI_ACCUM_TYPE]
#define long_accum_type_node            global_trees[TI_LACCUM_TYPE]
#define long_long_accum_type_node       global_trees[TI_LLACCUM_TYPE]
#define unsigned_short_accum_type_node  global_trees[TI_USACCUM_TYPE]
#define unsigned_accum_type_node        global_trees[TI_UACCUM_TYPE]
#define unsigned_long_accum_type_node   global_trees[TI_ULACCUM_TYPE]
#define unsigned_long_long_accum_type_node \
					global_trees[TI_ULLACCUM_TYPE]
#define qq_type_node                    global_trees[TI_QQ_TYPE]
#define hq_type_node                    global_trees[TI_HQ_TYPE]
#define sq_type_node                    global_trees[TI_SQ_TYPE]
#define dq_type_node                    global_trees[TI_DQ_TYPE]
#define tq_type_node                    global_trees[TI_TQ_TYPE]
#define uqq_type_node                   global_trees[TI_UQQ_TYPE]
#define uhq_type_node                   global_trees[TI_UHQ_TYPE]
#define usq_type_node                   global_trees[TI_USQ_TYPE]
#define udq_type_node                   global_trees[TI_UDQ_TYPE]
#define utq_type_node                   global_trees[TI_UTQ_TYPE]
#define sat_qq_type_node                global_trees[TI_SAT_QQ_TYPE]
#define sat_hq_type_node                global_trees[TI_SAT_HQ_TYPE]
#define sat_sq_type_node                global_trees[TI_SAT_SQ_TYPE]
#define sat_dq_type_node                global_trees[TI_SAT_DQ_TYPE]
#define sat_tq_type_node                global_trees[TI_SAT_TQ_TYPE]
#define sat_uqq_type_node               global_trees[TI_SAT_UQQ_TYPE]
#define sat_uhq_type_node               global_trees[TI_SAT_UHQ_TYPE]
#define sat_usq_type_node               global_trees[TI_SAT_USQ_TYPE]
#define sat_udq_type_node               global_trees[TI_SAT_UDQ_TYPE]
#define sat_utq_type_node               global_trees[TI_SAT_UTQ_TYPE]
#define ha_type_node                    global_trees[TI_HA_TYPE]
#define sa_type_node                    global_trees[TI_SA_TYPE]
#define da_type_node                    global_trees[TI_DA_TYPE]
#define ta_type_node                    global_trees[TI_TA_TYPE]
#define uha_type_node                   global_trees[TI_UHA_TYPE]
#define usa_type_node                   global_trees[TI_USA_TYPE]
#define uda_type_node                   global_trees[TI_UDA_TYPE]
#define uta_type_node                   global_trees[TI_UTA_TYPE]
#define sat_ha_type_node                global_trees[TI_SAT_HA_TYPE]
#define sat_sa_type_node                global_trees[TI_SAT_SA_TYPE]
#define sat_da_type_node                global_trees[TI_SAT_DA_TYPE]
#define sat_ta_type_node                global_trees[TI_SAT_TA_TYPE]
#define sat_uha_type_node               global_trees[TI_SAT_UHA_TYPE]
#define sat_usa_type_node               global_trees[TI_SAT_USA_TYPE]
#define sat_uda_type_node               global_trees[TI_SAT_UDA_TYPE]
#define sat_uta_type_node               global_trees[TI_SAT_UTA_TYPE]

/* The node that should be placed at the end of a parameter list to
   indicate that the function does not take a variable number of
   arguments.  The TREE_VALUE will be void_type_node and there will be
   no TREE_CHAIN.  Language-independent code should not assume
   anything else about this node.  */
#define void_list_node                  global_trees[TI_VOID_LIST_NODE]

#define main_identifier_node		global_trees[TI_MAIN_IDENTIFIER]
#define MAIN_NAME_P(NODE) \
  (IDENTIFIER_NODE_CHECK (NODE) == main_identifier_node)

/* Optimization options (OPTIMIZATION_NODE) to use for default and current
   functions.  */
#define optimization_default_node	global_trees[TI_OPTIMIZATION_DEFAULT]
#define optimization_current_node	global_trees[TI_OPTIMIZATION_CURRENT]

/* Default/current target options (TARGET_OPTION_NODE).  */
#define target_option_default_node	global_trees[TI_TARGET_OPTION_DEFAULT]
#define target_option_current_node	global_trees[TI_TARGET_OPTION_CURRENT]

/* Default tree list option(), optimize() pragmas to be linked into the
   attribute list.  */
#define current_target_pragma		global_trees[TI_CURRENT_TARGET_PRAGMA]
#define current_optimize_pragma		global_trees[TI_CURRENT_OPTIMIZE_PRAGMA]

/* SCEV analyzer global shared trees.  */
#define chrec_not_analyzed_yet		NULL_TREE
#define chrec_dont_know			global_trees[TI_CHREC_DONT_KNOW]
#define chrec_known			global_trees[TI_CHREC_KNOWN]

#define char_type_node			integer_types[itk_char]
#define signed_char_type_node		integer_types[itk_signed_char]
#define unsigned_char_type_node		integer_types[itk_unsigned_char]
#define short_integer_type_node		integer_types[itk_short]
#define short_unsigned_type_node	integer_types[itk_unsigned_short]
#define integer_type_node		integer_types[itk_int]
#define unsigned_type_node		integer_types[itk_unsigned_int]
#define long_integer_type_node		integer_types[itk_long]
#define long_unsigned_type_node		integer_types[itk_unsigned_long]
#define long_long_integer_type_node	integer_types[itk_long_long]
#define long_long_unsigned_type_node	integer_types[itk_unsigned_long_long]

/* True if T is an erroneous expression.  */

inline bool
error_operand_p (const_tree t)
{
  return (t == error_mark_node
	  || (t && TREE_TYPE (t) == error_mark_node));
}

/* Return the number of elements encoded directly in a VECTOR_CST.  */

inline unsigned int
vector_cst_encoded_nelts (const_tree t)
{
  return VECTOR_CST_NPATTERNS (t) * VECTOR_CST_NELTS_PER_PATTERN (t);
}

extern tree generate_internal_label (const char *);
extern const char *prefix_for_internal_label (tree label);
extern tree decl_assembler_name (tree);
extern void overwrite_decl_assembler_name (tree decl, tree name);
extern tree decl_comdat_group (const_tree);
extern tree decl_comdat_group_id (const_tree);
extern const char *decl_section_name (const_tree);
extern void set_decl_section_name (tree, const char *);
extern void set_decl_section_name (tree, const_tree);
extern enum tls_model decl_tls_model (const_tree);
extern void set_decl_tls_model (tree, enum tls_model);

/* Compute the number of bytes occupied by 'node'.  This routine only
   looks at TREE_CODE and, if the code is TREE_VEC, TREE_VEC_LENGTH.  */

extern size_t tree_size (const_tree);

/* Compute the number of bytes occupied by a tree with code CODE.
   This function cannot be used for TREE_VEC or INTEGER_CST nodes,
   which are of variable length.  */
extern size_t tree_code_size (enum tree_code);

/* Allocate and return a new UID from the DECL_UID namespace.  */
extern int allocate_decl_uid (void);

/* Lowest level primitive for allocating a node.
   The TREE_CODE is the only argument.  Contents are initialized
   to zero except for a few of the common fields.  */

extern tree make_node (enum tree_code CXX_MEM_STAT_INFO);

/* Free tree node.  */

extern void free_node (tree);

/* Make a copy of a node, with all the same contents.  */

extern tree copy_node (tree CXX_MEM_STAT_INFO);

/* Make a copy of a chain of TREE_LIST nodes.  */

extern tree copy_list (tree);

/* Make a CASE_LABEL_EXPR.  */

extern tree build_case_label (tree, tree, tree);

/* Make a BINFO.  */
extern tree make_tree_binfo (unsigned CXX_MEM_STAT_INFO);

/* Make an INTEGER_CST.  */

extern tree make_int_cst (int, int CXX_MEM_STAT_INFO);

/* Make a TREE_VEC.  */

extern tree make_tree_vec (int CXX_MEM_STAT_INFO);

/* Grow a TREE_VEC.  */

extern tree grow_tree_vec (tree v, int CXX_MEM_STAT_INFO);

/* Treat a TREE_VEC as a range of trees, e.g.
   for (tree e : tree_vec_range (v)) { ... }  */

class tree_vec_range
{
  tree v;
public:
  tree_vec_range(tree v) : v(v) { }
  tree *begin() { return TREE_VEC_BEGIN (v); }
  tree *end() { return TREE_VEC_END (v); }
};

/* Construct various types of nodes.  */

extern tree build_nt (enum tree_code, ...);
extern tree build_nt_call_vec (tree, vec<tree, va_gc> *);

extern tree build0 (enum tree_code, tree CXX_MEM_STAT_INFO);
extern tree build1 (enum tree_code, tree, tree CXX_MEM_STAT_INFO);
extern tree build2 (enum tree_code, tree, tree, tree CXX_MEM_STAT_INFO);
extern tree build3 (enum tree_code, tree, tree, tree, tree CXX_MEM_STAT_INFO);
extern tree build4 (enum tree_code, tree, tree, tree, tree,
		    tree CXX_MEM_STAT_INFO);
extern tree build5 (enum tree_code, tree, tree, tree, tree, tree,
		    tree CXX_MEM_STAT_INFO);

/* _loc versions of build[1-5].  */

inline tree
build1_loc (location_t loc, enum tree_code code, tree type,
	    tree arg1 CXX_MEM_STAT_INFO)
{
  tree t = build1 (code, type, arg1 PASS_MEM_STAT);
  if (CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, loc);
  return t;
}

inline tree
build2_loc (location_t loc, enum tree_code code, tree type, tree arg0,
	    tree arg1 CXX_MEM_STAT_INFO)
{
  tree t = build2 (code, type, arg0, arg1 PASS_MEM_STAT);
  if (CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, loc);
  return t;
}

inline tree
build3_loc (location_t loc, enum tree_code code, tree type, tree arg0,
	    tree arg1, tree arg2 CXX_MEM_STAT_INFO)
{
  tree t = build3 (code, type, arg0, arg1, arg2 PASS_MEM_STAT);
  if (CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, loc);
  return t;
}

inline tree
build4_loc (location_t loc, enum tree_code code, tree type, tree arg0,
	    tree arg1, tree arg2, tree arg3 CXX_MEM_STAT_INFO)
{
  tree t = build4 (code, type, arg0, arg1, arg2, arg3 PASS_MEM_STAT);
  if (CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, loc);
  return t;
}

inline tree
build5_loc (location_t loc, enum tree_code code, tree type, tree arg0,
	    tree arg1, tree arg2, tree arg3, tree arg4 CXX_MEM_STAT_INFO)
{
  tree t = build5 (code, type, arg0, arg1, arg2, arg3,
			arg4 PASS_MEM_STAT);
  if (CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, loc);
  return t;
}

/* Constructs double_int from tree CST.  */

extern tree double_int_to_tree (tree, double_int);

extern tree wide_int_to_tree (tree type, const poly_wide_int_ref &cst);
extern tree force_fit_type (tree, const poly_wide_int_ref &, int, bool);

/* Create an INT_CST node with a CST value zero extended.  */

/* static inline */
extern tree build_int_cst (tree, poly_int64);
extern tree build_int_cstu (tree type, poly_uint64);
extern tree build_int_cst_type (tree, poly_int64);
extern tree make_vector (unsigned, unsigned CXX_MEM_STAT_INFO);
extern tree build_vector_from_ctor (tree, const vec<constructor_elt, va_gc> *);
extern tree build_vector_from_val (tree, tree);
extern tree build_uniform_cst (tree, tree);
extern tree build_vec_series (tree, tree, tree);
extern tree build_index_vector (tree, poly_uint64, poly_uint64);
extern tree build_vector_a_then_b (tree, unsigned int, tree, tree);
extern void recompute_constructor_flags (tree);
extern void verify_constructor_flags (tree);
extern tree build_constructor (tree, vec<constructor_elt, va_gc> * CXX_MEM_STAT_INFO);
extern tree build_constructor_single (tree, tree, tree);
extern tree build_constructor_from_list (tree, tree);
extern tree build_constructor_from_vec (tree, const vec<tree, va_gc> *);
extern tree build_constructor_va (tree, int, ...);
extern tree build_clobber (tree, enum clobber_kind = CLOBBER_UNDEF);
extern tree build_real_from_int_cst (tree, const_tree);
extern tree build_real_from_wide (tree, const wide_int_ref &, signop);
extern tree build_complex (tree, tree, tree);
extern tree build_complex_inf (tree, bool);
extern tree build_each_one_cst (tree);
extern tree build_one_cst (tree);
extern tree build_minus_one_cst (tree);
extern tree build_all_ones_cst (tree);
extern tree build_zero_cst (tree);
extern tree build_replicated_int_cst (tree, unsigned, HOST_WIDE_INT);
extern tree sign_mask_for (tree);
extern tree build_string (unsigned, const char * = NULL);
extern tree build_poly_int_cst (tree, const poly_wide_int_ref &);
extern tree build_tree_list (tree, tree CXX_MEM_STAT_INFO);
extern tree build_tree_list_vec (const vec<tree, va_gc> * CXX_MEM_STAT_INFO);
extern tree build_decl (location_t, enum tree_code,
			tree, tree CXX_MEM_STAT_INFO);
extern tree build_debug_expr_decl (tree type);
extern tree build_fn_decl (const char *, tree);
extern tree build_translation_unit_decl (tree);
extern tree build_block (tree, tree, tree, tree);
extern tree build_empty_stmt (location_t);
extern tree build_omp_clause (location_t, enum omp_clause_code);

extern tree build_vl_exp (enum tree_code, int CXX_MEM_STAT_INFO);

extern tree build_call_nary (tree, tree, int, ...);
extern tree build_call_valist (tree, tree, int, va_list);
#define build_call_array(T1,T2,N,T3)\
   build_call_array_loc (UNKNOWN_LOCATION, T1, T2, N, T3)
extern tree build_call_array_loc (location_t, tree, tree, int, const tree *);
extern tree build_call_vec (tree, tree, const vec<tree, va_gc> *);
extern tree build_call_expr_loc_array (location_t, tree, int, tree *);
extern tree build_call_expr_loc_vec (location_t, tree, vec<tree, va_gc> *);
extern tree build_call_expr_loc (location_t, tree, int, ...);
extern tree build_call_expr (tree, int, ...);
extern tree build_call_expr_internal_loc (location_t, enum internal_fn,
					  tree, int, ...);
extern tree build_call_expr_internal_loc_array (location_t, enum internal_fn,
						tree, int, const tree *);
extern tree maybe_build_call_expr_loc (location_t, combined_fn, tree,
				       int, ...);
extern tree build_alloca_call_expr (tree, unsigned int, HOST_WIDE_INT);
extern tree build_string_literal (unsigned, const char * = NULL,
				  tree = char_type_node,
				  unsigned HOST_WIDE_INT = HOST_WIDE_INT_M1U);
inline tree build_string_literal (const char *p)
{ return build_string_literal (strlen (p) + 1, p); }
inline tree build_string_literal (tree t)
{
  return build_string_literal (IDENTIFIER_LENGTH (t) + 1,
			       IDENTIFIER_POINTER (t));
}

/* Construct various nodes representing data types.  */

extern tree signed_or_unsigned_type_for (int, tree);
extern tree signed_type_for (tree);
extern tree unsigned_type_for (tree);
extern bool is_truth_type_for (tree, tree);
extern bool tree_zero_one_valued_p (tree);
extern tree truth_type_for (tree);
extern tree build_pointer_type_for_mode (tree, machine_mode, bool);
extern tree build_pointer_type (tree);
extern tree build_reference_type_for_mode (tree, machine_mode, bool);
extern tree build_reference_type (tree);
extern tree build_vector_type_for_mode (tree, machine_mode);
extern tree build_vector_type (tree, poly_int64);
extern tree build_truth_vector_type_for_mode (poly_uint64, machine_mode);
extern tree build_opaque_vector_type (tree, poly_int64);
extern tree build_index_type (tree);
extern tree build_array_type_1 (tree, tree, bool, bool, bool);
extern tree build_array_type (tree, tree, bool = false);
extern tree build_nonshared_array_type (tree, tree);
extern tree build_array_type_nelts (tree, poly_uint64);
extern tree build_function_type (tree, tree, bool = false);
extern tree build_function_type_list (tree, ...);
extern tree build_varargs_function_type_list (tree, ...);
extern tree build_function_type_array (tree, int, tree *);
extern tree build_varargs_function_type_array (tree, int, tree *);
#define build_function_type_vec(RET, V) \
  build_function_type_array (RET, vec_safe_length (V), vec_safe_address (V))
#define build_varargs_function_type_vec(RET, V) \
  build_varargs_function_type_array (RET, vec_safe_length (V), \
				     vec_safe_address (V))
extern tree build_method_type_directly (tree, tree, tree);
extern tree build_method_type (tree, tree);
extern tree build_offset_type (tree, tree);
extern tree build_complex_type (tree, bool named = false);
extern tree array_type_nelts_minus_one (const_tree);
extern tree array_type_nelts_top (tree);

extern tree value_member (tree, tree);
extern tree purpose_member (const_tree, tree);
extern bool vec_member (const_tree, vec<tree, va_gc> *);
extern tree chain_index (int, tree);

/* Arguments may be null.  */
extern bool tree_int_cst_equal (const_tree, const_tree);

/* The following predicates are safe to call with a null argument.  */
extern bool tree_fits_shwi_p (const_tree) ATTRIBUTE_PURE;
extern bool tree_fits_poly_int64_p (const_tree) ATTRIBUTE_PURE;
extern bool tree_fits_uhwi_p (const_tree) ATTRIBUTE_PURE;
extern bool tree_fits_poly_uint64_p (const_tree) ATTRIBUTE_PURE;

extern HOST_WIDE_INT tree_to_shwi (const_tree)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_PURE;
extern poly_int64 tree_to_poly_int64 (const_tree)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_PURE;
extern unsigned HOST_WIDE_INT tree_to_uhwi (const_tree)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_PURE;
extern poly_uint64 tree_to_poly_uint64 (const_tree)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_PURE;
#if !defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 4003)
extern inline __attribute__ ((__gnu_inline__)) HOST_WIDE_INT
tree_to_shwi (const_tree t)
{
  gcc_assert (tree_fits_shwi_p (t));
  return TREE_INT_CST_LOW (t);
}

extern inline __attribute__ ((__gnu_inline__)) unsigned HOST_WIDE_INT
tree_to_uhwi (const_tree t)
{
  gcc_assert (tree_fits_uhwi_p (t));
  return TREE_INT_CST_LOW (t);
}
#if NUM_POLY_INT_COEFFS == 1
extern inline __attribute__ ((__gnu_inline__)) poly_int64
tree_to_poly_int64 (const_tree t)
{
  gcc_assert (tree_fits_poly_int64_p (t));
  return TREE_INT_CST_LOW (t);
}

extern inline __attribute__ ((__gnu_inline__)) poly_uint64
tree_to_poly_uint64 (const_tree t)
{
  gcc_assert (tree_fits_poly_uint64_p (t));
  return TREE_INT_CST_LOW (t);
}
#endif
#endif
extern int tree_int_cst_sgn (const_tree);
extern int tree_int_cst_sign_bit (const_tree);
extern unsigned int tree_int_cst_min_precision (tree, signop);
extern tree excess_precision_type (tree);

/* Recursively examines the array elements of TYPE, until a non-array
   element type is found.  */

inline tree
strip_array_types (tree type)
{
  while (TREE_CODE (type) == ARRAY_TYPE)
    type = TREE_TYPE (type);

  return type;
}

/* Recursively traverse down pointer type layers to pointee type.  */

inline const_tree
strip_pointer_types (const_tree type)
{
  while (POINTER_TYPE_P (type))
    type = TREE_TYPE (type);

  return type;
}

/* Desription of the reason why the argument of valid_constant_size_p
   is not a valid size.  */
enum cst_size_error {
  cst_size_ok,
  cst_size_not_constant,
  cst_size_negative,
  cst_size_too_big,
  cst_size_overflow
};

extern bool valid_constant_size_p (const_tree, cst_size_error * = NULL);
extern tree max_object_size ();

/* Return true if T holds a value that can be represented as a poly_int64
   without loss of precision.  Store the value in *VALUE if so.  */

inline bool
poly_int_tree_p (const_tree t, poly_int64 *value)
{
  if (tree_fits_poly_int64_p (t))
    {
      *value = tree_to_poly_int64 (t);
      return true;
    }
  return false;
}

/* Return true if T holds a value that can be represented as a poly_uint64
   without loss of precision.  Store the value in *VALUE if so.  */

inline bool
poly_int_tree_p (const_tree t, poly_uint64 *value)
{
  if (tree_fits_poly_uint64_p (t))
    {
      *value = tree_to_poly_uint64 (t);
      return true;
    }
  return false;
}

/* From expmed.cc.  Since rtl.h is included after tree.h, we can't
   put the prototype here.  Rtl.h does declare the prototype if
   tree.h had been included.  */

extern tree make_tree (tree, rtx);

/* Returns true iff CAND and BASE have equivalent language-specific
   qualifiers.  */

extern bool check_lang_type (const_tree cand, const_tree base);

/* Returns true iff unqualified CAND and BASE are equivalent.  */

extern bool check_base_type (const_tree cand, const_tree base);

/* Check whether CAND is suitable to be returned from get_qualified_type
   (BASE, TYPE_QUALS).  */

extern bool check_qualified_type (const_tree, const_tree, int);

/* Return a version of the TYPE, qualified as indicated by the
   TYPE_QUALS, if one exists.  If no qualified version exists yet,
   return NULL_TREE.  */

extern tree get_qualified_type (tree, int);

/* Like get_qualified_type, but creates the type if it does not
   exist.  This function never returns NULL_TREE.  */

extern tree build_qualified_type (tree, int CXX_MEM_STAT_INFO);

/* Create a variant of type T with alignment ALIGN.  */

extern tree build_aligned_type (tree, unsigned int);

/* Like build_qualified_type, but only deals with the `const' and
   `volatile' qualifiers.  This interface is retained for backwards
   compatibility with the various front-ends; new code should use
   build_qualified_type instead.  */

#define build_type_variant(TYPE, CONST_P, VOLATILE_P)			\
  build_qualified_type ((TYPE),						\
			((CONST_P) ? TYPE_QUAL_CONST : 0)		\
			| ((VOLATILE_P) ? TYPE_QUAL_VOLATILE : 0))

/* Make a copy of a type node.  */

extern tree build_distinct_type_copy (tree CXX_MEM_STAT_INFO);
extern tree build_variant_type_copy (tree CXX_MEM_STAT_INFO);

/* Given a hashcode and a ..._TYPE node (for which the hashcode was made),
   return a canonicalized ..._TYPE node, so that duplicates are not made.
   How the hash code is computed is up to the caller, as long as any two
   callers that could hash identical-looking type nodes agree.  */

extern hashval_t type_hash_canon_hash (tree);
extern tree type_hash_canon (unsigned int, tree);

extern tree convert (tree, tree);
extern tree size_in_bytes_loc (location_t, const_tree);
inline tree
size_in_bytes (const_tree t)
{
  return size_in_bytes_loc (input_location, t);
}

extern HOST_WIDE_INT int_size_in_bytes (const_tree);
extern HOST_WIDE_INT max_int_size_in_bytes (const_tree);
extern tree bit_position (const_tree);
extern tree byte_position (const_tree);
extern HOST_WIDE_INT int_byte_position (const_tree);

/* Type for sizes of data-type.  */

#define sizetype sizetype_tab[(int) stk_sizetype]
#define bitsizetype sizetype_tab[(int) stk_bitsizetype]
#define ssizetype sizetype_tab[(int) stk_ssizetype]
#define sbitsizetype sizetype_tab[(int) stk_sbitsizetype]
#define size_int(L) size_int_kind (L, stk_sizetype)
#define ssize_int(L) size_int_kind (L, stk_ssizetype)
#define bitsize_int(L) size_int_kind (L, stk_bitsizetype)
#define sbitsize_int(L) size_int_kind (L, stk_sbitsizetype)

/* Log2 of BITS_PER_UNIT.  */

#if BITS_PER_UNIT == 8
#define LOG2_BITS_PER_UNIT 3
#elif BITS_PER_UNIT == 16
#define LOG2_BITS_PER_UNIT 4
#else
#error Unknown BITS_PER_UNIT
#endif

/* Concatenate two lists (chains of TREE_LIST nodes) X and Y
   by making the last node in X point to Y.
   Returns X, except if X is 0 returns Y.  */

extern tree chainon (tree, tree);

/* Make a new TREE_LIST node from specified PURPOSE, VALUE and CHAIN.  */

extern tree tree_cons (tree, tree, tree CXX_MEM_STAT_INFO);

/* Return the last tree node in a chain.  */

extern tree tree_last (tree);

/* Reverse the order of elements in a chain, and return the new head.  */

extern tree nreverse (tree);

/* Returns the length of a chain of nodes
   (number of chain pointers to follow before reaching a null pointer).  */

extern int list_length (const_tree);

/* Returns the first/last FIELD_DECL in a RECORD_TYPE.  */

extern tree first_field (const_tree) ATTRIBUTE_NONNULL (1);
extern tree last_field (const_tree) ATTRIBUTE_NONNULL (1);

/* Given an initializer INIT, return TRUE if INIT is zero or some
   aggregate of zeros.  Otherwise return FALSE.  If NONZERO is not
   null, set *NONZERO if and only if INIT is known not to be all
   zeros.  The combination of return value of false and *NONZERO
   false implies that INIT may but need not be all zeros.  Other
   combinations indicate definitive answers.  */

extern bool initializer_zerop (const_tree, bool * = NULL);
extern bool initializer_each_zero_or_onep (const_tree);

extern tree vector_cst_elt (const_tree, unsigned int);

/* Given a vector VEC, return its first element if all elements are
   the same.  Otherwise return NULL_TREE.  */

extern tree uniform_vector_p (const_tree);

/* If the argument is INTEGER_CST, return it.  If the argument is vector
   with all elements the same INTEGER_CST, return that INTEGER_CST.  Otherwise
   return NULL_TREE.  */

extern tree uniform_integer_cst_p (tree);

extern int single_nonzero_element (const_tree);

/* Given a CONSTRUCTOR CTOR, return the element values as a vector.  */

extern vec<tree, va_gc> *ctor_to_vec (tree);

/* zerop (tree x) is nonzero if X is a constant of value 0.  */

extern bool zerop (const_tree);

/* integer_zerop (tree x) is nonzero if X is an integer constant of value 0.  */

extern bool integer_zerop (const_tree);

/* integer_onep (tree x) is nonzero if X is an integer constant of value 1.  */

extern bool integer_onep (const_tree);

/* integer_onep (tree x) is nonzero if X is an integer constant of value 1, or
   a vector or complex where each part is 1.  */

extern bool integer_each_onep (const_tree);

/* integer_all_onesp (tree x) is nonzero if X is an integer constant
   all of whose significant bits are 1.  */

extern bool integer_all_onesp (const_tree);

/* integer_minus_onep (tree x) is nonzero if X is an integer constant of
   value -1.  */

extern bool integer_minus_onep (const_tree);

/* integer_pow2p (tree x) is nonzero is X is an integer constant with
   exactly one bit 1.  */

extern bool integer_pow2p (const_tree);

/* Checks to see if T is a constant or a constant vector and if each element E
   adheres to ~E + 1 == pow2 then return ~E otherwise NULL_TREE.  */

extern tree bitmask_inv_cst_vector_p (tree);

/* integer_nonzerop (tree x) is nonzero if X is an integer constant
   with a nonzero value.  */

extern bool integer_nonzerop (const_tree);

/* integer_truep (tree x) is nonzero if X is an integer constant of value 1 or
   a vector where each element is an integer constant of value -1.  */

extern bool integer_truep (const_tree);

extern bool cst_and_fits_in_hwi (const_tree);
extern tree num_ending_zeros (const_tree);

/* fixed_zerop (tree x) is nonzero if X is a fixed-point constant of
   value 0.  */

extern bool fixed_zerop (const_tree);

/* staticp (tree x) is nonzero if X is a reference to data allocated
   at a fixed address in memory.  Returns the outermost data.  */

extern tree staticp (tree);

/* save_expr (EXP) returns an expression equivalent to EXP
   but it can be used multiple times within context CTX
   and only evaluate EXP once.  */

extern tree save_expr (tree);

/* Return true if T is an object with invariant address.  */

extern bool address_invariant_p (tree);

/* Return true if T is function-invariant.  */

extern bool tree_invariant_p (tree);

/* Look inside EXPR into any simple arithmetic operations.  Return the
   outermost non-arithmetic or non-invariant node.  */

extern tree skip_simple_arithmetic (tree);

/* Look inside EXPR into simple arithmetic operations involving constants.
   Return the outermost non-arithmetic or non-constant node.  */

extern tree skip_simple_constant_arithmetic (tree);

/* Return which tree structure is used by T.  */

enum tree_node_structure_enum tree_node_structure (const_tree);

/* Return true if EXP contains a PLACEHOLDER_EXPR, i.e. if it represents a
   size or offset that depends on a field within a record.  */

extern bool contains_placeholder_p (const_tree);

/* This macro calls the above function but short-circuits the common
   case of a constant to save time.  Also check for null.  */

#define CONTAINS_PLACEHOLDER_P(EXP) \
  ((EXP) != 0 && ! TREE_CONSTANT (EXP) && contains_placeholder_p (EXP))

/* Return true if any part of the structure of TYPE involves a PLACEHOLDER_EXPR
   directly.  This includes size, bounds, qualifiers (for QUAL_UNION_TYPE) and
   field positions.  */

extern bool type_contains_placeholder_p (tree);

/* Given a tree EXP, find all occurrences of references to fields
   in a PLACEHOLDER_EXPR and place them in vector REFS without
   duplicates.  Also record VAR_DECLs and CONST_DECLs.  Note that
   we assume here that EXP contains only arithmetic expressions
   or CALL_EXPRs with PLACEHOLDER_EXPRs occurring only in their
   argument list.  */

extern void find_placeholder_in_expr (tree, vec<tree> *);

/* This macro calls the above function but short-circuits the common
   case of a constant to save time and also checks for NULL.  */

#define FIND_PLACEHOLDER_IN_EXPR(EXP, V) \
do {					 \
  if((EXP) && !TREE_CONSTANT (EXP))	 \
    find_placeholder_in_expr (EXP, V);	 \
} while (0)

/* Given a tree EXP, a FIELD_DECL F, and a replacement value R,
   return a tree with all occurrences of references to F in a
   PLACEHOLDER_EXPR replaced by R.  Also handle VAR_DECLs and
   CONST_DECLs.  Note that we assume here that EXP contains only
   arithmetic expressions or CALL_EXPRs with PLACEHOLDER_EXPRs
   occurring only in their argument list.  */

extern tree substitute_in_expr (tree, tree, tree);

/* This macro calls the above function but short-circuits the common
   case of a constant to save time and also checks for NULL.  */

#define SUBSTITUTE_IN_EXPR(EXP, F, R) \
  ((EXP) == 0 || TREE_CONSTANT (EXP) ? (EXP) : substitute_in_expr (EXP, F, R))

/* Similar, but look for a PLACEHOLDER_EXPR in EXP and find a replacement
   for it within OBJ, a tree that is an object or a chain of references.  */

extern tree substitute_placeholder_in_expr (tree, tree);

/* This macro calls the above function but short-circuits the common
   case of a constant to save time and also checks for NULL.  */

#define SUBSTITUTE_PLACEHOLDER_IN_EXPR(EXP, OBJ) \
  ((EXP) == 0 || TREE_CONSTANT (EXP) ? (EXP)	\
   : substitute_placeholder_in_expr (EXP, OBJ))


/* stabilize_reference (EXP) returns a reference equivalent to EXP
   but it can be used multiple times
   and only evaluate the subexpressions once.  */

extern tree stabilize_reference (tree);

/* Return EXP, stripped of any conversions to wider types
   in such a way that the result of converting to type FOR_TYPE
   is the same as if EXP were converted to FOR_TYPE.
   If FOR_TYPE is 0, it signifies EXP's type.  */

extern tree get_unwidened (tree, tree);

/* Return OP or a simpler expression for a narrower value
   which can be sign-extended or zero-extended to give back OP.
   Store in *UNSIGNEDP_PTR either 1 if the value should be zero-extended
   or 0 if the value should be sign-extended.  */

extern tree get_narrower (tree, int *);

/* Return true if T is an expression that get_inner_reference handles.  */

inline bool
handled_component_p (const_tree t)
{
  switch (TREE_CODE (t))
    {
    case COMPONENT_REF:
    case BIT_FIELD_REF:
    case ARRAY_REF:
    case ARRAY_RANGE_REF:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case VIEW_CONVERT_EXPR:
      return true;

    default:
      return false;
    }
}

/* Return true T is a component with reverse storage order.  */

inline bool
reverse_storage_order_for_component_p (tree t)
{
  /* The storage order only applies to scalar components.  */
  if (AGGREGATE_TYPE_P (TREE_TYPE (t))
      || POINTER_TYPE_P (TREE_TYPE (t))
      || VECTOR_TYPE_P (TREE_TYPE (t)))
    return false;

  if (TREE_CODE (t) == REALPART_EXPR || TREE_CODE (t) == IMAGPART_EXPR)
    t = TREE_OPERAND (t, 0);

  switch (TREE_CODE (t))
    {
    case ARRAY_REF:
    case COMPONENT_REF:
      /* ??? Fortran can take COMPONENT_REF of a VOID_TYPE.  */
      /* ??? UBSan can take COMPONENT_REF of a REFERENCE_TYPE.  */
      return AGGREGATE_TYPE_P (TREE_TYPE (TREE_OPERAND (t, 0)))
	     && TYPE_REVERSE_STORAGE_ORDER (TREE_TYPE (TREE_OPERAND (t, 0)));

    case BIT_FIELD_REF:
    case MEM_REF:
      return REF_REVERSE_STORAGE_ORDER (t);

    case ARRAY_RANGE_REF:
    case VIEW_CONVERT_EXPR:
    default:
      return false;
    }
}

/* Return true if T is a storage order barrier, i.e. a VIEW_CONVERT_EXPR
   that can modify the storage order of objects.  Note that, even if the
   TYPE_REVERSE_STORAGE_ORDER flag is set on both the inner type and the
   outer type, a VIEW_CONVERT_EXPR can modify the storage order because
   it can change the partition of the aggregate object into scalars.  */

inline bool
storage_order_barrier_p (const_tree t)
{
  if (TREE_CODE (t) != VIEW_CONVERT_EXPR)
    return false;

  if (AGGREGATE_TYPE_P (TREE_TYPE (t))
      && TYPE_REVERSE_STORAGE_ORDER (TREE_TYPE (t)))
    return true;

  tree op = TREE_OPERAND (t, 0);

  if (AGGREGATE_TYPE_P (TREE_TYPE (op))
      && TYPE_REVERSE_STORAGE_ORDER (TREE_TYPE (op)))
    return true;

  return reverse_storage_order_for_component_p (op);
}

/* Given a DECL or TYPE, return the scope in which it was declared, or
   NUL_TREE if there is no containing scope.  */

extern tree get_containing_scope (const_tree);

/* Returns the ultimate TRANSLATION_UNIT_DECL context of DECL or NULL.  */

extern const_tree get_ultimate_context (const_tree);

/* Return the FUNCTION_DECL which provides this _DECL with its context,
   or zero if none.  */
extern tree decl_function_context (const_tree);

/* Return the RECORD_TYPE, UNION_TYPE, or QUAL_UNION_TYPE which provides
   this _DECL with its context, or zero if none.  */
extern tree decl_type_context (const_tree);

/* Return true if EXPR is the real constant zero.  */
extern bool real_zerop (const_tree);

/* Initialize the iterator I with arguments from function FNDECL  */

inline void
function_args_iter_init (function_args_iterator *i, const_tree fntype)
{
  i->next = TYPE_ARG_TYPES (fntype);
}

/* Return a pointer that holds the next argument if there are more arguments to
   handle, otherwise return NULL.  */

inline tree *
function_args_iter_cond_ptr (function_args_iterator *i)
{
  return (i->next) ? &TREE_VALUE (i->next) : NULL;
}

/* Return the next argument if there are more arguments to handle, otherwise
   return NULL.  */

inline tree
function_args_iter_cond (function_args_iterator *i)
{
  return (i->next) ? TREE_VALUE (i->next) : NULL_TREE;
}

/* Advance to the next argument.  */
inline void
function_args_iter_next (function_args_iterator *i)
{
  gcc_assert (i->next != NULL_TREE);
  i->next = TREE_CHAIN (i->next);
}

/* Returns true if a BLOCK has a source location.
   BLOCK_SOURCE_LOCATION is set only to inlined function entry points,
   so the function returns true for all but the innermost and outermost
   blocks into which an expression has been inlined.  */

inline bool
inlined_function_outer_scope_p (const_tree block)
{
 return LOCATION_LOCUS (BLOCK_SOURCE_LOCATION (block)) != UNKNOWN_LOCATION;
}

/* Loop over all function arguments of FNTYPE.  In each iteration, PTR is set
   to point to the next tree element.  ITER is an instance of
   function_args_iterator used to iterate the arguments.  */
#define FOREACH_FUNCTION_ARGS_PTR(FNTYPE, PTR, ITER)			\
  for (function_args_iter_init (&(ITER), (FNTYPE));			\
       (PTR = function_args_iter_cond_ptr (&(ITER))) != NULL;		\
       function_args_iter_next (&(ITER)))

/* Loop over all function arguments of FNTYPE.  In each iteration, TREE is set
   to the next tree element.  ITER is an instance of function_args_iterator
   used to iterate the arguments.  */
#define FOREACH_FUNCTION_ARGS(FNTYPE, TREE, ITER)			\
  for (function_args_iter_init (&(ITER), (FNTYPE));			\
       (TREE = function_args_iter_cond (&(ITER))) != NULL_TREE;		\
       function_args_iter_next (&(ITER)))

/* In tree.cc */
extern unsigned crc32_unsigned_n (unsigned, unsigned, unsigned);
extern unsigned crc32_string (unsigned, const char *);
inline unsigned
crc32_unsigned (unsigned chksum, unsigned value)
{
  return crc32_unsigned_n (chksum, value, 4);
}
inline unsigned
crc32_byte (unsigned chksum, char byte)
{
  return crc32_unsigned_n (chksum, byte, 1);
}
extern void clean_symbol_name (char *);
extern tree get_file_function_name (const char *);
extern tree get_callee_fndecl (const_tree);
extern combined_fn get_call_combined_fn (const_tree);
extern int type_num_arguments (const_tree);
extern tree type_argument_type (const_tree, unsigned) ATTRIBUTE_NONNULL (1);
extern bool associative_tree_code (enum tree_code);
extern bool commutative_tree_code (enum tree_code);
extern bool commutative_ternary_tree_code (enum tree_code);
extern bool operation_can_overflow (enum tree_code);
extern bool operation_no_trapping_overflow (tree, enum tree_code);
extern tree upper_bound_in_type (tree, tree);
extern tree lower_bound_in_type (tree, tree);
extern bool operand_equal_for_phi_arg_p (const_tree, const_tree);
extern tree create_artificial_label (location_t);
extern const char *get_name (tree);
extern bool stdarg_p (const_tree);
extern bool prototype_p (const_tree);
extern bool auto_var_p (const_tree);
extern bool auto_var_in_fn_p (const_tree, const_tree);
extern tree build_low_bits_mask (tree, unsigned);
extern bool tree_nop_conversion_p (const_tree, const_tree);
extern tree tree_strip_nop_conversions (tree);
extern tree tree_strip_sign_nop_conversions (tree);
extern const_tree strip_invariant_refs (const_tree);
extern tree strip_zero_offset_components (tree);
extern tree lhd_gcc_personality (void);
extern void assign_assembler_name_if_needed (tree);
extern bool warn_deprecated_use (tree, tree);
extern void error_unavailable_use (tree, tree);
extern tree cache_integer_cst (tree, bool might_duplicate = false);
extern const char *combined_fn_name (combined_fn);

/* Returns true if X is a typedef decl.  */

inline bool
is_typedef_decl (const_tree x)
{
  return (x && TREE_CODE (x) == TYPE_DECL
	  && DECL_ORIGINAL_TYPE (x) != NULL_TREE);
}

/* Returns true iff TYPE is a type variant created for a typedef. */

inline bool
typedef_variant_p (const_tree type)
{
  return is_typedef_decl (TYPE_NAME (type));
}

/* Compare and hash for any structure which begins with a canonical
   pointer.  Assumes all pointers are interchangeable, which is sort
   of already assumed by gcc elsewhere IIRC.  */

inline int
struct_ptr_eq (const void *a, const void *b)
{
  const void * const * x = (const void * const *) a;
  const void * const * y = (const void * const *) b;
  return *x == *y;
}

inline hashval_t
struct_ptr_hash (const void *a)
{
  const void * const * x = (const void * const *) a;
  return (intptr_t)*x >> 4;
}

/* Return true if CODE can be treated as a truncating division.

   EXACT_DIV_EXPR can be treated as a truncating division in which the
   remainder is known to be zero.  However, if trunc_div_p gates the
   generation of new IL, the conservative choice for that new IL is
   TRUNC_DIV_EXPR rather than CODE.  Using CODE (EXACT_DIV_EXPR) would
   only be correct if the transformation preserves exactness.  */
inline bool
trunc_or_exact_div_p (tree_code code)
{
  return code == TRUNC_DIV_EXPR || code == EXACT_DIV_EXPR;
}

/* Return nonzero if CODE is a tree code that represents a truth value.  */
inline bool
truth_value_p (enum tree_code code)
{
  return (TREE_CODE_CLASS (code) == tcc_comparison
	  || code == TRUTH_AND_EXPR || code == TRUTH_ANDIF_EXPR
	  || code == TRUTH_OR_EXPR || code == TRUTH_ORIF_EXPR
	  || code == TRUTH_XOR_EXPR || code == TRUTH_NOT_EXPR);
}

/* Return whether TYPE is a type suitable for an offset for
   a POINTER_PLUS_EXPR.  */
inline bool
ptrofftype_p (tree type)
{
  return (INTEGRAL_TYPE_P (type)
	  && TYPE_PRECISION (type) == TYPE_PRECISION (sizetype)
	  && TYPE_UNSIGNED (type) == TYPE_UNSIGNED (sizetype));
}

/* Return true if the argument is a complete type or an array
   of unknown bound (whose type is incomplete but) whose elements
   have complete type.  */
inline bool
complete_or_array_type_p (const_tree type)
{
  return COMPLETE_TYPE_P (type)
         || (TREE_CODE (type) == ARRAY_TYPE
	     && COMPLETE_TYPE_P (TREE_TYPE (type)));
}

/* Return true if the value of T could be represented as a poly_widest_int.  */

inline bool
poly_int_tree_p (const_tree t)
{
  return (TREE_CODE (t) == INTEGER_CST || POLY_INT_CST_P (t));
}

/* Return the bit size of BIT_FIELD_REF T, in cases where it is known
   to be a poly_uint64.  (This is always true at the gimple level.)  */

inline poly_uint64
bit_field_size (const_tree t)
{
  return tree_to_poly_uint64 (TREE_OPERAND (t, 1));
}

/* Return the starting bit offset of BIT_FIELD_REF T, in cases where it is
   known to be a poly_uint64.  (This is always true at the gimple level.)  */

inline poly_uint64
bit_field_offset (const_tree t)
{
  return tree_to_poly_uint64 (TREE_OPERAND (t, 2));
}

extern tree strip_float_extensions (tree);
extern bool really_constant_p (const_tree);
extern bool ptrdiff_tree_p (const_tree, poly_int64 *);
extern bool decl_address_invariant_p (const_tree);
extern bool decl_address_ip_invariant_p (const_tree);
extern bool int_fits_type_p (const_tree, const_tree)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_NONNULL (2) ATTRIBUTE_PURE;
#ifndef GENERATOR_FILE
extern void get_type_static_bounds (const_tree, mpz_t, mpz_t);
#endif
extern bool variably_modified_type_p (tree, tree);
extern int tree_log2 (const_tree);
extern int tree_floor_log2 (const_tree);
extern unsigned int tree_ctz (const_tree);
extern int simple_cst_equal (const_tree, const_tree);

namespace inchash
{

extern void add_expr (const_tree, hash &, unsigned int = 0);

}

/* Compat version until all callers are converted. Return hash for
   TREE with SEED.  */
inline hashval_t iterative_hash_expr(const_tree tree, hashval_t seed)
{
  inchash::hash hstate (seed);
  inchash::add_expr (tree, hstate);
  return hstate.end ();
}

extern int compare_tree_int (const_tree, unsigned HOST_WIDE_INT);
extern bool type_list_equal (const_tree, const_tree);
extern bool chain_member (const_tree, const_tree);
extern void dump_tree_statistics (void);
extern void recompute_tree_invariant_for_addr_expr (tree);
extern bool needs_to_live_in_memory (const_tree);
extern tree reconstruct_complex_type (tree, tree);
extern bool real_onep (const_tree);
extern bool real_minus_onep (const_tree);
extern bool real_maybe_zerop (const_tree);
extern void init_ttree (void);
extern void build_common_tree_nodes (bool);
extern void build_common_builtin_nodes (void);
extern void tree_cc_finalize (void);
extern tree build_nonstandard_integer_type (unsigned HOST_WIDE_INT, int);
extern tree build_nonstandard_boolean_type (unsigned HOST_WIDE_INT);
extern tree build_bitint_type (unsigned HOST_WIDE_INT, int);
extern tree build_range_type (tree, tree, tree);
extern tree build_nonshared_range_type (tree, tree, tree);
extern bool subrange_type_for_debug_p (const_tree, tree *, tree *);
extern HOST_WIDE_INT int_cst_value (const_tree);
extern tree tree_block (tree);
extern void tree_set_block (tree, tree);
extern location_t *block_nonartificial_location (tree);
extern location_t tree_nonartificial_location (tree);
extern location_t tree_inlined_location (tree, bool = true);
extern tree block_ultimate_origin (const_tree);
extern tree get_binfo_at_offset (tree, poly_int64, tree);
extern bool virtual_method_call_p (const_tree, bool = false);
extern tree obj_type_ref_class (const_tree ref, bool = false);
extern bool types_same_for_odr (const_tree type1, const_tree type2);
extern bool contains_bitfld_component_ref_p (const_tree);
extern bool block_may_fallthru (const_tree);
extern void using_eh_for_cleanups (void);
extern bool using_eh_for_cleanups_p (void);
extern const char *get_tree_code_name (enum tree_code);
extern void set_call_expr_flags (tree, int);
extern tree walk_tree_1 (tree*, walk_tree_fn, void*, hash_set<tree>*,
			 walk_tree_lh);
extern tree walk_tree_without_duplicates_1 (tree*, walk_tree_fn, void*,
					    walk_tree_lh);
#define walk_tree(a,b,c,d) \
	walk_tree_1 (a, b, c, d, NULL)
#define walk_tree_without_duplicates(a,b,c) \
	walk_tree_without_duplicates_1 (a, b, c, NULL)

extern tree drop_tree_overflow (tree);

/* Given a memory reference expression T, return its base address.
   The base address of a memory reference expression is the main
   object being referenced.  */
extern tree get_base_address (tree t);

/* Return a tree of sizetype representing the size, in bytes, of the element
   of EXP, an ARRAY_REF or an ARRAY_RANGE_REF.  */
extern tree array_ref_element_size (tree);

/* Return a typenode for the "standard" C type with a given name.  */
extern tree get_typenode_from_name (const char *);

/* Return a tree representing the upper bound of the array mentioned in
   EXP, an ARRAY_REF or an ARRAY_RANGE_REF.  */
extern tree array_ref_up_bound (tree);

/* Return a tree representing the lower bound of the array mentioned in
   EXP, an ARRAY_REF or an ARRAY_RANGE_REF.  */
extern tree array_ref_low_bound (tree);

/* Returns true if REF is an array reference, a component reference,
   or a memory reference to an array whose actual size might be larger
   than its upper bound implies.  */
extern bool array_ref_flexible_size_p (tree, bool * = NULL);

/* Return a tree representing the offset, in bytes, of the field referenced
   by EXP.  This does not include any offset in DECL_FIELD_BIT_OFFSET.  */
extern tree component_ref_field_offset (tree);

/* Describes a "special" array member for a COMPONENT_REF.  */
enum struct special_array_member
  {
    none,	/* Not a special array member.  */
    int_0,	/* Interior array member with zero elements.  */
    trail_0,	/* Trailing array member with zero elements.  */
    trail_1,	/* Trailing array member with one element.  */
    trail_n,	/* Trailing array member with two or more elements.  */
    int_n	/* Interior array member with one or more elements.  */
  };

/* Determines the special array member type for a COMPONENT_REF.  */
extern special_array_member component_ref_sam_type (tree);

/* Return the size of the member referenced by the COMPONENT_REF, using
   its initializer expression if necessary in order to determine the size
   of an initialized flexible array member.  The size might be zero for
   an object with an uninitialized flexible array member or null if it
   cannot be determined.  */
extern tree component_ref_size (tree, special_array_member * = NULL);

/* Return true if the given node is a call to a .ACCESS_WITH_SIZE
   function.  */
extern bool is_access_with_size_p (const_tree);

/* Get the corresponding reference from the call to a .ACCESS_WITH_SIZE,
 * i.e. the first argument of this call.  Return NULL_TREE otherwise.  */
extern tree get_ref_from_access_with_size (tree);

extern int tree_map_base_eq (const void *, const void *);
extern unsigned int tree_map_base_hash (const void *);
extern bool tree_map_base_marked_p (const void *);
extern void DEBUG_FUNCTION verify_type (const_tree t);
extern bool gimple_canonical_types_compatible_p (const_tree, const_tree,
						 bool trust_type_canonical = true);
extern bool type_with_interoperable_signedness (const_tree);
extern bitmap get_nonnull_args (const_tree);
extern int get_range_pos_neg (tree, gimple * = NULL);

/* Return true for a valid pair of new and delete operators.  */
extern bool valid_new_delete_pair_p (tree, tree, bool * = NULL);

/* Return simplified tree code of type that is used for canonical type
   merging.  */
inline enum tree_code
tree_code_for_canonical_type_merging (enum tree_code code)
{
  /* By C standard, each enumerated type shall be compatible with char,
     a signed integer, or an unsigned integer.  The choice of type is
     implementation defined (in our case it depends on -fshort-enum).

     For this reason we make no distinction between ENUMERAL_TYPE and INTEGER
     type and compare only by their signedness and precision.  */
  if (code == ENUMERAL_TYPE)
    return INTEGER_TYPE;
  /* To allow inter-operability between languages having references and
     C, we consider reference types and pointers alike.  Note that this is
     not strictly necessary for C-Fortran 2008 interoperability because
     Fortran define C_PTR type that needs to be compatible with C pointers
     and we handle this one as ptr_type_node.  */
  if (code == REFERENCE_TYPE)
    return POINTER_TYPE;
  return code;
}

/* Return ture if get_alias_set care about TYPE_CANONICAL of given type.
   We don't define the types for pointers, arrays and vectors.  The reason is
   that pointers are handled specially: ptr_type_node accesses conflict with
   accesses to all other pointers.  This is done by alias.cc.
   Because alias sets of arrays and vectors are the same as types of their
   elements, we can't compute canonical type either.  Otherwise we could go
   form void *[10] to int *[10] (because they are equivalent for canonical type
   machinery) and get wrong TBAA.  */

inline bool
canonical_type_used_p (const_tree t)
{
  return !(POINTER_TYPE_P (t)
	   || TREE_CODE (t) == ARRAY_TYPE
	   || TREE_CODE (t) == VECTOR_TYPE);
}

/* Kinds of access to pass-by-reference arguments to functions.  */
enum access_mode
{
  access_none = 0,
  access_read_only = 1,
  access_write_only = 2,
  access_read_write = access_read_only | access_write_only,
  access_deferred = 4
};

#define tree_map_eq tree_map_base_eq
extern unsigned int tree_map_hash (const void *);
#define tree_map_marked_p tree_map_base_marked_p

#define tree_decl_map_eq tree_map_base_eq
extern unsigned int tree_decl_map_hash (const void *);
#define tree_decl_map_marked_p tree_map_base_marked_p

struct tree_decl_map_cache_hasher : ggc_cache_ptr_hash<tree_decl_map>
{
  static hashval_t hash (tree_decl_map *m) { return tree_decl_map_hash (m); }
  static bool
  equal (tree_decl_map *a, tree_decl_map *b)
  {
    return tree_decl_map_eq (a, b);
  }

  static int
  keep_cache_entry (tree_decl_map *&m)
  {
    return ggc_marked_p (m->base.from);
  }
};

#define tree_int_map_eq tree_map_base_eq
#define tree_int_map_hash tree_map_base_hash
#define tree_int_map_marked_p tree_map_base_marked_p

#define tree_vec_map_eq tree_map_base_eq
#define tree_vec_map_hash tree_decl_map_hash
#define tree_vec_map_marked_p tree_map_base_marked_p

struct tree_vec_map_cache_hasher : ggc_cache_ptr_hash<tree_vec_map>
{
  static hashval_t hash (tree_vec_map *m) { return DECL_UID (m->base.from); }

  static bool
  equal (tree_vec_map *a, tree_vec_map *b)
  {
    return a->base.from == b->base.from;
  }

  static int
  keep_cache_entry (tree_vec_map *&m)
  {
    return ggc_marked_p (m->base.from);
  }
};

/* Hasher for tree decls.  Pointer equality is enough here, but the DECL_UID
   is a better hash than the pointer value and gives a predictable traversal
   order.  Additionally it can be used across PCH save/restore.  */
struct tree_decl_hash : ggc_ptr_hash <tree_node>
{
  static inline hashval_t hash (tree);
};

inline hashval_t
tree_decl_hash::hash (tree t)
{
  return DECL_UID (t);
}

/* Similarly for types.  Uses TYPE_UID as hash function.  */
struct tree_type_hash : ggc_ptr_hash <tree_node>
{
  static inline hashval_t hash (tree);
};

inline hashval_t
tree_type_hash::hash (tree t)
{
  return TYPE_UID (t);
}

/* Hash for SSA_NAMEs in the same function.  Pointer equality is enough
   here, but the SSA_NAME_VERSION is a better hash than the pointer
   value and gives a predictable traversal order.  */
struct tree_ssa_name_hash : ggc_ptr_hash <tree_node>
{
  static inline hashval_t hash (tree);
};

inline hashval_t
tree_ssa_name_hash::hash (tree t)
{
  return SSA_NAME_VERSION (t);
}

/* Hasher for general trees, based on their TREE_HASH.  */
struct tree_hash : ggc_ptr_hash <tree_node>
{
  static hashval_t hash (tree);
};

inline hashval_t
tree_hash::hash (tree t)
{
  return TREE_HASH (t);
}

/* A hash_map of two trees for use with GTY((cache)).  Garbage collection for
   such a map will not mark keys, and will mark values if the key is already
   marked.  */
struct tree_cache_traits
  : simple_cache_map_traits<default_hash_traits<tree>, tree> { };
typedef hash_map<tree,tree,tree_cache_traits> tree_cache_map;

/* Similarly, but use DECL_UID as hash function rather than pointer hashing.
   This is for hash_maps from decls to trees that need to work across PCH.  */
struct decl_tree_cache_traits
  : simple_cache_map_traits<tree_decl_hash, tree> { };
typedef hash_map<tree,tree,decl_tree_cache_traits> decl_tree_cache_map;

/* Similarly, but use TYPE_UID as hash function rather than pointer hashing.
   This is for hash_maps from types to trees that need to work across PCH.  */
struct type_tree_cache_traits
  : simple_cache_map_traits<tree_type_hash, tree> { };
typedef hash_map<tree,tree,type_tree_cache_traits> type_tree_cache_map;

/* Similarly to decl_tree_cache_map, but without caching.  */
struct decl_tree_traits
  : simple_hashmap_traits<tree_decl_hash, tree> { };
typedef hash_map<tree,tree,decl_tree_traits> decl_tree_map;

/* Initialize the abstract argument list iterator object ITER with the
   arguments from CALL_EXPR node EXP.  */
inline void
init_call_expr_arg_iterator (tree exp, call_expr_arg_iterator *iter)
{
  iter->t = exp;
  iter->n = call_expr_nargs (exp);
  iter->i = 0;
}

inline void
init_const_call_expr_arg_iterator (const_tree exp, const_call_expr_arg_iterator *iter)
{
  iter->t = exp;
  iter->n = call_expr_nargs (exp);
  iter->i = 0;
}

/* Return the next argument from abstract argument list iterator object ITER,
   and advance its state.  Return NULL_TREE if there are no more arguments.  */
inline tree
next_call_expr_arg (call_expr_arg_iterator *iter)
{
  tree result;
  if (iter->i >= iter->n)
    return NULL_TREE;
  result = CALL_EXPR_ARG (iter->t, iter->i);
  iter->i++;
  return result;
}

inline const_tree
next_const_call_expr_arg (const_call_expr_arg_iterator *iter)
{
  const_tree result;
  if (iter->i >= iter->n)
    return NULL_TREE;
  result = CALL_EXPR_ARG (iter->t, iter->i);
  iter->i++;
  return result;
}

/* Initialize the abstract argument list iterator object ITER, then advance
   past and return the first argument.  Useful in for expressions, e.g.
     for (arg = first_call_expr_arg (exp, &iter); arg;
          arg = next_call_expr_arg (&iter))   */
inline tree
first_call_expr_arg (tree exp, call_expr_arg_iterator *iter)
{
  init_call_expr_arg_iterator (exp, iter);
  return next_call_expr_arg (iter);
}

inline const_tree
first_const_call_expr_arg (const_tree exp, const_call_expr_arg_iterator *iter)
{
  init_const_call_expr_arg_iterator (exp, iter);
  return next_const_call_expr_arg (iter);
}

/* Test whether there are more arguments in abstract argument list iterator
   ITER, without changing its state.  */
inline bool
more_call_expr_args_p (const call_expr_arg_iterator *iter)
{
  return (iter->i < iter->n);
}

/* Iterate through each argument ARG of CALL_EXPR CALL, using variable ITER
   (of type call_expr_arg_iterator) to hold the iteration state.  */
#define FOR_EACH_CALL_EXPR_ARG(arg, iter, call)			\
  for ((arg) = first_call_expr_arg ((call), &(iter)); (arg);	\
       (arg) = next_call_expr_arg (&(iter)))

#define FOR_EACH_CONST_CALL_EXPR_ARG(arg, iter, call)			\
  for ((arg) = first_const_call_expr_arg ((call), &(iter)); (arg);	\
       (arg) = next_const_call_expr_arg (&(iter)))

/* Return true if tree node T is a language-specific node.  */
inline bool
is_lang_specific (const_tree t)
{
  return TREE_CODE (t) == LANG_TYPE || TREE_CODE (t) >= NUM_TREE_CODES;
}

/* Valid builtin number.  */
#define BUILTIN_VALID_P(FNCODE) \
  (IN_RANGE ((int)FNCODE, ((int)BUILT_IN_NONE) + 1, ((int) END_BUILTINS) - 1))

/* Obtain a pointer to the identifier string holding the asm name for
   BUILTIN, a BUILT_IN code.  This is handy if the target
   mangles/overrides the function name that implements the
   builtin.  */
#define BUILTIN_ASM_NAME_PTR(BUILTIN) \
  (IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (builtin_decl_explicit (BUILTIN))))

/* Return the tree node for an explicit standard builtin function or NULL.  */
inline tree
builtin_decl_explicit (enum built_in_function fncode)
{
  gcc_checking_assert (BUILTIN_VALID_P (fncode));

  return builtin_info[(size_t)fncode].decl;
}

/* Return the tree node for an implicit builtin function or NULL.  */
inline tree
builtin_decl_implicit (enum built_in_function fncode)
{
  size_t uns_fncode = (size_t)fncode;
  gcc_checking_assert (BUILTIN_VALID_P (fncode));

  if (!builtin_info[uns_fncode].implicit_p)
    return NULL_TREE;

  return builtin_info[uns_fncode].decl;
}

/* For BUILTIN_UNREACHABLE, use one of these or
   gimple_build_builtin_unreachable instead of one of the above.  */
extern tree builtin_decl_unreachable ();
extern tree build_builtin_unreachable (location_t);

/* Set explicit builtin function nodes and whether it is an implicit
   function.  */

inline void
set_builtin_decl (enum built_in_function fncode, tree decl, bool implicit_p)
{
  size_t ufncode = (size_t)fncode;

  gcc_checking_assert (BUILTIN_VALID_P (fncode)
		       && (decl != NULL_TREE || !implicit_p));

  builtin_info[ufncode].decl = decl;
  builtin_info[ufncode].implicit_p = implicit_p;
  builtin_info[ufncode].declared_p = false;
}

/* Set the implicit flag for a builtin function.  */

inline void
set_builtin_decl_implicit_p (enum built_in_function fncode, bool implicit_p)
{
  size_t uns_fncode = (size_t)fncode;

  gcc_checking_assert (BUILTIN_VALID_P (fncode)
		       && builtin_info[uns_fncode].decl != NULL_TREE);

  builtin_info[uns_fncode].implicit_p = implicit_p;
}

/* Set the declared flag for a builtin function.  */

inline void
set_builtin_decl_declared_p (enum built_in_function fncode, bool declared_p)
{
  size_t uns_fncode = (size_t)fncode;

  gcc_checking_assert (BUILTIN_VALID_P (fncode)
		       && builtin_info[uns_fncode].decl != NULL_TREE);

  builtin_info[uns_fncode].declared_p = declared_p;
}

/* Return whether the standard builtin function can be used as an explicit
   function.  */

inline bool
builtin_decl_explicit_p (enum built_in_function fncode)
{
  gcc_checking_assert (BUILTIN_VALID_P (fncode));
  return (builtin_info[(size_t)fncode].decl != NULL_TREE);
}

/* Return whether the standard builtin function can be used implicitly.  */

inline bool
builtin_decl_implicit_p (enum built_in_function fncode)
{
  size_t uns_fncode = (size_t)fncode;

  gcc_checking_assert (BUILTIN_VALID_P (fncode));
  return (builtin_info[uns_fncode].decl != NULL_TREE
	  && builtin_info[uns_fncode].implicit_p);
}

/* Return whether the standard builtin function was declared.  */

inline bool
builtin_decl_declared_p (enum built_in_function fncode)
{
  size_t uns_fncode = (size_t)fncode;

  gcc_checking_assert (BUILTIN_VALID_P (fncode));
  return (builtin_info[uns_fncode].decl != NULL_TREE
	  && builtin_info[uns_fncode].declared_p);
}

/* Determine if the function identified by FNDECL is one that
   makes sense to match by name, for those places where we detect
   "magic" functions by name.

   Return true if FNDECL has a name and is an extern fndecl at file scope.
   FNDECL must be a non-NULL decl.

   Avoid using this, as it's generally better to use attributes rather
   than to check for functions by name.  */

inline bool
maybe_special_function_p (const_tree fndecl)
{
  tree name_decl = DECL_NAME (fndecl);
  if (name_decl
      /* Exclude functions not at the file scope, or not `extern',
	 since they are not the magic functions we would otherwise
	 think they are.  */
      && (DECL_CONTEXT (fndecl) == NULL_TREE
	  || TREE_CODE (DECL_CONTEXT (fndecl)) == TRANSLATION_UNIT_DECL)
      && TREE_PUBLIC (fndecl))
    return true;
  return false;
}

/* Return true if T (assumed to be a DECL) is a global variable.
   A variable is considered global if its storage is not automatic.  */

inline bool
is_global_var (const_tree t)
{
  return (TREE_STATIC (t) || DECL_EXTERNAL (t));
}

/* Return true if VAR may be aliased.  A variable is considered as
   maybe aliased if it has its address taken by the local TU
   or possibly by another TU and might be modified through a pointer.  */

inline bool
may_be_aliased (const_tree var)
{
  return (TREE_CODE (var) != CONST_DECL
	  && (TREE_PUBLIC (var)
	      || DECL_EXTERNAL (var)
	      || TREE_ADDRESSABLE (var))
	  && !((TREE_STATIC (var) || TREE_PUBLIC (var) || DECL_EXTERNAL (var))
	       && (TREE_READONLY (var)
		   || (TREE_CODE (var) == VAR_DECL
		       && DECL_NONALIASED (var)))));
}

/* Return pointer to optimization flags of FNDECL.  */
inline struct cl_optimization *
opts_for_fn (const_tree fndecl)
{
  tree fn_opts = DECL_FUNCTION_SPECIFIC_OPTIMIZATION (fndecl);
  if (fn_opts == NULL_TREE)
    fn_opts = optimization_default_node;
  return TREE_OPTIMIZATION (fn_opts);
}

/* Return pointer to target flags of FNDECL.  */
inline cl_target_option *
target_opts_for_fn (const_tree fndecl)
{
  tree fn_opts = DECL_FUNCTION_SPECIFIC_TARGET (fndecl);
  if (fn_opts == NULL_TREE)
    fn_opts = target_option_default_node;
  return fn_opts == NULL_TREE ? NULL : TREE_TARGET_OPTION (fn_opts);
}

/* opt flag for function FNDECL, e.g. opts_for_fn (fndecl, optimize) is
   the optimization level of function fndecl.  */
#define opt_for_fn(fndecl, opt) (opts_for_fn (fndecl)->x_##opt)

/* For anonymous aggregate types, we need some sort of name to
   hold on to.  In practice, this should not appear, but it should
   not be harmful if it does.  Identifiers returned will be
   IDENTIFIER_ANON_P.  */
extern tree make_anon_name ();

/* The tree and const_tree overload templates.   */
namespace wi
{
  class unextended_tree
  {
  private:
    const_tree m_t;

  public:
    unextended_tree () {}
    unextended_tree (const_tree t) : m_t (t) {}

    unsigned int get_precision () const;
    const HOST_WIDE_INT *get_val () const;
    unsigned int get_len () const;
    const_tree get_tree () const { return m_t; }
  };

  template <>
  struct int_traits <unextended_tree>
  {
    static const enum precision_type precision_type = VAR_PRECISION;
    static const bool host_dependent_precision = false;
    static const bool is_sign_extended = false;
    static const bool needs_write_val_arg = false;
  };

  template <int N>
  class extended_tree
  {
  private:
    const_tree m_t;

  public:
    extended_tree () {}
    extended_tree (const_tree);

    unsigned int get_precision () const;
    const HOST_WIDE_INT *get_val () const;
    unsigned int get_len () const;
    const_tree get_tree () const { return m_t; }
  };

  template <int N>
  struct int_traits <extended_tree <N> >
  {
    static const enum precision_type precision_type
      = N == ADDR_MAX_PRECISION ? INL_CONST_PRECISION : CONST_PRECISION;
    static const bool host_dependent_precision = false;
    static const bool is_sign_extended = true;
    static const bool needs_write_val_arg = false;
    static const unsigned int precision = N;
  };

  typedef extended_tree <WIDEST_INT_MAX_PRECISION> widest_extended_tree;
  typedef extended_tree <ADDR_MAX_PRECISION> offset_extended_tree;

  typedef const generic_wide_int <widest_extended_tree> tree_to_widest_ref;
  typedef const generic_wide_int <offset_extended_tree> tree_to_offset_ref;
  typedef const generic_wide_int<wide_int_ref_storage<false, false> >
    tree_to_wide_ref;

  tree_to_widest_ref to_widest (const_tree);
  tree_to_offset_ref to_offset (const_tree);
  tree_to_wide_ref to_wide (const_tree);
  wide_int to_wide (const_tree, unsigned int);

  typedef const poly_int <NUM_POLY_INT_COEFFS,
			  generic_wide_int <widest_extended_tree> >
    tree_to_poly_widest_ref;
  typedef const poly_int <NUM_POLY_INT_COEFFS,
			  generic_wide_int <offset_extended_tree> >
    tree_to_poly_offset_ref;
  typedef const poly_int <NUM_POLY_INT_COEFFS,
			  generic_wide_int <unextended_tree> >
    tree_to_poly_wide_ref;

  tree_to_poly_widest_ref to_poly_widest (const_tree);
  tree_to_poly_offset_ref to_poly_offset (const_tree);
  tree_to_poly_wide_ref to_poly_wide (const_tree);

  template <int N>
  struct ints_for <generic_wide_int <extended_tree <N> >, INL_CONST_PRECISION>
  {
    typedef generic_wide_int <extended_tree <N> > extended;
    static extended zero (const extended &);
  };

  template <int N>
  struct ints_for <generic_wide_int <extended_tree <N> >, CONST_PRECISION>
  {
    typedef generic_wide_int <extended_tree <N> > extended;
    static extended zero (const extended &);
  };

  template <>
  struct ints_for <generic_wide_int <unextended_tree>, VAR_PRECISION>
  {
    typedef generic_wide_int <unextended_tree> unextended;
    static unextended zero (const unextended &);
  };
}

/* Used to convert a tree to a widest2_int like this:
   widest2_int foo = widest2_int_cst (some_tree).  */
typedef generic_wide_int <wi::extended_tree <WIDEST_INT_MAX_PRECISION * 2> >
  widest2_int_cst;

/* Refer to INTEGER_CST T as though it were a widest_int.

   This function gives T's actual numerical value, influenced by the
   signedness of its type.  For example, a signed byte with just the
   top bit set would be -128 while an unsigned byte with the same
   bit pattern would be 128.

   This is the right choice when operating on groups of INTEGER_CSTs
   that might have different signedness or precision.  It is also the
   right choice in code that specifically needs an approximation of
   infinite-precision arithmetic instead of normal modulo arithmetic.

   The approximation of infinite precision is good enough for realistic
   numbers of additions and subtractions of INTEGER_CSTs (where
   "realistic" includes any number less than 1 << 31) but it cannot
   represent the result of multiplying the two largest supported
   INTEGER_CSTs.  The overflow-checking form of wi::mul provides a way
   of multiplying two arbitrary INTEGER_CSTs and checking that the
   result is representable as a widest_int.

   Note that any overflow checking done on these values is relative to
   the range of widest_int rather than the range of a TREE_TYPE.

   Calling this function should have no overhead in release builds,
   so it is OK to call it several times for the same tree.  If it is
   useful for readability reasons to reduce the number of calls,
   it is more efficient to use:

     wi::tree_to_widest_ref wt = wi::to_widest (t);

   instead of:

     widest_int wt = wi::to_widest (t).  */

inline wi::tree_to_widest_ref
wi::to_widest (const_tree t)
{
  return t;
}

/* Refer to INTEGER_CST T as though it were an offset_int.

   This function is an optimisation of wi::to_widest for cases
   in which T is known to be a bit or byte count in the range
   (-(2 ^ (N + BITS_PER_UNIT)), 2 ^ (N + BITS_PER_UNIT)), where N is
   the target's address size in bits.

   This is the right choice when operating on bit or byte counts as
   untyped numbers rather than M-bit values.  The wi::to_widest comments
   about addition, subtraction and multiplication apply here: sequences
   of 1 << 31 additions and subtractions do not induce overflow, but
   multiplying the largest sizes might.  Again,

     wi::tree_to_offset_ref wt = wi::to_offset (t);

   is more efficient than:

     offset_int wt = wi::to_offset (t).  */

inline wi::tree_to_offset_ref
wi::to_offset (const_tree t)
{
  return t;
}

/* Refer to INTEGER_CST T as though it were a wide_int.

   In contrast to the approximation of infinite-precision numbers given
   by wi::to_widest and wi::to_offset, this function treats T as a
   signless collection of N bits, where N is the precision of T's type.
   As with machine registers, signedness is determined by the operation
   rather than the operands; for example, there is a distinction between
   signed and unsigned division.

   This is the right choice when operating on values with the same type
   using normal modulo arithmetic.  The overflow-checking forms of things
   like wi::add check whether the result can be represented in T's type.

   Calling this function should have no overhead in release builds,
   so it is OK to call it several times for the same tree.  If it is
   useful for readability reasons to reduce the number of calls,
   it is more efficient to use:

     wi::tree_to_wide_ref wt = wi::to_wide (t);

   instead of:

     wide_int wt = wi::to_wide (t).  */

inline wi::tree_to_wide_ref
wi::to_wide (const_tree t)
{
  return wi::storage_ref (&TREE_INT_CST_ELT (t, 0), TREE_INT_CST_NUNITS (t),
			  TYPE_PRECISION (TREE_TYPE (t)));
}

/* Convert INTEGER_CST T to a wide_int of precision PREC, extending or
   truncating as necessary.  When extending, use sign extension if T's
   type is signed and zero extension if T's type is unsigned.  */

inline wide_int
wi::to_wide (const_tree t, unsigned int prec)
{
  return wide_int::from (wi::to_wide (t), prec, TYPE_SIGN (TREE_TYPE (t)));
}

template <int N>
inline wi::extended_tree <N>::extended_tree (const_tree t)
  : m_t (t)
{
  gcc_checking_assert (TYPE_PRECISION (TREE_TYPE (t)) <= N);
}

template <int N>
inline unsigned int
wi::extended_tree <N>::get_precision () const
{
  return N;
}

template <int N>
inline const HOST_WIDE_INT *
wi::extended_tree <N>::get_val () const
{
  return &TREE_INT_CST_ELT (m_t, 0);
}

template <int N>
inline unsigned int
wi::extended_tree <N>::get_len () const
{
  if (N == ADDR_MAX_PRECISION)
    {
      /* to_offset can only be applied to trees that are offset_int-sized
	 or smaller.  EXT_LEN is correct if it fits, otherwise the constant
	 must be exactly the precision of offset_int and so LEN is correct.  */
      unsigned int ext_len = TREE_INT_CST_EXT_NUNITS (m_t);
      if (ext_len <= OFFSET_INT_ELTS)
	return ext_len;
      return TREE_INT_CST_NUNITS (m_t);
    }
  else if (N >= WIDEST_INT_MAX_PRECISION)
    return TREE_INT_CST_EXT_NUNITS (m_t);
  else
    /* This class is designed to be used for specific output precisions
       and needs to be as fast as possible, so there is no fallback for
       other casees.  */
    gcc_unreachable ();
}

inline unsigned int
wi::unextended_tree::get_precision () const
{
  return TYPE_PRECISION (TREE_TYPE (m_t));
}

inline const HOST_WIDE_INT *
wi::unextended_tree::get_val () const
{
  return &TREE_INT_CST_ELT (m_t, 0);
}

inline unsigned int
wi::unextended_tree::get_len () const
{
  return TREE_INT_CST_NUNITS (m_t);
}

/* Return the value of a POLY_INT_CST in its native precision.  */

inline wi::tree_to_poly_wide_ref
poly_int_cst_value (const_tree x)
{
  poly_int <NUM_POLY_INT_COEFFS, generic_wide_int <wi::unextended_tree> > res;
  for (unsigned int i = 0; i < NUM_POLY_INT_COEFFS; ++i)
    res.coeffs[i] = POLY_INT_CST_COEFF (x, i);
  return res;
}

/* Access INTEGER_CST or POLY_INT_CST tree T as if it were a
   poly_widest_int.  See wi::to_widest for more details.  */

inline wi::tree_to_poly_widest_ref
wi::to_poly_widest (const_tree t)
{
  if (POLY_INT_CST_P (t))
    {
      poly_int <NUM_POLY_INT_COEFFS,
		generic_wide_int <widest_extended_tree> > res;
      for (unsigned int i = 0; i < NUM_POLY_INT_COEFFS; ++i)
	res.coeffs[i] = POLY_INT_CST_COEFF (t, i);
      return res;
    }
  return t;
}

/* Access INTEGER_CST or POLY_INT_CST tree T as if it were a
   poly_offset_int.  See wi::to_offset for more details.  */

inline wi::tree_to_poly_offset_ref
wi::to_poly_offset (const_tree t)
{
  if (POLY_INT_CST_P (t))
    {
      poly_int <NUM_POLY_INT_COEFFS,
		generic_wide_int <offset_extended_tree> > res;
      for (unsigned int i = 0; i < NUM_POLY_INT_COEFFS; ++i)
	res.coeffs[i] = POLY_INT_CST_COEFF (t, i);
      return res;
    }
  return t;
}

/* Access INTEGER_CST or POLY_INT_CST tree T as if it were a
   poly_wide_int.  See wi::to_wide for more details.  */

inline wi::tree_to_poly_wide_ref
wi::to_poly_wide (const_tree t)
{
  if (POLY_INT_CST_P (t))
    return poly_int_cst_value (t);
  return t;
}

template <int N>
inline generic_wide_int <wi::extended_tree <N> >
wi::ints_for <generic_wide_int <wi::extended_tree <N> >,
	      wi::INL_CONST_PRECISION>::zero (const extended &x)
{
  return build_zero_cst (TREE_TYPE (x.get_tree ()));
}

template <int N>
inline generic_wide_int <wi::extended_tree <N> >
wi::ints_for <generic_wide_int <wi::extended_tree <N> >,
	      wi::CONST_PRECISION>::zero (const extended &x)
{
  return build_zero_cst (TREE_TYPE (x.get_tree ()));
}

inline generic_wide_int <wi::unextended_tree>
wi::ints_for <generic_wide_int <wi::unextended_tree>,
	      wi::VAR_PRECISION>::zero (const unextended &x)
{
  return build_zero_cst (TREE_TYPE (x.get_tree ()));
}

namespace wi
{
  template <typename T>
  bool fits_to_boolean_p (const T &x, const_tree);

  template <typename T>
  bool fits_to_tree_p (const T &x, const_tree);

  wide_int min_value (const_tree);
  wide_int max_value (const_tree);
#ifndef GENERATOR_FILE
  wide_int from_mpz (const_tree, mpz_t, bool);
#endif
}

template <typename T>
bool
wi::fits_to_boolean_p (const T &x, const_tree type)
{
  typedef typename poly_int_traits<T>::int_type int_type;
  return (known_eq (x, int_type (0))
	  || known_eq (x, int_type (TYPE_UNSIGNED (type) ? 1 : -1)));
}

template <typename T>
bool
wi::fits_to_tree_p (const T &x, const_tree type)
{
  /* Non-standard boolean types can have arbitrary precision but various
     transformations assume that they can only take values 0 and +/-1.  */
  if (TREE_CODE (type) == BOOLEAN_TYPE)
    return fits_to_boolean_p (x, type);

  if (TYPE_UNSIGNED (type))
    return known_eq (x, zext (x, TYPE_PRECISION (type)));
  else
    return known_eq (x, sext (x, TYPE_PRECISION (type)));
}

/* Produce the smallest number that is represented in TYPE.  The precision
   and sign are taken from TYPE.  */
inline wide_int
wi::min_value (const_tree type)
{
  return min_value (TYPE_PRECISION (type), TYPE_SIGN (type));
}

/* Produce the largest number that is represented in TYPE.  The precision
   and sign are taken from TYPE.  */
inline wide_int
wi::max_value (const_tree type)
{
  return max_value (TYPE_PRECISION (type), TYPE_SIGN (type));
}

/* Return true if INTEGER_CST T1 is less than INTEGER_CST T2,
   extending both according to their respective TYPE_SIGNs.  */

inline bool
tree_int_cst_lt (const_tree t1, const_tree t2)
{
  return wi::to_widest (t1) < wi::to_widest (t2);
}

/* Return true if INTEGER_CST T1 is less than or equal to INTEGER_CST T2,
   extending both according to their respective TYPE_SIGNs.  */

inline bool
tree_int_cst_le (const_tree t1, const_tree t2)
{
  return wi::to_widest (t1) <= wi::to_widest (t2);
}

/* Returns -1 if T1 < T2, 0 if T1 == T2, and 1 if T1 > T2.  T1 and T2
   are both INTEGER_CSTs and their values are extended according to their
   respective TYPE_SIGNs.  */

inline int
tree_int_cst_compare (const_tree t1, const_tree t2)
{
  return wi::cmps (wi::to_widest (t1), wi::to_widest (t2));
}

/* FIXME - These declarations belong in builtins.h, expr.h and emit-rtl.h,
   but none of these files are allowed to be included from front ends.
   They should be split in two. One suitable for the FEs, the other suitable
   for the BE.  */

/* Assign the RTX to declaration.  */
extern void set_decl_rtl (tree, rtx);
extern bool complete_ctor_at_level_p (const_tree, HOST_WIDE_INT, const_tree);

/* Given an expression EXP that is a handled_component_p,
   look for the ultimate containing object, which is returned and specify
   the access position and size.  */
extern tree get_inner_reference (tree, poly_int64 *, poly_int64 *,
				 tree *, machine_mode *, int *, int *, int *);

extern tree build_personality_function (const char *);

struct GTY(()) int_n_trees_t {
  /* These parts are initialized at runtime */
  tree signed_type;
  tree unsigned_type;
};

/* This is also in machmode.h */
extern bool int_n_enabled_p[NUM_INT_N_ENTS];
extern GTY(()) struct int_n_trees_t int_n_trees[NUM_INT_N_ENTS];

/* Like bit_position, but return as an integer.  It must be representable in
   that way (since it could be a signed value, we don't have the
   option of returning -1 like int_size_in_byte can.  */

inline HOST_WIDE_INT
int_bit_position (const_tree field)
{
  return ((wi::to_offset (DECL_FIELD_OFFSET (field)) << LOG2_BITS_PER_UNIT)
	  + wi::to_offset (DECL_FIELD_BIT_OFFSET (field))).to_shwi ();
}

/* Return true if it makes sense to consider alias set for a type T.  */

inline bool
type_with_alias_set_p (const_tree t)
{
  /* Function and method types are never accessed as memory locations.  */
  if (TREE_CODE (t) == FUNCTION_TYPE || TREE_CODE (t) == METHOD_TYPE)
    return false;

  if (COMPLETE_TYPE_P (t))
    return true;

  /* Incomplete types cannot be accessed in general except for arrays
     where we can fetch its element despite we have no array bounds.  */
  if (TREE_CODE (t) == ARRAY_TYPE && COMPLETE_TYPE_P (TREE_TYPE (t)))
    return true;

  return false;
}

extern location_t set_block (location_t loc, tree block);

extern void gt_ggc_mx (tree &);
extern void gt_pch_nx (tree &);
extern void gt_pch_nx (tree &, gt_pointer_operator, void *);
extern void gt_ggc_mx (tree_raw_data *);
extern void gt_pch_nx (tree_raw_data *);
extern void gt_pch_nx (tree_raw_data *, gt_pointer_operator, void *);

extern bool nonnull_arg_p (const_tree);
extern bool is_empty_type (const_tree);
extern bool default_is_empty_record (const_tree);
extern bool flexible_array_type_p (const_tree);
extern HOST_WIDE_INT arg_int_size_in_bytes (const_tree);
extern tree arg_size_in_bytes (const_tree);
extern bool expr_type_first_operand_type_p (tree_code);

extern location_t
set_source_range (tree expr, location_t start, location_t finish);

extern location_t
set_source_range (tree expr, source_range src_range);

/* Return true if it makes sense to promote/demote from_type to to_type. */
inline bool
desired_pro_or_demotion_p (const_tree to_type, const_tree from_type)
{
  unsigned int to_type_precision = TYPE_PRECISION (to_type);

  /* OK to promote if to_type is no bigger than word_mode. */
  if (to_type_precision <= GET_MODE_PRECISION (word_mode))
    return true;

  /* Otherwise, allow only if narrowing or same precision conversions. */
  return to_type_precision <= TYPE_PRECISION (from_type);
}

/* Pointer type used to declare builtins before we have seen its real
   declaration.  */
class builtin_structptr_type
{
public:
  tree& node;
  tree& base;
  const char *str;
};
extern const builtin_structptr_type builtin_structptr_types[6];

/* Return true if type T has the same precision as its underlying mode.  */

inline bool
type_has_mode_precision_p (const_tree t)
{
  return known_eq (TYPE_PRECISION (t), GET_MODE_PRECISION (TYPE_MODE (t)));
}

/* Helper functions for fndecl_built_in_p.  */

inline bool
built_in_function_equal_p (built_in_function name0, built_in_function name1)
{
  return name0 == name1;
}

/* Recursive case for two or more names.  */

template <typename... F>
inline bool
built_in_function_equal_p (built_in_function name0, built_in_function name1,
			   built_in_function name2, F... names)
{
  return name0 == name1 || built_in_function_equal_p (name0, name2, names...);
}

/* Return true if a FUNCTION_DECL NODE is a GCC built-in function.

   Note that it is different from the DECL_IS_UNDECLARED_BUILTIN
   accessor, as this is impervious to user declaration.  */

inline bool
fndecl_built_in_p (const_tree node)
{
  return DECL_BUILT_IN_CLASS (node) != NOT_BUILT_IN;
}

/* Return true if a FUNCTION_DECL NODE is a GCC built-in function
   of class KLASS.  */

inline bool
fndecl_built_in_p (const_tree node, built_in_class klass)
{
  return fndecl_built_in_p (node) && DECL_BUILT_IN_CLASS (node) == klass;
}

/* Return true if a FUNCTION_DECL NODE is a GCC built-in function
   of class KLASS with name equal to NAME.  */

inline bool
fndecl_built_in_p (const_tree node, unsigned int name, built_in_class klass)
{
  return (fndecl_built_in_p (node, klass)
	  && DECL_UNCHECKED_FUNCTION_CODE (node) == name);
}

/* Return true if a FUNCTION_DECL NODE is a GCC built-in function
   of BUILT_IN_NORMAL class with name equal to NAME1 (or other mentioned
   NAMES).  */

template <typename... F>
inline bool
fndecl_built_in_p (const_tree node, built_in_function name1, F... names)
{
  return (fndecl_built_in_p (node, BUILT_IN_NORMAL)
	  && built_in_function_equal_p (DECL_FUNCTION_CODE (node),
					name1, names...));
}

/* A struct for encapsulating location information about an operator
   and the operation built from it.

   m_operator_loc is the location of the operator
   m_combined_loc is the location of the compound expression.

   For example, given "a && b" the, operator location is:
      a && b
        ^~
   and the combined location is:
      a && b
      ~~^~~~
   Capturing this information allows for class binary_op_rich_location
   to provide detailed information about e.g. type mismatches in binary
   operations where enough location information is available:

     arg_0 op arg_1
     ~~~~~ ^~ ~~~~~
       |        |
       |        arg1 type
       arg0 type

   falling back to just showing the combined location:

     arg_0 op arg_1
     ~~~~~~^~~~~~~~

   where it is not.  */

class op_location_t
{
public:
  location_t m_operator_loc;
  location_t m_combined_loc;

  /* 1-argument ctor, for constructing from a combined location.  */
  op_location_t (location_t combined_loc)
  : m_operator_loc (UNKNOWN_LOCATION), m_combined_loc (combined_loc)
  {}

  /* 2-argument ctor, for distinguishing between the operator's location
     and the combined location.  */
  op_location_t (location_t operator_loc, location_t combined_loc)
  : m_operator_loc (operator_loc), m_combined_loc (combined_loc)
  {}

  /* Implicitly convert back to a location_t, using the combined location.  */
  operator location_t () const { return m_combined_loc; }
};

/* Code that doesn't refer to any warning.  Has no effect on suppression
   functions.  */
constexpr opt_code no_warning = opt_code ();
/* Wildcard code that refers to all warnings.  */
constexpr opt_code all_warnings = N_OPTS;

/* Return the disposition for a warning (or all warnings by default)
   at a location.  */
extern bool warning_suppressed_at (location_t, opt_code = all_warnings);
/* Set the disposition for a warning (or all warnings by default)
   at a location to disabled by default.  */
extern bool suppress_warning_at (location_t, opt_code = all_warnings,
				 bool = true);
/* Overwrite warning disposition bitmap for a location with given spec.  */
extern void put_warning_spec_at (location_t loc, unsigned);
/* Copy warning disposition from one location to another.  */
extern void copy_warning (location_t, location_t);

/* Return the disposition for a warning (or all warnings by default)
   for an expression.  */
extern bool warning_suppressed_p (const_tree, opt_code = all_warnings);
/* Set the disposition for a warning (or all warnings by default)
   at a location to disabled by default.  */
extern void suppress_warning (tree, opt_code = all_warnings, bool = true)
  ATTRIBUTE_NONNULL (1);
/* Copy warning disposition from one expression to another.  */
extern void copy_warning (tree, const_tree);

/* Whether the tree might have a warning spec.  */
extern bool has_warning_spec (const_tree);
/* Retrieve warning spec bitmap for tree streaming.  */
extern unsigned get_warning_spec (const_tree);
/* Overwrite warning spec bitmap for a tree with given spec.  */
extern void put_warning_spec (tree, unsigned);

/* Return the zero-based number corresponding to the argument being
   deallocated if FNDECL is a deallocation function or an out-of-bounds
   value if it isn't.  */
extern unsigned fndecl_dealloc_argno (tree);

/* If an expression refers to a character array or pointer declared
   attribute nonstring, return a decl for that array or pointer and
   if nonnull, set the second argument to the referenced enclosing
   object or pointer.  Otherwise return null.  */
extern tree get_attr_nonstring_decl (tree, tree * = NULL);

extern int get_target_clone_attr_len (tree);

/* Returns the version string for a decl with target_version attribute.
   Returns an invalid string_slice if no attribute is present.  */
extern string_slice get_target_version (const tree);
/* Returns a vector of the version strings from a target_clones attribute on
   a decl.  Can also record the number of default versions found.  */
extern auto_vec<string_slice> get_clone_versions (const tree, int * = NULL);
/* Returns a vector of the version strings from a target_clones attribute
   directly.  */
extern auto_vec<string_slice> get_clone_attr_versions (const tree, int *);

#endif  /* GCC_TREE_H  */
