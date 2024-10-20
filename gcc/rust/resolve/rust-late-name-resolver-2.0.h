// Copyright (C) 2020-2024 Free Software Foundation, Inc.

// This file is part of GCC.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#ifndef RUST_LATE_NAME_RESOLVER_2_0_H
#define RUST_LATE_NAME_RESOLVER_2_0_H

#include "rust-ast-full.h"
#include "rust-default-resolver.h"

namespace Rust {
namespace Resolver2_0 {

class Late : public DefaultResolver
{
public:
  Late (NameResolutionContext &ctx);

  void go (AST::Crate &crate);

  void new_label (Identifier name, NodeId id);

  using DefaultResolver::visit;

  // some more label declarations
  void visit (AST::LetStmt &) override;
  // TODO: Do we need this?
  // void visit (AST::Method &) override;
  void visit (AST::SelfParam &) override;
  void visit (AST::TypeParam &) override;
  void visit (AST::LoopLabel &) override;

  // resolutions
  void visit (AST::IdentifierExpr &) override;
  void visit (AST::PathInExpression &) override;
  void visit (AST::LangItemPath &) override;
  void visit (AST::TypePath &) override;
  void visit (AST::Trait &) override;
  void visit (AST::TraitImpl &) override;
  void visit (AST::StructExprStruct &) override;
  void visit (AST::StructExprStructBase &) override;
  void visit (AST::StructExprStructFields &) override;
  void visit (AST::StructStruct &) override;
  void visit (AST::GenericArgs &) override;
  void visit (AST::GenericArg &);

  // entering patterns
  //   - ignore LiteralPattern
  void visit (AST::IdentifierPattern &) override;
  //   - ignore WildcardPattern
  //   - ignore RestPattern
  void visit (AST::RangePattern &) override;
  void visit (AST::ReferencePattern &) override;
  void visit (AST::StructPattern &) override;
  void visit (AST::TupleStructPattern &) override;
  void visit (AST::TuplePattern &) override;
  void visit (AST::GroupedPattern &) override;
  void visit (AST::SlicePattern &) override;
  void visit (AST::AltPattern &) override;

private:
  /* Setup Rust's builtin types (u8, i32, !...) in the resolver */
  void setup_builtin_types ();
};

class LatePattern : public Late
{
  using Late::visit;

public:
  void visit (AST::IdentifierPattern &) override;
  void visit (AST::AltPattern &) override;

  // override Late::visit for other patterns
  void visit (AST::RangePattern &p) override { DefaultResolver::visit (p); }
  void visit (AST::ReferencePattern &p) override { DefaultResolver::visit (p); }
  void visit (AST::StructPattern &p) override { DefaultResolver::visit (p); }
  void visit (AST::TupleStructPattern &p) override
  {
    DefaultResolver::visit (p);
  }
  void visit (AST::TuplePattern &p) override { DefaultResolver::visit (p); }
  void visit (AST::GroupedPattern &p) override { DefaultResolver::visit (p); }
  void visit (AST::SlicePattern &p) override { DefaultResolver::visit (p); }

  static void go (NameResolutionContext &ctx, AST::Pattern &pat);

private:
  using bindings_type
    = std::unordered_map<std::string, std::pair<location_t, NodeId>>;

  static bindings_type go_inner (NameResolutionContext &ctx, AST::Pattern &pat);

  void handle_ident (std::string s, location_t loc, NodeId id);

  LatePattern (NameResolutionContext &ctx) : Late (ctx) {}

  bindings_type bindings;
};

// TODO: Add missing mappings and data structures

} // namespace Resolver2_0
} // namespace Rust

#endif // ! RUST_LATE_NAME_RESOLVER_2_0_H
