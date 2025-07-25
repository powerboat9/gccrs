------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             E X P _ A T T R                              --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2025, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT; see file COPYING3.  If not, go to --
-- http://www.gnu.org/licenses for a complete copy of the license.          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

with Accessibility;  use Accessibility;
with Aspects;        use Aspects;
with Atree;          use Atree;
with Checks;         use Checks;
with Debug;          use Debug;
with Einfo;          use Einfo;
with Einfo.Entities; use Einfo.Entities;
with Einfo.Utils;    use Einfo.Utils;
with Elists;         use Elists;
with Exp_Atag;       use Exp_Atag;
with Exp_Ch3;        use Exp_Ch3;
with Exp_Ch6;        use Exp_Ch6;
with Exp_Ch9;        use Exp_Ch9;
with Exp_Dist;       use Exp_Dist;
with Exp_Imgv;       use Exp_Imgv;
with Exp_Pakd;       use Exp_Pakd;
with Exp_Strm;       use Exp_Strm;
with Exp_Put_Image;
with Exp_Tss;        use Exp_Tss;
with Exp_Util;       use Exp_Util;
with Expander;       use Expander;
with Freeze;         use Freeze;
with Gnatvsn;        use Gnatvsn;
with Itypes;         use Itypes;
with Lib;            use Lib;
with Namet;          use Namet;
with Nmake;          use Nmake;
with Nlists;         use Nlists;
with Opt;            use Opt;
with Restrict;       use Restrict;
with Rident;         use Rident;
with Rtsfind;        use Rtsfind;
with Sem;            use Sem;
with Sem_Aux;        use Sem_Aux;
with Sem_Ch6;        use Sem_Ch6;
with Sem_Ch7;        use Sem_Ch7;
with Sem_Ch8;        use Sem_Ch8;
with Sem_Eval;       use Sem_Eval;
with Sem_Res;        use Sem_Res;
with Sem_Util;       use Sem_Util;
with Sinfo;          use Sinfo;
with Sinfo.Nodes;    use Sinfo.Nodes;
with Sinfo.Utils;    use Sinfo.Utils;
with Snames;         use Snames;
with Stand;          use Stand;
with Stringt;        use Stringt;
with Strub;          use Strub;
with Tbuild;         use Tbuild;
with Ttypes;         use Ttypes;
with Uintp;          use Uintp;
with Uname;          use Uname;
with Urealp;         use Urealp;
with Validsw;        use Validsw;

with GNAT.HTable;

package body Exp_Attr is

   package Cached_Attribute_Ops is

      Map_Size : constant := 63;
      subtype Header_Num is Integer range 0 .. Map_Size - 1;

      function Attribute_Op_Hash (Id : Entity_Id) return Header_Num is
        (Header_Num (Id mod Map_Size));

      --  Caches used to avoid building duplicate subprograms for a single
      --  type/attribute pair (where the attribute is either Put_Image or
      --  one of the four streaming attributes). The type used as a key in
      --  in accessing these maps should not be the entity of a subtype.

      package Read_Map is new GNAT.HTable.Simple_HTable
        (Header_Num => Header_Num,
         Key        => Entity_Id,
         Element    => Entity_Id,
         No_Element => Empty,
         Hash       => Attribute_Op_Hash,
         Equal      => "=");

      package Write_Map is new GNAT.HTable.Simple_HTable
        (Header_Num => Header_Num,
         Key        => Entity_Id,
         Element    => Entity_Id,
         No_Element => Empty,
         Hash       => Attribute_Op_Hash,
         Equal      => "=");

      package Input_Map is new GNAT.HTable.Simple_HTable
        (Header_Num => Header_Num,
         Key        => Entity_Id,
         Element    => Entity_Id,
         No_Element => Empty,
         Hash       => Attribute_Op_Hash,
         Equal      => "=");

      package Output_Map is new GNAT.HTable.Simple_HTable
        (Header_Num => Header_Num,
         Key        => Entity_Id,
         Element    => Entity_Id,
         No_Element => Empty,
         Hash       => Attribute_Op_Hash,
         Equal      => "=");

      package Put_Image_Map is new GNAT.HTable.Simple_HTable
        (Header_Num => Header_Num,
         Key        => Entity_Id,
         Element    => Entity_Id,
         No_Element => Empty,
         Hash       => Attribute_Op_Hash,
         Equal      => "=");

      procedure Validate_Cached_Candidate
        (Subp     : in out Entity_Id;
         Attr_Ref : Node_Id);
      --  If Subp is non-empty but it is not callable from the point of
      --  Attr_Ref (perhaps because it is not visible from that point),
      --  then Subp is set to Empty. Otherwise, do nothing.

   end Cached_Attribute_Ops;

   -----------------------
   -- Local Subprograms --
   -----------------------

   function Build_Array_VS_Func
     (Attr       : Node_Id;
      Formal_Typ : Entity_Id;
      Array_Typ  : Entity_Id) return Entity_Id;
   --  Validate the components of an array type by means of a function. Return
   --  the entity of the validation function. The parameters are as follows:
   --
   --    * Attr - the 'Valid_Scalars attribute for which the function is
   --      generated.
   --
   --    * Formal_Typ - the type of the generated function's only formal
   --      parameter.
   --
   --    * Array_Typ - the array type whose components are to be validated

   function Build_Disp_Get_Task_Id_Call (Actual : Node_Id) return Node_Id;
   --  Build a call to Disp_Get_Task_Id, passing Actual as actual parameter

   function Build_Record_VS_Func
     (Attr       : Node_Id;
      Formal_Typ : Entity_Id;
      Rec_Typ    : Entity_Id) return Entity_Id;
   --  Validate the components, discriminants, and variants of a record type by
   --  means of a function. Return the entity of the validation function. The
   --  parameters are as follows:
   --
   --    * Attr - the 'Valid_Scalars attribute for which the function is
   --      generated.
   --
   --    * Formal_Typ - the type of the generated function's only formal
   --      parameter.
   --
   --    * Rec_Typ - the record type whose internals are to be validated

   function Default_Streaming_Unavailable (Typ : Entity_Id) return Boolean;
   --  In most cases, references to unavailable streaming attributes
   --  are rejected at compile time. In some obscure cases involving
   --  generics and formal derived types, the problem is dealt with at runtime.

   procedure Expand_Access_To_Protected_Op
     (N    : Node_Id;
      Pref : Node_Id;
      Typ  : Entity_Id);
   --  An attribute reference to a protected subprogram is transformed into
   --  a pair of pointers: one to the object, and one to the operations.
   --  This expansion is performed for 'Access and for 'Unrestricted_Access.

   procedure Expand_Fpt_Attribute
     (N    : Node_Id;
      Pkg  : RE_Id;
      Nam  : Name_Id;
      Args : List_Id);
   --  This procedure expands a call to a floating-point attribute function.
   --  N is the attribute reference node, and Args is a list of arguments to
   --  be passed to the function call. Pkg identifies the package containing
   --  the appropriate instantiation of System.Fat_Gen. Float arguments in Args
   --  have already been converted to the floating-point type for which Pkg was
   --  instantiated. The Nam argument is the relevant attribute processing
   --  routine to be called. This is the same as the attribute name.

   procedure Expand_Fpt_Attribute_R (N : Node_Id);
   --  This procedure expands a call to a floating-point attribute function
   --  that takes a single floating-point argument. The function to be called
   --  is always the same as the attribute name.

   procedure Expand_Fpt_Attribute_RI (N : Node_Id);
   --  This procedure expands a call to a floating-point attribute function
   --  that takes one floating-point argument and one integer argument. The
   --  function to be called is always the same as the attribute name.

   procedure Expand_Fpt_Attribute_RR (N : Node_Id);
   --  This procedure expands a call to a floating-point attribute function
   --  that takes two floating-point arguments. The function to be called
   --  is always the same as the attribute name.

   procedure Expand_Loop_Entry_Attribute (N : Node_Id);
   --  Handle the expansion of attribute 'Loop_Entry. As a result, the related
   --  loop may be converted into a conditional block. See body for details.

   procedure Expand_Min_Max_Attribute (N : Node_Id);
   --  Handle the expansion of attributes 'Max and 'Min

   procedure Expand_Pred_Succ_Attribute (N : Node_Id);
   --  Handles expansion of Pred or Succ attributes for case of non-real
   --  operand with overflow checking required.

   procedure Expand_Update_Attribute (N : Node_Id);
   --  Handle the expansion of attribute Update

   procedure Find_Fat_Info
     (T        : Entity_Id;
      Fat_Type : out Entity_Id;
      Fat_Pkg  : out RE_Id);
   --  Given a floating-point type T, identifies the package containing the
   --  attributes for this type (returned in Fat_Pkg), and the corresponding
   --  type for which this package was instantiated from Fat_Gen. Error if T
   --  is not a floating-point type.

   function Find_Stream_Subprogram
     (Typ      : Entity_Id;
      Nam      : TSS_Name_Type;
      Attr_Ref : Node_Id) return Entity_Id;
   --  Returns the stream-oriented subprogram attribute for Typ. For tagged
   --  types, the corresponding primitive operation is looked up, else the
   --  appropriate TSS from the type itself, or from its closest ancestor
   --  defining it, is returned. In both cases, inheritance of representation
   --  aspects is thus taken into account. Attr_Ref is used to identify the
   --  point from which the function result will be referenced.

   function Full_Base (T : Entity_Id) return Entity_Id;
   --  The stream functions need to examine the underlying representation of
   --  composite types. In some cases T may be non-private but its base type
   --  is, in which case the function returns the corresponding full view.

   function Get_Stream_Convert_Pragma (T : Entity_Id) return Node_Id;
   --  Given a type, find a corresponding stream convert pragma that applies to
   --  the implementation base type of this type (Typ). If found, return the
   --  pragma node, otherwise return Empty if no pragma is found.

   function Is_Constrained_Packed_Array (Typ : Entity_Id) return Boolean;
   --  Utility for array attributes, returns true on packed constrained
   --  arrays, and on access to same.

   function Is_Inline_Floating_Point_Attribute (N : Node_Id) return Boolean;
   --  Returns true iff the given node refers to an attribute call that
   --  can be expanded directly by the back end and does not need front end
   --  expansion. Typically used for rounding and truncation attributes that
   --  appear directly inside a conversion to integer.

   function Is_User_Defined_Enumeration_Type (Typ : Entity_Id) return Boolean;
   --  Returns True if Typ is a user-defined enumeration type, in the sense
   --  that its literals are declared in the source.

   function Interunit_Ref_OK
     (Subp_Unit, Attr_Ref_Unit : Node_Id) return Boolean is
       (In_Same_Extended_Unit (Subp_Unit, Attr_Ref_Unit)
         --  If subp declared in unit body, then we don't want to refer
         --  to it from within unit spec so return False in that case.
         and then not (not Is_Body (Unit (Attr_Ref_Unit))
                       and Is_Body (Unit (Subp_Unit))));
   --  Returns True if it is ok to refer to a cached subprogram declared in
   --  Subp_Unit from the point of an attribute reference occurring in
   --  Attr_Ref_Unit. Both arguments are usually N_Compilation_Nodes,
   --  although there are cases where Subp_Unit might be a type declared in
   --  package Standard (in which case the In_Same_Extended_Unit call will
   --  return False).

   package body Cached_Attribute_Ops is

      -------------------------------
      -- Validate_Cached_Candidate --
      -------------------------------

      procedure Validate_Cached_Candidate
        (Subp     : in out Entity_Id;
         Attr_Ref : Node_Id) is
      begin
         if No (Subp) then
            return;
         end if;

         declare
            Subp_Comp_Unit     : constant Node_Id :=
              Enclosing_Comp_Unit_Node (Subp);
            Attr_Ref_Comp_Unit : constant Node_Id :=
              Enclosing_Comp_Unit_Node (Attr_Ref);

            --  The preceding Enclosing_Comp_Unit_Node calls are needed
            --  (as opposed to changing Interunit_Ref_OK so that it could
            --  be passed Subp and Attr_Ref) because the games we play
            --  with source position info for these conjured-up routines can
            --  confuse In_Same_Extended_Unit (which is called from in
            --  Interunit_Ref_OK) in the case where one of these
            --  conjured-up routines contains an attribute reference
            --  denoting another such routine (e.g., if the Put_Image routine
            --  for a composite type contains a Some_Component_Type'Put_Image
            --  attribute reference). Calling Enclosing_Comp_Unit_Node first
            --  avoids the case where In_Same_Extended_Unit gets confused.

         begin
            if Interunit_Ref_OK (Subp_Comp_Unit, Attr_Ref_Comp_Unit)
              and then (Is_Library_Level_Entity (Subp)
                        or else Enclosing_Dynamic_Scope (Subp) =
                                Enclosing_Lib_Unit_Entity (Subp))
            then
               return;
            end if;
         end;

         --  We have previously tried being more ambitious here in hopes of
         --  referencing subprograms declared in other units (as opposed
         --  to generating a new copy for the current unit) if they are
         --  visible from the point of Attr_Ref. Unfortunately,
         --  we ran into problems with generating inconsistent linknames
         --  (e.g., a procedure declared with a name ending in "_304PI" being
         --  unsuccessfully referenced from another unit via a name ending in
         --  "_305PI"). So, after a fair amount of unsuccessful debugging,
         --   it was decided to abandon the effort.

         Subp := Empty;
      end Validate_Cached_Candidate;
   end Cached_Attribute_Ops;

   -------------------------
   -- Build_Array_VS_Func --
   -------------------------

   function Build_Array_VS_Func
     (Attr       : Node_Id;
      Formal_Typ : Entity_Id;
      Array_Typ  : Entity_Id) return Entity_Id
   is
      Loc      : constant Source_Ptr := Sloc (Attr);
      Comp_Typ : constant Entity_Id :=
        Validated_View (Component_Type (Array_Typ));

      function Validate_Component
        (Obj_Id  : Entity_Id;
         Indexes : List_Id) return Node_Id;
      --  Process a single component denoted by indexes Indexes. Obj_Id denotes
      --  the entity of the validation parameter. Return the check associated
      --  with the component.

      function Validate_Dimension
        (Obj_Id  : Entity_Id;
         Dim     : Int;
         Indexes : List_Id) return Node_Id;
      --  Process dimension Dim of the array type. Obj_Id denotes the entity
      --  of the validation parameter. Indexes is a list where each dimension
      --  deposits its loop variable, which will later identify a component.
      --  Return the loop associated with the current dimension.

      ------------------------
      -- Validate_Component --
      ------------------------

      function Validate_Component
        (Obj_Id  : Entity_Id;
         Indexes : List_Id) return Node_Id
      is
         Attr_Nam : Name_Id;

      begin
         if Is_Scalar_Type (Comp_Typ) then
            Attr_Nam := Name_Valid;
         else
            Attr_Nam := Name_Valid_Scalars;
         end if;

         --  Generate:
         --    if not Array_Typ (Obj_Id) (Indexes)'Valid[_Scalars] then
         --       return False;
         --    end if;

         return
           Make_If_Statement (Loc,
             Condition =>
               Make_Op_Not (Loc,
                 Right_Opnd =>
                   Make_Attribute_Reference (Loc,
                     Prefix         =>
                       Make_Indexed_Component (Loc,
                         Prefix      =>
                           Unchecked_Convert_To (Array_Typ,
                             New_Occurrence_Of (Obj_Id, Loc)),
                         Expressions => Indexes),
                     Attribute_Name => Attr_Nam)),

             Then_Statements => New_List (
               Make_Simple_Return_Statement (Loc,
                 Expression => New_Occurrence_Of (Standard_False, Loc))));
      end Validate_Component;

      ------------------------
      -- Validate_Dimension --
      ------------------------

      function Validate_Dimension
        (Obj_Id  : Entity_Id;
         Dim     : Int;
         Indexes : List_Id) return Node_Id
      is
         Index : Entity_Id;

      begin
         --  Validate the component once all dimensions have produced their
         --  individual loops.

         if Dim > Number_Dimensions (Array_Typ) then
            return Validate_Component (Obj_Id, Indexes);

         --  Process the current dimension

         else
            Index :=
              Make_Defining_Identifier (Loc, New_External_Name ('J', Dim));

            Append_To (Indexes, New_Occurrence_Of (Index, Loc));

            --  Generate:
            --    for J1 in Array_Typ (Obj_Id)'Range (1) loop
            --       for JN in Array_Typ (Obj_Id)'Range (N) loop
            --          if not Array_Typ (Obj_Id) (Indexes)'Valid[_Scalars]
            --          then
            --             return False;
            --          end if;
            --       end loop;
            --    end loop;

            return
              Make_Implicit_Loop_Statement (Attr,
                Identifier       => Empty,
                Iteration_Scheme =>
                  Make_Iteration_Scheme (Loc,
                    Loop_Parameter_Specification =>
                      Make_Loop_Parameter_Specification (Loc,
                        Defining_Identifier         => Index,
                        Discrete_Subtype_Definition =>
                          Make_Attribute_Reference (Loc,
                            Prefix          =>
                              Unchecked_Convert_To (Array_Typ,
                                New_Occurrence_Of (Obj_Id, Loc)),
                            Attribute_Name  => Name_Range,
                            Expressions     => New_List (
                              Make_Integer_Literal (Loc, Dim))))),
                Statements       => New_List (
                  Validate_Dimension (Obj_Id, Dim + 1, Indexes)));
         end if;
      end Validate_Dimension;

      --  Local variables

      Func_Id : constant Entity_Id := Make_Temporary (Loc, 'V');
      Indexes : constant List_Id   := New_List;
      Obj_Id  : constant Entity_Id := Make_Temporary (Loc, 'A');
      Stmts   : List_Id;

   --  Start of processing for Build_Array_VS_Func

   begin
      Stmts := New_List (Validate_Dimension (Obj_Id, 1, Indexes));

      --  Generate:
      --    return True;

      Append_To (Stmts,
        Make_Simple_Return_Statement (Loc,
          Expression => New_Occurrence_Of (Standard_True, Loc)));

      --  Generate:
      --    function Func_Id (Obj_Id : Formal_Typ) return Boolean is
      --    begin
      --       Stmts
      --    end Func_Id;

      Mutate_Ekind    (Func_Id, E_Function);
      Set_Is_Internal (Func_Id);
      Set_Is_Pure     (Func_Id);

      if not Debug_Generated_Code then
         Set_Debug_Info_Off (Func_Id);
      end if;

      Insert_Action (Attr,
        Make_Subprogram_Body (Loc,
          Specification              =>
            Make_Function_Specification (Loc,
              Defining_Unit_Name       => Func_Id,
              Parameter_Specifications => New_List (
                Make_Parameter_Specification (Loc,
                  Defining_Identifier => Obj_Id,
                  Parameter_Type      => New_Occurrence_Of (Formal_Typ, Loc))),
              Result_Definition        =>
                New_Occurrence_Of (Standard_Boolean, Loc)),
          Declarations               => New_List,
          Handled_Statement_Sequence =>
            Make_Handled_Sequence_Of_Statements (Loc,
              Statements => Stmts)));

      return Func_Id;
   end Build_Array_VS_Func;

   ---------------------------------
   -- Build_Disp_Get_Task_Id_Call --
   ---------------------------------

   function Build_Disp_Get_Task_Id_Call (Actual : Node_Id) return Node_Id is
      Loc  : constant Source_Ptr := Sloc (Actual);
      Typ  : constant Entity_Id  := Etype (Actual);
      Subp : constant Entity_Id  := Find_Prim_Op (Typ, Name_uDisp_Get_Task_Id);

   begin
      --  Generate:
      --    _Disp_Get_Task_Id (Actual)

      return
        Make_Function_Call (Loc,
          Name                   => New_Occurrence_Of (Subp, Loc),
          Parameter_Associations => New_List (Actual));
   end Build_Disp_Get_Task_Id_Call;

   --------------------------
   -- Build_Record_VS_Func --
   --------------------------

   function Build_Record_VS_Func
     (Attr       : Node_Id;
      Formal_Typ : Entity_Id;
      Rec_Typ    : Entity_Id) return Entity_Id
   is
      --  NOTE: The logic of Build_Record_VS_Func is intentionally passive.
      --  It generates code only when there are components, discriminants,
      --  or variant parts to validate.

      --  NOTE: The routines within Build_Record_VS_Func are intentionally
      --  unnested to avoid deep indentation of code.

      Loc : constant Source_Ptr := Sloc (Attr);

      procedure Validate_Component_List
        (Obj_Id    : Entity_Id;
         Comp_List : Node_Id;
         Stmts     : in out List_Id);
      --  Process all components and variant parts of component list Comp_List.
      --  Obj_Id denotes the entity of the validation parameter. All new code
      --  is added to list Stmts.

      procedure Validate_Field
        (Obj_Id : Entity_Id;
         Field  : Node_Id;
         Cond   : in out Node_Id);
      --  Process component declaration or discriminant specification Field.
      --  Obj_Id denotes the entity of the validation parameter. Cond denotes
      --  an "or else" conditional expression which contains the new code (if
      --  any).

      procedure Validate_Fields
        (Obj_Id : Entity_Id;
         Fields : List_Id;
         Stmts  : in out List_Id);
      --  Process component declarations or discriminant specifications in list
      --  Fields. Obj_Id denotes the entity of the validation parameter. All
      --  new code is added to list Stmts.

      procedure Validate_Variant
        (Obj_Id : Entity_Id;
         Var    : Node_Id;
         Alts   : in out List_Id);
      --  Process variant Var. Obj_Id denotes the entity of the validation
      --  parameter. Alts denotes a list of case statement alternatives which
      --  contains the new code (if any).

      procedure Validate_Variant_Part
        (Obj_Id   : Entity_Id;
         Var_Part : Node_Id;
         Stmts    : in out List_Id);
      --  Process variant part Var_Part. Obj_Id denotes the entity of the
      --  validation parameter. All new code is added to list Stmts.

      -----------------------------
      -- Validate_Component_List --
      -----------------------------

      procedure Validate_Component_List
        (Obj_Id    : Entity_Id;
         Comp_List : Node_Id;
         Stmts     : in out List_Id)
      is
         Var_Part : constant Node_Id := Variant_Part (Comp_List);

      begin
         --  Validate all components

         Validate_Fields
           (Obj_Id => Obj_Id,
            Fields => Component_Items (Comp_List),
            Stmts  => Stmts);

         --  Validate the variant part

         if Present (Var_Part) then
            Validate_Variant_Part
              (Obj_Id   => Obj_Id,
               Var_Part => Var_Part,
               Stmts    => Stmts);
         end if;
      end Validate_Component_List;

      --------------------
      -- Validate_Field --
      --------------------

      procedure Validate_Field
        (Obj_Id : Entity_Id;
         Field  : Node_Id;
         Cond   : in out Node_Id)
      is
         Field_Id  : constant Entity_Id := Defining_Entity (Field);
         Field_Nam : constant Name_Id   := Chars (Field_Id);
         Field_Typ : constant Entity_Id := Validated_View (Etype (Field_Id));
         Attr_Nam  : Name_Id;

      begin
         --  Do not process internally-generated fields. Note that checking for
         --  Comes_From_Source is not correct because this will eliminate the
         --  components within the corresponding record of a protected type.

         if Field_Nam in Name_uObject | Name_uParent | Name_uTag then
            null;

         --  Do not process fields without any scalar components

         elsif not Scalar_Part_Present (Field_Typ) then
            null;

         --  Otherwise the field needs to be validated. Use Make_Identifier
         --  rather than New_Occurrence_Of to identify the field because the
         --  wrong entity may be picked up when private types are involved.

         --  Generate:
         --    [or else] not Rec_Typ (Obj_Id).Item_Nam'Valid[_Scalars]

         else
            if Is_Scalar_Type (Field_Typ) then
               Attr_Nam := Name_Valid;
            else
               Attr_Nam := Name_Valid_Scalars;
            end if;

            Evolve_Or_Else (Cond,
              Make_Op_Not (Loc,
                Right_Opnd =>
                  Make_Attribute_Reference (Loc,
                    Prefix         =>
                      Make_Selected_Component (Loc,
                        Prefix        =>
                          Unchecked_Convert_To (Rec_Typ,
                            New_Occurrence_Of (Obj_Id, Loc)),
                        Selector_Name => Make_Identifier (Loc, Field_Nam)),
                    Attribute_Name => Attr_Nam)));
         end if;
      end Validate_Field;

      ---------------------
      -- Validate_Fields --
      ---------------------

      procedure Validate_Fields
        (Obj_Id : Entity_Id;
         Fields : List_Id;
         Stmts  : in out List_Id)
      is
         Cond  : Node_Id;
         Field : Node_Id;

      begin
         --  Assume that none of the fields are eligible for verification

         Cond := Empty;

         --  Validate all fields

         Field := First_Non_Pragma (Fields);
         while Present (Field) loop
            Validate_Field
              (Obj_Id => Obj_Id,
               Field  => Field,
               Cond   => Cond);

            Next_Non_Pragma (Field);
         end loop;

         --  Generate:
         --    if        not Rec_Typ (Obj_Id).Item_Nam_1'Valid[_Scalars]
         --      or else not Rec_Typ (Obj_Id).Item_Nam_N'Valid[_Scalars]
         --    then
         --       return False;
         --    end if;

         if Present (Cond) then
            Append_New_To (Stmts,
              Make_Implicit_If_Statement (Attr,
                Condition       => Cond,
                Then_Statements => New_List (
                  Make_Simple_Return_Statement (Loc,
                    Expression => New_Occurrence_Of (Standard_False, Loc)))));
         end if;
      end Validate_Fields;

      ----------------------
      -- Validate_Variant --
      ----------------------

      procedure Validate_Variant
        (Obj_Id : Entity_Id;
         Var    : Node_Id;
         Alts   : in out List_Id)
      is
         Stmts : List_Id;

      begin
         --  Assume that none of the components and variants are eligible for
         --  verification.

         Stmts := No_List;

         --  Validate components

         Validate_Component_List
           (Obj_Id    => Obj_Id,
            Comp_List => Component_List (Var),
            Stmts     => Stmts);

         --  Generate a null statement in case none of the components were
         --  verified because this will otherwise eliminate an alternative
         --  from the variant case statement and render the generated code
         --  illegal.

         if No (Stmts) then
            Append_New_To (Stmts, Make_Null_Statement (Loc));
         end if;

         --  Generate:
         --    when Discrete_Choices =>
         --       Stmts

         Append_New_To (Alts,
           Make_Case_Statement_Alternative (Loc,
             Discrete_Choices =>
               New_Copy_List_Tree (Discrete_Choices (Var)),
             Statements       => Stmts));
      end Validate_Variant;

      ---------------------------
      -- Validate_Variant_Part --
      ---------------------------

      procedure Validate_Variant_Part
        (Obj_Id   : Entity_Id;
         Var_Part : Node_Id;
         Stmts    : in out List_Id)
      is
         Vars : constant List_Id := Variants (Var_Part);
         Alts : List_Id;
         Var  : Node_Id;

      begin
         --  Assume that none of the variants are eligible for verification

         Alts := No_List;

         --  Validate variants

         Var := First_Non_Pragma (Vars);
         while Present (Var) loop
            Validate_Variant
              (Obj_Id => Obj_Id,
               Var    => Var,
               Alts   => Alts);

            Next_Non_Pragma (Var);
         end loop;

         --  Even though individual variants may lack eligible components, the
         --  alternatives must still be generated.

         pragma Assert (Present (Alts));

         --  Generate:
         --    case Rec_Typ (Obj_Id).Discriminant is
         --       when Discrete_Choices_1 =>
         --          Stmts_1
         --       when Discrete_Choices_N =>
         --          Stmts_N
         --    end case;

         Append_New_To (Stmts,
           Make_Case_Statement (Loc,
             Expression   =>
               Make_Selected_Component (Loc,
                 Prefix        =>
                   Unchecked_Convert_To (Rec_Typ,
                     New_Occurrence_Of (Obj_Id, Loc)),
                 Selector_Name => New_Copy_Tree (Name (Var_Part))),
             Alternatives => Alts));
      end Validate_Variant_Part;

      --  Local variables

      Func_Id  : constant Entity_Id := Make_Temporary (Loc, 'V');
      Obj_Id   : constant Entity_Id := Make_Temporary (Loc, 'R');
      Comps    : Node_Id;
      Stmts    : List_Id;
      Typ      : Entity_Id;
      Typ_Decl : Node_Id;
      Typ_Def  : Node_Id;
      Typ_Ext  : Node_Id;

   --  Start of processing for Build_Record_VS_Func

   begin
      Typ := Validated_View (Rec_Typ);

      --  Use the root type when dealing with a class-wide type

      if Is_Class_Wide_Type (Typ) then
         Typ := Validated_View (Root_Type (Typ));
      end if;

      Typ_Decl := Declaration_Node (Typ);
      Typ_Def  := Type_Definition (Typ_Decl);

      --  The components of a derived type are located in the extension part

      if Nkind (Typ_Def) = N_Derived_Type_Definition then
         Typ_Ext := Record_Extension_Part (Typ_Def);

         if Present (Typ_Ext) then
            Comps := Component_List (Typ_Ext);
         else
            Comps := Empty;
         end if;

      --  Otherwise the components are available in the definition

      else
         Comps := Component_List (Typ_Def);
      end if;

      --  The code generated by this routine is as follows:
      --
      --    function Func_Id (Obj_Id : Formal_Typ) return Boolean is
      --    begin
      --       if not        Rec_Typ (Obj_Id).Discriminant_1'Valid[_Scalars]
      --         or else not Rec_Typ (Obj_Id).Discriminant_N'Valid[_Scalars]
      --       then
      --          return False;
      --       end if;
      --
      --       if not        Rec_Typ (Obj_Id).Component_1'Valid[_Scalars]
      --         or else not Rec_Typ (Obj_Id).Component_N'Valid[_Scalars]
      --       then
      --          return False;
      --       end if;
      --
      --       case Discriminant_1 is
      --          when Choice_1 =>
      --             if not        Rec_Typ (Obj_Id).Component_1'Valid[_Scalars]
      --               or else not Rec_Typ (Obj_Id).Component_N'Valid[_Scalars]
      --             then
      --                return False;
      --             end if;
      --
      --             case Discriminant_N is
      --                ...
      --          when Choice_N =>
      --             ...
      --       end case;
      --
      --       return True;
      --    end Func_Id;

      --  Assume that the record type lacks eligible components, discriminants,
      --  and variant parts.

      Stmts := No_List;

      --  Validate the discriminants

      if not Is_Unchecked_Union (Rec_Typ) then
         Validate_Fields
           (Obj_Id => Obj_Id,
            Fields => Discriminant_Specifications (Typ_Decl),
            Stmts  => Stmts);
      end if;

      --  Validate the components and variant parts

      Validate_Component_List
        (Obj_Id    => Obj_Id,
         Comp_List => Comps,
         Stmts     => Stmts);

      --  Generate:
      --    return True;

      Append_New_To (Stmts,
        Make_Simple_Return_Statement (Loc,
          Expression => New_Occurrence_Of (Standard_True, Loc)));

      --  Generate:
      --    function Func_Id (Obj_Id : Formal_Typ) return Boolean is
      --    begin
      --       Stmts
      --    end Func_Id;

      Mutate_Ekind    (Func_Id, E_Function);
      Set_Is_Internal (Func_Id);
      Set_Is_Pure     (Func_Id);

      if not Debug_Generated_Code then
         Set_Debug_Info_Off (Func_Id);
      end if;

      Insert_Action (Attr,
        Make_Subprogram_Body (Loc,
          Specification =>
            Make_Function_Specification (Loc,
              Defining_Unit_Name       => Func_Id,
              Parameter_Specifications => New_List (
                Make_Parameter_Specification (Loc,
                  Defining_Identifier => Obj_Id,
                  Parameter_Type      => New_Occurrence_Of (Formal_Typ, Loc))),
              Result_Definition        =>
                New_Occurrence_Of (Standard_Boolean, Loc)),
          Declarations               => New_List,
          Handled_Statement_Sequence =>
            Make_Handled_Sequence_Of_Statements (Loc,
              Statements => Stmts)),
        Suppress => Discriminant_Check);

      return Func_Id;
   end Build_Record_VS_Func;

   -----------------------------------
   -- Default_Streaming_Unavailable --
   -----------------------------------

   function Default_Streaming_Unavailable (Typ : Entity_Id) return Boolean is
      Btyp : constant Entity_Id := Implementation_Base_Type (Typ);
   begin
      if Is_Immutably_Limited_Type (Btyp)
        and then not Is_Tagged_Type (Btyp)
        and then not (Ekind (Btyp) = E_Record_Type
                      and then Present (Corresponding_Concurrent_Type (Btyp)))
      then
         pragma Assert (In_Instance_Body);
         return True;
      end if;
      return False;
   end Default_Streaming_Unavailable;

   -----------------------------------
   -- Expand_Access_To_Protected_Op --
   -----------------------------------

   procedure Expand_Access_To_Protected_Op
     (N    : Node_Id;
      Pref : Node_Id;
      Typ  : Entity_Id)
   is
      --  The value of the attribute_reference is a record containing two
      --  fields: an access to the protected object, and an access to the
      --  subprogram itself. The prefix is an identifier or a selected
      --  component.

      function Has_By_Protected_Procedure_Prefixed_View return Boolean;
      --  Determine whether Pref denotes the prefixed class-wide interface
      --  view of a procedure with synchronization kind By_Protected_Procedure.

      ----------------------------------------------
      -- Has_By_Protected_Procedure_Prefixed_View --
      ----------------------------------------------

      function Has_By_Protected_Procedure_Prefixed_View return Boolean is
      begin
         return Nkind (Pref) = N_Selected_Component
           and then Nkind (Prefix (Pref)) in N_Has_Entity
           and then Present (Entity (Prefix (Pref)))
           and then Is_Class_Wide_Type (Etype (Entity (Prefix (Pref))))
           and then (Is_Synchronized_Interface (Etype (Entity (Prefix (Pref))))
                       or else
                     Is_Protected_Interface (Etype (Entity (Prefix (Pref)))))
           and then Is_By_Protected_Procedure (Entity (Selector_Name (Pref)));
      end Has_By_Protected_Procedure_Prefixed_View;

      --  Local variables

      Loc     : constant Source_Ptr := Sloc (N);
      Agg     : Node_Id;
      Btyp    : constant Entity_Id := Base_Type (Typ);
      Sub     : Entity_Id          := Empty;
      Sub_Ref : Node_Id;
      E_T     : constant Entity_Id := Equivalent_Type (Btyp);
      Acc     : constant Entity_Id :=
                  Etype (Next_Component (First_Component (E_T)));
      Obj_Ref : Node_Id;
      Curr    : Entity_Id;

   --  Start of processing for Expand_Access_To_Protected_Op

   begin
      --  Within the body of the protected type, the prefix designates a local
      --  operation, and the object is the first parameter of the corresponding
      --  protected body of the current enclosing operation.

      if Is_Entity_Name (Pref) then
         --  All indirect calls are external calls, so must do locking and
         --  barrier reevaluation, even if the 'Access occurs within the
         --  protected body. Hence the call to External_Subprogram, as opposed
         --  to Protected_Body_Subprogram, below. See RM-9.5(5). This means
         --  that indirect calls from within the same protected body will
         --  deadlock, as allowed by RM-9.5.1(8,15,17).

         Sub := New_Occurrence_Of (External_Subprogram (Entity (Pref)), Loc);

         --  Don't traverse the scopes when the attribute occurs within an init
         --  proc, because we directly use the _init formal of the init proc in
         --  that case.

         Curr := Current_Scope;
         if not Is_Init_Proc (Curr) then
            pragma Assert (In_Open_Scopes (Scope (Entity (Pref))));

            while Scope (Curr) /= Scope (Entity (Pref)) loop
               Curr := Scope (Curr);
            end loop;
         end if;

         --  In case of protected entries the first formal of its Protected_
         --  Body_Subprogram is the address of the object.

         if Ekind (Curr) = E_Entry then
            Obj_Ref :=
               New_Occurrence_Of
                 (First_Formal
                   (Protected_Body_Subprogram (Curr)), Loc);

         --  If the current scope is an init proc, then use the address of the
         --  _init formal as the object reference.

         elsif Is_Init_Proc (Curr) then
            Obj_Ref :=
              Make_Attribute_Reference (Loc,
                Prefix         => New_Occurrence_Of (First_Formal (Curr), Loc),
                Attribute_Name => Name_Address);

         --  In case of protected subprograms the first formal of its
         --  Protected_Body_Subprogram is the object and we get its address.

         else
            Obj_Ref :=
              Make_Attribute_Reference (Loc,
                Prefix =>
                   New_Occurrence_Of
                     (First_Formal
                        (Protected_Body_Subprogram (Curr)), Loc),
                Attribute_Name => Name_Address);
         end if;

      elsif Has_By_Protected_Procedure_Prefixed_View then
         Obj_Ref :=
           Make_Attribute_Reference (Loc,
             Prefix => Relocate_Node (Prefix (Pref)),
               Attribute_Name => Name_Address);

         --  Analyze the object address with expansion disabled. Required
         --  because its expansion would displace the pointer to the object,
         --  which is not correct at this stage since the object type is a
         --  class-wide interface type and we are dispatching a call to a
         --  thunk (which would erroneously displace the pointer again).

         Expander_Mode_Save_And_Set (False);
         Analyze (Obj_Ref);
         Set_Analyzed (Obj_Ref);
         Expander_Mode_Restore;

      --  Case where the prefix is not an entity name. Find the
      --  version of the protected operation to be called from
      --  outside the protected object.

      else
         Sub :=
           New_Occurrence_Of
             (External_Subprogram
               (Entity (Selector_Name (Pref))), Loc);

         Obj_Ref :=
           Make_Attribute_Reference (Loc,
             Prefix => Relocate_Node (Prefix (Pref)),
               Attribute_Name => Name_Address);
      end if;

      if Has_By_Protected_Procedure_Prefixed_View then
         declare
            Ctrl_Tag  : Node_Id := Duplicate_Subexpr (Prefix (Pref));
            Prim_Addr : Node_Id;
            Subp      : constant Entity_Id := Entity (Selector_Name (Pref));
            Typ       : constant Entity_Id :=
                          Etype (Etype (Entity (Prefix (Pref))));
         begin
            --  The target subprogram is a thunk; retrieve its address from
            --  its secondary dispatch table slot.

            Build_Get_Prim_Op_Address (Loc,
              Typ      => Typ,
              Tag_Node => Ctrl_Tag,
              Position => DT_Position (Subp),
              New_Node => Prim_Addr);

            --  Mark the access to the target subprogram as an access to the
            --  dispatch table and perform an unchecked type conversion to such
            --  access type. This is required to allow the backend to properly
            --  identify and handle the access to the dispatch table slot on
            --  targets where the dispatch table contains descriptors (instead
            --  of pointers).

            Set_Is_Dispatch_Table_Entity (Acc);
            Sub_Ref := Unchecked_Convert_To (Acc, Prim_Addr);
            Analyze (Sub_Ref);

            Agg :=
              Make_Aggregate (Loc,
                Expressions => New_List (Obj_Ref, Sub_Ref));
         end;

      --  Common case

      else
         Sub_Ref :=
           Make_Attribute_Reference (Loc,
             Prefix         => Sub,
             Attribute_Name => Name_Access);

         --  We set the type of the access reference to the already generated
         --  access_to_subprogram type, and declare the reference analyzed,
         --  to prevent further expansion when the enclosing aggregate is
         --  analyzed.

         Set_Etype (Sub_Ref, Acc);
         Set_Analyzed (Sub_Ref);

         Agg :=
           Make_Aggregate (Loc,
             Expressions => New_List (Obj_Ref, Sub_Ref));

         --  Sub_Ref has been marked as analyzed, but we still need to make
         --  sure Sub is correctly frozen.

         Freeze_Before (N, Entity (Sub));
      end if;

      Rewrite (N, Agg);
      Analyze_And_Resolve (N, E_T);

      --  For subsequent analysis, the node must retain its type. The backend
      --  will replace it with the equivalent type where needed.

      Set_Etype (N, Typ);
   end Expand_Access_To_Protected_Op;

   --------------------------
   -- Expand_Fpt_Attribute --
   --------------------------

   procedure Expand_Fpt_Attribute
     (N    : Node_Id;
      Pkg  : RE_Id;
      Nam  : Name_Id;
      Args : List_Id)
   is
      Loc : constant Source_Ptr := Sloc (N);
      Typ : constant Entity_Id  := Etype (N);
      Fnm : Node_Id;

   begin
      --  The function name is the selected component Attr_xxx.yyy where
      --  Attr_xxx is the package name, and yyy is the argument Nam.

      --  Note: it would be more usual to have separate RE entries for each
      --  of the entities in the Fat packages, but first they have identical
      --  names (so we would have to have lots of renaming declarations to
      --  meet the normal RE rule of separate names for all runtime entities),
      --  and second there would be an awful lot of them.

      Fnm :=
        Make_Selected_Component (Loc,
          Prefix        => New_Occurrence_Of (RTE (Pkg), Loc),
          Selector_Name => Make_Identifier (Loc, Nam));

      --  The generated call is given the provided set of parameters, and then
      --  wrapped in a conversion which converts the result to the target type.

      Rewrite (N,
        Convert_To (Typ,
          Make_Function_Call (Loc,
            Name                   => Fnm,
            Parameter_Associations => Args)));

      Analyze_And_Resolve (N, Typ);
   end Expand_Fpt_Attribute;

   ----------------------------
   -- Expand_Fpt_Attribute_R --
   ----------------------------

   --  The single argument is converted to its root type to call the
   --  appropriate runtime function, with the actual call being built
   --  by Expand_Fpt_Attribute

   procedure Expand_Fpt_Attribute_R (N : Node_Id) is
      E1  : constant Node_Id := First (Expressions (N));
      Ftp : Entity_Id;
      Pkg : RE_Id;
   begin
      Find_Fat_Info (Etype (E1), Ftp, Pkg);
      Expand_Fpt_Attribute
        (N, Pkg, Attribute_Name (N),
         New_List (Unchecked_Convert_To (Ftp, Relocate_Node (E1))));
   end Expand_Fpt_Attribute_R;

   -----------------------------
   -- Expand_Fpt_Attribute_RI --
   -----------------------------

   --  The first argument is converted to its root type and the second
   --  argument is converted to standard long long integer to call the
   --  appropriate runtime function, with the actual call being built
   --  by Expand_Fpt_Attribute

   procedure Expand_Fpt_Attribute_RI (N : Node_Id) is
      E1  : constant Node_Id := First (Expressions (N));
      E2  : constant Node_Id := Next (E1);
      Ftp : Entity_Id;
      Pkg : RE_Id;
   begin
      Find_Fat_Info (Etype (E1), Ftp, Pkg);
      Expand_Fpt_Attribute
        (N, Pkg, Attribute_Name (N),
         New_List (
           Unchecked_Convert_To (Ftp, Relocate_Node (E1)),
           Unchecked_Convert_To (Standard_Integer, Relocate_Node (E2))));
   end Expand_Fpt_Attribute_RI;

   -----------------------------
   -- Expand_Fpt_Attribute_RR --
   -----------------------------

   --  The two arguments are converted to their root types to call the
   --  appropriate runtime function, with the actual call being built
   --  by Expand_Fpt_Attribute

   procedure Expand_Fpt_Attribute_RR (N : Node_Id) is
      E1  : constant Node_Id := First (Expressions (N));
      E2  : constant Node_Id := Next (E1);
      Ftp : Entity_Id;
      Pkg : RE_Id;

   begin
      Find_Fat_Info (Etype (E1), Ftp, Pkg);
      Expand_Fpt_Attribute
        (N, Pkg, Attribute_Name (N),
         New_List (
           Unchecked_Convert_To (Ftp, Relocate_Node (E1)),
           Unchecked_Convert_To (Ftp, Relocate_Node (E2))));
   end Expand_Fpt_Attribute_RR;

   ---------------------------------
   -- Expand_Loop_Entry_Attribute --
   ---------------------------------

   procedure Expand_Loop_Entry_Attribute (N : Node_Id) is
      procedure Build_Conditional_Block
        (Loc       : Source_Ptr;
         Cond      : Node_Id;
         Loop_Stmt : Node_Id;
         If_Stmt   : out Node_Id;
         Blk_Stmt  : out Node_Id);
      --  Create a block Blk_Stmt with an empty declarative list and a single
      --  loop Loop_Stmt. The block is encased in an if statement If_Stmt with
      --  condition Cond. If_Stmt is Empty when there is no condition provided.

      function Is_Array_Iteration (N : Node_Id) return Boolean;
      --  Determine whether loop statement N denotes an Ada 2012 iteration over
      --  an array object.

      -----------------------------
      -- Build_Conditional_Block --
      -----------------------------

      procedure Build_Conditional_Block
        (Loc       : Source_Ptr;
         Cond      : Node_Id;
         Loop_Stmt : Node_Id;
         If_Stmt   : out Node_Id;
         Blk_Stmt  : out Node_Id)
      is
      begin
         --  Do not reanalyze the original loop statement because it is simply
         --  being relocated.

         Set_Analyzed (Loop_Stmt);

         Blk_Stmt :=
           Make_Block_Statement (Loc,
             Declarations               => New_List,
             Handled_Statement_Sequence =>
               Make_Handled_Sequence_Of_Statements (Loc,
                 Statements => New_List (Loop_Stmt)));

         if Present (Cond) then
            If_Stmt :=
              Make_If_Statement (Loc,
                Condition       => Cond,
                Then_Statements => New_List (Blk_Stmt));
         else
            If_Stmt := Empty;
         end if;
      end Build_Conditional_Block;

      ------------------------
      -- Is_Array_Iteration --
      ------------------------

      function Is_Array_Iteration (N : Node_Id) return Boolean is
         Stmt : constant Node_Id := Original_Node (N);
         Iter : Node_Id;

      begin
         if Nkind (Stmt) = N_Loop_Statement
           and then Present (Iteration_Scheme (Stmt))
           and then Present (Iterator_Specification (Iteration_Scheme (Stmt)))
         then
            Iter := Iterator_Specification (Iteration_Scheme (Stmt));

            return
              Of_Present (Iter) and then Is_Array_Type (Etype (Name (Iter)));
         end if;

         return False;
      end Is_Array_Iteration;

      --  Local variables

      Pref      : constant Node_Id    := Prefix (N);
      Base_Typ  : constant Entity_Id  := Base_Type (Etype (Pref));
      Exprs     : constant List_Id    := Expressions (N);
      Loc       : constant Source_Ptr := Sloc (N);
      Aux_Decl  : Node_Id;
      Blk       : Node_Id := Empty;
      Decls     : List_Id;
      Installed : Boolean;
      Loop_Id   : Entity_Id;
      Loop_Stmt : Node_Id;
      Result    : Node_Id := Empty;
      Scheme    : Node_Id;
      Temp_Decl : Node_Id;
      Temp_Id   : Entity_Id;

   --  Start of processing for Expand_Loop_Entry_Attribute

   begin
      --  Step 1: Find the related loop

      --  The loop label variant of attribute 'Loop_Entry already has all the
      --  information in its expression.

      if Present (Exprs) then
         Loop_Id   := Entity (First (Exprs));
         Loop_Stmt := Label_Construct (Parent (Loop_Id));

      --  Climb the parent chain to find the nearest enclosing loop. Skip
      --  all internally generated loops for quantified expressions and for
      --  element iterators over multidimensional arrays because the pragma
      --  applies to source loop.

      else
         Loop_Stmt := N;
         while Present (Loop_Stmt) loop
            if Nkind (Loop_Stmt) = N_Loop_Statement
              and then Nkind (Original_Node (Loop_Stmt)) = N_Loop_Statement
              and then Comes_From_Source (Original_Node (Loop_Stmt))
            then
               exit;
            end if;

            Loop_Stmt := Parent (Loop_Stmt);
         end loop;

         Loop_Id := Entity (Identifier (Loop_Stmt));
      end if;

      --  Step 2: Transform the loop

      --  The loop has already been transformed during the expansion of a prior
      --  'Loop_Entry attribute. Retrieve the declarative list of the block.

      if Has_Loop_Entry_Attributes (Loop_Id) then

         --  When the related loop name appears as the argument of attribute
         --  Loop_Entry, the corresponding label construct is the generated
         --  block statement. This is because the expander reuses the label.

         if Nkind (Loop_Stmt) = N_Block_Statement then
            Decls := Declarations (Loop_Stmt);

         --  In all other cases, the loop must appear in the handled sequence
         --  of statements of the generated block.

         else
            pragma Assert
              (Nkind (Parent (Loop_Stmt)) = N_Handled_Sequence_Of_Statements
                and then
                  Nkind (Parent (Parent (Loop_Stmt))) = N_Block_Statement);

            Decls := Declarations (Parent (Parent (Loop_Stmt)));
         end if;

      --  Transform the loop into a conditional block

      else
         Set_Has_Loop_Entry_Attributes (Loop_Id);
         Scheme := Iteration_Scheme (Loop_Stmt);

         --  Infinite loops are transformed into:

         --    declare
         --       Temp1 : constant <type of Pref1> := <Pref1>;
         --       . . .
         --       TempN : constant <type of PrefN> := <PrefN>;
         --    begin
         --       loop
         --          <original source statements with attribute rewrites>
         --       end loop;
         --    end;

         if No (Scheme) then
            Build_Conditional_Block (Loc,
              Cond      => Empty,
              Loop_Stmt => Relocate_Node (Loop_Stmt),
              If_Stmt   => Result,
              Blk_Stmt  => Blk);

            Result := Blk;

         --  While loops are transformed into:

         --    function Fnn return Boolean is
         --    begin
         --       <condition actions>
         --       return <condition>;
         --    end Fnn;

         --    if Fnn then
         --       declare
         --          Temp1 : constant <type of Pref1> := <Pref1>;
         --          . . .
         --          TempN : constant <type of PrefN> := <PrefN>;
         --       begin
         --          loop
         --             <original source statements with attribute rewrites>
         --             exit when not Fnn;
         --          end loop;
         --       end;
         --    end if;

         --  Note that loops over iterators and containers are already
         --  converted into while loops.

         elsif Present (Condition (Scheme)) then
            declare
               Func_Decl : Node_Id;
               Func_Id   : Entity_Id;
               Stmts     : List_Id;

            begin
               Func_Id := Make_Temporary (Loc, 'F');

               --  Wrap the condition of the while loop in a Boolean function.
               --  This avoids the duplication of the same code which may lead
               --  to gigi issues with respect to multiple declaration of the
               --  same entity in the presence of side effects or checks. Note
               --  that the condition actions must also be relocated into the
               --  wrapping function because they may contain itypes, e.g. in
               --  the case of a comparison involving slices.

               --  Generate:
               --    <condition actions>
               --    return <condition>;

               if Present (Condition_Actions (Scheme)) then
                  Stmts := Condition_Actions (Scheme);
               else
                  Stmts := New_List;
               end if;

               Append_To (Stmts,
                 Make_Simple_Return_Statement (Loc,
                   Expression =>
                     New_Copy_Tree (Condition (Scheme),
                       New_Scope => Func_Id)));

               --  Generate:
               --    function Fnn return Boolean is
               --    begin
               --       <Stmts>
               --    end Fnn;

               Func_Decl :=
                 Make_Subprogram_Body (Loc,
                   Specification              =>
                     Make_Function_Specification (Loc,
                       Defining_Unit_Name => Func_Id,
                       Result_Definition  =>
                         New_Occurrence_Of (Standard_Boolean, Loc)),
                   Declarations               => Empty_List,
                   Handled_Statement_Sequence =>
                     Make_Handled_Sequence_Of_Statements (Loc,
                       Statements => Stmts));

               --  The function is inserted before the related loop. Make sure
               --  to analyze it in the context of the loop's enclosing scope.

               Push_Scope (Scope (Loop_Id));
               Insert_Action (Loop_Stmt, Func_Decl);
               Pop_Scope;

               --  The analysis of the condition may have generated entities
               --  (such as itypes) that are now used within the function.
               --  Adjust their scopes accordingly so that their use appears
               --  in their scope of definition.

               declare
                  Ent : Entity_Id;

               begin
                  Ent := First_Entity (Loop_Id);

                  while Present (Ent) loop
                     --  Various entities that now occur within the function
                     --  need to have their scope reset, but not all entities
                     --  associated with Loop_Id are now inside the function.
                     --  The function entity itself and loop parameters can
                     --  be outside the function, and there may be others.
                     --  It's not clear how the determination of what entity
                     --  scopes need to be adjusted can be made accurately.
                     --  Perhaps it will be necessary to traverse the function
                     --  body to find the exact entities whose scopes need to
                     --  be reset to the function's Entity_Id. ???

                     if Ekind (Ent) /= E_Loop_Parameter
                       and then Ent /= Func_Id
                     then
                        Set_Scope (Ent, Func_Id);
                     end if;

                     Next_Entity (Ent);
                  end loop;
               end;

               --  Transform the original while loop into an infinite loop
               --  where the last statement checks the negated condition. This
               --  placement ensures that the condition will not be evaluated
               --  twice on the first iteration.

               Set_Iteration_Scheme (Loop_Stmt, Empty);
               Scheme := Empty;

               --  Generate:
               --    exit when not Fnn;

               Append_To (Statements (Loop_Stmt),
                 Make_Exit_Statement (Loc,
                   Condition =>
                     Make_Op_Not (Loc,
                       Right_Opnd =>
                         Make_Function_Call (Loc,
                           Name => New_Occurrence_Of (Func_Id, Loc)))));

               Build_Conditional_Block (Loc,
                 Cond      =>
                   Make_Function_Call (Loc,
                     Name => New_Occurrence_Of (Func_Id, Loc)),
                 Loop_Stmt => Relocate_Node (Loop_Stmt),
                 If_Stmt   => Result,
                 Blk_Stmt  => Blk);
            end;

         --  Ada 2012 iteration over an array is transformed into:

         --    if <Array_Nam>'Length (1) > 0
         --      and then <Array_Nam>'Length (N) > 0
         --    then
         --       declare
         --          Temp1 : constant <type of Pref1> := <Pref1>;
         --          . . .
         --          TempN : constant <type of PrefN> := <PrefN>;
         --       begin
         --          for X in ... loop  --  multiple loops depending on dims
         --             <original source statements with attribute rewrites>
         --          end loop;
         --       end;
         --    end if;

         elsif Is_Array_Iteration (Loop_Stmt) then
            declare
               Array_Nam : constant Entity_Id :=
                             Entity (Name (Iterator_Specification
                              (Iteration_Scheme (Original_Node (Loop_Stmt)))));
               Num_Dims  : constant Pos :=
                             Number_Dimensions (Etype (Array_Nam));
               Cond      : Node_Id := Empty;
               Check     : Node_Id;

            begin
               --  Generate a check which determines whether all dimensions of
               --  the array are non-null.

               for Dim in 1 .. Num_Dims loop
                  Check :=
                    Make_Op_Gt (Loc,
                      Left_Opnd  =>
                        Make_Attribute_Reference (Loc,
                          Prefix         => New_Occurrence_Of (Array_Nam, Loc),
                          Attribute_Name => Name_Length,
                          Expressions    => New_List (
                            Make_Integer_Literal (Loc, Dim))),
                      Right_Opnd =>
                        Make_Integer_Literal (Loc, 0));

                  if No (Cond) then
                     Cond := Check;
                  else
                     Cond :=
                       Make_And_Then (Loc,
                         Left_Opnd  => Cond,
                         Right_Opnd => Check);
                  end if;
               end loop;

               Build_Conditional_Block (Loc,
                 Cond      => Cond,
                 Loop_Stmt => Relocate_Node (Loop_Stmt),
                 If_Stmt   => Result,
                 Blk_Stmt  => Blk);
            end;

         --  For loops are transformed into:

         --    if <Low> <= <High> then
         --       declare
         --          Temp1 : constant <type of Pref1> := <Pref1>;
         --          . . .
         --          TempN : constant <type of PrefN> := <PrefN>;
         --       begin
         --          for <Def_Id> in <Low> .. <High> loop
         --             <original source statements with attribute rewrites>
         --          end loop;
         --       end;
         --    end if;

         elsif Present (Loop_Parameter_Specification (Scheme)) then
            declare
               Loop_Spec : constant Node_Id :=
                             Loop_Parameter_Specification (Scheme);
               Cond      : Node_Id;
               Subt_Def  : Node_Id;

            begin
               Subt_Def := Discrete_Subtype_Definition (Loop_Spec);

               --  When the loop iterates over a subtype indication with a
               --  range, use the low and high bounds of the subtype itself.

               if Nkind (Subt_Def) = N_Subtype_Indication then
                  Subt_Def := Scalar_Range (Etype (Subt_Def));
               end if;

               pragma Assert (Nkind (Subt_Def) = N_Range);

               --  Generate
               --    Low <= High

               Cond :=
                 Make_Op_Le (Loc,
                   Left_Opnd  => New_Copy_Tree (Low_Bound (Subt_Def)),
                   Right_Opnd => New_Copy_Tree (High_Bound (Subt_Def)));

               Build_Conditional_Block (Loc,
                 Cond      => Cond,
                 Loop_Stmt => Relocate_Node (Loop_Stmt),
                 If_Stmt   => Result,
                 Blk_Stmt  => Blk);
            end;
         end if;

         Decls := Declarations (Blk);
      end if;

      --  Step 3: Create a constant to capture the value of the prefix at the
      --  entry point into the loop.

      Temp_Id := Make_Temporary (Loc, 'P');

      --  Preserve the tag of the prefix by offering a specific view of the
      --  class-wide version of the prefix.

      if Is_Tagged_Type (Base_Typ) then
         Tagged_Case : declare
            CW_Temp : Entity_Id;
            CW_Typ  : Entity_Id;

         begin
            --  Generate:
            --    CW_Temp : constant Base_Typ'Class := Base_Typ'Class (Pref);

            CW_Temp := Make_Temporary (Loc, 'T');
            CW_Typ  := Class_Wide_Type (Base_Typ);

            Aux_Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier => CW_Temp,
                Constant_Present    => True,
                Object_Definition   => New_Occurrence_Of (CW_Typ, Loc),
                Expression          =>
                  Convert_To (CW_Typ, Relocate_Node (Pref)));
            Append_To (Decls, Aux_Decl);

            --  Generate:
            --    Temp : Base_Typ renames Base_Typ (CW_Temp);

            Temp_Decl :=
              Make_Object_Renaming_Declaration (Loc,
                Defining_Identifier => Temp_Id,
                Subtype_Mark        => New_Occurrence_Of (Base_Typ, Loc),
                Name                =>
                  Convert_To (Base_Typ, New_Occurrence_Of (CW_Temp, Loc)));
            Append_To (Decls, Temp_Decl);
         end Tagged_Case;

      --  Untagged case

      else
         Untagged_Case : declare
            Temp_Expr : Node_Id;

         begin
            Aux_Decl := Empty;

            Temp_Expr := Relocate_Node (Pref);

            --  For Etype (Temp_Expr) in some cases we cannot use either
            --  Etype (Pref) or Base_Typ. So we set Etype (Temp_Expr) to null
            --  and mark Temp_Expr as requiring analysis. Rather than trying
            --  to sort out exactly when this is needed, we do it
            --  unconditionally.
            --  One case where this is needed is when
            --     1) Pref is an N_Selected_Component name that
            --        refers to a component which is subject to a
            --        discriminant-dependent constraint; and
            --     2) The prefix of that N_Selected_Component refers to a
            --        formal parameter with an unconstrained subtype; and
            --     3) Pref has only been preanalyzed (so that
            --        Build_Actual_Subtype_Of_Component has not been called
            --        and Etype (Pref) equals the Etype of the component).

            Set_Etype (Temp_Expr, Empty);
            Set_Analyzed (Temp_Expr, False);

            --  Generate:
            --    Temp : constant Base_Typ := Pref;

            Temp_Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier => Temp_Id,
                Constant_Present    => True,
                Object_Definition   => New_Occurrence_Of (Base_Typ, Loc),
                Expression          => Temp_Expr);
            Append_To (Decls, Temp_Decl);
         end Untagged_Case;
      end if;

      --  Step 4: Analyze all bits

      Installed := Current_Scope = Scope (Loop_Id);

      --  Depending on the pracement of attribute 'Loop_Entry relative to the
      --  associated loop, ensure the proper visibility for analysis.

      if not Installed then
         Push_Scope (Scope (Loop_Id));
      end if;

      --  Analyze constant declaration with simple value propagation disabled,
      --  because the values at the loop entry might be different than the
      --  values at the occurrence of Loop_Entry attribute.

      declare
         Save_Debug_Flag_MM : constant Boolean := Debug_Flag_MM;
      begin
         Debug_Flag_MM := True;

         if Present (Aux_Decl) then
            Analyze (Aux_Decl);
         end if;

         Analyze (Temp_Decl);

         Debug_Flag_MM := Save_Debug_Flag_MM;
      end;

      --  If the conditional block has just been created, then analyze it;
      --  otherwise it was analyzed when a previous 'Loop_Entry was expanded.

      if Present (Result) then
         Rewrite (Loop_Stmt, Result);
         Analyze (Loop_Stmt);
      end if;

      Rewrite (N, New_Occurrence_Of (Temp_Id, Loc));
      Analyze (N);

      if not Installed then
         Pop_Scope;
      end if;
   end Expand_Loop_Entry_Attribute;

   ------------------------------
   -- Expand_Min_Max_Attribute --
   ------------------------------

   procedure Expand_Min_Max_Attribute (N : Node_Id) is
   begin
      --  Min and Max are handled by the back end (except that static cases
      --  have already been evaluated during semantic processing, although the
      --  back end should not count on this). The one bit of special processing
      --  required in the normal case is that these two attributes typically
      --  generate conditionals in the code, so check the relevant restriction.

      Check_Restriction (No_Implicit_Conditionals, N);
   end Expand_Min_Max_Attribute;

   ----------------------------------
   -- Expand_N_Attribute_Reference --
   ----------------------------------

   procedure Expand_N_Attribute_Reference (N : Node_Id) is
      Loc   : constant Source_Ptr := Sloc (N);
      Pref  : constant Node_Id    := Prefix (N);
      Exprs : constant List_Id    := Expressions (N);

      generic
         with procedure Build_Type_Attr_Subprogram
                (Typ  : Entity_Id;
                 Decl : out Node_Id;
                 Subp : out Entity_Id);
      procedure Build_And_Insert_Type_Attr_Subp
                  (Typ      : Entity_Id;
                   Decl     : out Node_Id;
                   Subp     : out Entity_Id;
                   Attr_Ref : Node_Id);

      --  If we have two calls to (for example)
      --  Some_Untagged_Record_Type'Put_Image, we'd like
      --  to generate just one procedure and call it twice (as opposed to
      --  generating two effectively-identical procedures). Hoisting the
      --  declaration of the procedure ensures that a second such attribute
      --  reference in the current library unit will not need to generate a
      --  second procedure.

      function Get_Integer_Type (Typ : Entity_Id) return Entity_Id;
      --  Return a small integer type appropriate for the enumeration type

      procedure Rewrite_Attribute_Proc_Call (Pname : Entity_Id);
      --  Rewrites an attribute for Read, Write, Output, or Put_Image with a
      --  call to the appropriate TSS procedure. Pname is the entity for the
      --  procedure to call.

      procedure Read_Controlling_Tag
        (P_Type : Entity_Id; Cntrl : out Node_Id);
      --  Read the external tag from the stream and use it to construct the
      --  controlling operand for a dispatching call.

      procedure Write_Controlling_Tag (P_Type : Entity_Id);
      --  Write the external tag of the given attribute prefix type to
      --  the stream. Also perform the accompanying accessibility check.

      -------------------------------------
      -- Build_And_Insert_Type_Attr_Subp --
      -------------------------------------

      procedure Build_And_Insert_Type_Attr_Subp
        (Typ      : Entity_Id;
         Decl     : out Node_Id;
         Subp     : out Entity_Id;
         Attr_Ref : Node_Id)
      is
         procedure Build;
         procedure Build is
         begin
            Build_Type_Attr_Subprogram
              (Typ  => Typ,
               Decl => Decl,
               Subp => Subp);
         end Build;

         Ancestor        : Node_Id := Attr_Ref;
         Insertion_Scope : Entity_Id := Empty;
         Insertion_Point : Node_Id := Empty;
         Insert_Before   : Boolean := False;
         Typ_Comp_Unit   : Node_Id := Enclosing_Comp_Unit_Node (Typ);
      begin
         --  handle no-enclosing-comp-unit cases
         if No (Typ_Comp_Unit) then
            if Is_Itype (Typ) then
               Typ_Comp_Unit := Enclosing_Comp_Unit_Node
                                  (Associated_Node_For_Itype (Typ));
            elsif Sloc (Typ) <= Standard_Location then
               Typ_Comp_Unit := Typ; -- not a comp unit node, but that's ok
            end if;
            pragma Assert (Present (Typ_Comp_Unit));
         end if;

         if Interunit_Ref_OK (Typ_Comp_Unit,
                              Enclosing_Comp_Unit_Node (Attr_Ref))
            --  See comment accompanying earlier call to Interunit_Ref_OK
            --  for discussion of these Enclosing_Comp_Unit_Node calls.
         then
            --  Typ is declared in the current unit, so
            --  we want to hoist to the same scope as Typ.

            Insertion_Scope := Scope (Typ);
            Insertion_Point := Freeze_Node (Typ);
         else
            --  Typ is declared in a different unit, so
            --  hoist to library level.

            pragma Assert (Is_Library_Level_Entity (Typ));

            while Present (Ancestor) loop
               if Is_List_Member (Ancestor) then
                  Insertion_Point := First (List_Containing (Ancestor));

                  --  A hazard to avoid here is use-before-definition
                  --  errors that can result when we have two of these
                  --  subprograms where one calls the other (e.g., given
                  --  Put_Image procedures for a composite type and
                  --  for a component type, the former will often call
                  --  the latter). At the time a subprogram is inserted,
                  --  we know that the one and only call to it is
                  --  somewhere in the subtree rooted at Ancestor.
                  --  So that placement constraint is easy to satisfy.
                  --  But if we construct another subprogram later and
                  --  if that second subprogram calls the first one,
                  --  then we need to be careful not to place the
                  --  second one ahead of the first one. That is the goal
                  --  of this loop. This may need to be revised if it turns
                  --  out that other stuff is being inserted on the list,
                  --  so that the loop terminates too early.

                  --  On the other hand, it seems like inserting things
                  --  earlier offers more opportunities for sharing.
                  --  If Ancestor occurs in the statement list of a
                  --  subprogram body (ignore the HSS node for now),
                  --  then perhaps we should look for an insertion site
                  --  in the decl list of the subprogram body and only
                  --  look in the statement list if the decl list is empty.
                  --  Similarly if Ancestor occors in the private decls list
                  --  for a package spec that has a non-empty visible
                  --  decls list. No examples where this would result in more
                  --  sharing and less duplication have been observed, so this
                  --  is just speculation.

                  while Insertion_Point /= Ancestor
                    and then Nkind (Insertion_Point) = N_Subprogram_Body
                    and then not Comes_From_Source (Insertion_Point)
                  loop
                     Next (Insertion_Point);
                  end loop;

                  pragma Assert (Present (Insertion_Point));
               end if;
               Ancestor := Parent (Ancestor);
            end loop;

            if Present (Insertion_Point) then
               Insert_Before := True;
               Insertion_Scope :=
                 Find_Enclosing_Scope (Insertion_Point);
            end if;
         end if;

         if Present (Insertion_Point)
           and Present (Insertion_Scope)
         then
            Push_Scope (Insertion_Scope);
            Build;
            if Insert_Before then
               Insert_Action
                 (Insertion_Point, Ins_Action => Decl);
            else
               Insert_Action_After
                 (Insertion_Point, Ins_Action => Decl);
            end if;
            Pop_Scope;
         else
            --  Hoisting was unsuccessful, so no need to
            --  Push/Pop a scope.

            Build;
            Insert_Action (Attr_Ref, Ins_Action => Decl);
         end if;
      end Build_And_Insert_Type_Attr_Subp;

      ----------------------
      -- Get_Integer_Type --
      ----------------------

      function Get_Integer_Type (Typ : Entity_Id) return Entity_Id is
         Siz : constant Uint := Esize (Base_Type (Typ));

      begin
         --  We need to accommodate invalid values of the base type since we
         --  accept them for Enum_Rep and Pos, so we reason on the Esize.

         return Small_Integer_Type_For (Siz, Uns => Is_Unsigned_Type (Typ));
      end Get_Integer_Type;

      ---------------------------------
      -- Rewrite_Attribute_Proc_Call --
      ---------------------------------

      procedure Rewrite_Attribute_Proc_Call (Pname : Entity_Id) is
         Item       : constant Node_Id   := Next (First (Exprs));
         Item_Typ   : constant Entity_Id := Etype (Item);
         Formal     : constant Entity_Id := Next_Formal (First_Formal (Pname));
         Formal_Typ : constant Entity_Id := Etype (Formal);
         Is_Written : constant Boolean   := Ekind (Formal) /= E_In_Parameter;

      begin
         --  The expansion depends on Item, the second actual, which is
         --  the object being streamed in or out.

         --  If the item is a component of a packed array type, and
         --  a conversion is needed on exit, we introduce a temporary to
         --  hold the value, because otherwise the packed reference will
         --  not be properly expanded.

         if Nkind (Item) = N_Indexed_Component
           and then Is_Packed (Base_Type (Etype (Prefix (Item))))
           and then Base_Type (Item_Typ) /= Base_Type (Formal_Typ)
           and then Is_Written
         then
            declare
               Temp : constant Entity_Id := Make_Temporary (Loc, 'V');
               Decl : Node_Id;
               Assn : Node_Id;

            begin
               Decl :=
                 Make_Object_Declaration (Loc,
                   Defining_Identifier => Temp,
                   Object_Definition   => New_Occurrence_Of (Formal_Typ, Loc));
               Set_Etype (Temp, Formal_Typ);

               Assn :=
                 Make_Assignment_Statement (Loc,
                   Name       => New_Copy_Tree (Item),
                   Expression =>
                     Unchecked_Convert_To
                       (Item_Typ, New_Occurrence_Of (Temp, Loc)));

               Rewrite (Item, New_Occurrence_Of (Temp, Loc));
               Insert_Actions (N,
                 New_List (
                   Decl,
                   Make_Procedure_Call_Statement (Loc,
                     Name                   => New_Occurrence_Of (Pname, Loc),
                     Parameter_Associations => Exprs),
                   Assn));

               Rewrite (N, Make_Null_Statement (Loc));
               return;
            end;
         end if;

         --  For the class-wide dispatching cases, and for cases in which
         --  the base type of the second argument matches the base type of
         --  the corresponding formal parameter (that is to say the stream
         --  operation is not inherited), we are all set, and can use the
         --  argument unchanged.

         if not Is_Class_Wide_Type (Entity (Pref))
           and then not Is_Class_Wide_Type (Etype (Item))
           and then Base_Type (Item_Typ) /= Base_Type (Formal_Typ)
         then
            --  Perform an unchecked conversion when either the argument or
            --  the formal parameter are of a private type.

            if (Is_Private_Type (Base_Type (Formal_Typ))
                or else Is_Private_Type (Base_Type (Item_Typ)))
              and then (Is_By_Reference_Type (Formal_Typ) or else
                        not Is_Written)
            then
               Rewrite (Item,
                 Unchecked_Convert_To (Formal_Typ, Relocate_Node (Item)));

            --  Otherwise perform a regular type conversion to ensure that all
            --  relevant checks are installed.

            else
               Rewrite (Item, Convert_To (Formal_Typ, Relocate_Node (Item)));
            end if;

            --  For untagged derived types set Assignment_OK, to prevent
            --  copies from being created when the unchecked conversion
            --  is expanded (which would happen in Remove_Side_Effects
            --  if Expand_N_Unchecked_Conversion were allowed to call
            --  Force_Evaluation). The copy could violate Ada semantics in
            --  cases such as an actual that is an out parameter. Note that
            --  this approach is also used in exp_ch7 for calls to controlled
            --  type operations to prevent problems with actuals wrapped in
            --  unchecked conversions.

            if Is_Untagged_Derivation (Etype (Expression (Item))) then
               Set_Assignment_OK (Item);
            end if;
         end if;

         --  The stream operation to call might be a renaming created by an
         --  attribute definition clause, and might not be frozen yet. Ensure
         --  that it has the necessary extra formals.

         if not Is_Frozen (Pname) then
            Create_Extra_Formals (Pname);
         end if;

         --  And now rewrite the call

         Rewrite (N,
           Make_Procedure_Call_Statement (Loc,
             Name                   => New_Occurrence_Of (Pname, Loc),
             Parameter_Associations => Exprs));

         Analyze (N);
      end Rewrite_Attribute_Proc_Call;

      --------------------------
      -- Read_Controlling_Tag --
      --------------------------

      procedure Read_Controlling_Tag
        (P_Type : Entity_Id; Cntrl : out Node_Id)
      is
         Strm    : constant Node_Id := First (Exprs);
         Expr    : Node_Id; -- call to Descendant_Tag
         Get_Tag : Node_Id; -- expression to read the 'Tag

      begin
         --  Read the internal tag (RM 13.13.2(34)) and use it to
         --  initialize a dummy tag value. We used to unconditionally
         --  generate:
         --
         --     Descendant_Tag (String'Input (Strm), P_Type);
         --
         --  which turns into a call to String_Input_Blk_IO. However,
         --  if the input is malformed, that could try to read an
         --  enormous String, causing chaos. So instead we call
         --  String_Input_Tag, which does the same thing as
         --  String_Input_Blk_IO, except that if the String is
         --  absurdly long, it raises an exception.
         --
         --  However, if the No_Stream_Optimizations restriction
         --  is active, we disable this unnecessary attempt at
         --  robustness; we really need to read the string
         --  character-by-character.
         --
         --  This value is used only to provide a controlling
         --  argument for the eventual _Input call. Descendant_Tag is
         --  called rather than Internal_Tag to ensure that we have a
         --  tag for a type that is descended from the prefix type and
         --  declared at the same accessibility level (the exception
         --  Tag_Error will be raised otherwise). The level check is
         --  required for Ada 2005 because tagged types can be
         --  extended in nested scopes (AI-344).

         --  Note: we used to generate an explicit declaration of a
         --  constant Ada.Tags.Tag object, and use an occurrence of
         --  this constant in Cntrl, but this caused a secondary stack
         --  leak.

         if Restriction_Active (No_Stream_Optimizations) then
            Get_Tag :=
              Make_Attribute_Reference (Loc,
                Prefix         =>
                  New_Occurrence_Of (Standard_String, Loc),
                Attribute_Name => Name_Input,
                Expressions    => New_List (
                  Relocate_Node (Duplicate_Subexpr (Strm))));
         else
            Get_Tag :=
              Make_Function_Call (Loc,
                Name                   =>
                  New_Occurrence_Of
                    (RTE (RE_String_Input_Tag), Loc),
                Parameter_Associations => New_List (
                  Relocate_Node (Duplicate_Subexpr (Strm))));
         end if;

         Expr :=
           Make_Function_Call (Loc,
             Name                   =>
               New_Occurrence_Of (RTE (RE_Descendant_Tag), Loc),
             Parameter_Associations => New_List (
               Get_Tag,
               Make_Attribute_Reference (Loc,
                 Prefix         => New_Occurrence_Of (P_Type, Loc),
                 Attribute_Name => Name_Tag)));

         Set_Etype (Expr, RTE (RE_Tag));

         --  Construct a controlling operand for a dispatching call.

         Cntrl := Unchecked_Convert_To (P_Type, Expr);
         Set_Etype (Cntrl, P_Type);
         Set_Parent (Cntrl, N);
      end Read_Controlling_Tag;

      ----------------------------
      --  Write_Controlling_Tag --
      ----------------------------

      procedure Write_Controlling_Tag (P_Type : Entity_Id) is
         Strm : constant Node_Id := First (Exprs);
         Item : constant Node_Id := Next (Strm);
      begin
         --  Ada 2005 (AI-344): Check that the accessibility level
         --  of the type of the output object is not deeper than
         --  that of the attribute's prefix type.

         --  if Get_Access_Level (Item'Tag)
         --       /= Get_Access_Level (P_Type'Tag)
         --  then
         --     raise Tag_Error;
         --  end if;

         --  String'Output (Strm, External_Tag (Item'Tag));

         --  We cannot figure out a practical way to implement this
         --  accessibility check on virtual machines, so we omit it.

         if Ada_Version >= Ada_2005
           and then Tagged_Type_Expansion
         then
            Insert_Action (N,
              Make_Implicit_If_Statement (N,
                Condition =>
                  Make_Op_Ne (Loc,
                    Left_Opnd  =>
                      Build_Get_Access_Level (Loc,
                        Make_Attribute_Reference (Loc,
                          Prefix         =>
                            Relocate_Node (
                              Duplicate_Subexpr (Item,
                                Name_Req => True)),
                          Attribute_Name => Name_Tag)),

                    Right_Opnd =>
                      Make_Integer_Literal (Loc,
                        Type_Access_Level (P_Type))),

                Then_Statements =>
                  New_List (Make_Raise_Statement (Loc,
                              New_Occurrence_Of (
                                RTE (RE_Tag_Error), Loc)))));
         end if;

         Insert_Action (N,
           Make_Attribute_Reference (Loc,
             Prefix => New_Occurrence_Of (Standard_String, Loc),
             Attribute_Name => Name_Output,
             Expressions => New_List (
               Relocate_Node (Duplicate_Subexpr (Strm)),
               Make_Function_Call (Loc,
                 Name =>
                   New_Occurrence_Of (RTE (RE_External_Tag), Loc),
                 Parameter_Associations => New_List (
                  Make_Attribute_Reference (Loc,
                    Prefix =>
                      Relocate_Node
                        (Duplicate_Subexpr (Item, Name_Req => True)),
                    Attribute_Name => Name_Tag))))));
      end Write_Controlling_Tag;

      Typ  : constant Entity_Id    := Etype (N);
      Btyp : constant Entity_Id    := Base_Type (Typ);
      Ptyp : constant Entity_Id    := Etype (Pref);
      Id   : constant Attribute_Id := Get_Attribute_Id (Attribute_Name (N));

   --  Start of processing for Expand_N_Attribute_Reference

   begin
      --  Do required validity checking, if enabled.
      --
      --  Skip check for output parameters of an Asm instruction (since their
      --  valuesare not set till after the attribute has been elaborated),
      --  for the arguments of a 'Read attribute reference (since the
      --  scalar argument is an OUT scalar) and for the arguments of a
      --  'Has_Same_Storage or 'Overlaps_Storage attribute reference (which not
      --  considered to be reads of their prefixes and expressions, see
      --  RM 13.3(73.10/3)).

      if Validity_Checks_On and then Validity_Check_Operands
        and then Id /= Attribute_Asm_Output
        and then Id /= Attribute_Read
        and then Id /= Attribute_Has_Same_Storage
        and then Id /= Attribute_Overlaps_Storage
      then
         declare
            Expr : Node_Id;
         begin
            Expr := First (Expressions (N));
            while Present (Expr) loop
               Ensure_Valid (Expr);
               Next (Expr);
            end loop;
         end;
      end if;

      --  Ada 2005 (AI-318-02): If attribute prefix is a call to a build-in-
      --  place function, then a temporary return object needs to be created
      --  and access to it must be passed to the function.

      if Is_Build_In_Place_Function_Call (Pref) then

         --  If attribute is 'Old, the context is a postcondition, and
         --  the temporary must go in the corresponding subprogram, not
         --  the postcondition function or any created blocks, as when
         --  the attribute appears in a quantified expression. This is
         --  handled below in the expansion of the attribute.

         if Attribute_Name (Parent (Pref)) = Name_Old then
            null;
         else
            Make_Build_In_Place_Call_In_Anonymous_Context (Pref);
         end if;

      --  Ada 2005 (AI-318-02): Specialization of the previous case for prefix
      --  containing build-in-place function calls whose returned object covers
      --  interface types.

      elsif Present (Unqual_BIP_Iface_Function_Call (Pref)) then
         Make_Build_In_Place_Iface_Call_In_Anonymous_Context (Pref);
      end if;

      --  If prefix is a protected type name, this is a reference to the
      --  current instance of the type. For a component definition, nothing
      --  to do (expansion will occur in the init proc). In other contexts,
      --  rewrite into reference to current instance.

      if Is_Protected_Self_Reference (Pref)
        and then not
          (Nkind (Parent (N)) in N_Index_Or_Discriminant_Constraint
                               | N_Discriminant_Association
            and then Nkind (Parent (Parent (Parent (Parent (N))))) =
                                                      N_Component_Definition)

         --  No action needed for these attributes since the current instance
         --  will be rewritten to be the name of the _object parameter
         --  associated with the enclosing protected subprogram (see below).

        and then Id /= Attribute_Access
        and then Id /= Attribute_Unchecked_Access
        and then Id /= Attribute_Unrestricted_Access
      then
         Rewrite (Pref, Concurrent_Ref (Pref));
         Analyze (Pref);
      end if;

      --  Remaining processing depends on specific attribute

      --  Note: individual sections of the following case statement are
      --  allowed to assume there is no code after the case statement, and
      --  are legitimately allowed to execute return statements if they have
      --  nothing more to do.

      case Id is

      --  Internal attributes used to deal with Ada 2012 delayed aspects. These
      --  were already rejected by the parser. Thus they shouldn't appear here.

      when Internal_Attribute_Id =>
         raise Program_Error;

      ------------
      -- Access --
      ------------

      when Attribute_Access
         | Attribute_Unchecked_Access
         | Attribute_Unrestricted_Access
      =>
         Access_Cases : declare
            Ref_Object : constant Node_Id := Get_Referenced_Object (Pref);
            Btyp_DDT   : Entity_Id;

            procedure Add_Implicit_Interface_Type_Conversion;
            --  Ada 2005 (AI-251): The designated type is an interface type;
            --  add an implicit type conversion to force the displacement of
            --  the pointer to reference the secondary dispatch table.

            function Enclosing_Object (N : Node_Id) return Node_Id;
            --  If N denotes a compound name (selected component, indexed
            --  component, or slice), returns the name of the outermost such
            --  enclosing object. Otherwise returns N. If the object is a
            --  renaming, then the renamed object is returned.

            --------------------------------------------
            -- Add_Implicit_Interface_Type_Conversion --
            --------------------------------------------

            procedure Add_Implicit_Interface_Type_Conversion is
            begin
               pragma Assert (Is_Interface (Btyp_DDT));

               --  Handle cases were no action is required.

               if not Comes_From_Source (N)
                 and then not Comes_From_Source (Ref_Object)
                 and then (Nkind (Ref_Object) not in N_Has_Chars
                             or else Chars (Ref_Object) /= Name_uInit)
               then
                  return;
               end if;

               --  Common case

               if Nkind (Ref_Object) /= N_Explicit_Dereference then

                  --  No implicit conversion required if types match, or if
                  --  the prefix is the class_wide_type of the interface. In
                  --  either case passing an object of the interface type has
                  --  already set the pointer correctly.

                  if Btyp_DDT = Etype (Ref_Object)
                    or else
                      (Is_Class_Wide_Type (Etype (Ref_Object))
                         and then
                       Class_Wide_Type (Btyp_DDT) = Etype (Ref_Object))
                  then
                     null;

                  else
                     Rewrite (Prefix (N),
                       Convert_To (Btyp_DDT,
                         New_Copy_Tree (Prefix (N))));

                     Analyze_And_Resolve (Prefix (N), Btyp_DDT);
                  end if;

               --  When the object is an explicit dereference, convert the
               --  dereference's prefix.

               else
                  declare
                     Obj_DDT : constant Entity_Id :=
                                 Base_Type
                                   (Directly_Designated_Type
                                     (Etype (Prefix (Ref_Object))));
                  begin
                     --  No implicit conversion required if designated types
                     --  match.

                     if Obj_DDT /= Btyp_DDT
                       and then not (Is_Class_Wide_Type (Obj_DDT)
                                      and then Etype (Obj_DDT) = Btyp_DDT)
                     then
                        Rewrite (N,
                          Convert_To (Typ,
                            New_Copy_Tree (Prefix (Ref_Object))));
                        Analyze_And_Resolve (N, Typ);
                     end if;
                  end;
               end if;
            end Add_Implicit_Interface_Type_Conversion;

            ----------------------
            -- Enclosing_Object --
            ----------------------

            function Enclosing_Object (N : Node_Id) return Node_Id is
               Obj_Name : Node_Id;

            begin
               Obj_Name := N;
               while Nkind (Obj_Name) in N_Selected_Component
                                       | N_Indexed_Component
                                       | N_Slice
               loop
                  Obj_Name := Prefix (Obj_Name);
               end loop;

               return Get_Referenced_Object (Obj_Name);
            end Enclosing_Object;

            --  Local declarations

            Enc_Object : Node_Id := Enclosing_Object (Ref_Object);

         --  Start of processing for Access_Cases

         begin
            Btyp_DDT := Designated_Type (Btyp);

            --  When Enc_Object is a view conversion then RM 3.10.2 (9)
            --  applies and we obtain the expression being converted.
            --  Otherwise we do not dig any deeper since a conversion
            --  might generate a copy and we can't assume it will be as
            --  long-lived as the original.

            while Nkind (Enc_Object) = N_Type_Conversion
              and then Is_View_Conversion (Enc_Object)
            loop
               Enc_Object := Expression (Enc_Object);
            end loop;

            --  Handle designated types that come from the limited view

            if From_Limited_With (Btyp_DDT)
              and then Has_Non_Limited_View (Btyp_DDT)
            then
               Btyp_DDT := Non_Limited_View (Btyp_DDT);
            end if;

            --  In order to improve the text of error messages, the designated
            --  type of access-to-subprogram itypes is set by the semantics as
            --  the associated subprogram entity (see sem_attr). Now we replace
            --  such node with the proper E_Subprogram_Type itype.

            if Id = Attribute_Unrestricted_Access
              and then Is_Subprogram (Directly_Designated_Type (Typ))
            then
               --  The following conditions ensure that this special management
               --  is done only for "Address!(Prim'Unrestricted_Access)" nodes.
               --  At this stage other cases in which the designated type is
               --  still a subprogram (instead of an E_Subprogram_Type) are
               --  wrong because the semantics must have overridden the type of
               --  the node with the type imposed by the context.

               if Nkind (Parent (N)) = N_Unchecked_Type_Conversion
                 and then Is_RTE (Etype (Parent (N)), RE_Prim_Ptr)
               then
                  Set_Etype (N, RTE (RE_Prim_Ptr));

               else
                  declare
                     Subp       : constant Entity_Id :=
                                    Directly_Designated_Type (Typ);
                     Etyp       : Entity_Id;
                     Extra      : Entity_Id := Empty;
                     New_Formal : Entity_Id;
                     Old_Formal : Entity_Id := First_Formal (Subp);
                     Subp_Typ   : Entity_Id;

                  begin
                     Subp_Typ := Create_Itype (E_Subprogram_Type, N);
                     Copy_Strub_Mode (Subp_Typ, Subp);
                     Set_Etype (Subp_Typ, Etype (Subp));
                     Set_Returns_By_Ref (Subp_Typ, Returns_By_Ref (Subp));

                     if Present (Old_Formal) then
                        New_Formal := New_Copy (Old_Formal);
                        Set_First_Entity (Subp_Typ, New_Formal);

                        loop
                           Set_Scope (New_Formal, Subp_Typ);
                           Etyp := Etype (New_Formal);

                           --  Handle itypes. There is no need to duplicate
                           --  here the itypes associated with record types
                           --  (i.e the implicit full view of private types).

                           if Is_Itype (Etyp)
                             and then Ekind (Base_Type (Etyp)) /= E_Record_Type
                           then
                              Extra := New_Copy (Etyp);
                              Set_Parent (Extra, New_Formal);
                              Set_Etype (New_Formal, Extra);
                              Set_Scope (Extra, Subp_Typ);
                           end if;

                           Extra := New_Formal;
                           Next_Formal (Old_Formal);
                           exit when No (Old_Formal);

                           Link_Entities (New_Formal, New_Copy (Old_Formal));
                           Next_Entity   (New_Formal);
                        end loop;

                        Unlink_Next_Entity (New_Formal);
                        Set_Last_Entity (Subp_Typ, Extra);
                     end if;

                     --  Now that the explicit formals have been duplicated,
                     --  any extra formals needed by the subprogram must be
                     --  created.

                     if Present (Extra) then
                        Set_Extra_Formal (Extra, Empty);
                     end if;

                     Create_Extra_Formals (Subp_Typ);
                     Set_Directly_Designated_Type (Typ, Subp_Typ);
                  end;
               end if;
            end if;

            if Is_Access_Protected_Subprogram_Type (Btyp) then
               Expand_Access_To_Protected_Op (N, Pref, Typ);

            elsif Is_Access_Subprogram_Type (Btyp)
              and then Is_Entity_Name (Pref)
            then
               --  If prefix is a subprogram that has class-wide preconditions
               --  and an indirect-call wrapper (ICW) of the subprogram is
               --  available then replace the prefix by the ICW.

               if Present (Class_Preconditions (Entity (Pref)))
                 and then Present (Indirect_Call_Wrapper (Entity (Pref)))
               then
                  Rewrite (Pref,
                    New_Occurrence_Of
                      (Indirect_Call_Wrapper (Entity (Pref)), Loc));
                  Analyze_And_Resolve (N, Typ);
               end if;

               --  Ensure the availability of the extra formals to check that
               --  they match.

               if not Is_Frozen (Entity (Pref))
                 or else From_Limited_With (Etype (Entity (Pref)))
               then
                  Create_Extra_Formals (Entity (Pref));
               end if;

               if not Is_Frozen (Btyp_DDT)
                 or else From_Limited_With (Etype (Btyp_DDT))
               then
                  Create_Extra_Formals (Btyp_DDT);
               end if;

               pragma Assert
                 (Extra_Formals_Match_OK
                   (E => Entity (Pref), Ref_E => Btyp_DDT));

            --  If prefix is a type name, this is a reference to the current
            --  instance of the type, within its initialization procedure.

            elsif Is_Entity_Name (Pref)
              and then Is_Type (Entity (Pref))
            then
               declare
                  Par    : Node_Id;
                  Formal : Entity_Id;

               begin
                  --  If the current instance name denotes a task type, then
                  --  the access attribute is rewritten to be the name of the
                  --  "_task" parameter associated with the task type's task
                  --  procedure. An unchecked conversion is applied to ensure
                  --  a type match in cases of expander-generated calls (e.g.
                  --  init procs).

                  if Is_Task_Type (Entity (Pref)) then
                     Formal :=
                       First_Entity (Get_Task_Body_Procedure (Entity (Pref)));
                     while Present (Formal) loop
                        exit when Chars (Formal) = Name_uTask;
                        Next_Entity (Formal);
                     end loop;

                     pragma Assert (Present (Formal));

                     Rewrite (N,
                       Unchecked_Convert_To (Typ,
                         New_Occurrence_Of (Formal, Loc)));
                     Set_Etype (N, Typ);

                  elsif Is_Protected_Type (Entity (Pref)) then

                     --  No action needed for current instance located in a
                     --  component definition (expansion will occur in the
                     --  init proc)

                     if Is_Protected_Type (Current_Scope) then
                        null;

                     --  If the current instance reference is located in a
                     --  protected subprogram or entry then rewrite the access
                     --  attribute to be the name of the "_object" parameter.
                     --  An unchecked conversion is applied to ensure a type
                     --  match in cases of expander-generated calls (e.g. init
                     --  procs).

                     --  The code may be nested in a block, so find enclosing
                     --  scope that is a protected operation.

                     else
                        declare
                           Subp : Entity_Id;

                        begin
                           Subp := Current_Scope;
                           while Ekind (Subp) in E_Loop | E_Block loop
                              Subp := Scope (Subp);
                           end loop;

                           Formal :=
                             First_Entity
                               (Protected_Body_Subprogram (Subp));

                           --  For a protected subprogram the _Object parameter
                           --  is the protected record, so we create an access
                           --  to it. The _Object parameter of an entry is an
                           --  address.

                           if Ekind (Subp) = E_Entry then
                              Rewrite (N,
                                Unchecked_Convert_To (Typ,
                                  New_Occurrence_Of (Formal, Loc)));
                              Set_Etype (N, Typ);

                           else
                              Rewrite (N,
                                Unchecked_Convert_To (Typ,
                                  Make_Attribute_Reference (Loc,
                                    Attribute_Name => Name_Unrestricted_Access,
                                    Prefix         =>
                                      New_Occurrence_Of (Formal, Loc))));
                              Analyze_And_Resolve (N);
                           end if;
                        end;
                     end if;

                  --  The expression must appear in a default expression,
                  --  (which in the initialization procedure is the right-hand
                  --  side of an assignment), and not in a discriminant
                  --  constraint.

                  else
                     Par := Parent (N);
                     while Present (Par) loop
                        exit when Nkind (Par) = N_Assignment_Statement;

                        if Nkind (Par) = N_Component_Declaration then
                           return;
                        end if;

                        Par := Parent (Par);
                     end loop;

                     if Present (Par) then
                        Rewrite (N,
                          Make_Attribute_Reference (Loc,
                            Prefix => Make_Identifier (Loc, Name_uInit),
                            Attribute_Name  => Attribute_Name (N)));

                        Analyze_And_Resolve (N, Typ);
                     end if;
                  end if;
               end;

            --  If the prefix of an Access attribute is a dereference of an
            --  access parameter (or a renaming of such a dereference, or a
            --  subcomponent of such a dereference) and the context is a
            --  general access type (including the type of an object or
            --  component with an access_definition, but not the anonymous
            --  type of an access parameter or access discriminant), then
            --  apply an accessibility check to the access parameter. We used
            --  to rewrite the access parameter as a type conversion, but that
            --  could only be done if the immediate prefix of the Access
            --  attribute was the dereference, and didn't handle cases where
            --  the attribute is applied to a subcomponent of the dereference,
            --  since there's generally no available, appropriate access type
            --  to convert to in that case. The attribute is passed as the
            --  point to insert the check, because the access parameter may
            --  come from a renaming, possibly in a different scope, and the
            --  check must be associated with the attribute itself.

            elsif Id = Attribute_Access
              and then Nkind (Enc_Object) = N_Explicit_Dereference
              and then Is_Entity_Name (Prefix (Enc_Object))
              and then (Ekind (Btyp) = E_General_Access_Type
                         or else Is_Local_Anonymous_Access (Btyp))
              and then Is_Formal (Entity (Prefix (Enc_Object)))
              and then Ekind (Etype (Entity (Prefix (Enc_Object))))
                         = E_Anonymous_Access_Type
              and then Present (Extra_Accessibility
                                (Entity (Prefix (Enc_Object))))
              and then not No_Dynamic_Accessibility_Checks_Enabled (Enc_Object)
            then
               Apply_Accessibility_Check (Prefix (Enc_Object), Typ, N);

               --  Ada 2005 (AI-251): If the designated type is an interface we
               --  add an implicit conversion to force the displacement of the
               --  pointer to reference the secondary dispatch table.

               if Is_Interface (Btyp_DDT) then
                  Add_Implicit_Interface_Type_Conversion;
               end if;

            --  Ada 2005 (AI-251): If the designated type is an interface we
            --  add an implicit conversion to force the displacement of the
            --  pointer to reference the secondary dispatch table.

            elsif Is_Interface (Btyp_DDT) then
               Add_Implicit_Interface_Type_Conversion;
            end if;
         end Access_Cases;

      --------------
      -- Adjacent --
      --------------

      --  Transforms 'Adjacent into a call to the floating-point attribute
      --  function Adjacent in Fat_xxx (where xxx is the root type)

      when Attribute_Adjacent =>
         Expand_Fpt_Attribute_RR (N);

      -------------
      -- Address --
      -------------

      when Attribute_Address => Address : declare
         Task_Proc : Entity_Id;

         function Is_Unnested_Component_Init (N : Node_Id) return Boolean;
         --  Returns True if N is being used to initialize a component of
         --  an activation record object where the component corresponds to
         --  the object denoted by the prefix of the attribute N.

         function Is_Unnested_Component_Init (N : Node_Id) return Boolean is
         begin
            return Present (Parent (N))
              and then Nkind (Parent (N)) = N_Assignment_Statement
              and then Is_Entity_Name (Pref)
              and then Present (Activation_Record_Component (Entity (Pref)))
              and then Nkind (Name (Parent (N))) = N_Selected_Component
              and then Entity (Selector_Name (Name (Parent (N)))) =
                         Activation_Record_Component (Entity (Pref));
         end Is_Unnested_Component_Init;

      --  Start of processing for Address

      begin
         --  If the prefix is a task or a task type, the useful address is that
         --  of the procedure for the task body, i.e. the actual program unit.
         --  We replace the original entity with that of the procedure.

         if Is_Entity_Name (Pref)
           and then Is_Task_Type (Entity (Pref))
         then
            Task_Proc := Next_Entity (Root_Type (Ptyp));

            while Present (Task_Proc) loop
               exit when Ekind (Task_Proc) = E_Procedure
                 and then Etype (First_Formal (Task_Proc)) =
                                  Corresponding_Record_Type (Ptyp);
               Next_Entity (Task_Proc);
            end loop;

            if Present (Task_Proc) then
               Set_Entity (Pref, Task_Proc);
               Set_Etype  (Pref, Etype (Task_Proc));
            end if;

         --  Similarly, the address of a protected operation is the address
         --  of the corresponding protected body, regardless of the protected
         --  object from which it is selected.

         elsif Nkind (Pref) = N_Selected_Component
           and then Is_Subprogram (Entity (Selector_Name (Pref)))
           and then Is_Protected_Type (Scope (Entity (Selector_Name (Pref))))
         then
            Rewrite (Pref,
              New_Occurrence_Of (
                External_Subprogram (Entity (Selector_Name (Pref))), Loc));

         elsif Nkind (Pref) = N_Explicit_Dereference
           and then Ekind (Ptyp) = E_Subprogram_Type
           and then Convention (Ptyp) = Convention_Protected
         then
            --  The prefix is be a dereference of an access_to_protected_
            --  subprogram. The desired address is the second component of
            --  the record that represents the access.

            declare
               Addr : constant Entity_Id := Etype (N);
               Ptr  : constant Node_Id   := Prefix (Pref);
               T    : constant Entity_Id :=
                        Equivalent_Type (Base_Type (Etype (Ptr)));

            begin
               Rewrite (N,
                 Unchecked_Convert_To (Addr,
                   Make_Selected_Component (Loc,
                     Prefix => Unchecked_Convert_To (T, Ptr),
                     Selector_Name => New_Occurrence_Of (
                       Next_Entity (First_Entity (T)), Loc))));

               Analyze_And_Resolve (N, Addr);
            end;

         --  'Address is an actual parameter of the call to the implicit
         --  subprogram To_Pointer instantiated with a class-wide interface
         --  type; its expansion requires adding an implicit type conversion
         --  to force displacement of the "this" pointer.

         elsif Tagged_Type_Expansion
           and then Nkind (Parent (N)) = N_Function_Call
           and then Nkind (Name (Parent (N))) in N_Has_Entity
           and then Is_Intrinsic_Subprogram (Entity (Name (Parent (N))))
           and then Chars (Entity (Name (Parent (N)))) = Name_To_Pointer
           and then Is_Interface (Designated_Type (Etype (Parent (N))))
           and then Is_Class_Wide_Type (Designated_Type (Etype (Parent (N))))
         then
            declare
               Iface_Typ : constant Entity_Id :=
                             Designated_Type (Etype (Parent (N)));
            begin
               Rewrite (Pref, Convert_To (Iface_Typ, Relocate_Node (Pref)));
               Analyze_And_Resolve (Pref, Iface_Typ);
               return;
            end;

         --  Ada 2005 (AI-251): Class-wide interface objects are always
         --  "displaced" to reference the tag associated with the interface
         --  type. In order to obtain the real address of such objects we
         --  generate a call to a run-time subprogram that returns the base
         --  address of the object. This call is not generated in cases where
         --  the attribute is being used to initialize a component of an
         --  activation record object where the component corresponds to
         --  prefix of the attribute (for back ends that require "unnesting"
         --  of nested subprograms), since the address needs to be assigned
         --  as-is to such components. Likewise for a return object.

         elsif Tagged_Type_Expansion
           and then Is_Class_Wide_Type (Ptyp)
           and then Is_Interface (Underlying_Type (Ptyp))
           and then not (Is_Entity_Name (Pref)
                          and then (Is_Subprogram (Entity (Pref))
                                     or else Is_Return_Object (Entity (Pref))))
           and then not Is_Unnested_Component_Init (N)
         then
            Rewrite (N,
              Make_Function_Call (Loc,
                Name => New_Occurrence_Of (RTE (RE_Base_Address), Loc),
                Parameter_Associations => New_List (Relocate_Node (N))));
            Analyze (N);
            return;
         end if;

         --  Deal with packed array reference, other cases are handled by
         --  the back end.

         if Involves_Packed_Array_Reference (Pref) then
            Expand_Packed_Address_Reference (N);
         end if;
      end Address;

      ---------------
      -- Alignment --
      ---------------

      when Attribute_Alignment => Alignment : declare
         New_Node : Node_Id;

      begin
         --  For class-wide types, X'Class'Alignment is transformed into a
         --  direct reference to the Alignment of the class type, so that the
         --  back end does not have to deal with the X'Class'Alignment
         --  reference.

         if Is_Entity_Name (Pref)
           and then Is_Class_Wide_Type (Entity (Pref))
         then
            Rewrite (Prefix (N), New_Occurrence_Of (Entity (Pref), Loc));
            return;

         --  For x'Alignment applied to an object of a class wide type,
         --  transform X'Alignment into a call to the predefined primitive
         --  operation _Alignment applied to X.

         elsif Is_Class_Wide_Type (Ptyp) then
            New_Node :=
              Make_Attribute_Reference (Loc,
                Prefix         => Pref,
                Attribute_Name => Name_Tag);

            New_Node := Build_Get_Alignment (Loc, New_Node);

            --  Case where the context is an unchecked conversion to a specific
            --  integer type. We directly convert from the alignment's type.

            if Nkind (Parent (N)) = N_Unchecked_Type_Conversion then
               Rewrite (N, New_Node);
               Analyze_And_Resolve (N);
               return;

            --  Case where the context is a specific integer type with which
            --  the original attribute was compatible. But the alignment has a
            --  specific type in a-tags.ads (Standard.Natural) so, in order to
            --  preserve type compatibility, we must convert explicitly.

            elsif Typ /= Standard_Natural then
               New_Node := Convert_To (Typ, New_Node);
            end if;

            Rewrite (N, New_Node);
            Analyze_And_Resolve (N, Typ);
            return;

         --  For all other cases, we just have to deal with the case of
         --  the fact that the result can be universal.

         else
            Apply_Universal_Integer_Attribute_Checks (N);
         end if;
      end Alignment;

      ---------------------------
      -- Asm_Input, Asm_Output --
      ---------------------------

      --  The Asm_Input and Asm_Output attributes are not expanded at this
      --  stage, but will be eliminated in the expansion of the Asm call,
      --  see Exp_Intr for details. So the back end will never see them.

      when Attribute_Asm_Input
         | Attribute_Asm_Output
      =>
         null;

      ---------
      -- Bit --
      ---------

      --  We compute this if a packed array reference was present, otherwise we
      --  leave the computation up to the back end.

      when Attribute_Bit =>
         if Involves_Packed_Array_Reference (Pref) then
            Expand_Packed_Bit_Reference (N);
         else
            Apply_Universal_Integer_Attribute_Checks (N);
         end if;

      ------------------
      -- Bit_Position --
      ------------------

      --  We leave the computation up to the back end, since we don't know what
      --  layout will be chosen if no component clause was specified.

      when Attribute_Bit_Position =>
         Apply_Universal_Integer_Attribute_Checks (N);

      ------------------
      -- Body_Version --
      ------------------

      --  A reference to P'Body_Version or P'Version is expanded to

      --     Vnn : Unsigned;
      --     pragma Import (C, Vnn, "uuuuT");
      --     ...
      --     Get_Version_String (Vnn)

      --  where uuuu is the unit name (dots replaced by double underscore)
      --  and T is B for the cases of Body_Version, or Version applied to a
      --  subprogram acting as its own spec, and S for Version applied to a
      --  subprogram spec or package. This sequence of code references the
      --  unsigned constant created in the main program by the binder.

      --  A special exception occurs for Standard, where the string returned
      --  is a copy of the library string in gnatvsn.ads.

      when Attribute_Body_Version
         | Attribute_Version
      =>
         Version : declare
            E    : constant Entity_Id := Make_Temporary (Loc, 'V');
            Pent : Entity_Id;
            S    : String_Id;

         begin
            --  If not library unit, get to containing library unit

            Pent := Entity (Pref);
            while Pent /= Standard_Standard
              and then Scope (Pent) /= Standard_Standard
              and then not Is_Child_Unit (Pent)
            loop
               Pent := Scope (Pent);
            end loop;

            --  Special case Standard and Standard.ASCII

            if Pent = Standard_Standard or else Pent = Standard_ASCII then
               Rewrite (N,
                 Make_String_Literal (Loc,
                   Strval => Verbose_Library_Version));

            --  All other cases

            else
               --  Build required string constant

               Get_Name_String (Get_Unit_Name (Pent));

               Start_String;
               for J in 1 .. Name_Len - 2 loop
                  if Name_Buffer (J) = '.' then
                     Store_String_Chars ("__");
                  else
                     Store_String_Char (Get_Char_Code (Name_Buffer (J)));
                  end if;
               end loop;

               --  Case of subprogram acting as its own spec, always use body

               if Nkind (Declaration_Node (Pent)) in N_Subprogram_Specification
                 and then Nkind (Parent (Declaration_Node (Pent))) =
                            N_Subprogram_Body
                 and then Acts_As_Spec (Parent (Declaration_Node (Pent)))
               then
                  Store_String_Chars ("B");

               --  Case of no body present, always use spec

               elsif not Unit_Requires_Body (Pent) then
                  Store_String_Chars ("S");

               --  Otherwise use B for Body_Version, S for spec

               elsif Id = Attribute_Body_Version then
                  Store_String_Chars ("B");
               else
                  Store_String_Chars ("S");
               end if;

               S := End_String;
               Lib.Version_Referenced (S);

               --  Insert the object declaration

               Insert_Actions (N, New_List (
                 Make_Object_Declaration (Loc,
                   Defining_Identifier => E,
                   Object_Definition   =>
                     New_Occurrence_Of (RTE (RE_Unsigned), Loc))));

               --  Set entity as imported with correct external name

               Set_Is_Imported (E);
               Set_Interface_Name (E, Make_String_Literal (Loc, S));

               --  Set entity as internal to ensure proper Sprint output of its
               --  implicit importation.

               Set_Is_Internal (E);

               --  And now rewrite original reference

               Rewrite (N,
                 Make_Function_Call (Loc,
                   Name                   =>
                     New_Occurrence_Of (RTE (RE_Get_Version_String), Loc),
                   Parameter_Associations => New_List (
                     New_Occurrence_Of (E, Loc))));
            end if;

            Analyze_And_Resolve (N, RTE (RE_Version_String));
         end Version;

      -------------
      -- Ceiling --
      -------------

      --  Transforms 'Ceiling into a call to the floating-point attribute
      --  function Ceiling in Fat_xxx (where xxx is the root type)

      when Attribute_Ceiling =>
         Expand_Fpt_Attribute_R (N);

      --------------
      -- Callable --
      --------------

      --  Transforms 'Callable attribute into a call to the Callable function

      when Attribute_Callable =>

         --  We have an object of a task interface class-wide type as a prefix
         --  to Callable. Generate:
         --    callable (Task_Id (Pref._disp_get_task_id));

         if Ada_Version >= Ada_2005
           and then Ekind (Ptyp) = E_Class_Wide_Type
           and then Is_Interface (Ptyp)
           and then Is_Task_Interface (Ptyp)
         then
            Rewrite (N,
              Make_Function_Call (Loc,
                Name                   =>
                  New_Occurrence_Of (RTE (RE_Callable), Loc),
                Parameter_Associations => New_List (
                  Unchecked_Convert_To
                    (RTE (RO_ST_Task_Id),
                     Build_Disp_Get_Task_Id_Call (Pref)))));

         else
            Rewrite (N, Build_Call_With_Task (Pref, RTE (RE_Callable)));
         end if;

         Analyze_And_Resolve (N, Standard_Boolean);

      ------------
      -- Caller --
      ------------

      --  Transforms 'Caller attribute into a call to either the
      --  Task_Entry_Caller or the Protected_Entry_Caller function.

      when Attribute_Caller => Caller : declare
         Id_Kind    : constant Entity_Id := RTE (RO_AT_Task_Id);
         Ent        : constant Entity_Id := Entity (Pref);
         Conctype   : constant Entity_Id := Scope (Ent);
         Nest_Depth : Nat := 0;
         Name       : Node_Id;
         S          : Entity_Id;

      begin
         --  Protected case

         if Is_Protected_Type (Conctype) then
            case Corresponding_Runtime_Package (Conctype) is
               when System_Tasking_Protected_Objects_Entries =>
                  Name :=
                    New_Occurrence_Of
                      (RTE (RE_Protected_Entry_Caller), Loc);

               when System_Tasking_Protected_Objects_Single_Entry =>
                  Name :=
                    New_Occurrence_Of
                      (RTE (RE_Protected_Single_Entry_Caller), Loc);

               when others =>
                  raise Program_Error;
            end case;

            Rewrite (N,
              Unchecked_Convert_To (Id_Kind,
                Make_Function_Call (Loc,
                  Name => Name,
                  Parameter_Associations => New_List (
                    New_Occurrence_Of
                      (Find_Protection_Object (Current_Scope), Loc)))));

         --  Task case

         else
            --  Determine the nesting depth of the E'Caller attribute, that
            --  is, how many accept statements are nested within the accept
            --  statement for E at the point of E'Caller. The runtime uses
            --  this depth to find the specified entry call.

            for J in reverse 0 .. Scope_Stack.Last loop
               S := Scope_Stack.Table (J).Entity;

               --  We should not reach the scope of the entry, as it should
               --  already have been checked in Sem_Attr that this attribute
               --  reference is within a matching accept statement.

               pragma Assert (S /= Conctype);

               if S = Ent then
                  exit;

               elsif Is_Entry (S) then
                  Nest_Depth := Nest_Depth + 1;
               end if;
            end loop;

            Rewrite (N,
              Unchecked_Convert_To (Id_Kind,
                Make_Function_Call (Loc,
                  Name =>
                    New_Occurrence_Of (RTE (RE_Task_Entry_Caller), Loc),
                  Parameter_Associations => New_List (
                    Make_Integer_Literal (Loc,
                      Intval => Nest_Depth)))));
         end if;

         Analyze_And_Resolve (N, Id_Kind);
      end Caller;

      --------------------
      -- Component_Size --
      --------------------

      --  Component_Size is handled by the back end

      when Attribute_Component_Size =>
         Apply_Universal_Integer_Attribute_Checks (N);

      -------------
      -- Compose --
      -------------

      --  Transforms 'Compose into a call to the floating-point attribute
      --  function Compose in Fat_xxx (where xxx is the root type)

      --  Note: we strictly should have special code here to deal with the
      --  case of absurdly negative arguments (less than Integer'First)
      --  which will return a (signed) zero value, but it hardly seems
      --  worth the effort. Absurdly large positive arguments will raise
      --  constraint error which is fine.

      when Attribute_Compose =>
         Expand_Fpt_Attribute_RI (N);

      -----------------
      -- Constrained --
      -----------------

      when Attribute_Constrained => Constrained : declare
         Formal_Ent : constant Entity_Id := Param_Entity (Pref);

      begin
         --  Reference to a parameter where the value is passed as an extra
         --  actual, corresponding to the extra formal referenced by the
         --  Extra_Constrained field of the corresponding formal. If this
         --  is an entry in-parameter, it is replaced by a constant renaming
         --  for which Extra_Constrained is never created.

         if Present (Formal_Ent)
           and then Ekind (Formal_Ent) /= E_Constant
           and then Present (Extra_Constrained (Formal_Ent))
         then
            Rewrite (N,
              New_Occurrence_Of
                (Extra_Constrained (Formal_Ent), Loc));

         --  If the prefix is an access to object, the attribute applies to
         --  the designated object, so rewrite with an explicit dereference.

         elsif Is_Access_Type (Ptyp)
           and then
             (not Is_Entity_Name (Pref) or else Is_Object (Entity (Pref)))
         then
            Rewrite (Pref,
              Make_Explicit_Dereference (Loc, Relocate_Node (Pref)));

         --  For variables with a Extra_Constrained field, we use the
         --  corresponding entity.

         elsif Nkind (Pref) = N_Identifier
           and then Ekind (Entity (Pref)) = E_Variable
           and then Present (Extra_Constrained (Entity (Pref)))
         then
            Rewrite (N,
              New_Occurrence_Of
                (Extra_Constrained (Entity (Pref)), Loc));

         --  For all other cases, we can tell at compile time

         else
            --  For access type, apply access check as needed

            if Is_Entity_Name (Pref)
              and then not Is_Type (Entity (Pref))
              and then Is_Access_Type (Ptyp)
            then
               Apply_Access_Check (N);
            end if;

            Rewrite (N,
              New_Occurrence_Of
                (Boolean_Literals
                   (Exp_Util.Attribute_Constrained_Static_Value (Pref)), Loc));
         end if;

         Analyze_And_Resolve (N, Standard_Boolean);
      end Constrained;

      ---------------
      -- Copy_Sign --
      ---------------

      --  Transforms 'Copy_Sign into a call to the floating-point attribute
      --  function Copy_Sign in Fat_xxx (where xxx is the root type).

      when Attribute_Copy_Sign =>
         Expand_Fpt_Attribute_RR (N);

      -----------
      -- Count --
      -----------

      --  Transforms 'Count attribute into a call to the Count function

      when Attribute_Count => Count : declare
         Call     : Node_Id;
         Conctyp  : Entity_Id;
         Entnam   : Node_Id;
         Entry_Id : Entity_Id;
         Index    : Node_Id;
         Name     : Node_Id;

      begin
         --  If the prefix is a member of an entry family, retrieve both
         --  entry name and index. For a simple entry there is no index.

         if Nkind (Pref) = N_Indexed_Component then
            Entnam := Prefix (Pref);
            Index := First (Expressions (Pref));
         else
            Entnam := Pref;
            Index := Empty;
         end if;

         Entry_Id := Entity (Entnam);

         --  Find the concurrent type in which this attribute is referenced
         --  (there had better be one).

         Conctyp := Current_Scope;
         while not Is_Concurrent_Type (Conctyp) loop
            Conctyp := Scope (Conctyp);
         end loop;

         --  Protected case

         if Is_Protected_Type (Conctyp) then

            --  No need to transform 'Count into a function call if the current
            --  scope has been eliminated. In this case such transformation is
            --  also not viable because the enclosing protected object is not
            --  available.

            if Is_Eliminated (Current_Scope) then
               return;
            end if;

            case Corresponding_Runtime_Package (Conctyp) is
               when System_Tasking_Protected_Objects_Entries =>
                  Name := New_Occurrence_Of (RTE (RE_Protected_Count), Loc);

                  Call :=
                    Make_Function_Call (Loc,
                      Name                   => Name,
                      Parameter_Associations => New_List (
                        New_Occurrence_Of
                          (Find_Protection_Object (Current_Scope), Loc),
                        Entry_Index_Expression
                          (Loc, Entry_Id, Index, Scope (Entry_Id))));

               when System_Tasking_Protected_Objects_Single_Entry =>
                  Name :=
                    New_Occurrence_Of (RTE (RE_Protected_Count_Entry), Loc);

                  Call :=
                    Make_Function_Call (Loc,
                      Name                   => Name,
                      Parameter_Associations => New_List (
                        New_Occurrence_Of
                          (Find_Protection_Object (Current_Scope), Loc)));

               when others =>
                  raise Program_Error;
            end case;

         --  Task case

         else
            Call :=
              Make_Function_Call (Loc,
                Name => New_Occurrence_Of (RTE (RE_Task_Count), Loc),
                Parameter_Associations => New_List (
                  Entry_Index_Expression (Loc,
                    Entry_Id, Index, Scope (Entry_Id))));
         end if;

         --  The call returns type Natural but the context is universal integer
         --  so any integer type is allowed. The attribute was already resolved
         --  so its Etype is the required result type. If the base type of the
         --  context type is other than Standard.Integer we put in a conversion
         --  to the required type. This can be a normal typed conversion since
         --  both input and output types of the conversion are integer types

         if Base_Type (Typ) /= Base_Type (Standard_Integer) then
            Rewrite (N, Convert_To (Typ, Call));
         else
            Rewrite (N, Call);
         end if;

         Analyze_And_Resolve (N, Typ);
      end Count;

      ---------------------
      -- Descriptor_Size --
      ---------------------

      --  Descriptor_Size is handled by the back end

      when Attribute_Descriptor_Size =>
         Apply_Universal_Integer_Attribute_Checks (N);

      ---------------
      -- Elab_Body --
      ---------------

      --  This processing is shared by Elab_Spec

      --  What we do is to insert the following declarations

      --     procedure tnn;
      --     pragma Import (C, enn, "name___elabb/s");

      --  and then the Elab_Body/Spec attribute is replaced by a reference
      --  to this defining identifier.

      when Attribute_Elab_Body
         | Attribute_Elab_Spec
      =>
         --  Leave attribute unexpanded in CodePeer mode: the gnat2scil
         --  back-end knows how to handle these attributes directly.

         if CodePeer_Mode then
            return;
         end if;

         Elab_Body : declare
            Ent  : constant Entity_Id := Make_Temporary (Loc, 'E');
            Str  : String_Id;
            Lang : Node_Id;

            procedure Make_Elab_String (Nod : Node_Id);
            --  Given Nod, an identifier, or a selected component, put the
            --  image into the current string literal, with double underline
            --  between components.

            ----------------------
            -- Make_Elab_String --
            ----------------------

            procedure Make_Elab_String (Nod : Node_Id) is
            begin
               if Nkind (Nod) = N_Selected_Component then
                  Make_Elab_String (Prefix (Nod));
                  Store_String_Char ('_');
                  Store_String_Char ('_');
                  Get_Name_String (Chars (Selector_Name (Nod)));

               else
                  pragma Assert (Nkind (Nod) = N_Identifier);
                  Get_Name_String (Chars (Nod));
               end if;

               Store_String_Chars (Name_Buffer (1 .. Name_Len));
            end Make_Elab_String;

         --  Start of processing for Elab_Body/Elab_Spec

         begin
            --  First we need to prepare the string literal for the name of
            --  the elaboration routine to be referenced.

            Start_String;
            Make_Elab_String (Pref);
            Store_String_Chars ("___elab");
            Lang := Make_Identifier (Loc, Name_C);

            if Id = Attribute_Elab_Body then
               Store_String_Char ('b');
            else
               Store_String_Char ('s');
            end if;

            Str := End_String;

            Insert_Actions (N, New_List (
              Make_Subprogram_Declaration (Loc,
                Specification =>
                  Make_Procedure_Specification (Loc,
                    Defining_Unit_Name => Ent)),

              Make_Pragma (Loc,
                Chars                        => Name_Import,
                Pragma_Argument_Associations => New_List (
                  Make_Pragma_Argument_Association (Loc, Expression => Lang),

                  Make_Pragma_Argument_Association (Loc,
                    Expression => Make_Identifier (Loc, Chars (Ent))),

                  Make_Pragma_Argument_Association (Loc,
                    Expression => Make_String_Literal (Loc, Str))))));

            Set_Entity (N, Ent);
            Rewrite (N, New_Occurrence_Of (Ent, Loc));
         end Elab_Body;

      --------------------
      -- Elab_Subp_Body --
      --------------------

      --  Always ignored. In CodePeer mode, gnat2scil knows how to handle
      --  this attribute directly, and if we are not in CodePeer mode it is
      --  entirely ignored ???

      when Attribute_Elab_Subp_Body =>
         return;

      ----------------
      -- Elaborated --
      ----------------

      --  Elaborated is always True for preelaborated units, predefined units,
      --  pure units and units which have Elaborate_Body pragmas. These units
      --  have no elaboration entity.

      --  Note: The Elaborated attribute is never passed to the back end

      when Attribute_Elaborated => Elaborated : declare
         Elab_Id : constant Entity_Id := Elaboration_Entity (Entity (Pref));

      begin
         if Present (Elab_Id) then
            Rewrite (N,
              Make_Op_Ne (Loc,
                Left_Opnd  => New_Occurrence_Of (Elab_Id, Loc),
                Right_Opnd => Make_Integer_Literal (Loc, Uint_0)));

            Analyze_And_Resolve (N, Typ);
         else
            Rewrite (N, New_Occurrence_Of (Standard_True, Loc));
         end if;
      end Elaborated;

      --------------
      -- Enum_Rep --
      --------------

      when Attribute_Enum_Rep => Enum_Rep : declare
         Expr : Node_Id;

      begin
         --  Get the expression, which is X for Enum_Type'Enum_Rep (X) or
         --  X'Enum_Rep.

         if Is_Non_Empty_List (Exprs) then
            Expr := First (Exprs);
         else
            Expr := Pref;
         end if;

         --  If not constant-folded, Enum_Type'Enum_Rep (X) or X'Enum_Rep
         --  expands to

         --    target-type!(X)

         --  This is an unchecked conversion from the enumeration type to the
         --  target integer type, which is treated by the back end as a normal
         --  integer conversion, treating the enumeration type as an integer,
         --  which is exactly what we want. Unlike for the Pos attribute, we
         --  cannot use a regular conversion since the associated check would
         --  involve comparing the converted bounds, i.e. would involve the use
         --  of 'Pos instead 'Enum_Rep for these bounds.

         --  However the target type is universal integer in most cases, which
         --  is a very large type, so in the case of an enumeration type, we
         --  first convert to a small signed integer type in order not to lose
         --  the size information.

         if Is_Enumeration_Type (Ptyp) then
            Rewrite (N, Unchecked_Convert_To (Get_Integer_Type (Ptyp), Expr));
            Convert_To_And_Rewrite (Typ, N);

         --  Deal with integer types (replace by conversion)

         else
            Rewrite (N, Convert_To (Typ, Expr));
         end if;

         Analyze_And_Resolve (N, Typ);
      end Enum_Rep;

      --------------
      -- Enum_Val --
      --------------

      when Attribute_Enum_Val => Enum_Val : declare
         Expr : Node_Id;
         Btyp : constant Entity_Id  := Base_Type (Ptyp);

      begin
         --  X'Enum_Val (Y) expands to

         --    [constraint_error when _rep_to_pos (Y, False) = -1, msg]
         --    X!(Y);

         Expr := Unchecked_Convert_To (Ptyp, First (Exprs));

         --  Ensure that the expression is not truncated since the "bad" bits
         --  are desired.

         if Nkind (Expr) = N_Unchecked_Type_Conversion then
            Set_No_Truncation (Expr);
         end if;

         Insert_Action (N,
           Make_Raise_Constraint_Error (Loc,
             Condition =>
               Make_Op_Eq (Loc,
                 Left_Opnd =>
                   Make_Function_Call (Loc,
                     Name =>
                       New_Occurrence_Of (TSS (Btyp, TSS_Rep_To_Pos), Loc),
                     Parameter_Associations => New_List (
                       Relocate_Node (Duplicate_Subexpr (Expr)),
                         New_Occurrence_Of (Standard_False, Loc))),

                 Right_Opnd => Make_Integer_Literal (Loc, -1)),
             Reason => CE_Range_Check_Failed));

         Rewrite (N, Expr);
         Analyze_And_Resolve (N, Ptyp);
      end Enum_Val;

      --------------
      -- Exponent --
      --------------

      --  Transforms 'Exponent into a call to the floating-point attribute
      --  function Exponent in Fat_xxx (where xxx is the root type)

      when Attribute_Exponent =>
         Expand_Fpt_Attribute_R (N);

      ------------------
      -- External_Tag --
      ------------------

      --  transforme X'External_Tag into Ada.Tags.External_Tag (X'tag)

      when Attribute_External_Tag =>
         Rewrite (N,
           Make_Function_Call (Loc,
             Name                   =>
               New_Occurrence_Of (RTE (RE_External_Tag), Loc),
             Parameter_Associations => New_List (
               Make_Attribute_Reference (Loc,
                 Attribute_Name => Name_Tag,
                 Prefix         => Prefix (N)))));

         Analyze_And_Resolve (N, Standard_String);

      -----------------------
      -- Finalization_Size --
      -----------------------

      when Attribute_Finalization_Size => Finalization_Size : declare
         function Calculate_Header_Size return Node_Id;
         --  Generate a runtime call to calculate the size of the hidden header
         --  along with any added padding which would precede a heap-allocated
         --  object of the prefix type.

         ---------------------------
         -- Calculate_Header_Size --
         ---------------------------

         function Calculate_Header_Size return Node_Id is
         begin
            --  Generate:
            --    Typ (Header_Size_With_Padding (Pref'Alignment))

            return
              Convert_To (Typ,
                Make_Function_Call (Loc,
                  Name                   =>
                    New_Occurrence_Of (RTE (RE_Header_Size_With_Padding), Loc),

                  Parameter_Associations => New_List (
                    Make_Attribute_Reference (Loc,
                      Prefix         => New_Copy_Tree (Pref),
                      Attribute_Name => Name_Alignment))));
         end Calculate_Header_Size;

         --  Local variables

         P_Loc : constant Source_Ptr := Sloc (Pref);
         Size  : Entity_Id;

      --  Start of processing for Finalization_Size

      begin
         --  If the prefix is an interface type, generate code to obtain its
         --  address and displace it to reference the base of the object.

         if Is_Interface (Ptyp) then
            --  Generate:
            --    Ptyp!(tag_ptr!($base_address (ptr.all'address)).all)

            Rewrite (Pref,
              Unchecked_Convert_To (Ptyp,
                Make_Explicit_Dereference (P_Loc,
                  Unchecked_Convert_To (RTE (RE_Tag_Ptr),
                    Make_Function_Call (P_Loc,
                      Name => New_Occurrence_Of
                                (RTE (RE_Base_Address), P_Loc),
                      Parameter_Associations =>
                        New_List (
                          Make_Attribute_Reference (P_Loc,
                            Prefix => Duplicate_Subexpr (Pref),
                            Attribute_Name => Name_Address)))))));
            Analyze_And_Resolve (Pref, Ptyp);
         end if;

         --  If the prefix is the dereference of an access value subject to
         --  pragma No_Heap_Finalization, then no header has been added.

         if Nkind (Pref) = N_Explicit_Dereference
           and then No_Heap_Finalization (Etype (Prefix (Pref)))
         then
            Rewrite (N, Make_Integer_Literal (Loc, 0));

         --  An object of a class-wide type first requires a runtime check to
         --  determine whether it is actually controlled or not. Depending on
         --  the outcome of this check, the Finalization_Size of the object
         --  may be zero or some positive value.
         --
         --  In this scenario, Pref'Finalization_Size is expanded into
         --
         --    Size : Integer := 0;
         --
         --    if Needs_Finalization (Pref'Tag) then
         --       Size := Integer (Header_Size_With_Padding (Pref'Alignment));
         --    end if;
         --
         --  and the attribute reference is replaced with a reference to Size.

         elsif Is_Class_Wide_Type (Ptyp) then
            Size := Make_Temporary (Loc, 'S');

            Insert_Actions (N, New_List (

              --  Generate:
              --    Size : Integer := 0;

              Make_Object_Declaration (Loc,
                Defining_Identifier => Size,
                Object_Definition   =>
                  New_Occurrence_Of (Standard_Integer, Loc),
                Expression          => Make_Integer_Literal (Loc, 0)),

              --  Generate:
              --    if Needs_Finalization (Pref'Tag) then
              --       Size :=
              --         Integer (Header_Size_With_Padding (Pref'Alignment));
              --    end if;

              Make_If_Statement (Loc,
                Condition              =>
                  Make_Function_Call (Loc,
                    Name                   =>
                      New_Occurrence_Of (RTE (RE_Needs_Finalization), Loc),

                    Parameter_Associations => New_List (
                      Make_Attribute_Reference (Loc,
                        Prefix         => New_Copy_Tree (Pref),
                        Attribute_Name => Name_Tag))),

                Then_Statements        => New_List (
                   Make_Assignment_Statement (Loc,
                     Name       => New_Occurrence_Of (Size, Loc),
                     Expression =>
                       Convert_To
                         (Standard_Integer, Calculate_Header_Size))))));

            Rewrite (N, New_Occurrence_Of (Size, Loc));

         --  The prefix is known to be controlled at compile time and to
         --  require strict finalization. Calculate Finalization_Size by
         --  calling function Header_Size_With_Padding.

         elsif Needs_Finalization (Ptyp)
           and then not Has_Relaxed_Finalization (Ptyp)
         then
            Rewrite (N, Calculate_Header_Size);

         --  The prefix is not an object with controlled parts, so its
         --  Finalization_Size is zero.

         else
            Rewrite (N, Make_Integer_Literal (Loc, 0));
         end if;

         --  Due to cases where the entity type of the attribute is already
         --  resolved the rewritten N must get re-resolved to its appropriate
         --  type.

         Analyze_And_Resolve (N, Typ);
      end Finalization_Size;

      -----------------
      -- First, Last --
      -----------------

      when Attribute_First
         | Attribute_Last
      =>
         --  If the prefix type is a constrained packed array type which
         --  already has a Packed_Array_Impl_Type representation defined, then
         --  replace this attribute with a direct reference to the attribute of
         --  the appropriate index subtype (since otherwise the back end will
         --  try to give us the value of 'First for this implementation type).
         --  Do not do this if Ptyp depends on a discriminant as its bounds
         --  are only available through N.

         if Is_Constrained_Packed_Array (Ptyp)
           and then not Size_Depends_On_Discriminant (Ptyp)
         then
            Rewrite (N,
              Make_Attribute_Reference (Loc,
                Attribute_Name => Attribute_Name (N),
                Prefix         =>
                  New_Occurrence_Of (Get_Index_Subtype (N), Loc)));
            Analyze_And_Resolve (N, Typ);

         --  For a constrained array type, if the bound is a reference to an
         --  entity which is not a discriminant, just replace with a direct
         --  reference. Note that this must be in keeping with what is done
         --  for scalar types in order for range checks to be elided in loops.

         --  However, avoid doing it if the array type is public because, in
         --  this case, we effectively rely on the back end to create public
         --  symbols with consistent names across units for the array bounds.

         elsif Is_Array_Type (Ptyp)
           and then Is_Constrained (Ptyp)
           and then not Is_Public (Ptyp)
         then
            declare
               Bnd : Node_Id;

            begin
               if Id = Attribute_First then
                  Bnd := Type_Low_Bound (Get_Index_Subtype (N));
               else
                  Bnd := Type_High_Bound (Get_Index_Subtype (N));
               end if;

               if Is_Entity_Name (Bnd)
                 and then Ekind (Entity (Bnd)) /= E_Discriminant
               then
                  Rewrite (N, New_Occurrence_Of (Entity (Bnd), Loc));
               end if;
            end;

         --  For access type, apply access check as needed

         elsif Is_Access_Type (Ptyp) then
            Apply_Access_Check (N);

         --  For scalar type, if the bound is a reference to an entity, just
         --  replace with a direct reference. Note that we can only have a
         --  reference to a constant entity at this stage, anything else would
         --  have already been rewritten.

         elsif Is_Scalar_Type (Ptyp) then
            declare
               Bnd : Node_Id;

            begin
               if Id = Attribute_First then
                  Bnd := Type_Low_Bound (Ptyp);
               else
                  Bnd := Type_High_Bound (Ptyp);
               end if;

               if Is_Entity_Name (Bnd) then
                  Rewrite (N, New_Occurrence_Of (Entity (Bnd), Loc));
               end if;
            end;
         end if;

      ---------------
      -- First_Bit --
      ---------------

      --  We leave the computation up to the back end, since we don't know what
      --  layout will be chosen if no component clause was specified.

      when Attribute_First_Bit =>
         Apply_Universal_Integer_Attribute_Checks (N);

      --------------------------------
      -- Fixed_Value, Integer_Value --
      --------------------------------

      --  We transform

      --     fixtype'Fixed_Value (integer-value)
      --     inttype'Integer_Value (fixed-value)

      --  into

      --     fixtype (integer-value)
      --     inttype (fixed-value)

      --  respectively.

      --  We set Conversion_OK on the conversion because we do not want it
      --  to go through the fixed-point conversion circuits.

      when Attribute_Fixed_Value
         | Attribute_Integer_Value
      =>
         Rewrite (N, OK_Convert_To (Entity (Pref), First (Exprs)));

         --  Note that it might appear that a properly analyzed unchecked
         --  conversion would be just fine here, but that's not the case,
         --  since the full range checks performed by the following calls
         --  are critical.

         Apply_Type_Conversion_Checks (N);

         --  Note that Apply_Type_Conversion_Checks only deals with the
         --  overflow checks on conversions involving fixed-point types
         --  so we must apply range checks manually on them and expand.

         Apply_Scalar_Range_Check
           (Expression (N), Etype (N), Fixed_Int => True);

         Set_Analyzed (N);
         Expand (N);

      -----------
      -- Floor --
      -----------

      --  Transforms 'Floor into a call to the floating-point attribute
      --  function Floor in Fat_xxx (where xxx is the root type)

      when Attribute_Floor =>
         Expand_Fpt_Attribute_R (N);

      ----------
      -- Fore --
      ----------

      --  For the fixed-point type Typ:

      --    Typ'Fore

      --  expands into

      --    System.Fore_xx (ftyp (Typ'First), ftyp (Typ'Last) [,pm])

      --    For decimal fixed-point types
      --      xx   = Decimal{32,64,128}
      --      ftyp = Integer_{32,64,128}
      --      pm   = Typ'Scale

      --    For the most common ordinary fixed-point types
      --      xx   = Fixed{32,64,128}
      --      ftyp = Integer_{32,64,128}
      --      pm   = numerator of Typ'Small
      --             denominator of Typ'Small
      --             min (scale of Typ'Small, 0)

      --    For other ordinary fixed-point types
      --      xx   = Fixed
      --      ftyp = Long_Float
      --      pm   = none

      --  Note that we know that the type is a nonstatic subtype, or Fore would
      --  have been computed statically in Eval_Attribute.

      when Attribute_Fore =>
         declare
            Arg_List : List_Id;
            Fid      : RE_Id;
            Ftyp     : Entity_Id;

         begin
            if Is_Decimal_Fixed_Point_Type (Ptyp) then
               if Esize (Ptyp) <= 32 then
                  Fid  := RE_Fore_Decimal32;
                  Ftyp := RTE (RE_Integer_32);
               elsif Esize (Ptyp) <= 64 then
                  Fid  := RE_Fore_Decimal64;
                  Ftyp := RTE (RE_Integer_64);
               else
                  Fid  := RE_Fore_Decimal128;
                  Ftyp := RTE (RE_Integer_128);
               end if;

            else
               declare
                  Num : constant Uint := Norm_Num (Small_Value (Ptyp));
                  Den : constant Uint := Norm_Den (Small_Value (Ptyp));
                  Max : constant Uint := UI_Max (Num, Den);
                  Min : constant Uint := UI_Min (Num, Den);
                  Siz : constant Uint := Esize (Ptyp);

               begin
                  if Siz <= 32
                    and then Max <= Uint_2 ** 31
                    and then (Min = Uint_1
                               or else Num < Den
                               or else Num < Uint_10 ** 8)
                  then
                     Fid  := RE_Fore_Fixed32;
                     Ftyp := RTE (RE_Integer_32);
                  elsif Siz <= 64
                    and then Max <= Uint_2 ** 63
                    and then (Min = Uint_1
                               or else Num < Den
                               or else Num < Uint_10 ** 17)
                  then
                     Fid  := RE_Fore_Fixed64;
                     Ftyp := RTE (RE_Integer_64);
                  elsif System_Max_Integer_Size = 128
                    and then Max <= Uint_2 ** 127
                    and then (Min = Uint_1
                               or else Num < Den
                               or else Num < Uint_10 ** 37)
                  then
                     Fid  := RE_Fore_Fixed128;
                     Ftyp := RTE (RE_Integer_128);
                  else
                     Fid  := RE_Fore_Fixed;
                     Ftyp := Standard_Long_Float;
                  end if;
               end;
            end if;

            Arg_List := New_List (
              Convert_To (Ftyp,
                Make_Attribute_Reference (Loc,
                  Prefix         => New_Occurrence_Of (Ptyp, Loc),
                  Attribute_Name => Name_First)));

            Append_To (Arg_List,
              Convert_To (Ftyp,
                Make_Attribute_Reference (Loc,
                  Prefix         => New_Occurrence_Of (Ptyp, Loc),
                  Attribute_Name => Name_Last)));

            --  For decimal, append Scale and also set to do literal conversion

            if Is_Decimal_Fixed_Point_Type (Ptyp) then
               Set_Conversion_OK (First (Arg_List));
               Set_Conversion_OK (Next (First (Arg_List)));

               Append_To (Arg_List,
                 Make_Integer_Literal (Loc, Scale_Value (Ptyp)));

            --  For ordinary fixed-point types, append Num, Den and Scale
            --  parameters and also set to do literal conversion

            elsif Fid /= RE_Fore_Fixed then
               Set_Conversion_OK (First (Arg_List));
               Set_Conversion_OK (Next (First (Arg_List)));

               Append_To (Arg_List,
                 Make_Integer_Literal (Loc, -Norm_Num (Small_Value (Ptyp))));

               Append_To (Arg_List,
                 Make_Integer_Literal (Loc, -Norm_Den (Small_Value (Ptyp))));

               declare
                  Val   : Ureal := Small_Value (Ptyp);
                  Scale : Int   := 0;

               begin
                  while Val >= Ureal_10 loop
                     Val := Val / Ureal_10;
                     Scale := Scale - 1;
                  end loop;

                  Append_To (Arg_List,
                     Make_Integer_Literal (Loc, UI_From_Int (Scale)));
               end;
            end if;

            Rewrite (N,
              Convert_To (Typ,
                Make_Function_Call (Loc,
                  Name                   =>
                    New_Occurrence_Of (RTE (Fid), Loc),
                  Parameter_Associations => Arg_List)));

            Analyze_And_Resolve (N, Typ);
         end;

      --------------
      -- Fraction --
      --------------

      --  Transforms 'Fraction into a call to the floating-point attribute
      --  function Fraction in Fat_xxx (where xxx is the root type)

      when Attribute_Fraction =>
         Expand_Fpt_Attribute_R (N);

      --------------
      -- From_Any --
      --------------

      when Attribute_From_Any => From_Any : declare
         Decls : constant List_Id := New_List;

      begin
         Rewrite (N,
           Build_From_Any_Call (Ptyp,
             Relocate_Node (First (Exprs)),
             Decls));
         Insert_Actions (N, Decls);
         Analyze_And_Resolve (N, Ptyp);
      end From_Any;

      ----------------------
      -- Has_Same_Storage --
      ----------------------

      when Attribute_Has_Same_Storage => Has_Same_Storage : declare
         X : constant Node_Id := Pref;
         Y : constant Node_Id := First (Exprs);
         --  The arguments

         X_Addr : Node_Id;
         Y_Addr : Node_Id;
         --  Rhe expressions for their addresses

         X_Size : Node_Id;
         Y_Size : Node_Id;
         --  Rhe expressions for their sizes

      begin
         --  The attribute is expanded as:

         --    (X'address = Y'address)
         --      and then (X'Size = Y'Size)
         --      and then (X'Size /= 0)      (AI12-0077)

         --  If both arguments have the same Etype the second conjunct can be
         --  omitted.

         X_Addr :=
           Make_Attribute_Reference (Loc,
             Attribute_Name => Name_Address,
             Prefix         => New_Copy_Tree (X));

         Y_Addr :=
           Make_Attribute_Reference (Loc,
             Attribute_Name => Name_Address,
             Prefix         => New_Copy_Tree (Y));

         X_Size :=
           Make_Attribute_Reference (Loc,
             Attribute_Name => Name_Size,
             Prefix         => New_Copy_Tree (X));

         if Etype (X) = Etype (Y) then
            Rewrite (N,
              Make_And_Then (Loc,
                Left_Opnd  =>
                  Make_Op_Eq (Loc,
                    Left_Opnd  => X_Addr,
                    Right_Opnd => Y_Addr),
                Right_Opnd =>
                  Make_Op_Ne (Loc,
                    Left_Opnd  => X_Size,
                    Right_Opnd => Make_Integer_Literal (Loc, 0))));
         else
            Y_Size :=
              Make_Attribute_Reference (Loc,
                Attribute_Name => Name_Size,
                Prefix         => New_Copy_Tree (Y));

            Rewrite (N,
              Make_And_Then (Loc,
                Left_Opnd  =>
                  Make_Op_Eq (Loc,
                    Left_Opnd  => X_Addr,
                    Right_Opnd => Y_Addr),
                Right_Opnd =>
                  Make_And_Then (Loc,
                    Left_Opnd  =>
                      Make_Op_Eq (Loc,
                        Left_Opnd  => X_Size,
                        Right_Opnd => Y_Size),
                    Right_Opnd =>
                      Make_Op_Ne (Loc,
                        Left_Opnd  => New_Copy_Tree (X_Size),
                        Right_Opnd => Make_Integer_Literal (Loc, 0)))));
         end if;

         Analyze_And_Resolve (N, Standard_Boolean);
      end Has_Same_Storage;

      --------------
      -- Identity --
      --------------

      --  For an exception returns a reference to the exception data:
      --      Exception_Id!(Prefix'Reference)

      --  For a task it returns a reference to the _task_id component of
      --  corresponding record:

      --    taskV!(Prefix)._Task_Id, converted to the type Task_Id defined

      --  in Ada.Task_Identification

      when Attribute_Identity => Identity : declare
         Id_Kind : Entity_Id;

      begin
         if Ptyp = Standard_Exception_Type then
            Id_Kind := RTE (RE_Exception_Id);

            if Present (Renamed_Entity (Entity (Pref))) then
               Set_Entity (Pref, Renamed_Entity (Entity (Pref)));
            end if;

            Rewrite (N,
              Unchecked_Convert_To (Id_Kind, Make_Reference (Loc, Pref)));
         else
            Id_Kind := RTE (RO_AT_Task_Id);

            --  If the prefix is a task interface, the Task_Id is obtained
            --  dynamically through a dispatching call, as for other task
            --  attributes applied to interfaces.

            if Ada_Version >= Ada_2005
              and then Ekind (Ptyp) = E_Class_Wide_Type
              and then Is_Interface (Ptyp)
              and then Is_Task_Interface (Ptyp)
            then
               Rewrite (N,
                 Unchecked_Convert_To
                   (Id_Kind, Build_Disp_Get_Task_Id_Call (Pref)));

            else
               Rewrite (N,
                 Unchecked_Convert_To (Id_Kind, Concurrent_Ref (Pref)));
            end if;
         end if;

         Analyze_And_Resolve (N, Id_Kind);
      end Identity;

      -----------
      -- Image --
      -----------

      when Attribute_Image =>

         --  Leave attribute unexpanded in CodePeer mode: the gnat2scil
         --  back-end knows how to handle this attribute directly.

         if CodePeer_Mode then
            return;
         end if;

         Exp_Imgv.Expand_Image_Attribute (N);

      ---------
      -- Img --
      ---------

      --  X'Img is expanded to typ'Image (X), where typ is the type of X

      when Attribute_Img =>
         Exp_Imgv.Expand_Image_Attribute (N);

      -----------
      -- Index --
      -----------

      --  Transforms 'Index attribute into a reference to the second formal of
      --  the wrapper built for an entry family that has contract cases (see
      --  Exp_Ch9.Build_Contract_Wrapper).

      when Attribute_Index => Index : declare
         Entry_Id  : constant Entity_Id := Entity (Pref);
         Entry_Idx : constant Entity_Id :=
                       Next_Entity
                         (First_Entity (Contract_Wrapper (Entry_Id)));
      begin
         Rewrite (N, New_Occurrence_Of (Entry_Idx, Loc));
         Analyze_And_Resolve (N, Typ);
      end Index;

      -----------------
      -- Initialized --
      -----------------

      --  For execution, we could either implement an approximation of this
      --  aspect, or use Valid_Scalars as a first approximation. For now we do
      --  the latter.

      when Attribute_Initialized =>

         --  Do not expand 'Initialized in CodePeer mode, it will be handled
         --  by the back-end directly.

         if CodePeer_Mode then
            return;
         end if;

         Rewrite
           (N,
            Make_Attribute_Reference
              (Sloc           => Loc,
               Prefix         => Pref,
               Attribute_Name => Name_Valid_Scalars,
               Expressions    => Exprs));

         Analyze_And_Resolve (N);

      -----------
      -- Input --
      -----------

      when Attribute_Input => Input : declare
         P_Type  : constant Entity_Id := Entity (Pref);
         B_Type  : constant Entity_Id := Base_Type (P_Type);
         U_Type  : constant Entity_Id := Underlying_Type (P_Type);
         Strm    : constant Node_Id   := First (Exprs);
         Fname   : Entity_Id;
         Decl    : Node_Id;
         Call    : Node_Id;
         Prag    : Node_Id;
         Arg2    : Node_Id;
         Rfunc   : Node_Id;

         Cntrl   : Node_Id := Empty;
         --  Value for controlling argument in call. Always Empty except in
         --  the dispatching (class-wide type) case, where it is a reference
         --  to the dummy object initialized to the right internal tag.

         procedure Freeze_Stream_Subprogram (F : Entity_Id);
         --  The expansion of the attribute reference may generate a call to
         --  a user-defined stream subprogram that is frozen by the call. This
         --  can lead to access-before-elaboration problem if the reference
         --  appears in an object declaration and the subprogram body has not
         --  been seen. The freezing of the subprogram requires special code
         --  because it appears in an expanded context where expressions do
         --  not freeze their constituents.

         ------------------------------
         -- Freeze_Stream_Subprogram --
         ------------------------------

         procedure Freeze_Stream_Subprogram (F : Entity_Id) is
            Decl : constant Node_Id := Unit_Declaration_Node (F);
            Bod  : Node_Id;

         begin
            --  If this is user-defined subprogram, the corresponding
            --  stream function appears as a renaming-as-body, and the
            --  user subprogram must be retrieved by tree traversal.

            if Present (Decl)
              and then Nkind (Decl) = N_Subprogram_Declaration
              and then Present (Corresponding_Body (Decl))
            then
               Bod := Corresponding_Body (Decl);

               if Nkind (Unit_Declaration_Node (Bod)) =
                 N_Subprogram_Renaming_Declaration
               then
                  Set_Is_Frozen (Entity (Name (Unit_Declaration_Node (Bod))));
               end if;
            end if;
         end Freeze_Stream_Subprogram;

      --  Start of processing for Input

      begin
         --  If no underlying type, we have an error that will be diagnosed
         --  elsewhere, so here we just completely ignore the expansion.

         if No (U_Type) then
            return;
         end if;

         --  Stream operations can appear in user code even if the restriction
         --  No_Streams is active (for example, when instantiating a predefined
         --  container). In that case rewrite the attribute as a Raise to
         --  prevent any run-time use.

         if Restriction_Active (No_Streams) then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Stream_Operation_Not_Allowed));
            Set_Etype (N, B_Type);
            return;
         end if;

         --  If there is a TSS for Input, just call it

         Fname := Find_Stream_Subprogram (P_Type, TSS_Stream_Input, N);

         if No (Fname) then

            --  If there is a Stream_Convert pragma, use it, we rewrite

            --     sourcetyp'Input (stream)

            --  as

            --     sourcetyp (streamread (strmtyp'Input (stream)));

            --  where streamread is the given Read function that converts an
            --  argument of type strmtyp to type sourcetyp or a type from which
            --  it is derived (extra conversion required for the derived case).

            Prag := Get_Stream_Convert_Pragma (P_Type);

            if Present (Prag) then
               Arg2  := Next (First (Pragma_Argument_Associations (Prag)));
               Rfunc := Entity (Expression (Arg2));

               Rewrite (N,
                 Convert_To (B_Type,
                   Make_Function_Call (Loc,
                     Name => New_Occurrence_Of (Rfunc, Loc),
                     Parameter_Associations => New_List (
                       Make_Attribute_Reference (Loc,
                         Prefix =>
                           New_Occurrence_Of
                             (Etype (First_Formal (Rfunc)), Loc),
                         Attribute_Name => Name_Input,
                         Expressions => Exprs)))));

               Analyze_And_Resolve (N, B_Type);
               return;

            --  Limited types

            elsif Default_Streaming_Unavailable (U_Type) then
               --  Do the same thing here as is done above in the
               --  case where a No_Streams restriction is active.

               Rewrite (N,
                 Make_Raise_Program_Error (Loc,
                   Reason => PE_Stream_Operation_Not_Allowed));
               Set_Etype (N, B_Type);
               return;

            --  Elementary types

            elsif Is_Elementary_Type (U_Type) then

               --  A special case arises if we have a defined _Read routine,
               --  since in this case we are required to call this routine.

               if Present (Find_Inherited_TSS (P_Type, TSS_Stream_Read)) then
                  Build_Record_Or_Elementary_Input_Function
                    (P_Type, Decl, Fname);
                  Insert_Action (N, Decl);

               --  For normal cases, we call the I_xxx routine directly

               else
                  Rewrite (N, Build_Elementary_Input_Call (N));
                  Analyze_And_Resolve (N, P_Type);
                  return;
               end if;

            --  Array type case

            elsif Is_Array_Type (U_Type) then
               declare
                  procedure Build_And_Insert_Array_Input_Func is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Array_Input_Function);
               begin
                  Build_And_Insert_Array_Input_Func
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Fname,
                     Attr_Ref => N);
               end;

            --  Dispatching case with class-wide type

            elsif Is_Class_Wide_Type (P_Type) then

               if Is_Mutably_Tagged_Type (P_Type) then

                  --  In mutably tagged case, rewrite
                  --    T'Class'Input (Strm)
                  --  as (roughly)
                  --    declare
                  --       Result : T'Class;
                  --       T'Class'Read (Strm, Result);
                  --    begin
                  --      Result;
                  --    end;

                  declare
                     Result_Temp : constant Entity_Id :=
                       Make_Temporary (Loc, 'I');

                     --  Gets default initialization
                     Result_Temp_Decl : constant Node_Id :=
                       Make_Object_Declaration (Loc,
                         Defining_Identifier => Result_Temp,
                         Object_Definition =>
                           New_Occurrence_Of (P_Type, Loc));

                     function Result_Temp_Name return Node_Id is
                       (New_Occurrence_Of (Result_Temp, Loc));

                     Actions : constant List_Id := New_List (
                       Result_Temp_Decl,
                       Make_Attribute_Reference (Loc,
                         Prefix => New_Occurrence_Of (P_Type, Loc),
                         Attribute_Name => Name_Read,
                         Expressions => New_List (
                           Relocate_Node (Strm), Result_Temp_Name)));
                  begin
                     Rewrite (N, Make_Expression_With_Actions (Loc,
                                   Actions, Result_Temp_Name));
                     Analyze_And_Resolve (N, P_Type);
                     return;
                  end;
               end if;

               --  No need to do anything else compiling under restriction
               --  No_Dispatching_Calls. During the semantic analysis we
               --  already notified such violation.

               if Restriction_Active (No_Dispatching_Calls) then
                  return;
               end if;

               Read_Controlling_Tag (P_Type, Cntrl);
               Fname := Find_Prim_Op (Root_Type (P_Type), TSS_Stream_Input);

            --  For tagged types, use the primitive Input function

            elsif Is_Tagged_Type (U_Type) then
               Fname := Find_Prim_Op (U_Type, TSS_Stream_Input);

            --  All other record type cases, including protected records. The
            --  latter only arise for expander generated code for handling
            --  shared passive partition access.

            else
               pragma Assert
                 (Is_Record_Type (U_Type) or else Is_Protected_Type (U_Type));

               --  Ada 2005 (AI-216): Program_Error is raised executing default
               --  implementation of the Input attribute of an unchecked union
               --  type if the type lacks default discriminant values.

               if Is_Unchecked_Union (Base_Type (U_Type))
                 and then
                 No (Discriminant_Default_Value (First_Discriminant (U_Type)))
               then
                  Rewrite (N,
                    Make_Raise_Program_Error (Loc,
                      Reason => PE_Unchecked_Union_Restriction));
                  Set_Etype (N, B_Type);
                  return;
               end if;

               --  Build the type's Input function, passing the subtype rather
               --  than its base type, because checks are needed in the case of
               --  constrained discriminants (see Ada 2012 AI05-0192).
               --
               --  ??? Is this correct in the case where the prefix of the
               --  attribute is a constrained subtype of a type whose
               --  first named subtype is unconstrained? Shouldn't we be
               --  passing in the first named subtype of the type?

               declare
                  procedure Build_And_Insert_Record_Input_Func is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Record_Or_Elementary_Input_Function);
               begin
                  Build_And_Insert_Record_Input_Func
                    (Typ      => U_Type,
                     Decl     => Decl,
                     Subp     => Fname,
                     Attr_Ref => N);
               end;

               if Nkind (Parent (N)) = N_Object_Declaration
                 and then Is_Record_Type (U_Type)
               then
                  --  The stream function may contain calls to user-defined
                  --  Read procedures for individual components.

                  declare
                     Comp : Entity_Id;
                     Func : Entity_Id;

                  begin
                     Comp := First_Component (U_Type);
                     while Present (Comp) loop
                        Func :=
                          Find_Stream_Subprogram
                            (Etype (Comp), TSS_Stream_Read, N);

                        if Present (Func) then
                           Freeze_Stream_Subprogram (Func);
                        end if;

                        Next_Component (Comp);
                     end loop;
                  end;
               end if;
            end if;
         end if;

         --  If we fall through, Fname is the function to be called. The result
         --  is obtained by calling the appropriate function, then converting
         --  the result. The conversion does a subtype check.

         Call :=
           Make_Function_Call (Loc,
             Name => New_Occurrence_Of (Fname, Loc),
             Parameter_Associations => New_List (
                Relocate_Node (Strm)));

         Set_Controlling_Argument (Call, Cntrl);
         Rewrite (N, Unchecked_Convert_To (P_Type, Call));
         Analyze_And_Resolve (N, P_Type);

         if Nkind (Parent (N)) = N_Object_Declaration then
            Freeze_Stream_Subprogram (Fname);
         end if;

         if not Is_Tagged_Type (P_Type) then
            Cached_Attribute_Ops.Input_Map.Set (U_Type, Fname);
         end if;
      end Input;

      -------------------
      -- Invalid_Value --
      -------------------

      when Attribute_Invalid_Value =>
         Rewrite (N, Get_Simple_Init_Val (Ptyp, N));

         --  The value produced may be a conversion of a literal, which must be
         --  resolved to establish its proper type.

         Analyze_And_Resolve (N);

      --------------
      -- Last_Bit --
      --------------

      --  We leave the computation up to the back end, since we don't know what
      --  layout will be chosen if no component clause was specified.

      when Attribute_Last_Bit =>
         Apply_Universal_Integer_Attribute_Checks (N);

      ------------------
      -- Leading_Part --
      ------------------

      --  Transforms 'Leading_Part into a call to the floating-point attribute
      --  function Leading_Part in Fat_xxx (where xxx is the root type)

      --  Note: strictly, we should generate special case code to deal with
      --  absurdly large positive arguments (greater than Integer'Last), which
      --  result in returning the first argument unchanged, but it hardly seems
      --  worth the effort. We raise constraint error for absurdly negative
      --  arguments which is fine.

      when Attribute_Leading_Part =>
         Expand_Fpt_Attribute_RI (N);

      ------------
      -- Length --
      ------------

      when Attribute_Length => Length : declare
         Ityp : Entity_Id;
         Xnum : Uint;

      begin
         --  Processing for packed array types

         if Is_Packed_Array (Ptyp) then
            Ityp := Get_Index_Subtype (N);

            --  If the index type, Ityp, is an enumeration type with holes,
            --  then we calculate X'Length explicitly using

            --     Typ'Max
            --       (0, Ityp'Pos (X'Last  (N)) -
            --           Ityp'Pos (X'First (N)) + 1);

            --  Since the bounds in the template are the representation values
            --  and the back end would get the wrong value.

            if Is_Enumeration_Type (Ityp)
              and then Present (Enum_Pos_To_Rep (Base_Type (Ityp)))
            then
               if No (Exprs) then
                  Xnum := Uint_1;
               else
                  Xnum := Expr_Value (First (Expressions (N)));
               end if;

               Rewrite (N,
                 Make_Attribute_Reference (Loc,
                   Prefix         => New_Occurrence_Of (Typ, Loc),
                   Attribute_Name => Name_Max,
                   Expressions    => New_List
                     (Make_Integer_Literal (Loc, 0),

                      Make_Op_Add (Loc,
                        Left_Opnd =>
                          Make_Op_Subtract (Loc,
                            Left_Opnd =>
                              Make_Attribute_Reference (Loc,
                                Prefix => New_Occurrence_Of (Ityp, Loc),
                                Attribute_Name => Name_Pos,

                                Expressions => New_List (
                                  Make_Attribute_Reference (Loc,
                                    Prefix => Duplicate_Subexpr (Pref),
                                   Attribute_Name => Name_Last,
                                    Expressions => New_List (
                                      Make_Integer_Literal (Loc, Xnum))))),

                            Right_Opnd =>
                              Make_Attribute_Reference (Loc,
                                Prefix => New_Occurrence_Of (Ityp, Loc),
                                Attribute_Name => Name_Pos,

                                Expressions => New_List (
                                  Make_Attribute_Reference (Loc,
                                    Prefix =>
                                      Duplicate_Subexpr_No_Checks (Pref),
                                   Attribute_Name => Name_First,
                                    Expressions => New_List (
                                      Make_Integer_Literal (Loc, Xnum)))))),

                        Right_Opnd => Make_Integer_Literal (Loc, 1)))));

               Analyze_And_Resolve (N, Typ, Suppress => All_Checks);
               return;

            --  If the prefix type is a constrained packed array type which
            --  already has a Packed_Array_Impl_Type representation defined,
            --  then replace this attribute with a reference to 'Range_Length
            --  of the appropriate index subtype (since otherwise the
            --  back end will try to give us the value of 'Length for
            --  this implementation type).

            elsif Is_Constrained (Ptyp) then
               Rewrite (N,
                 Make_Attribute_Reference (Loc,
                   Attribute_Name => Name_Range_Length,
                   Prefix => New_Occurrence_Of (Ityp, Loc)));
               Analyze_And_Resolve (N, Typ);
            end if;

         --  Access type case

         elsif Is_Access_Type (Ptyp) then
            Apply_Access_Check (N);

            --  If the designated type is a packed array type, then we convert
            --  the reference to:

            --    typ'Max (0, 1 +
            --                xtyp'Pos (Pref'Last (Expr)) -
            --                xtyp'Pos (Pref'First (Expr)));

            --  This is a bit complex, but it is the easiest thing to do that
            --  works in all cases including enum types with holes xtyp here
            --  is the appropriate index type.

            declare
               Dtyp : constant Entity_Id := Designated_Type (Ptyp);
               Xtyp : Entity_Id;

            begin
               if Is_Packed_Array (Dtyp) then
                  Xtyp := Get_Index_Subtype (N);

                  Rewrite (N,
                    Make_Attribute_Reference (Loc,
                      Prefix         => New_Occurrence_Of (Typ, Loc),
                      Attribute_Name => Name_Max,
                      Expressions    => New_List (
                        Make_Integer_Literal (Loc, 0),

                        Make_Op_Add (Loc,
                          Make_Integer_Literal (Loc, 1),
                          Make_Op_Subtract (Loc,
                            Left_Opnd =>
                              Make_Attribute_Reference (Loc,
                                Prefix => New_Occurrence_Of (Xtyp, Loc),
                                Attribute_Name => Name_Pos,
                                Expressions    => New_List (
                                  Make_Attribute_Reference (Loc,
                                    Prefix => Duplicate_Subexpr (Pref),
                                    Attribute_Name => Name_Last,
                                    Expressions =>
                                      New_Copy_List (Exprs)))),

                            Right_Opnd =>
                              Make_Attribute_Reference (Loc,
                                Prefix => New_Occurrence_Of (Xtyp, Loc),
                                Attribute_Name => Name_Pos,
                                Expressions    => New_List (
                                  Make_Attribute_Reference (Loc,
                                    Prefix =>
                                      Duplicate_Subexpr_No_Checks (Pref),
                                    Attribute_Name => Name_First,
                                    Expressions =>
                                      New_Copy_List (Exprs)))))))));

                  Analyze_And_Resolve (N, Typ);
               end if;
            end;

         --  Overflow-related transformations need Length attribute rewritten
         --  using non-attribute expressions. So generate
         --   (if Pref'First > Pref'Last
         --    then 0
         --    else ((Pref'Last - Pref'First) + 1)) .

         elsif Overflow_Check_Mode in Minimized_Or_Eliminated

            --  This Comes_From_Source test fixes a regression test failure
            --  involving a Length attribute reference generated as part of
            --  the expansion of a concatentation operator; it is unclear
            --  whether this is the right solution to that problem.

            and then Comes_From_Source (N)

            --  This Base_Type equality test is so that we only perform this
            --  transformation if we can do it without introducing
            --  a type conversion anywhere in the resulting expansion;
            --  a type conversion is just as bad as a Length attribute
            --  reference for those overflow-related transformations.

            and then Btyp = Base_Type (Get_Index_Subtype (N))

         then
            declare
               function Prefix_Bound
                 (Bound_Attr_Name : Name_Id; Is_First_Copy : Boolean := False)
                 return Node_Id;
               --  constructs a Pref'First or Pref'Last attribute reference

               ------------------
               -- Prefix_Bound --
               ------------------

               function Prefix_Bound
                 (Bound_Attr_Name : Name_Id; Is_First_Copy : Boolean := False)
                 return Node_Id
               is
                  Prefix : constant Node_Id :=
                    (if Is_First_Copy
                     then Duplicate_Subexpr (Pref)
                     else Duplicate_Subexpr_No_Checks (Pref));
               begin
                  return Make_Attribute_Reference (Loc,
                           Prefix         => Prefix,
                           Attribute_Name => Bound_Attr_Name,
                           Expressions    => New_Copy_List (Exprs));
               end Prefix_Bound;
            begin
               Rewrite (N,
                 Make_If_Expression (Loc,
                   Expressions =>
                     New_List (
                       Node1 => Make_Op_Gt (Loc,
                                  Prefix_Bound (Name_First,
                                                Is_First_Copy => True),
                                  Prefix_Bound (Name_Last)),
                       Node2 => Make_Integer_Literal (Loc, 0),
                       Node3 => Make_Op_Add (Loc,
                                  Make_Op_Subtract (Loc,
                                    Prefix_Bound (Name_Last),
                                    Prefix_Bound (Name_First)),
                                  Make_Integer_Literal (Loc, 1)))));

               Analyze_And_Resolve (N, Typ);
            end;

         --  Otherwise leave it to the back end

         else
            Apply_Universal_Integer_Attribute_Checks (N);
         end if;
      end Length;

      --  Attribute Loop_Entry is replaced with a reference to a constant value
      --  which captures the prefix at the entry point of the related loop. The
      --  loop itself may be transformed into a conditional block.

      when Attribute_Loop_Entry =>
         Expand_Loop_Entry_Attribute (N);

      -------------
      -- Machine --
      -------------

      --  Transforms 'Machine into a call to the floating-point attribute
      --  function Machine in Fat_xxx (where xxx is the root type).
      --  Expansion is avoided for cases the back end can handle directly.

      when Attribute_Machine =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      ----------------------
      -- Machine_Rounding --
      ----------------------

      --  Transforms 'Machine_Rounding into a call to the floating-point
      --  attribute function Machine_Rounding in Fat_xxx (where xxx is the root
      --  type). Expansion is avoided for cases the back end can handle
      --  directly.

      when Attribute_Machine_Rounding =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      ------------------
      -- Machine_Size --
      ------------------

      --  Machine_Size is equivalent to Object_Size, so transform it into
      --  Object_Size and that way the back end never sees Machine_Size.

      when Attribute_Machine_Size =>
         Rewrite (N,
           Make_Attribute_Reference (Loc,
             Prefix => Prefix (N),
             Attribute_Name => Name_Object_Size));

         Analyze_And_Resolve (N, Typ);

      ----------
      -- Make --
      ----------

      when Attribute_Make =>
         declare
            Params    : List_Id;
            Param     : Node_Id;
            Par       : Node_Id;
            Construct : Entity_Id;
            Obj       : Node_Id := Empty;
            Make_Expr : Node_Id := N;

            Formal       : Entity_Id;
            Replace_Expr : Node_Id;
            Init_Param   : Node_Id;
            Construct_Call : Node_Id;
            Curr_Nam : Node_Id := Empty;

            function Replace_Formal_Ref
              (N : Node_Id) return Traverse_Result;

            function Replace_Formal_Ref
              (N : Node_Id) return Traverse_Result is
            begin
               if Is_Entity_Name (N)
                 and then Chars (Formal) = Chars (N)
               then
                  Rewrite (N,
                    New_Copy_Tree (Replace_Expr));
               end if;

               return OK;
            end Replace_Formal_Ref;

            procedure Search_And_Replace_Formal is new
              Traverse_Proc (Replace_Formal_Ref);

         begin
            --  Remove side effects for constructor call

            Param := First (Expressions (N));
            while Present (Param) loop
               if Nkind (Param) = N_Parameter_Association then
                  Remove_Side_Effects (Explicit_Actual_Parameter (Param),
                                       Check_Side_Effects => False);
               else
                  Remove_Side_Effects (Param, Check_Side_Effects => False);
               end if;

               Next (Param);
            end loop;

            --  Construct the parameters list

            Params := New_Copy_List (Expressions (N));
            if Is_Empty_List (Params) then
               Params := New_List;
            end if;

            --  Identify the enclosing parent for the non-copy cases

            Par := Parent (N);
            if Nkind (Par) = N_Qualified_Expression then
               Par       := Parent (Par);
               Make_Expr := Par;
            end if;
            if Nkind (Par) = N_Allocator then
               Par := Parent (Par);
               Curr_Nam := Make_Explicit_Dereference
                            (Loc, Prefix => Empty);
               Obj := Curr_Nam;
            end if;

            declare
               Base_Obj : Node_Id := Empty;
               Typ_Comp : Entity_Id;
               Agg_Comp : Entity_Id;
               Comp_Nam : Node_Id := Empty;
            begin
               while Nkind (Par) not in N_Object_Declaration
                                      | N_Assignment_Statement
               loop
                  if Nkind (Par) = N_Aggregate then
                     Typ_Comp := First_Entity (Etype (Par));
                     Agg_Comp := First (Expressions (Par));
                     loop
                        if No (Agg_Comp) then
                           return;
                        end if;

                        if Agg_Comp = Make_Expr then
                           Comp_Nam :=
                             Make_Selected_Component (Loc,
                               Prefix => Empty,
                               Selector_Name =>
                                 New_Occurrence_Of (Typ_Comp, Loc));

                           Make_Expr := Parent (Make_Expr);
                           Par       := Parent (Par);
                           exit;
                        end if;

                        Next_Entity (Typ_Comp);
                        Next (Agg_Comp);
                     end loop;
                  elsif Nkind (Par) = N_Component_Association then
                     Comp_Nam :=
                       Make_Selected_Component (Loc,
                         Prefix => Empty,
                         Selector_Name =>
                           Make_Identifier (Loc,
                             (Chars (First (Choices (Par))))));

                     Make_Expr := Parent (Parent (Make_Expr));
                     Par       := Parent (Parent (Par));
                  else
                     declare
                        Temp : constant Entity_Id :=
                          Make_Temporary (Loc, 'T', N);
                     begin
                        Rewrite (N,
                          Make_Expression_With_Actions (Loc,
                            Actions => New_List (
                              Make_Object_Declaration (Loc,
                                Defining_Identifier => Temp,
                                Object_Definition   =>
                                  New_Occurrence_Of (Typ, Loc),
                                Expression          =>
                                  New_Copy_Tree (N))),
                            Expression => New_Occurrence_Of (Temp, Loc)));
                        Analyze_And_Resolve (N);
                        return;
                     end;
                  end if;

                  if No (Curr_Nam) then
                     Curr_Nam := Comp_Nam;
                     Obj      := Curr_Nam;
                  elsif Has_Prefix (Curr_Nam) then
                     Set_Prefix (Curr_Nam, Comp_Nam);
                     Curr_Nam := Comp_Nam;
                  end if;
               end loop;

               Base_Obj := (case Nkind (Par) is
                              when N_Assignment_Statement =>
                                 New_Copy_Tree (Name (Par)),
                              when N_Object_Declaration =>
                                 New_Occurrence_Of
                                   (Defining_Identifier (Par), Loc),
                              when others => (raise Program_Error));

               if Present (Curr_Nam) then
                  Set_Prefix (Curr_Nam, Base_Obj);
               else
                  Obj := Base_Obj;
               end if;
            end;

            Prepend_To (Params, Obj);

            --  Find the constructor we are interested in by doing a
            --  pseudo-pass to resolve the constructor call.

            declare
               Dummy_Params : List_Id := New_Copy_List (Expressions (N));
               Dummy_Self   : Node_Id;
               Dummy_Block  : Node_Id;
               Dummy_Call   : Node_Id;
               Dummy_Id     : Entity_Id := Make_Temporary (Loc, 'D', N);
            begin
               if Is_Empty_List (Dummy_Params) then
                  Dummy_Params := New_List;
               end if;

               Dummy_Self := Make_Object_Declaration (Loc,
                               Defining_Identifier => Dummy_Id,
                               Object_Definition   =>
                                  New_Occurrence_Of (Typ, Loc));
               Prepend_To (Dummy_Params, New_Occurrence_Of (Dummy_Id, Loc));

               Dummy_Call := Make_Procedure_Call_Statement (Loc,
                               Parameter_Associations => Dummy_Params,
                               Name                   =>
                                 (if not Has_Prefix (Pref) then
                                     Make_Identifier (Loc,
                                       Chars (Constructor_Name (Typ)))
                                  else
                                     Make_Expanded_Name (Loc,
                                       Chars =>
                                         Chars (Constructor_Name (Typ)),
                                       Prefix =>
                                         New_Copy_Tree (Prefix (Pref)),
                                       Selector_Name =>
                                         Make_Identifier (Loc,
                                          Chars (Constructor_Name (Typ))))));
               Set_Is_Expanded_Constructor_Call (Dummy_Call, True);

               Dummy_Block := Make_Block_Statement (Loc,
                                Declarations => New_List (Dummy_Self),
                                Handled_Statement_Sequence =>
                                  Make_Handled_Sequence_Of_Statements (Loc,
                                    Statements => New_List (Dummy_Call)));

               Expander_Active := False;

               Insert_After_And_Analyze
                 (Enclosing_Declaration_Or_Statement (Par), Dummy_Block);

               Expander_Active := True;

               --  Finally, we can get the constructor based on our pseudo-pass

               Construct := Entity (Name (Dummy_Call));

               --  Replace the Typ'Make attribute with an aggregate featuring
               --  then relevant aggregate from the correct constructor's
               --  Inializeaspect if it is present - otherwise, simply use a
               --  box.

               if Has_Aspect (Construct, Aspect_Initialize) then
                  Rewrite (N,
                    New_Copy_Tree
                      (Find_Value_Of_Aspect (Construct, Aspect_Initialize)));

                  Param  := Next (First (Params));
                  Formal := Next_Entity (First_Entity (Construct));
                  while Present (Param) loop
                     if Nkind (Param) = N_Parameter_Association then
                        Formal := Selector_Name (Param);
                        Replace_Expr := Explicit_Actual_Parameter (Param);
                     else
                        Replace_Expr := Param;
                     end if;

                     Init_Param := First (Component_Associations (N));
                     while Present (Init_Param) loop
                        Search_And_Replace_Formal (Expression (Init_Param));

                        Next (Init_Param);
                     end loop;

                     if Nkind (Param) /= N_Parameter_Association then
                        Next_Entity (Formal);
                     end if;
                     Next (Param);
                  end loop;

                  Init_Param := First (Component_Associations (N));
                  while Present (Init_Param) loop
                     if Nkind (Expression (Init_Param)) = N_Attribute_Reference
                       and then Attribute_Name
                                  (Expression (Init_Param)) = Name_Make
                     then
                        Insert_After (Par,
                          Make_Assignment_Statement (Loc,
                            Name       =>
                              Make_Selected_Component (Loc,
                                Prefix =>
                                  New_Copy_Tree (First (Params)),
                                Selector_Name =>
                                  Make_Identifier (Loc,
                                    Chars (First (Choices (Init_Param))))),
                            Expression =>
                              New_Copy_Tree (Expression (Init_Param))));

                        Rewrite (Expression (Init_Param),
                          Make_Aggregate (Loc,
                            Expressions            => New_List,
                            Component_Associations => New_List (
                              Make_Component_Association (Loc,
                                Choices     =>
                                  New_List (Make_Others_Choice (Loc)),
                                Expression  => Empty,
                                Box_Present => True))));
                     end if;

                     Next (Init_Param);
                  end loop;
               else
                  Rewrite (N,
                    Make_Aggregate (Loc,
                      Expressions            => New_List,
                      Component_Associations => New_List (
                        Make_Component_Association (Loc,
                          Choices     => New_List (Make_Others_Choice (Loc)),
                          Expression  => Empty,
                          Box_Present => True))));
               end if;

               --  Rewrite this block to be null and pretend it didn't happen

               Rewrite (Dummy_Block, Make_Null_Statement (Loc));
            end;

            Analyze_And_Resolve (N, Typ);

            --  Finally, insert the constructor call

            Construct_Call :=
              Make_Procedure_Call_Statement (Loc,
                Name                   =>
                  New_Occurrence_Of (Construct, Loc),
                Parameter_Associations => Params);

            Set_Is_Expanded_Constructor_Call (Construct_Call);
            Insert_After (Par, Construct_Call);
         end;

      --------------
      -- Mantissa --
      --------------

      --  The only case that can get this far is the dynamic case of the old
      --  Ada 83 Mantissa attribute for the fixed-point case. For this case,
      --  we expand:

      --    typ'Mantissa

      --  into

      --    ityp (System.Mantissa.Mantissa_Value
      --           (Integer'Integer_Value (typ'First),
      --            Integer'Integer_Value (typ'Last)));

      when Attribute_Mantissa =>
         Rewrite (N,
           Convert_To (Typ,
             Make_Function_Call (Loc,
               Name                   =>
                 New_Occurrence_Of (RTE (RE_Mantissa_Value), Loc),

               Parameter_Associations => New_List (
                 Make_Attribute_Reference (Loc,
                   Prefix         => New_Occurrence_Of (Standard_Integer, Loc),
                   Attribute_Name => Name_Integer_Value,
                   Expressions    => New_List (
                     Make_Attribute_Reference (Loc,
                       Prefix         => New_Occurrence_Of (Ptyp, Loc),
                       Attribute_Name => Name_First))),

                 Make_Attribute_Reference (Loc,
                   Prefix         => New_Occurrence_Of (Standard_Integer, Loc),
                   Attribute_Name => Name_Integer_Value,
                   Expressions    => New_List (
                     Make_Attribute_Reference (Loc,
                       Prefix         => New_Occurrence_Of (Ptyp, Loc),
                       Attribute_Name => Name_Last)))))));

         Analyze_And_Resolve (N, Typ);

      ---------
      -- Max --
      ---------

      when Attribute_Max =>
         Expand_Min_Max_Attribute (N);

      ----------------------------------
      -- Max_Size_In_Storage_Elements --
      ----------------------------------

      when Attribute_Max_Size_In_Storage_Elements => declare
         Typ : constant Entity_Id := Etype (N);

      begin
         --  Tranform T'Class'Max_Size_In_Storage_Elements (for any T) into
         --  Storage_Count'Pos (Storage_Count'Last), because it must include
         --  all descendants, which can be arbitrarily large. Note that the
         --  back end must not see any 'Class attribute references.
         --  The 'Pos is to make it be of type universal_integer.
         --
         --  ???If T'Class'Size is specified, it should probably affect
         --  T'Class'Max_Size_In_Storage_Elements accordingly.

         if Is_Entity_Name (Pref)
           and then Is_Class_Wide_Type (Entity (Pref))
         then
            declare
               Storage_Count_Type : constant Entity_Id :=
                 RTE (RE_Storage_Count);
               Attr : constant Node_Id :=
                 Make_Attribute_Reference (Loc,
                   Prefix => New_Occurrence_Of (Storage_Count_Type, Loc),
                   Attribute_Name => Name_Pos,
                   Expressions => New_List (
                     Make_Attribute_Reference (Loc,
                       Prefix => New_Occurrence_Of (Storage_Count_Type, Loc),
                       Attribute_Name => Name_Last)));
            begin
               Rewrite (N, Attr);
               Analyze_And_Resolve (N, Typ);
               return;
            end;

         --  Heap-allocated controlled objects contain two extra pointers which
         --  are not part of the actual type. Transform the attribute reference
         --  into a runtime expression to add the size of the hidden header.

         elsif Needs_Finalization (Ptyp)
           and then not Header_Size_Added (N)
         then
            Set_Header_Size_Added (N);

            --  Generate:
            --    P'Max_Size_In_Storage_Elements +
            --      Typ (Header_Size_With_Padding (Ptyp'Alignment))

            Rewrite (N,
              Make_Op_Add (Loc,
                Left_Opnd  => Relocate_Node (N),
                Right_Opnd =>
                  Convert_To (Typ,
                    Make_Function_Call (Loc,
                      Name                   =>
                        New_Occurrence_Of
                          (RTE (RE_Header_Size_With_Padding), Loc),

                      Parameter_Associations => New_List (
                        Make_Attribute_Reference (Loc,
                          Prefix         =>
                            New_Occurrence_Of (Ptyp, Loc),
                          Attribute_Name => Name_Alignment))))));

            Analyze_And_Resolve (N, Typ);
            return;
         end if;

         --  In the other cases apply the required checks

         Apply_Universal_Integer_Attribute_Checks (N);
      end;

      --------------------
      -- Mechanism_Code --
      --------------------

      when Attribute_Mechanism_Code =>

         --  We must replace the prefix in the renamed case

         if Is_Entity_Name (Pref)
           and then Present (Alias (Entity (Pref)))
         then
            Set_Renamed_Subprogram (Pref, Alias (Entity (Pref)));
         end if;

      ---------
      -- Min --
      ---------

      when Attribute_Min =>
         Expand_Min_Max_Attribute (N);

      ---------
      -- Mod --
      ---------

      when Attribute_Mod => Mod_Case : declare
         Arg  : constant Node_Id := Relocate_Node (First (Exprs));
         Hi   : constant Node_Id := Type_High_Bound (Base_Type (Etype (Arg)));
         Modv : constant Uint    := Modulus (Btyp);

      begin

         --  This is not so simple. The issue is what type to use for the
         --  computation of the modular value. In addition we need to use
         --  the base type as above to retrieve a static bound for the
         --  comparisons that follow.

         --  The easy case is when the modulus value is within the bounds
         --  of the signed integer type of the argument. In this case we can
         --  just do the computation in that signed integer type, and then
         --  do an ordinary conversion to the target type.

         if Modv <= Expr_Value (Hi) then
            Rewrite (N,
              Convert_To (Btyp,
                Make_Op_Mod (Loc,
                  Left_Opnd  => Arg,
                  Right_Opnd => Make_Integer_Literal (Loc, Modv))));

         --  Here we know that the modulus is larger than type'Last of the
         --  integer type. There are two cases to consider:

         --    a) The integer value is non-negative. In this case, it is
         --    returned as the result (since it is less than the modulus).

         --    b) The integer value is negative. In this case, we know that the
         --    result is modulus + value, where the value might be as small as
         --    -modulus. The trouble is what type do we use to do the subtract.
         --    No type will do, since modulus can be as big as 2**128, and no
         --    integer type accommodates this value. Let's do bit of algebra

         --         modulus + value
         --      =  modulus - (-value)
         --      =  (modulus - 1) - (-value - 1)

         --    Now modulus - 1 is certainly in range of the modular type.
         --    -value is in the range 1 .. modulus, so -value -1 is in the
         --    range 0 .. modulus-1 which is in range of the modular type.
         --    Furthermore, (-value - 1) can be expressed as -(value + 1)
         --    which we can compute using the integer base type.

         --  Once this is done we analyze the if expression without range
         --  checks, because we know everything is in range, and we want
         --  to prevent spurious warnings on either branch.

         else
            Rewrite (N,
              Make_If_Expression (Loc,
                Expressions => New_List (
                  Make_Op_Ge (Loc,
                    Left_Opnd  => Duplicate_Subexpr (Arg),
                    Right_Opnd => Make_Integer_Literal (Loc, 0)),

                  Convert_To (Btyp,
                    Duplicate_Subexpr_No_Checks (Arg)),

                  Make_Op_Subtract (Loc,
                    Left_Opnd =>
                      Make_Integer_Literal (Loc,
                        Intval => Modv - 1),
                    Right_Opnd =>
                      Convert_To (Btyp,
                        Make_Op_Minus (Loc,
                          Right_Opnd =>
                            Make_Op_Add (Loc,
                              Left_Opnd  => Duplicate_Subexpr_No_Checks (Arg),
                              Right_Opnd =>
                                Make_Integer_Literal (Loc,
                                  Intval => 1))))))));

         end if;

         Analyze_And_Resolve (N, Btyp, Suppress => All_Checks);
      end Mod_Case;

      -----------
      -- Model --
      -----------

      --  Transforms 'Model into a call to the floating-point attribute
      --  function Model in Fat_xxx (where xxx is the root type).
      --  Expansion is avoided for cases the back end can handle directly.

      when Attribute_Model =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      -----------------
      -- Object_Size --
      -----------------

      --  The processing for Object_Size shares the processing for Size

      ---------
      -- Old --
      ---------

      when Attribute_Old => Old : declare
         CW_Temp : Entity_Id;
         CW_Typ  : Entity_Id;
         Decl    : Node_Id;
         Ins_Nod : Node_Id;
         Temp    : Entity_Id;

         use Old_Attr_Util.Conditional_Evaluation;
         use Old_Attr_Util.Indirect_Temps;
      begin
         --  'Old can only appear in the case where local contract-related
         --  wrapper has been generated with the purpose of wrapping the
         --  original declarations and statements.

         Temp := Make_Temporary (Loc, 'T', Pref);

         --  Set the entity kind now in order to mark the temporary as a
         --  handler of attribute 'Old's prefix.

         Mutate_Ekind (Temp, E_Constant);
         Set_Stores_Attribute_Old_Prefix (Temp);

         --  Locate the insertion place of the internal temporary that saves
         --  the 'Old value.

         Ins_Nod := N;
         while Nkind (Ins_Nod) /= N_Subprogram_Body loop
            Ins_Nod := Parent (Ins_Nod);
         end loop;

         pragma Assert (Present (Wrapped_Statements
           (if Acts_As_Spec (Ins_Nod)
              then Defining_Unit_Name (Specification (Ins_Nod))
              else Corresponding_Spec (Ins_Nod))));

         Ins_Nod := Last (Declarations (Ins_Nod));

         if Eligible_For_Conditional_Evaluation (N) then
            declare
               Eval_Stmts : constant List_Id := New_List;

               procedure Append_For_Indirect_Temp
                 (N : Node_Id; Is_Eval_Stmt : Boolean);
               --  Append either a declaration (which is to be elaborated
               --  unconditionally) or an evaluation statement (which is
               --  to be executed conditionally).

               ------------------------------
               -- Append_For_Indirect_Temp --
               ------------------------------

               procedure Append_For_Indirect_Temp
                 (N : Node_Id; Is_Eval_Stmt : Boolean)
               is
               begin
                  if Is_Eval_Stmt then
                     Append_To (Eval_Stmts, N);
                  else
                     Insert_Before_And_Analyze (Ins_Nod, N);
                  end if;
               end Append_For_Indirect_Temp;

               procedure Declare_Indirect_Temporary is new
                 Declare_Indirect_Temp
                   (Append_Item => Append_For_Indirect_Temp);
            begin
               Declare_Indirect_Temporary
                 (Attr_Prefix => Pref, Indirect_Temp => Temp);

               Insert_After_And_Analyze (
                 Ins_Nod,
                 Make_If_Statement
                   (Sloc            => Loc,
                    Condition       => Conditional_Evaluation_Condition  (N),
                    Then_Statements => Eval_Stmts));

               Rewrite (N, Indirect_Temp_Value
                             (Temp => Temp,
                              Typ  => Etype (Pref),
                              Loc  => Loc));
               return;
            end;

         --  Preserve the tag of the prefix by offering a specific view of the
         --  class-wide version of the prefix.

         elsif Is_Tagged_Type (Typ) then

            --  Generate:
            --    CW_Temp : constant Typ'Class := Typ'Class (Pref);

            CW_Temp := Make_Temporary (Loc, 'T');
            CW_Typ  := Class_Wide_Type (Typ);

            Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier => CW_Temp,
                Constant_Present    => True,
                Object_Definition   => New_Occurrence_Of (CW_Typ, Loc),
                Expression          =>
                  Convert_To (CW_Typ, Relocate_Node (Pref)));

            Insert_Before_And_Analyze (Ins_Nod, Decl);

            --  Generate:
            --    Temp : Typ renames Typ (CW_Temp);

            Insert_Before_And_Analyze (Ins_Nod,
              Make_Object_Renaming_Declaration (Loc,
                Defining_Identifier => Temp,
                Subtype_Mark        => New_Occurrence_Of (Typ, Loc),
                Name                =>
                  Convert_To (Typ, New_Occurrence_Of (CW_Temp, Loc))));

            Set_Stores_Attribute_Old_Prefix (CW_Temp);

         --  Non-tagged case

         else
            --  Generate:
            --    Temp : constant Typ := Pref;

            Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier => Temp,
                Constant_Present    => True,
                Object_Definition   => New_Occurrence_Of (Typ, Loc),
                Expression          => Relocate_Node (Pref));

            Insert_Before_And_Analyze (Ins_Nod, Decl);

         end if;

         --  Ensure that the prefix of attribute 'Old is valid. The check must
         --  be inserted after the expansion of the attribute has taken place
         --  to reflect the new placement of the prefix.

         if Validity_Checks_On and then Validity_Check_Operands then

            --  Object declaration that captures the attribute prefix might
            --  be rewritten into object renaming declaration.

            if Nkind (Decl) = N_Object_Declaration then
               Ensure_Valid (Expression (Decl));
            else
               pragma Assert (Nkind (Decl) = N_Object_Renaming_Declaration
                              and then Is_Rewrite_Substitution (Decl));
               Ensure_Valid (Name (Decl));
            end if;
         end if;

         Rewrite (N, New_Occurrence_Of (Temp, Loc));
      end Old;

      ----------------------
      -- Overlaps_Storage --
      ----------------------

      when Attribute_Overlaps_Storage => Overlaps_Storage : declare
         X : constant Node_Id := Pref;
         Y : constant Node_Id := First (Exprs);
         --  The arguments

         X_Addr, Y_Addr : Node_Id;

         --  The expressions for their integer addresses

         X_Size, Y_Size : Node_Id;

         --  The expressions for their sizes

         Cond : Node_Id;

      begin
         --  Attribute expands into:

         --    (if X'Size = 0 or else Y'Size = 0 then
         --       False
         --     else
         --       (if X'Address <= Y'Address then
         --         (X'Address + X'Size - 1) >= Y'Address
         --        else
         --         (Y'Address + Y'Size - 1) >= X'Address))

         --  with the proper address operations. We convert addresses to
         --  integer addresses to use predefined arithmetic. The size is
         --  expressed in storage units. We add copies of X_Addr and Y_Addr
         --  to prevent the appearance of the same node in two places in
         --  the tree.

         X_Addr :=
           Unchecked_Convert_To (RTE (RE_Integer_Address),
             Make_Attribute_Reference (Loc,
               Attribute_Name => Name_Address,
               Prefix         => New_Copy_Tree (X)));

         Y_Addr :=
           Unchecked_Convert_To (RTE (RE_Integer_Address),
             Make_Attribute_Reference (Loc,
               Attribute_Name => Name_Address,
               Prefix         => New_Copy_Tree (Y)));

         X_Size :=
           Make_Op_Divide (Loc,
             Left_Opnd  =>
               Make_Attribute_Reference (Loc,
                 Attribute_Name => Name_Size,
                 Prefix         => New_Copy_Tree (X)),
             Right_Opnd =>
               Make_Integer_Literal (Loc, System_Storage_Unit));

         Y_Size :=
           Make_Op_Divide (Loc,
             Left_Opnd  =>
               Make_Attribute_Reference (Loc,
                 Attribute_Name => Name_Size,
                 Prefix         => New_Copy_Tree (Y)),
             Right_Opnd =>
               Make_Integer_Literal (Loc, System_Storage_Unit));

         Cond :=
            Make_Op_Le (Loc,
              Left_Opnd  => X_Addr,
              Right_Opnd => Y_Addr);

         --  Perform the rewriting

         Rewrite (N,
           Make_If_Expression (Loc, New_List (

             --  Generate a check for zero-sized things like a null record with
             --  size zero or an array with zero length since they have no
             --  opportunity of overlapping.

             --  Without this check, a zero-sized object can trigger a false
             --  runtime result if it's compared against another object in
             --  its declarative region, due to the zero-sized object having
             --  the same address.

             Make_Or_Else (Loc,
               Left_Opnd  =>
                 Make_Op_Eq (Loc,
                   Left_Opnd  =>
                     Make_Attribute_Reference (Loc,
                       Attribute_Name => Name_Size,
                       Prefix         => New_Copy_Tree (X)),
                   Right_Opnd => Make_Integer_Literal (Loc, 0)),
               Right_Opnd =>
                 Make_Op_Eq (Loc,
                   Left_Opnd  =>
                     Make_Attribute_Reference (Loc,
                       Attribute_Name => Name_Size,
                       Prefix         => New_Copy_Tree (Y)),
                   Right_Opnd => Make_Integer_Literal (Loc, 0))),

             New_Occurrence_Of (Standard_False, Loc),

             --  Non-zero-size overlap check

             Make_If_Expression (Loc, New_List (
               Cond,

               Make_Op_Ge (Loc,
                 Left_Opnd   =>
                   Make_Op_Add (Loc,
                    Left_Opnd  => New_Copy_Tree (X_Addr),
                     Right_Opnd =>
                       Make_Op_Subtract (Loc,
                         Left_Opnd  => X_Size,
                         Right_Opnd => Make_Integer_Literal (Loc, 1))),
                 Right_Opnd => Y_Addr),

               Make_Op_Ge (Loc,
                 Left_Opnd   =>
                   Make_Op_Add (Loc,
                     Left_Opnd  => New_Copy_Tree (Y_Addr),
                     Right_Opnd =>
                       Make_Op_Subtract (Loc,
                         Left_Opnd  => Y_Size,
                         Right_Opnd => Make_Integer_Literal (Loc, 1))),
                 Right_Opnd => X_Addr))))));

         Analyze_And_Resolve (N, Standard_Boolean);
      end Overlaps_Storage;

      ------------
      -- Output --
      ------------

      when Attribute_Output => Output : declare
         P_Type  : constant Entity_Id := Entity (Pref);
         U_Type  : constant Entity_Id := Underlying_Type (P_Type);
         Pname   : Entity_Id;
         Decl    : Node_Id;
         Prag    : Node_Id;
         Arg3    : Node_Id;
         Wfunc   : Node_Id;

      begin
         --  If no underlying type, we have an error that will be diagnosed
         --  elsewhere, so here we just completely ignore the expansion.

         if No (U_Type) then
            return;
         end if;

         --  Stream operations can appear in user code even if the restriction
         --  No_Streams is active (for example, when instantiating a predefined
         --  container). In that case rewrite the attribute as a Raise to
         --  prevent any run-time use.

         if Restriction_Active (No_Streams) then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Stream_Operation_Not_Allowed));
            Set_Etype (N, Standard_Void_Type);
            return;
         end if;

         --  If TSS for Output is present, just call it

         Pname := Find_Stream_Subprogram (P_Type, TSS_Stream_Output, N);

         if No (Pname) then

            --  If there is a Stream_Convert pragma, use it, we rewrite

            --     sourcetyp'Output (stream, Item)

            --  as

            --     strmtyp'Output (Stream, strmwrite (acttyp (Item)));

            --  where strmwrite is the given Write function that converts an
            --  argument of type sourcetyp or a type acctyp, from which it is
            --  derived to type strmtyp. The conversion to acttyp is required
            --  for the derived case.

            Prag := Get_Stream_Convert_Pragma (P_Type);

            if Present (Prag) then
               Arg3 :=
                 Next (Next (First (Pragma_Argument_Associations (Prag))));
               Wfunc := Entity (Expression (Arg3));

               Rewrite (N,
                 Make_Attribute_Reference (Loc,
                   Prefix => New_Occurrence_Of (Etype (Wfunc), Loc),
                   Attribute_Name => Name_Output,
                   Expressions => New_List (
                   Relocate_Node (First (Exprs)),
                     Make_Function_Call (Loc,
                       Name => New_Occurrence_Of (Wfunc, Loc),
                       Parameter_Associations => New_List (
                         OK_Convert_To (Etype (First_Formal (Wfunc)),
                           Relocate_Node (Next (First (Exprs)))))))));

               Analyze (N);
               return;

            --  Limited types

            elsif Default_Streaming_Unavailable (U_Type) then
               --  Do the same thing here as is done above in the
               --  case where a No_Streams restriction is active.

               Rewrite (N,
                 Make_Raise_Program_Error (Loc,
                   Reason => PE_Stream_Operation_Not_Allowed));
               Set_Etype (N, Standard_Void_Type);
               return;

            --  For elementary types, we call the W_xxx routine directly. Note
            --  that the effect of Write and Output is identical for the case
            --  of an elementary type (there are no discriminants or bounds).

            elsif Is_Elementary_Type (U_Type) then

               --  A special case arises if we have a defined _Write routine,
               --  since in this case we are required to call this routine.

               if Present (Find_Inherited_TSS (P_Type, TSS_Stream_Write)) then
                  Build_Record_Or_Elementary_Output_Procedure
                    (P_Type, Decl, Pname);
                  Insert_Action (N, Decl);

               --  For normal cases, we call the W_xxx routine directly

               else
                  Rewrite (N, Build_Elementary_Write_Call (N));
                  Analyze (N);
                  return;
               end if;

            --  Array type case

            elsif Is_Array_Type (U_Type) then
               declare
                  procedure Build_And_Insert_Array_Output_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Array_Output_Procedure);
               begin
                  Build_And_Insert_Array_Output_Proc
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;

            --  In the mutably tagged case, T'Class'Output calls T'Class'Write;
            --  T'Write will take care of writing out the external tag.

            elsif Is_Mutably_Tagged_Type (P_Type) then
               Set_Attribute_Name (N, Name_Write);
               Analyze (N);
               return;

            --  Class-wide case, first output external tag, then dispatch
            --  to the appropriate primitive Output function (RM 13.13.2(31)).

            elsif Is_Class_Wide_Type (P_Type) then

               --  No need to do anything else compiling under restriction
               --  No_Dispatching_Calls. During the semantic analysis we
               --  already notified such violation.

               if Restriction_Active (No_Dispatching_Calls) then
                  return;
               end if;

               Write_Controlling_Tag (P_Type);

               Pname := Find_Prim_Op (U_Type, TSS_Stream_Output);

            --  Tagged type case, use the primitive Output function

            elsif Is_Tagged_Type (U_Type) then
               Pname := Find_Prim_Op (U_Type, TSS_Stream_Output);

            --  All other record type cases, including protected records.
            --  The latter only arise for expander generated code for
            --  handling shared passive partition access.

            else
               pragma Assert
                 (Is_Record_Type (U_Type) or else Is_Protected_Type (U_Type));

               --  Ada 2005 (AI-216): Program_Error is raised when executing
               --  the default implementation of the Output attribute of an
               --  unchecked union type if the type lacks default discriminant
               --  values.

               if Is_Unchecked_Union (Base_Type (U_Type))
                 and then
                 No (Discriminant_Default_Value (First_Discriminant (U_Type)))
               then
                  Rewrite (N,
                    Make_Raise_Program_Error (Loc,
                      Reason => PE_Unchecked_Union_Restriction));
                  Set_Etype (N, Standard_Void_Type);
                  return;
               end if;

               declare
                  procedure Build_And_Insert_Record_Output_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Record_Or_Elementary_Output_Procedure);
               begin
                  Build_And_Insert_Record_Output_Proc
                    (Typ      => Base_Type (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;
            end if;
         end if;

         --  If we fall through, Pname is the name of the procedure to call

         Rewrite_Attribute_Proc_Call (Pname);

         if not Is_Tagged_Type (P_Type) then
            Cached_Attribute_Ops.Output_Map.Set (U_Type, Pname);
         end if;
      end Output;

      ---------
      -- Pos --
      ---------

      --  For enumeration types, with a non-standard representation we generate
      --  a call to the _Rep_To_Pos function created when the type was frozen.
      --  The call has the form:

      --    _rep_to_pos (expr, flag)

      --  The parameter flag is True if range checks are enabled, causing
      --  Program_Error to be raised if the expression has an invalid
      --  representation, and False if range checks are suppressed.

      --  For enumeration types with a standard representation, Pos can be
      --  rewritten as a simple conversion with Conversion_OK set.

      --  For integer types, Pos is equivalent to a simple integer conversion
      --  and we rewrite it as such.

      when Attribute_Pos => Pos : declare
         Expr : constant Node_Id := First (Exprs);
         Etyp : Entity_Id := Base_Type (Ptyp);

      begin
         --  Deal with zero/non-zero boolean values

         if Is_Boolean_Type (Etyp) then
            Adjust_Condition (Expr);
            Etyp := Standard_Boolean;
            Set_Prefix (N, New_Occurrence_Of (Standard_Boolean, Loc));
         end if;

         --  Case of enumeration type

         if Is_Enumeration_Type (Etyp) then

            --  Non-standard enumeration type (generate call)

            if Present (Enum_Pos_To_Rep (Etyp)) then
               Append_To (Exprs, Rep_To_Pos_Flag (Etyp, Loc));
               Rewrite (N,
                 Convert_To (Typ,
                   Make_Function_Call (Loc,
                     Name =>
                       New_Occurrence_Of (TSS (Etyp, TSS_Rep_To_Pos), Loc),
                     Parameter_Associations => Exprs)));

            --  Standard enumeration type (replace by conversion)

            --  This is simply a direct conversion from the enumeration type to
            --  the target integer type, which is treated by the back end as a
            --  normal integer conversion, treating the enumeration type as an
            --  integer, which is exactly what we want. We set Conversion_OK to
            --  make sure that the analyzer does not complain about what might
            --  be an illegal conversion.

            --  However the target type is universal integer in most cases,
            --  which is a very large type, so we first convert to a small
            --  signed integer type in order not to lose the size information.

            else
               Rewrite (N, OK_Convert_To (Get_Integer_Type (Ptyp), Expr));
               Convert_To_And_Rewrite (Typ, N);

            end if;

         --  Deal with integer types (replace by conversion)

         else
            Rewrite (N, Convert_To (Typ, Expr));
         end if;

         Analyze_And_Resolve (N, Typ);
      end Pos;

      --------------
      -- Position --
      --------------

      --  We leave the computation up to the back end, since we don't know what
      --  layout will be chosen if no component clause was specified.

      when Attribute_Position =>
         Apply_Universal_Integer_Attribute_Checks (N);

      ----------
      -- Pred --
      ----------

      --  1. Deal with enumeration types with holes.
      --  2. For floating-point, generate call to attribute function.
      --  3. For other cases, deal with constraint checking.

      when Attribute_Pred => Pred : declare
         Etyp : constant Entity_Id := Base_Type (Ptyp);

      begin
         --  For enumeration types with non-standard representations, we
         --  expand typ'Pred (x) into:

         --    Pos_To_Rep (Rep_To_Pos (x) - 1)

         --  if the representation is non-contiguous, and just x - 1 if it is
         --  after having dealt with constraint checking.

         if Is_Enumeration_Type (Etyp)
           and then Present (Enum_Pos_To_Rep (Etyp))
         then
            if Has_Contiguous_Rep (Etyp) then
               if not Range_Checks_Suppressed (Ptyp) then
                  Set_Do_Range_Check (First (Exprs), False);
                  Expand_Pred_Succ_Attribute (N);
               end if;

               Rewrite (N,
                 Unchecked_Convert_To (Etyp,
                    Make_Op_Subtract (Loc,
                       Left_Opnd  =>
                         Unchecked_Convert_To (
                           Integer_Type_For
                             (Esize (Etyp), Is_Unsigned_Type (Etyp)),
                           First (Exprs)),
                       Right_Opnd =>
                         Make_Integer_Literal (Loc, 1))));

            else
               --  Add Boolean parameter depending on check suppression

               Append_To (Exprs, Rep_To_Pos_Flag (Ptyp, Loc));
               Rewrite (N,
                 Make_Indexed_Component (Loc,
                   Prefix =>
                     New_Occurrence_Of
                       (Enum_Pos_To_Rep (Etyp), Loc),
                   Expressions => New_List (
                     Make_Op_Subtract (Loc,
                       Left_Opnd =>
                         Make_Function_Call (Loc,
                           Name =>
                             New_Occurrence_Of
                               (TSS (Etyp, TSS_Rep_To_Pos), Loc),
                           Parameter_Associations => Exprs),
                       Right_Opnd => Make_Integer_Literal (Loc, 1)))));
            end if;

            --  Suppress checks since they have all been done above

            Analyze_And_Resolve (N, Typ, Suppress => All_Checks);

         --  For floating-point, we transform 'Pred into a call to the Pred
         --  floating-point attribute function in Fat_xxx (xxx is root type).
         --  Note that this function takes care of the overflow case.

         elsif Is_Floating_Point_Type (Ptyp) then
            Expand_Fpt_Attribute_R (N);
            Analyze_And_Resolve (N, Typ);

         --  For modular types, nothing to do (no overflow, since wraps)

         elsif Is_Modular_Integer_Type (Ptyp) then
            null;

         --  For other types, if argument is marked as needing a range check or
         --  overflow checking is enabled, we must generate a check.

         elsif not Overflow_Checks_Suppressed (Ptyp)
           or else Do_Range_Check (First (Exprs))
         then
            Set_Do_Range_Check (First (Exprs), False);
            Expand_Pred_Succ_Attribute (N);
         end if;
      end Pred;

      ----------------------------------
      -- Preelaborable_Initialization --
      ----------------------------------

      when Attribute_Preelaborable_Initialization =>

         --  This attribute should already be folded during analysis, but if
         --  for some reason it hasn't been, we fold it now.

         Fold_Uint
           (N,
            UI_From_Int
              (Boolean'Pos (Has_Preelaborable_Initialization (Ptyp))),
            Static => False);

      --------------
      -- Priority --
      --------------

      --  Ada 2005 (AI-327): Dynamic ceiling priorities

      --  We rewrite X'Priority as the following run-time call:

      --     Get_Ceiling (X._Object)

      --  Note that although X'Priority is notionally an object, it is quite
      --  deliberately not defined as an aliased object in the RM. This means
      --  that it works fine to rewrite it as a call, without having to worry
      --  about complications that would other arise from X'Priority'Access,
      --  which is illegal, because of the lack of aliasing.

      when Attribute_Priority => Priority : declare
         Call        : Node_Id;
         New_Itype   : Entity_Id;
         Object_Parm : Node_Id;
         Prottyp     : Entity_Id;
         RT_Subprg   : RE_Id;
         Subprg      : Entity_Id;

      begin
         --  Look for the enclosing protected type

         Prottyp := Current_Scope;
         while not Is_Protected_Type (Prottyp) loop
            Prottyp := Scope (Prottyp);
         end loop;

         pragma Assert (Is_Protected_Type (Prottyp));

         --  Generate the actual of the call

         Subprg := Current_Scope;
         while not (Is_Subprogram_Or_Entry (Subprg)
                    and then Present (Protected_Body_Subprogram (Subprg)))
         loop
            Subprg := Scope (Subprg);
         end loop;

         --  Use of 'Priority inside protected entries and barriers (in both
         --  cases the type of the first formal of their expanded subprogram
         --  is Address).

         if Etype (First_Entity (Protected_Body_Subprogram (Subprg))) =
              RTE (RE_Address)
         then
            --  In the expansion of protected entries the type of the first
            --  formal of the Protected_Body_Subprogram is an Address. In order
            --  to reference the _object component we generate:

            --    type T is access p__ptTV;
            --    freeze T []

            New_Itype := Create_Itype (E_Access_Type, N);
            Set_Etype (New_Itype, New_Itype);
            Set_Directly_Designated_Type (New_Itype,
              Corresponding_Record_Type (Prottyp));
            Freeze_Itype (New_Itype, N);

            --  Generate:
            --    T!(O)._object'unchecked_access

            Object_Parm :=
              Make_Attribute_Reference (Loc,
                Prefix          =>
                  Make_Selected_Component (Loc,
                    Prefix        =>
                      Unchecked_Convert_To (New_Itype,
                        New_Occurrence_Of
                          (First_Entity (Protected_Body_Subprogram (Subprg)),
                           Loc)),
                    Selector_Name => Make_Identifier (Loc, Name_uObject)),
                 Attribute_Name => Name_Unchecked_Access);

         --  Use of 'Priority inside a protected subprogram

         else
            Object_Parm :=
              Make_Attribute_Reference (Loc,
                 Prefix         =>
                   Make_Selected_Component (Loc,
                     Prefix        =>
                       New_Occurrence_Of
                         (First_Entity (Protected_Body_Subprogram (Subprg)),
                         Loc),
                     Selector_Name => Make_Identifier (Loc, Name_uObject)),
                 Attribute_Name => Name_Unchecked_Access);
         end if;

         --  Select the appropriate run-time subprogram

         if Has_Entries (Prottyp) then
            RT_Subprg := RO_PE_Get_Ceiling;
         else
            RT_Subprg := RE_Get_Ceiling;
         end if;

         Call :=
           Make_Function_Call (Loc,
             Name                   =>
               New_Occurrence_Of (RTE (RT_Subprg), Loc),
             Parameter_Associations => New_List (Object_Parm));

         Rewrite (N, Call);

         --  Avoid the generation of extra checks on the pointer to the
         --  protected object.

         Analyze_And_Resolve (N, Typ, Suppress => Access_Check);
      end Priority;

      ---------------
      -- Put_Image --
      ---------------

      when Attribute_Put_Image => Put_Image : declare
         use Exp_Put_Image;
         U_Type : constant Entity_Id := Underlying_Type (Entity (Pref));
         C_Type : Entity_Id;
         Pname  : Entity_Id;
         Decl   : Node_Id;

      begin
         --  If no underlying type, we have an error that will be diagnosed
         --  elsewhere, so here we just completely ignore the expansion.

         if No (U_Type) then
            return;
         end if;

         --  If there is a TSS for Put_Image, just call it. This is true for
         --  tagged types (if enabled) and if there is a user-specified
         --  Put_Image.

         Pname := TSS (U_Type, TSS_Put_Image);
         if No (Pname) then
            if Is_Tagged_Type (U_Type) and then Is_Derived_Type (U_Type) then
               Pname := Find_Optional_Prim_Op (U_Type, TSS_Put_Image);
            else
               Pname := Find_Inherited_TSS (U_Type, TSS_Put_Image);
            end if;
         end if;

         if No (Pname) then
            if Is_String_Type (U_Type) then
               declare
                  R : constant Entity_Id := Root_Type (U_Type);

               begin
                  if Is_Private_Type (R) then
                     C_Type := Component_Type (Full_View (R));
                  else
                     C_Type := Component_Type (R);
                  end if;

                  C_Type := Root_Type (Underlying_Type (C_Type));
               end;
            end if;

            --  If Put_Image is disabled, call the "unknown" version

            if not Put_Image_Enabled (U_Type) then
               Rewrite (N, Build_Unknown_Put_Image_Call (N));
               Analyze (N);
               return;

            --  For elementary types, we call the routine in System.Put_Images
            --  directly.

            elsif Is_Elementary_Type (U_Type) then
               Rewrite (N, Build_Elementary_Put_Image_Call (N));
               Analyze (N);
               return;

            --  String type objects, including custom string types, and
            --  excluding C arrays.

            elsif Is_String_Type (U_Type)
              and then C_Type in Standard_Character
                               | Standard_Wide_Character
                               | Standard_Wide_Wide_Character
              and then (not RTU_Loaded (Interfaces_C)
                          or else Enclosing_Lib_Unit_Entity (U_Type)
                                    /= RTU_Entity (Interfaces_C))
            then
               Rewrite (N, Build_String_Put_Image_Call (N));
               Analyze (N);
               return;

            elsif Is_Array_Type (U_Type) then
               Pname := Cached_Attribute_Ops.Put_Image_Map.Get (U_Type);
               Cached_Attribute_Ops.Validate_Cached_Candidate
                 (Pname, Attr_Ref => N);
               if No (Pname) then
                  declare
                     procedure Build_And_Insert_Array_Put_Image_Proc is
                       new Build_And_Insert_Type_Attr_Subp
                             (Build_Array_Put_Image_Procedure);

                  begin
                     Build_And_Insert_Array_Put_Image_Proc
                       (Typ      => U_Type,
                        Decl     => Decl,
                           Subp     => Pname,
                           Attr_Ref => N);
                  end;

                  Cached_Attribute_Ops.Put_Image_Map.Set (U_Type, Pname);
               end if;

            --  Tagged type case, use the primitive Put_Image function. Note
            --  that this will dispatch in the class-wide case which is what we
            --  want.

            elsif Is_Tagged_Type (U_Type) then
               Pname := Find_Optional_Prim_Op (U_Type, TSS_Put_Image);

               --  ????Need Find_Optional_Prim_Op instead of Find_Prim_Op,
               --  because we might be deriving from a predefined type, which
               --  currently has Put_Image_Enabled False.

               if No (Pname) then
                  Rewrite (N, Build_Unknown_Put_Image_Call (N));
                  Analyze (N);
                  return;
               end if;

            elsif Is_Protected_Type (U_Type) then
               Rewrite (N, Build_Protected_Put_Image_Call (N));
               Analyze (N);
               return;

            elsif Is_Task_Type (U_Type) then
               Rewrite (N, Build_Task_Put_Image_Call (N));
               Analyze (N);
               return;

            --  All other record type cases

            else
               pragma Assert (Is_Record_Type (U_Type));
               declare
                  Base_Typ : constant Entity_Id := Full_Base (U_Type);
               begin
                  Pname := Cached_Attribute_Ops.Put_Image_Map.Get (Base_Typ);
                  Cached_Attribute_Ops.Validate_Cached_Candidate
                    (Pname, Attr_Ref => N);
                  if No (Pname) then
                     declare
                        procedure Build_And_Insert_Record_Put_Image_Proc is
                          new Build_And_Insert_Type_Attr_Subp
                                (Build_Record_Put_Image_Procedure);

                     begin
                        Build_And_Insert_Record_Put_Image_Proc
                          (Typ      => Base_Typ,
                           Decl     => Decl,
                           Subp     => Pname,
                           Attr_Ref => N);
                     end;

                     Cached_Attribute_Ops.Put_Image_Map.Set (Base_Typ, Pname);
                  end if;
               end;
            end if;
         end if;

         --  If we fall through, Pname is the procedure to be called

         Rewrite_Attribute_Proc_Call (Pname);
      end Put_Image;

      ------------------
      -- Range_Length --
      ------------------

      when Attribute_Range_Length =>

         --  The only special processing required is for the case where
         --  Range_Length is applied to an enumeration type with holes.
         --  In this case we transform

         --     X'Range_Length

         --  to

         --     X'Pos (X'Last) - X'Pos (X'First) + 1

         --  So that the result reflects the proper Pos values instead
         --  of the underlying representations.

         if Is_Enumeration_Type (Ptyp)
           and then Has_Non_Standard_Rep (Ptyp)
         then
            Rewrite (N,
              Make_Op_Add (Loc,
                Left_Opnd  =>
                  Make_Op_Subtract (Loc,
                    Left_Opnd  =>
                      Make_Attribute_Reference (Loc,
                        Attribute_Name => Name_Pos,
                        Prefix         => New_Occurrence_Of (Ptyp, Loc),
                        Expressions    => New_List (
                          Make_Attribute_Reference (Loc,
                            Attribute_Name => Name_Last,
                            Prefix         =>
                              New_Occurrence_Of (Ptyp, Loc)))),

                    Right_Opnd =>
                      Make_Attribute_Reference (Loc,
                        Attribute_Name => Name_Pos,
                        Prefix         => New_Occurrence_Of (Ptyp, Loc),
                        Expressions    => New_List (
                          Make_Attribute_Reference (Loc,
                            Attribute_Name => Name_First,
                            Prefix         =>
                              New_Occurrence_Of (Ptyp, Loc))))),

                Right_Opnd => Make_Integer_Literal (Loc, 1)));

            Analyze_And_Resolve (N, Typ);

         --  For all other cases, the attribute is handled by the back end, but
         --  we need to deal with the case of the range check on a universal
         --  integer.

         else
            Apply_Universal_Integer_Attribute_Checks (N);
         end if;

      ------------
      -- Reduce --
      ------------

      when Attribute_Reduce =>
         declare
            E1  : constant Node_Id   := First (Exprs);
            E2  : constant Node_Id   := Next (E1);
            Bnn : constant Entity_Id := Make_Temporary (Loc, 'B', N);

            Accum_Typ : Entity_Id := Empty;
            New_Loop  : Node_Id;

            function Build_Stat (Comp : Node_Id) return Node_Id;
            --  The reducer can be a function, a procedure whose first
            --  parameter is in-out, or an attribute that is a function,
            --  which (for now) can only be Min/Max. This subprogram
            --  builds the corresponding computation for the generated loop
            --  and retrieves the accumulator type as per RM 4.5.10(19/5).

            ----------------
            -- Build_Stat --
            ----------------

            function Build_Stat (Comp : Node_Id) return Node_Id is
               Stat : Node_Id;

            begin
               if Nkind (E1) = N_Attribute_Reference then
                  Accum_Typ := Base_Type (Entity (Prefix (E1)));
                  Stat := Make_Assignment_Statement (Loc,
                            Name => New_Occurrence_Of (Bnn, Loc),
                            Expression => Make_Attribute_Reference (Loc,
                              Attribute_Name => Attribute_Name (E1),
                              Prefix => New_Copy (Prefix (E1)),
                              Expressions => New_List (
                                New_Occurrence_Of (Bnn, Loc),
                                Comp)));

               elsif Ekind (Entity (E1)) = E_Procedure then
                  Accum_Typ := Etype (First_Formal (Entity (E1)));
                  Stat := Make_Procedure_Call_Statement (Loc,
                            Name => New_Occurrence_Of (Entity (E1), Loc),
                               Parameter_Associations => New_List (
                                 New_Occurrence_Of (Bnn, Loc),
                                 Comp));

               else
                  Accum_Typ := Etype (Entity (E1));
                  Stat := Make_Assignment_Statement (Loc,
                            Name => New_Occurrence_Of (Bnn, Loc),
                            Expression => Make_Function_Call (Loc,
                              Name => New_Occurrence_Of (Entity (E1), Loc),
                              Parameter_Associations => New_List (
                                New_Occurrence_Of (Bnn, Loc),
                                Comp)));
               end if;

               --  Try to cope if E1 is wrong because it is an overloaded
               --  subprogram that happens to be the first candidate
               --  on a homonym chain, but that resolution candidate turns
               --  out to be the wrong one.
               --  This workaround usually gets the right type, but it can
               --  yield the wrong subtype of that type.

               if Base_Type (Accum_Typ) /= Base_Type (Etype (N)) then
                  Accum_Typ := Etype (N);
               end if;

               --  Try to cope with wrong E1 when Etype (N) doesn't help
               if Is_Universal_Numeric_Type (Accum_Typ) then
                  if Is_Array_Type (Etype (Prefix (N))) then
                     Accum_Typ := Component_Type (Etype (Prefix (N)));
                  else
                     --  Further hackery can be added here when there is a
                     --  demonstrated need.
                     null;
                  end if;
               end if;

               return Stat;
            end Build_Stat;

         --  If the prefix is an aggregate, its unique component is an
         --  Iterated_Element, and we create a loop out of its iterator.
         --  The iterated_component_association is parsed as a loop parameter
         --  specification with "in" or as a container iterator with "of".

         begin
            if Nkind (Prefix (N)) = N_Aggregate then
               declare
                  Stream  : constant Node_Id :=
                              First (Component_Associations (Prefix (N)));
                  Expr    : constant Node_Id := Expression (Stream);
                  Id      : constant Node_Id := Defining_Identifier (Stream);
                  It_Spec : constant Node_Id :=
                                             Iterator_Specification (Stream);
                  Ch      : Node_Id;
                  Iter    : Node_Id;

               begin
                  --  Iteration may be given by an element iterator:

                  if Nkind (Stream) = N_Iterated_Component_Association
                      and then Present (It_Spec)
                      and then Of_Present (It_Spec)
                  then
                     Iter :=
                       Make_Iteration_Scheme (Loc,
                         Iterator_Specification =>
                           Relocate_Node (It_Spec),
                         Loop_Parameter_Specification => Empty);

                  else
                     Ch   := First (Discrete_Choices (Stream));
                     Iter :=
                      Make_Iteration_Scheme (Loc,
                        Iterator_Specification => Empty,
                        Loop_Parameter_Specification =>
                          Make_Loop_Parameter_Specification  (Loc,
                            Defining_Identifier => New_Copy (Id),
                            Discrete_Subtype_Definition =>
                              Relocate_Node (Ch)));
                  end if;

                  New_Loop := Make_Loop_Statement (Loc,
                    Iteration_Scheme => Iter,
                      End_Label => Empty,
                      Statements =>
                        New_List (Build_Stat (Relocate_Node (Expr))));
               end;

            else
               --  If the prefix is a name, we construct an element iterator
               --  over it. Its expansion will verify that it is an array or
               --  a container with the proper aspects.

               declare
                  Elem : constant Entity_Id := Make_Temporary (Loc, 'E', N);

                  Iter : Node_Id;

               begin
                  Iter :=
                    Make_Iterator_Specification (Loc,
                      Defining_Identifier => Elem,
                      Subtype_Indication  => Empty,
                      Of_Present          => True,
                      Name                => Relocate_Node (Prefix (N)));

                  New_Loop := Make_Loop_Statement (Loc,
                    Iteration_Scheme =>
                      Make_Iteration_Scheme (Loc,
                        Iterator_Specification => Iter,
                        Loop_Parameter_Specification => Empty),
                      End_Label => Empty,
                      Statements => New_List (
                        Build_Stat (New_Occurrence_Of (Elem, Loc))));

               end;
            end if;

            Rewrite (N,
               Make_Expression_With_Actions (Loc,
                 Actions    => New_List (
                   Make_Object_Declaration (Loc,
                     Defining_Identifier => Bnn,
                     Object_Definition   =>
                       New_Occurrence_Of (Accum_Typ, Loc),
                     Expression => Relocate_Node (E2)), New_Loop),
                 Expression => New_Occurrence_Of (Bnn, Loc)));

            Analyze_And_Resolve (N, Accum_Typ);
         end;

      ----------
      -- Read --
      ----------

      when Attribute_Read => Read : declare
         P_Type  : constant Entity_Id := Entity (Pref);
         B_Type  : constant Entity_Id := Base_Type (P_Type);
         U_Type  : constant Entity_Id := Underlying_Type (P_Type);
         Cntrl   : Node_Id := Empty; -- nonempty only if P_Type mutably tagged
         Pname   : Entity_Id;
         Decl    : Node_Id;
         Prag    : Node_Id;
         Arg2    : Node_Id;
         Rfunc   : Node_Id;
         Lhs     : Node_Id;
         Rhs     : Node_Id;

      begin
         --  If no underlying type, we have an error that will be diagnosed
         --  elsewhere, so here we just completely ignore the expansion.

         if No (U_Type) then
            return;
         end if;

         --  Stream operations can appear in user code even if the restriction
         --  No_Streams is active (for example, when instantiating a predefined
         --  container). In that case rewrite the attribute as a Raise to
         --  prevent any run-time use.

         if Restriction_Active (No_Streams) then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Stream_Operation_Not_Allowed));
            Set_Etype (N, B_Type);
            return;
         end if;

         --  The simple case, if there is a TSS for Read, just call it

         Pname := Find_Stream_Subprogram (P_Type, TSS_Stream_Read, N);

         if No (Pname) then

            --  If there is a Stream_Convert pragma, use it, we rewrite

            --     sourcetyp'Read (stream, Item)

            --  as

            --     Item := sourcetyp (strmread (strmtyp'Input (Stream)));

            --  where strmread is the given Read function that converts an
            --  argument of type strmtyp to type sourcetyp or a type from which
            --  it is derived. The conversion to sourcetyp is required in the
            --  latter case.

            --  A special case arises if Item is a type conversion in which
            --  case, we have to expand to:

            --     Itemx := typex (strmread (strmtyp'Input (Stream)));

            --  where Itemx is the expression of the type conversion (i.e.
            --  the actual object), and typex is the type of Itemx.

            Prag := Get_Stream_Convert_Pragma (P_Type);

            if Present (Prag) then
               Arg2  := Next (First (Pragma_Argument_Associations (Prag)));
               Rfunc := Entity (Expression (Arg2));
               Lhs := Relocate_Node (Next (First (Exprs)));
               Rhs :=
                 OK_Convert_To (B_Type,
                   Make_Function_Call (Loc,
                     Name => New_Occurrence_Of (Rfunc, Loc),
                     Parameter_Associations => New_List (
                       Make_Attribute_Reference (Loc,
                         Prefix =>
                           New_Occurrence_Of
                             (Etype (First_Formal (Rfunc)), Loc),
                         Attribute_Name => Name_Input,
                         Expressions => New_List (
                           Relocate_Node (First (Exprs)))))));

               if Nkind (Lhs) = N_Type_Conversion then
                  Lhs := Expression (Lhs);
                  Rhs := Convert_To (Etype (Lhs), Rhs);
               end if;

               Rewrite (N,
                 Make_Assignment_Statement (Loc,
                   Name       => Lhs,
                   Expression => Rhs));
               Set_Assignment_OK (Lhs);
               Analyze (N);
               return;

            --  Limited types

            elsif Default_Streaming_Unavailable (U_Type) then
               --  Do the same thing here as is done above in the
               --  case where a No_Streams restriction is active.

               Rewrite (N,
                 Make_Raise_Program_Error (Loc,
                   Reason => PE_Stream_Operation_Not_Allowed));
               Set_Etype (N, B_Type);
               return;

            --  For elementary types, we call the I_xxx routine using the first
            --  parameter and then assign the result into the second parameter.
            --  We set Assignment_OK to deal with the conversion case.

            elsif Is_Elementary_Type (U_Type) then
               declare
                  Lhs : Node_Id;
                  Rhs : Node_Id;

               begin
                  Lhs := Relocate_Node (Next (First (Exprs)));
                  Rhs := Build_Elementary_Input_Call (N);

                  if Nkind (Lhs) = N_Type_Conversion then
                     Lhs := Expression (Lhs);
                     Rhs := Convert_To (Etype (Lhs), Rhs);
                  end if;

                  Set_Assignment_OK (Lhs);

                  Rewrite (N,
                    Make_Assignment_Statement (Loc,
                      Name       => Lhs,
                      Expression => Rhs));

                  Analyze (N);
                  return;
               end;

            --  Array type case

            elsif Is_Array_Type (U_Type) then
               declare
                  procedure Build_And_Insert_Array_Read_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Array_Read_Procedure);
               begin
                  Build_And_Insert_Array_Read_Proc
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;

            --  Tagged type case, use the primitive Read function. Note that
            --  this will dispatch in the class-wide case which is what we want

            elsif Is_Tagged_Type (U_Type) then

               if Is_Mutably_Tagged_Type (U_Type) then
                  Read_Controlling_Tag (P_Type, Cntrl);
               end if;

               Pname := Find_Prim_Op (U_Type, TSS_Stream_Read);

            --  All other record type cases, including protected records. The
            --  latter only arise for expander generated code for handling
            --  shared passive partition access.

            else
               pragma Assert
                 (Is_Record_Type (U_Type) or else Is_Protected_Type (U_Type));

               --  Ada 2005 (AI-216): Program_Error is raised when executing
               --  the default implementation of the Read attribute of an
               --  Unchecked_Union type. We replace the attribute with a
               --  raise statement (rather than inserting it before) to handle
               --  properly the case of an unchecked union that is a record
               --  component.

               if Is_Unchecked_Union (Base_Type (U_Type)) then
                  Rewrite (N,
                    Make_Raise_Program_Error (Loc,
                      Reason => PE_Unchecked_Union_Restriction));
                  Set_Etype (N, B_Type);
                  return;
               end if;

               declare
                  procedure Build_Record_Read_Proc
                    (Typ  : Entity_Id;
                     Decl : out Node_Id;
                     Subp : out Entity_Id);

                  procedure Build_Record_Read_Proc
                    (Typ  : Entity_Id;
                     Decl : out Node_Id;
                     Subp : out Entity_Id) is
                  begin
                     if Has_Defaulted_Discriminants (Typ) then
                        Build_Mutable_Record_Read_Procedure
                          (Typ, Decl, Subp);
                     else
                        Build_Record_Read_Procedure
                          (Typ, Decl, Subp);
                     end if;
                  end Build_Record_Read_Proc;

                  procedure Build_And_Insert_Record_Read_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Record_Read_Proc);
               begin
                  Build_And_Insert_Record_Read_Proc
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;
            end if;
         end if;

         Rewrite_Attribute_Proc_Call (Pname);

         if Present (Cntrl) then
            pragma Assert (Is_Mutably_Tagged_Type (U_Type));
            pragma Assert (Nkind (N) = N_Procedure_Call_Statement);

            --  Assign the Tag value that was read from the stream
            --  to the tag of the out-mode actual parameter so that
            --  we dispatch correctly. This isn't quite right.
            --  We should assign a complete object (not just
            --  the tag), but that would require a dispatching call to
            --  perform default initialization of the source object and
            --  dispatching default init calls are currently not supported.

            declare
               function Select_Tag (Prefix : Node_Id) return Node_Id is
                 (Make_Selected_Component (Loc,
                    Prefix => Prefix,
                    Selector_Name =>
                      New_Occurrence_Of (First_Tag_Component
                                           (Etype (Prefix)), Loc)));

               Controlling_Actual : constant Node_Id :=
                 Next (First (Parameter_Associations (N)));

               pragma Assert (Is_Controlling_Actual (Controlling_Actual));

               Assign_Tag : Node_Id;
            begin
               Remove_Side_Effects (Controlling_Actual, Name_Req => True);

               Assign_Tag :=
                 Make_Assignment_Statement (Loc,
                   Name =>
                     Select_Tag (New_Copy_Tree (Controlling_Actual)),
                   Expression => Select_Tag (Cntrl));

               Insert_Before (Before => N, Node => Assign_Tag);
               Analyze (Assign_Tag);
            end;
         end if;

         if not Is_Tagged_Type (P_Type) then
            Cached_Attribute_Ops.Read_Map.Set (U_Type, Pname);
         end if;
      end Read;

      ---------
      -- Ref --
      ---------

      --  Ref is identical to To_Address, see To_Address for processing

      ---------------
      -- Remainder --
      ---------------

      --  Transforms 'Remainder into a call to the floating-point attribute
      --  function Remainder in Fat_xxx (where xxx is the root type)

      when Attribute_Remainder =>
         Expand_Fpt_Attribute_RR (N);

      ------------
      -- Result --
      ------------

      --  Transform 'Result into reference to _Result formal. At the point
      --  where a legal 'Result attribute is expanded, we know that we are in
      --  the context of a _Postcondition function with a _Result parameter.

      when Attribute_Result =>
         Rewrite (N, Make_Identifier (Loc, Chars => Name_uResult));
         Analyze_And_Resolve (N, Typ);

      -----------
      -- Round --
      -----------

      --  The handling of the Round attribute is delicate when the operand is
      --  universal fixed. In this case, the processing in Sem_Attr introduced
      --  a conversion to universal real, reflecting the semantics of Round,
      --  but we do not want anything to do with universal real at run time,
      --  since this corresponds to using floating-point arithmetic.

      --  What we have now is that the Etype of the Round attribute correctly
      --  indicates the final result type. The operand of the Round is the
      --  conversion to universal real, described above, and the operand of
      --  this conversion is the actual operand of Round, which may be the
      --  special case of a fixed point multiplication or division.

      --  The expander will expand first the operand of the conversion, then
      --  the conversion, and finally the round attribute itself, since we
      --  always work inside out. But we cannot simply process naively in this
      --  order. In the semantic world where universal fixed and real really
      --  exist and have infinite precision, there is no problem, but in the
      --  implementation world, where universal real is a floating-point type,
      --  we would get the wrong result.

      --  So the approach is as follows. When expanding a multiply or divide
      --  whose type is universal fixed, Fixup_Universal_Fixed_Operation will
      --  look up and skip the conversion to universal real if its parent is
      --  a Round attribute, taking information from this attribute node. In
      --  the other cases, Expand_N_Type_Conversion does the same by looking
      --  at its parent to see if it is a Round attribute, before calling the
      --  fixed-point expansion routine.

      --  This means that by the time we get to expanding the Round attribute
      --  itself, the Round is nothing more than a type conversion (and will
      --  often be a null type conversion), so we just replace it with the
      --  appropriate conversion operation.

      when Attribute_Round =>
         if Etype (First (Exprs)) = Etype (N) then
            Rewrite (N, Relocate_Node (First (Exprs)));
         else
            Rewrite (N, Convert_To (Etype (N), First (Exprs)));
            Set_Rounded_Result (N);
         end if;
         Analyze_And_Resolve (N);

      --------------
      -- Rounding --
      --------------

      --  Transforms 'Rounding into a call to the floating-point attribute
      --  function Rounding in Fat_xxx (where xxx is the root type)
      --  Expansion is avoided for cases the back end can handle directly.

      when Attribute_Rounding =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      -------------
      -- Scaling --
      -------------

      --  Transforms 'Scaling into a call to the floating-point attribute
      --  function Scaling in Fat_xxx (where xxx is the root type)

      when Attribute_Scaling =>
         Expand_Fpt_Attribute_RI (N);

      ----------------------------------------
      -- Simple_Storage_Pool & Storage_Pool --
      ----------------------------------------

      when Attribute_Simple_Storage_Pool | Attribute_Storage_Pool =>
         Rewrite (N,
           Make_Type_Conversion (Loc,
             Subtype_Mark => New_Occurrence_Of (Etype (N), Loc),
             Expression   => New_Occurrence_Of (Entity (N), Loc)));
         Analyze_And_Resolve (N, Typ);

      ----------
      -- Size --
      ----------

      when Attribute_Object_Size
         | Attribute_Size
         | Attribute_Value_Size
         | Attribute_VADS_Size
      =>
         Size : declare
            New_Node : Node_Id;

         begin
            --  Processing for VADS_Size case. Note that this processing
            --  removes all traces of VADS_Size from the tree, and completes
            --  all required processing for VADS_Size by translating the
            --  attribute reference to an appropriate Size or Object_Size
            --  reference.

            if Id = Attribute_VADS_Size
              or else (Use_VADS_Size and then Id = Attribute_Size)
            then
               --  If the size is specified, then we simply use the specified
               --  size. This applies to both types and objects. The size of an
               --  object can be specified in the following ways:

               --    An explicit size clause is given for an object
               --    A component size is specified for an indexed component
               --    A component clause is specified for a selected component
               --    The object is a component of a packed composite object

               --  If the size is specified, then VADS_Size of an object

               if (Is_Entity_Name (Pref)
                    and then Present (Size_Clause (Entity (Pref))))
                 or else
                   (Nkind (Pref) = N_Component_Clause
                     and then (Present (Component_Clause
                                        (Entity (Selector_Name (Pref))))
                                or else Is_Packed (Etype (Prefix (Pref)))))
                 or else
                   (Nkind (Pref) = N_Indexed_Component
                     and then (Known_Component_Size (Etype (Prefix (Pref)))
                                or else Is_Packed (Etype (Prefix (Pref)))))
               then
                  Set_Attribute_Name (N, Name_Size);

               --  Otherwise if we have an object rather than a type, then
               --  the VADS_Size attribute applies to the type of the object,
               --  rather than the object itself. This is one of the respects
               --  in which VADS_Size differs from Size.

               else
                  if (not Is_Entity_Name (Pref)
                       or else not Is_Type (Entity (Pref)))
                    and then (Is_Scalar_Type (Ptyp)
                               or else Is_Constrained (Ptyp))
                  then
                     Rewrite (Pref, New_Occurrence_Of (Ptyp, Loc));
                  end if;

                  --  For a scalar type for which no size was explicitly given,
                  --  VADS_Size means Object_Size. This is the other respect in
                  --  which VADS_Size differs from Size.

                  if Is_Scalar_Type (Ptyp)
                    and then No (Size_Clause (Ptyp))
                  then
                     Set_Attribute_Name (N, Name_Object_Size);

                  --  In all other cases, Size and VADS_Size are the same

                  else
                     Set_Attribute_Name (N, Name_Size);
                  end if;
               end if;
            end if;

            --  If the prefix is X'Class, transform it into a direct reference
            --  to the class-wide type, because the back end must not see a
            --  'Class reference.

            if Is_Entity_Name (Pref)
              and then Is_Class_Wide_Type (Entity (Pref))
            then
               Rewrite (Prefix (N), New_Occurrence_Of (Entity (Pref), Loc));
               return;

            --  For X'Size applied to an object of a class-wide type, transform
            --  X'Size into a call to the primitive operation _Size applied to
            --  X.

            elsif Is_Class_Wide_Type (Ptyp) then

               --  No need to do anything else compiling under restriction
               --  No_Dispatching_Calls. During the semantic analysis we
               --  already noted this restriction violation.

               if Restriction_Active (No_Dispatching_Calls) then
                  return;
               end if;

               New_Node :=
                 Make_Function_Call (Loc,
                   Name                  =>
                     New_Occurrence_Of (Find_Prim_Op (Ptyp, Name_uSize), Loc),
                  Parameter_Associations => New_List (Pref));

               if Typ /= Standard_Long_Long_Integer then

                  --  The context is a specific integer type with which the
                  --  original attribute was compatible. The function has a
                  --  specific type as well, so to preserve the compatibility
                  --  we must convert explicitly.

                  New_Node := Convert_To (Typ, New_Node);
               end if;

               Rewrite (N, New_Node);
               Analyze_And_Resolve (N, Typ);
               return;
            end if;

            --  Call Expand_Size_Attribute to do the final part of the
            --  expansion which is shared with GNATprove expansion.

            Expand_Size_Attribute (N);
         end Size;

      ------------------
      -- Storage_Size --
      ------------------

      when Attribute_Storage_Size => Storage_Size : declare
         Alloc_Op : Entity_Id := Empty;

      begin

         --  Access type case, always go to the root type

         --  The case of access types results in a value of zero for the case
         --  where no storage size attribute clause has been given. If a
         --  storage size has been given, then the attribute is converted
         --  to a reference to the variable used to hold this value.

         if Is_Access_Type (Ptyp) then
            if Present (Storage_Size_Variable (Root_Type (Ptyp))) then
               Rewrite (N,
                 Convert_To (Typ,
                   Make_Attribute_Reference (Loc,
                     Prefix => New_Occurrence_Of
                       (Etype (Storage_Size_Variable (Root_Type (Ptyp))), Loc),
                     Attribute_Name => Name_Max,
                     Expressions => New_List (
                       Make_Integer_Literal (Loc, 0),
                       New_Occurrence_Of
                         (Storage_Size_Variable (Root_Type (Ptyp)), Loc)))));

            elsif Present (Associated_Storage_Pool (Root_Type (Ptyp))) then

               --  If the access type is associated with a simple storage pool
               --  object, then attempt to locate the optional Storage_Size
               --  function of the simple storage pool type. If not found,
               --  then the result will default to zero.

               if Present (Get_Rep_Pragma (Root_Type (Ptyp),
                                           Name_Simple_Storage_Pool_Type))
               then
                  declare
                     Pool_Type : constant Entity_Id :=
                                   Base_Type (Etype (Entity (N)));

                  begin
                     Alloc_Op := Get_Name_Entity_Id (Name_Storage_Size);
                     while Present (Alloc_Op) loop
                        if Scope (Alloc_Op) = Scope (Pool_Type)
                          and then Present (First_Formal (Alloc_Op))
                          and then Etype (First_Formal (Alloc_Op)) = Pool_Type
                        then
                           exit;
                        end if;

                        Alloc_Op := Homonym (Alloc_Op);
                     end loop;
                  end;

               --  In the normal Storage_Pool case, retrieve the primitive
               --  function associated with the pool type.

               else
                  Alloc_Op :=
                    Find_Prim_Op
                      (Etype (Associated_Storage_Pool (Root_Type (Ptyp))),
                       Attribute_Name (N));
               end if;

               --  If Storage_Size wasn't found (can only occur in the simple
               --  storage pool case), then simply use zero for the result.

               if No (Alloc_Op) then
                  Rewrite (N, Make_Integer_Literal (Loc, 0));

               --  Otherwise, rewrite the allocator as a call to pool type's
               --  Storage_Size function.

               else
                  Rewrite (N,
                    Convert_To (Typ,
                      Make_Function_Call (Loc,
                        Name =>
                          New_Occurrence_Of (Alloc_Op, Loc),

                        Parameter_Associations => New_List (
                          New_Occurrence_Of
                            (Associated_Storage_Pool
                               (Root_Type (Ptyp)), Loc)))));
               end if;

            else
               Rewrite (N, Make_Integer_Literal (Loc, 0));
            end if;

            Analyze_And_Resolve (N, Typ);

         --  For tasks, we retrieve the size directly from the TCB. The
         --  size may depend on a discriminant of the type, and therefore
         --  can be a per-object expression, so type-level information is
         --  not sufficient in general. There are four cases to consider:

         --  a) If the attribute appears within a task body, the designated
         --    TCB is obtained by a call to Self.

         --  b) If the prefix of the attribute is the name of a task object,
         --  the designated TCB is the one stored in the corresponding record.

         --  c) If the prefix is a task type, the size is obtained from the
         --  size variable created for each task type

         --  d) If no Storage_Size was specified for the type, there is no
         --  size variable, and the value is a system-specific default.

         else
            if In_Open_Scopes (Ptyp) then

               --  Storage_Size (Self)

               Rewrite (N,
                 Convert_To (Typ,
                   Make_Function_Call (Loc,
                     Name =>
                       New_Occurrence_Of (RTE (RE_Storage_Size), Loc),
                     Parameter_Associations =>
                       New_List (
                         Make_Function_Call (Loc,
                           Name =>
                             New_Occurrence_Of (RTE (RE_Self), Loc))))));

            elsif not Is_Entity_Name (Pref)
              or else not Is_Type (Entity (Pref))
            then
               --  Storage_Size (Rec (Obj).Size)

               Rewrite (N,
                 Convert_To (Typ,
                   Make_Function_Call (Loc,
                     Name =>
                       New_Occurrence_Of (RTE (RE_Storage_Size), Loc),
                       Parameter_Associations =>
                          New_List (
                            Make_Selected_Component (Loc,
                              Prefix =>
                                Unchecked_Convert_To (
                                  Corresponding_Record_Type (Ptyp),
                                    New_Copy_Tree (Pref)),
                              Selector_Name =>
                                 Make_Identifier (Loc, Name_uTask_Id))))));

            elsif Present (Storage_Size_Variable (Ptyp)) then

               --  Static Storage_Size pragma given for type: retrieve value
               --  from its allocated storage variable.

               Rewrite (N,
                 Convert_To (Typ,
                   Make_Function_Call (Loc,
                     Name => New_Occurrence_Of (
                       RTE (RE_Adjust_Storage_Size), Loc),
                     Parameter_Associations =>
                       New_List (
                         New_Occurrence_Of (
                           Storage_Size_Variable (Ptyp), Loc)))));
            else
               --  Get system default

               Rewrite (N,
                 Convert_To (Typ,
                   Make_Function_Call (Loc,
                     Name =>
                       New_Occurrence_Of (
                        RTE (RE_Default_Stack_Size), Loc))));
            end if;

            Analyze_And_Resolve (N, Typ);
         end if;
      end Storage_Size;

      -----------------
      -- Stream_Size --
      -----------------

      when Attribute_Stream_Size =>
         Rewrite (N,
           Make_Integer_Literal (Loc, Intval => Get_Stream_Size (Ptyp)));
         Analyze_And_Resolve (N, Typ);

      ----------
      -- Succ --
      ----------

      --  1. Deal with enumeration types with holes.
      --  2. For floating-point, generate call to attribute function.
      --  3. For other cases, deal with constraint checking.

      when Attribute_Succ => Succ : declare
         Etyp : constant Entity_Id := Base_Type (Ptyp);

      begin
         --  For enumeration types with non-standard representations, we
         --  expand typ'Pred (x) into:

         --    Pos_To_Rep (Rep_To_Pos (x) + 1)

         --  if the representation is non-contiguous, and just x + 1 if it is
         --  after having dealt with constraint checking.

         if Is_Enumeration_Type (Etyp)
           and then Present (Enum_Pos_To_Rep (Etyp))
         then
            if Has_Contiguous_Rep (Etyp) then
               if not Range_Checks_Suppressed (Ptyp) then
                  Set_Do_Range_Check (First (Exprs), False);
                  Expand_Pred_Succ_Attribute (N);
               end if;

               Rewrite (N,
                 Unchecked_Convert_To (Etyp,
                    Make_Op_Add (Loc,
                       Left_Opnd  =>
                         Unchecked_Convert_To (
                           Integer_Type_For
                             (Esize (Etyp), Is_Unsigned_Type (Etyp)),
                           First (Exprs)),
                       Right_Opnd =>
                         Make_Integer_Literal (Loc, 1))));

            else
               --  Add Boolean parameter depending on check suppression

               Append_To (Exprs, Rep_To_Pos_Flag (Ptyp, Loc));
               Rewrite (N,
                 Make_Indexed_Component (Loc,
                   Prefix =>
                     New_Occurrence_Of
                       (Enum_Pos_To_Rep (Etyp), Loc),
                   Expressions => New_List (
                     Make_Op_Add (Loc,
                       Left_Opnd =>
                         Make_Function_Call (Loc,
                           Name =>
                             New_Occurrence_Of
                               (TSS (Etyp, TSS_Rep_To_Pos), Loc),
                           Parameter_Associations => Exprs),
                       Right_Opnd => Make_Integer_Literal (Loc, 1)))));
            end if;

            --  Suppress checks since they have all been done above

            Analyze_And_Resolve (N, Typ, Suppress => All_Checks);

         --  For floating-point, we transform 'Succ into a call to the Succ
         --  floating-point attribute function in Fat_xxx (xxx is root type).
         --  Note that this function takes care of the overflow case.

         elsif Is_Floating_Point_Type (Ptyp) then
            Expand_Fpt_Attribute_R (N);
            Analyze_And_Resolve (N, Typ);

         --  For modular types, nothing to do (no overflow, since wraps)

         elsif Is_Modular_Integer_Type (Ptyp) then
            null;

         --  For other types, if argument is marked as needing a range check or
         --  overflow checking is enabled, we must generate a check.

         elsif not Overflow_Checks_Suppressed (Ptyp)
           or else Do_Range_Check (First (Exprs))
         then
            Set_Do_Range_Check (First (Exprs), False);
            Expand_Pred_Succ_Attribute (N);
         end if;
      end Succ;

      ---------
      -- Tag --
      ---------

      --  Transforms X'Tag into a direct reference to the tag of X

      when Attribute_Tag => Tag : declare
         Ttyp           : Entity_Id;
         Prefix_Is_Type : Boolean;

      begin
         if Is_Entity_Name (Pref) and then Is_Type (Entity (Pref)) then
            Ttyp := Entity (Pref);
            Prefix_Is_Type := True;
         else
            Ttyp := Ptyp;
            Prefix_Is_Type := False;
         end if;

         --  In the case of a class-wide equivalent type without a parent,
         --  the _Tag component has been built in Make_CW_Equivalent_Type
         --  manually and must be referenced directly.

         if Ekind (Ttyp) = E_Class_Wide_Subtype
           and then Present (Equivalent_Type (Ttyp))
           and then No (Parent_Subtype (Equivalent_Type (Ttyp)))
         then
            Ttyp := Equivalent_Type (Ttyp);

         --  In all the other cases of class-wide type, including an equivalent
         --  type with a parent, the _Tag component ultimately present is that
         --  of the root type.

         elsif Is_Class_Wide_Type (Ttyp) then
            Ttyp := Root_Type (Ttyp);
         end if;

         Ttyp := Underlying_Type (Ttyp);

         --  Ada 2005: The type may be a synchronized tagged type, in which
         --  case the tag information is stored in the corresponding record.

         if Is_Concurrent_Type (Ttyp) then
            Ttyp := Corresponding_Record_Type (Ttyp);
         end if;

         if Prefix_Is_Type then

            --  For VMs we leave the type attribute unexpanded because
            --  there's not a dispatching table to reference.

            if Tagged_Type_Expansion then
               Rewrite (N,
                 Unchecked_Convert_To (RTE (RE_Tag),
                   New_Occurrence_Of
                     (Node (First_Elmt (Access_Disp_Table (Ttyp))), Loc)));
               Analyze_And_Resolve (N, RTE (RE_Tag));
            end if;

         --  Ada 2005 (AI-251): The use of 'Tag in the sources always
         --  references the primary tag of the actual object. If 'Tag is
         --  applied to class-wide interface objects we generate code that
         --  displaces "this" to reference the base of the object.

         elsif Comes_From_Source (N)
            and then Is_Class_Wide_Type (Etype (Prefix (N)))
            and then Is_Interface (Underlying_Type (Etype (Prefix (N))))
         then
            --  Generate:
            --    (To_Tag_Ptr (Prefix'Address)).all

            --  Note that Prefix'Address is recursively expanded into a call
            --  to Base_Address (Obj.Tag)

            --  Not needed for VM targets, since all handled by the VM

            if Tagged_Type_Expansion then
               Rewrite (N,
                 Make_Explicit_Dereference (Loc,
                   Unchecked_Convert_To (RTE (RE_Tag_Ptr),
                     Make_Attribute_Reference (Loc,
                       Prefix => Relocate_Node (Pref),
                       Attribute_Name => Name_Address))));
               Analyze_And_Resolve (N, RTE (RE_Tag));
            end if;

         else
            Rewrite (N,
              Make_Selected_Component (Loc,
                Prefix => Relocate_Node (Pref),
                Selector_Name =>
                  New_Occurrence_Of (First_Tag_Component (Ttyp), Loc)));
            Analyze_And_Resolve (N, RTE (RE_Tag));
         end if;
      end Tag;

      ----------------
      -- Terminated --
      ----------------

      --  Transforms 'Terminated attribute into a call to Terminated function

      when Attribute_Terminated => Terminated : begin

         --  The prefix of Terminated is of a task interface class-wide type.
         --  Generate:
         --    terminated (Task_Id (_disp_get_task_id (Pref)));

         if Ada_Version >= Ada_2005
           and then Ekind (Ptyp) = E_Class_Wide_Type
           and then Is_Interface (Ptyp)
           and then Is_Task_Interface (Ptyp)
         then
            Rewrite (N,
              Make_Function_Call (Loc,
                Name                   =>
                  New_Occurrence_Of (RTE (RE_Terminated), Loc),
                Parameter_Associations => New_List (
                  Unchecked_Convert_To
                    (RTE (RO_ST_Task_Id),
                     Build_Disp_Get_Task_Id_Call (Pref)))));

         elsif Restricted_Profile then
            Rewrite (N,
              Build_Call_With_Task (Pref, RTE (RE_Restricted_Terminated)));

         else
            Rewrite (N,
              Build_Call_With_Task (Pref, RTE (RE_Terminated)));
         end if;

         Analyze_And_Resolve (N, Standard_Boolean);
      end Terminated;

      ----------------
      -- To_Address --
      ----------------

      --  Transforms System'To_Address (X) and System.Address'Ref (X) into
      --  unchecked conversion from (integral) type of X to type address. If
      --  the To_Address is a static expression, the transformed expression
      --  also needs to be static, because we do some legality checks (e.g.
      --  for Thread_Local_Storage) after this transformation.

      when Attribute_Ref
         | Attribute_To_Address
      =>
         To_Address : declare
            Is_Static : constant Boolean := Is_Static_Expression (N);

         begin
            Rewrite (N,
              Unchecked_Convert_To (RTE (RE_Address),
                Relocate_Node (First (Exprs))));
            Set_Is_Static_Expression (N, Is_Static);

            Analyze_And_Resolve (N, RTE (RE_Address));
         end To_Address;

      ------------
      -- To_Any --
      ------------

      when Attribute_To_Any => To_Any : declare
         Decls : constant List_Id := New_List;
      begin
         Rewrite (N,
           Build_To_Any_Call
             (Loc,
              Convert_To (Ptyp,
              Relocate_Node (First (Exprs))), Decls));
         Insert_Actions (N, Decls);
         Analyze_And_Resolve (N, RTE (RE_Any));
      end To_Any;

      ----------------
      -- Truncation --
      ----------------

      --  Transforms 'Truncation into a call to the floating-point attribute
      --  function Truncation in Fat_xxx (where xxx is the root type).
      --  Expansion is avoided for cases the back end can handle directly.

      when Attribute_Truncation =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      --------------
      -- TypeCode --
      --------------

      when Attribute_TypeCode => TypeCode : declare
         Decls : constant List_Id := New_List;
      begin
         Rewrite (N, Build_TypeCode_Call (Loc, Ptyp, Decls));
         Insert_Actions (N, Decls);
         Analyze_And_Resolve (N, RTE (RE_TypeCode));
      end TypeCode;

      -----------------------
      -- Unbiased_Rounding --
      -----------------------

      --  Transforms 'Unbiased_Rounding into a call to the floating-point
      --  attribute function Unbiased_Rounding in Fat_xxx (where xxx is the
      --  root type). Expansion is avoided for cases the back end can handle
      --  directly.

      when Attribute_Unbiased_Rounding =>
         if not Is_Inline_Floating_Point_Attribute (N) then
            Expand_Fpt_Attribute_R (N);
         end if;

      ------------
      -- Update --
      ------------

      when Attribute_Update =>
         Expand_Update_Attribute (N);

      ---------------
      -- VADS_Size --
      ---------------

      --  The processing for VADS_Size is shared with Size

      ---------
      -- Val --
      ---------

      --  For enumeration types with a non-standard representation we use the
      --  _Pos_To_Rep array that was created when the type was frozen, unless
      --  the representation is contiguous in which case we use an addition.

      --  For enumeration types with a standard representation, Val can be
      --  rewritten as a simple conversion with Conversion_OK set.

      --  For integer types, Val is equivalent to a simple integer conversion
      --  and we rewrite it as such.

      when Attribute_Val => Val : declare
         Etyp : constant Entity_Id := Base_Type (Ptyp);
         Expr : constant Node_Id := First (Exprs);
         Rtyp : Entity_Id;

      begin
         --  Case of enumeration type

         if Is_Enumeration_Type (Etyp) then

            --  Non-contiguous non-standard enumeration type

            if Present (Enum_Pos_To_Rep (Etyp))
              and then not Has_Contiguous_Rep (Etyp)
            then
               Rewrite (N,
                 Make_Indexed_Component (Loc,
                   Prefix =>
                     New_Occurrence_Of (Enum_Pos_To_Rep (Etyp), Loc),
                   Expressions => New_List (
                     Convert_To (Standard_Integer, Expr))));

               Analyze_And_Resolve (N, Typ);

            --  Standard or contiguous non-standard enumeration type

            else
               --  If the argument is marked as requiring a range check then
               --  generate it here, after looking through a conversion to
               --  universal integer, if any.

               if Do_Range_Check (Expr) then
                  if Present (Enum_Pos_To_Rep (Etyp)) then
                     Rtyp := Enum_Pos_To_Rep (Etyp);
                  else
                     Rtyp := Etyp;
                  end if;

                  if Nkind (Expr) = N_Type_Conversion
                     and then Entity (Subtype_Mark (Expr)) = Universal_Integer
                  then
                     Generate_Range_Check
                       (Expression (Expr), Rtyp, CE_Range_Check_Failed);

                  else
                     Generate_Range_Check (Expr, Rtyp, CE_Range_Check_Failed);
                  end if;

                  Set_Do_Range_Check (Expr, False);
               end if;

               --  Contiguous non-standard enumeration type

               if Present (Enum_Pos_To_Rep (Etyp)) then
                  Rewrite (N,
                    Unchecked_Convert_To (Etyp,
                      Make_Op_Add (Loc,
                        Left_Opnd =>
                          Make_Integer_Literal (Loc,
                            Enumeration_Rep (First_Literal (Etyp))),
                        Right_Opnd =>
                          Unchecked_Convert_To (
                            Integer_Type_For
                              (Esize (Etyp), Is_Unsigned_Type (Etyp)),
                            Expr))));

               --  Standard enumeration type

               else
                  Rewrite (N, OK_Convert_To (Typ, Expr));
               end if;

               --  Suppress checks since the range check was done above
               --  and it guarantees that the addition cannot overflow.

               Analyze_And_Resolve (N, Typ, Suppress => All_Checks);
            end if;

         --  Deal with integer types

         elsif Is_Integer_Type (Etyp) then
            Rewrite (N, Convert_To (Typ, Expr));
            Analyze_And_Resolve (N, Typ);
         end if;
      end Val;

      -----------
      -- Valid --
      -----------

      --  The code for valid is dependent on the particular types involved.
      --  See separate sections below for the generated code in each case.

      when Attribute_Valid => Valid : declare
         PBtyp : Entity_Id := Implementation_Base_Type (Validated_View (Ptyp));
         pragma Assert (Is_Scalar_Type (PBtyp)
                          or else Serious_Errors_Detected > 0);

         --  The scalar base type, looking through private types

         Save_Validity_Checks_On : constant Boolean := Validity_Checks_On;
         --  Save the validity checking mode. We always turn off validity
         --  checking during process of 'Valid since this is one place
         --  where we do not want the implicit validity checks to interfere
         --  with the explicit validity check that the programmer is doing.

         function Make_Range_Test return Node_Id;
         --  Build the code for a range test of the form
         --    PBtyp!(Pref) in PBtyp!(Ptyp'First) .. PBtyp!(Ptyp'Last)

         ---------------------
         -- Make_Range_Test --
         ---------------------

         function Make_Range_Test return Node_Id is
            Temp : Node_Id;

         begin
            --  The prefix of attribute 'Valid should always denote an object
            --  reference. The reference is either coming directly from source
            --  or is produced by validity check expansion. The object may be
            --  wrapped in a conversion in which case the call to Unqual_Conv
            --  will yield it.

            --  If the prefix denotes a variable which captures the value of
            --  an object for validation purposes, use the variable in the
            --  range test. This ensures that no extra copies or extra reads
            --  are produced as part of the test. Generate:

            --    Temp : ... := Object;
            --    if not Temp in ... then

            if Is_Validation_Variable_Reference (Pref) then
               Temp := New_Occurrence_Of (Entity (Unqual_Conv (Pref)), Loc);

            --  Otherwise the prefix is either a source object or a constant
            --  produced by validity check expansion. Generate:

            --    Temp : constant ... := Pref;
            --    if not Temp in ... then

            else
               Temp := Duplicate_Subexpr (Pref);
            end if;

            declare
               Val_Typ : constant Entity_Id := Validated_View (Ptyp);
            begin
               return
                 Make_In (Loc,
                   Left_Opnd  => Unchecked_Convert_To (PBtyp, Temp),
                   Right_Opnd =>
                     Make_Range (Loc,
                       Low_Bound  =>
                         Unchecked_Convert_To (PBtyp,
                           Make_Attribute_Reference (Loc,
                             Prefix         =>
                               New_Occurrence_Of (Val_Typ, Loc),
                             Attribute_Name => Name_First)),
                       High_Bound =>
                         Unchecked_Convert_To (PBtyp,
                           Make_Attribute_Reference (Loc,
                             Prefix         =>
                               New_Occurrence_Of (Val_Typ, Loc),
                             Attribute_Name => Name_Last))));
            end;
         end Make_Range_Test;

         --  Local variables

         Tst : Node_Id;

      --  Start of processing for Attribute_Valid

      begin
         --  Do not expand sourced code 'Valid reference in CodePeer mode,
         --  will be handled by the back-end directly.

         if CodePeer_Mode and then Comes_From_Source (N) then
            return;
         end if;

         --  Turn off validity checks. We do not want any implicit validity
         --  checks to intefere with the explicit check from the attribute

         Validity_Checks_On := False;

         --  Floating-point case. This case is handled by the Valid attribute
         --  code in the floating-point attribute run-time library.

         if Is_Floating_Point_Type (PBtyp) then
            Float_Valid : declare
               Pkg : RE_Id;
               Ftp : Entity_Id;

               function Get_Fat_Entity (Nam : Name_Id) return Entity_Id;
               --  Return entity for Pkg.Nam

               --------------------
               -- Get_Fat_Entity --
               --------------------

               function Get_Fat_Entity (Nam : Name_Id) return Entity_Id is
                  Exp_Name : constant Node_Id :=
                    Make_Selected_Component (Loc,
                      Prefix        => New_Occurrence_Of (RTE (Pkg), Loc),
                      Selector_Name => Make_Identifier (Loc, Nam));
               begin
                  Find_Selected_Component (Exp_Name);
                  return Entity (Exp_Name);
               end Get_Fat_Entity;

            --  Start of processing for Float_Valid

            begin
               Find_Fat_Info (PBtyp, Ftp, Pkg);

               --  If the prefix is a reverse SSO component, or is possibly
               --  unaligned, first create a temporary copy that is in
               --  native SSO, and properly aligned. Make it Volatile to
               --  prevent folding in the back-end. Note that we use an
               --  intermediate constrained string type to initialize the
               --  temporary, as the value at hand might be invalid, and in
               --  that case it cannot be copied using a floating point
               --  register.

               if In_Reverse_Storage_Order_Object (Pref)
                 or else Is_Possibly_Unaligned_Object (Pref)
               then
                  declare
                     Temp : constant Entity_Id :=
                              Make_Temporary (Loc, 'F');

                     Fat_S : constant Entity_Id :=
                               Get_Fat_Entity (Name_S);
                     --  Constrained string subtype of appropriate size

                     Fat_P : constant Entity_Id :=
                               Get_Fat_Entity (Name_P);
                     --  Access to Fat_S

                     Decl : constant Node_Id :=
                              Make_Object_Declaration (Loc,
                                   Defining_Identifier => Temp,
                                   Aliased_Present     => True,
                                Object_Definition   =>
                                  New_Occurrence_Of (Ptyp, Loc));

                  begin
                     Set_Aspect_Specifications (Decl, New_List (
                       Make_Aspect_Specification (Loc,
                         Identifier =>
                           Make_Identifier (Loc, Name_Volatile))));

                     Insert_Actions (N,
                       New_List (
                         Decl,

                         Make_Assignment_Statement (Loc,
                           Name =>
                             Make_Explicit_Dereference (Loc,
                               Prefix =>
                                 Unchecked_Convert_To (Fat_P,
                                   Make_Attribute_Reference (Loc,
                                     Prefix =>
                                       New_Occurrence_Of (Temp, Loc),
                                     Attribute_Name =>
                                       Name_Unrestricted_Access))),
                           Expression =>
                             Unchecked_Convert_To (Fat_S,
                               Relocate_Node (Pref)))),

                       Suppress => All_Checks);

                     Rewrite (Pref, New_Occurrence_Of (Temp, Loc));
                  end;
               end if;

               --  We now have an object of the proper endianness and
               --  alignment, and can construct a Valid attribute.

               --  We make sure the prefix of this valid attribute is
               --  marked as not coming from source, to avoid losing
               --  warnings from 'Valid looking like a possible update.

               Set_Comes_From_Source (Pref, False);

               Expand_Fpt_Attribute
                 (N, Pkg, Name_Valid,
                  New_List (
                    Make_Attribute_Reference (Loc,
                      Prefix         => Unchecked_Convert_To (Ftp, Pref),
                      Attribute_Name => Name_Unrestricted_Access)));

               --  One more task, we still need a range check. Required
               --  only if we have a constraint, since the Valid routine
               --  catches infinities properly (infinities are never valid).

               --  The way we do the range check is simply to create the
               --  expression: Valid (N) and then Base_Type(Pref) in Typ.

               if not Subtypes_Statically_Match (Ptyp, PBtyp) then
                  Rewrite (N,
                    Make_And_Then (Loc,
                      Left_Opnd  => Relocate_Node (N),
                      Right_Opnd =>
                        Make_In (Loc,
                          Left_Opnd  => Convert_To (PBtyp, Pref),
                          Right_Opnd => New_Occurrence_Of (Ptyp, Loc))));
               end if;
            end Float_Valid;

         --  Enumeration type with holes

         --  For enumeration types with holes, the Pos value constructed by
         --  the Enum_Rep_To_Pos function built in Exp_Ch3 called with a
         --  second argument of False returns minus one for an invalid value,
         --  and the non-negative pos value for a valid value, so the
         --  expansion of X'Valid is simply:

         --     type(X)'Pos (X) >= 0

         --  We can't quite generate it that way because of the requirement
         --  for the non-standard second argument of False in the resulting
         --  rep_to_pos call, so we have to explicitly create:

         --     _rep_to_pos (X, False) >= 0

         --  If we have an enumeration subtype, we also check that the
         --  value is in range:

         --    _rep_to_pos (X, False) >= 0
         --      and then
         --       (X >= type(X)'First and then type(X)'Last <= X)

         elsif Is_Enumeration_Type (Ptyp)
           and then Present (Enum_Pos_To_Rep (PBtyp))
         then
            Tst :=
              Make_Op_Ge (Loc,
                Left_Opnd =>
                  Make_Function_Call (Loc,
                    Name =>
                      New_Occurrence_Of (TSS (PBtyp, TSS_Rep_To_Pos), Loc),
                    Parameter_Associations => New_List (
                      Pref,
                      New_Occurrence_Of (Standard_False, Loc))),
                Right_Opnd => Make_Integer_Literal (Loc, 0));

            --  Skip the range test for boolean types, as it buys us
            --  nothing. The function called above already fails for
            --  values different from both True and False.

            if Ptyp /= PBtyp and then not Is_Boolean_Type (PBtyp)
              and then
                (Type_Low_Bound (Ptyp) /= Type_Low_Bound (PBtyp)
                  or else
                 Type_High_Bound (Ptyp) /= Type_High_Bound (PBtyp))
            then
               --  The call to Make_Range_Test will create declarations
               --  that need a proper insertion point, but Pref is now
               --  attached to a node with no ancestor. Attach to tree
               --  even if it is to be rewritten below.

               Set_Parent (Tst, Parent (N));

               Tst :=
                 Make_And_Then (Loc,
                   Left_Opnd  => Make_Range_Test,
                   Right_Opnd => Tst);
            end if;

            Rewrite (N, Tst);

         --  Fortran convention booleans

         --  For the very special case of Fortran convention booleans, the
         --  value is always valid, since it is an integer with the semantics
         --  that non-zero is true, and any value is permissible.

         elsif Is_Boolean_Type (Ptyp)
           and then Convention (Ptyp) = Convention_Fortran
         then
            Rewrite (N, New_Occurrence_Of (Standard_True, Loc));

         --  For biased representations, we will be doing an unchecked
         --  conversion without unbiasing the result. That means that the range
         --  test has to take this into account, and the proper form of the
         --  test is:

         --    PBtyp!(Pref) < PBtyp!(Ptyp'Range_Length)

         elsif Has_Biased_Representation (Ptyp) then
            PBtyp := RTE (RE_Unsigned_32);
            Rewrite (N,
              Make_Op_Lt (Loc,
                Left_Opnd =>
                  Unchecked_Convert_To (PBtyp, Duplicate_Subexpr (Pref)),
                Right_Opnd =>
                  Unchecked_Convert_To (PBtyp,
                    Make_Attribute_Reference (Loc,
                      Prefix => New_Occurrence_Of (Ptyp, Loc),
                      Attribute_Name => Name_Range_Length))));

         --  For all other scalar types, what we want logically is a
         --  range test:

         --     X in type(X)'First .. type(X)'Last

         --  But that's precisely what won't work because of possible
         --  unwanted optimization (and indeed the basic motivation for
         --  the Valid attribute is exactly that this test does not work).
         --  What will work is:

         --     PBtyp!(X) >= PBtyp!(type(X)'First)
         --       and then
         --     PBtyp!(X) <= PBtyp!(type(X)'Last)

         --  where PBtyp is an integer type large enough to cover the full
         --  range of possible stored values (i.e. it is chosen on the basis
         --  of the size of the type, not the range of the values). We write
         --  this as two tests, rather than a range check, so that static
         --  evaluation will easily remove either or both of the checks if
         --  they can be statically determined to be true (this happens
         --  when the type of X is static and the range extends to the full
         --  range of stored values).

         --  Unsigned types. Note: it is safe to consider only whether the
         --  subtype is unsigned, since we will in that case be doing all
         --  unsigned comparisons based on the subtype range. Since we use the
         --  actual subtype object size, this is appropriate.

         --  For example, if we have

         --    subtype x is integer range 1 .. 200;
         --    for x'Object_Size use 8;

         --  Now the base type is signed, but objects of this type are bits
         --  unsigned, and doing an unsigned test of the range 1 to 200 is
         --  correct, even though a value greater than 127 looks signed to a
         --  signed comparison.

         else
            declare
               Uns  : constant Boolean :=
                 Is_Unsigned_Type (Validated_View (Ptyp));

               Size : Uint;
               P    : Node_Id := Pref;

            begin
               --  If the prefix is an object, use the Esize from this object
               --  to handle in a more user friendly way the case of objects
               --  or components with a large Size aspect: if a Size aspect is
               --  specified, we want to read a scalar value as large as the
               --  Size, unless the Size is larger than
               --  System_Max_Integer_Size.

               if Nkind (P) = N_Selected_Component then
                  P := Selector_Name (P);
               end if;

               if Nkind (P) in N_Has_Entity
                 and then Present (Entity (P))
                 and then Is_Object (Entity (P))
                 and then Known_Esize (Entity (P))
               then
                  if Esize (Entity (P)) <= System_Max_Integer_Size then
                     Size := Esize (Entity (P));
                  else
                     Size := UI_From_Int (System_Max_Integer_Size);
                  end if;
               else
                  Size := Esize (Ptyp);
               end if;

               PBtyp := Small_Integer_Type_For (Size, Uns);
               Rewrite (N, Make_Range_Test);
            end;
         end if;

         --  If a predicate is present, then we do the predicate test, even if
         --  within the predicate function (infinite recursion is warned about
         --  in Sem_Attr in that case).

         declare
            Pred_Func : constant Entity_Id := Predicate_Function (Ptyp);

         begin
            if Present (Pred_Func) then
               Rewrite (N,
                 Make_And_Then (Loc,
                   Left_Opnd  => Relocate_Node (N),
                   Right_Opnd => Make_Predicate_Call (Ptyp, Pref)));
            end if;
         end;

         Analyze_And_Resolve (N, Standard_Boolean);
         Validity_Checks_On := Save_Validity_Checks_On;
      end Valid;

      -----------------
      -- Valid_Value --
      -----------------

      when Attribute_Valid_Value =>
         Exp_Imgv.Expand_Valid_Value_Attribute (N);

      -------------------
      -- Valid_Scalars --
      -------------------

      when Attribute_Valid_Scalars => Valid_Scalars : declare
         Val_Typ : constant Entity_Id := Validated_View (Ptyp);
         Expr    : Node_Id;

      begin
         --  Assume that the prefix does not need validation

         Expr := Empty;

         --  Attribute 'Valid_Scalars is not supported on private tagged types;
         --  see a detailed explanation where this attribute is analyzed.

         if Is_Private_Type (Ptyp) and then Is_Tagged_Type (Ptyp) then
            null;

         --  Attribute 'Valid_Scalars evaluates to True when the type lacks
         --  scalars.

         elsif not Scalar_Part_Present (Val_Typ) then
            null;

         --  Attribute 'Valid_Scalars is the same as attribute 'Valid when the
         --  validated type is a scalar type. Generate:

         --    Val_Typ (Pref)'Valid

         elsif Is_Scalar_Type (Val_Typ) then
            Expr :=
              Make_Attribute_Reference (Loc,
                Prefix         =>
                  Unchecked_Convert_To (Val_Typ, New_Copy_Tree (Pref)),
                Attribute_Name => Name_Valid);

            --  Required by LLVM although the sizes are the same???

            if Nkind (Prefix (Expr)) = N_Unchecked_Type_Conversion then
               Set_No_Truncation (Prefix (Expr));
            end if;

         --  Validate the scalar components of an array by iterating over all
         --  dimensions of the array while checking individual components.

         elsif Is_Array_Type (Val_Typ) then
            Expr :=
              Make_Function_Call (Loc,
                Name                   =>
                  New_Occurrence_Of
                    (Build_Array_VS_Func
                      (Attr       => N,
                       Formal_Typ => Ptyp,
                       Array_Typ  => Val_Typ),
                    Loc),
                Parameter_Associations => New_List (Pref));

         --  Validate the scalar components, discriminants of a record type by
         --  examining the structure of a record type.

         elsif Is_Record_Type (Val_Typ) then
            Expr :=
              Make_Function_Call (Loc,
                Name                   =>
                  New_Occurrence_Of
                    (Build_Record_VS_Func
                      (Attr       => N,
                       Formal_Typ => Ptyp,
                       Rec_Typ    => Val_Typ),
                    Loc),
                Parameter_Associations => New_List (Pref));
         end if;

         --  Default the attribute to True when the type of the prefix does not
         --  need validation.

         if No (Expr) then
            Expr := New_Occurrence_Of (Standard_True, Loc);
         end if;

         Rewrite (N, Expr);
         Analyze_And_Resolve (N, Standard_Boolean);
         Set_Is_Static_Expression (N, False);
      end Valid_Scalars;

      -----------
      -- Value --
      -----------

      when Attribute_Value =>
         Exp_Imgv.Expand_Value_Attribute (N);

      -----------------
      -- Value_Size --
      -----------------

      --  The processing for Value_Size shares the processing for Size

      -------------
      -- Version --
      -------------

      --  The processing for Version shares the processing for Body_Version

      ----------------
      -- Wide_Image --
      ----------------

      when Attribute_Wide_Image =>
         --  Leave attribute unexpanded in CodePeer mode: the gnat2scil
         --  back-end knows how to handle this attribute directly.

         if CodePeer_Mode then
            return;
         end if;

         Exp_Imgv.Expand_Wide_Image_Attribute (N);

      ---------------------
      -- Wide_Wide_Image --
      ---------------------

      when Attribute_Wide_Wide_Image =>
         --  Leave attribute unexpanded in CodePeer mode: the gnat2scil
         --  back-end knows how to handle this attribute directly.

         if CodePeer_Mode then
            return;
         end if;

         Exp_Imgv.Expand_Wide_Wide_Image_Attribute (N);

      ----------------
      -- Wide_Value --
      ----------------

      --  We expand typ'Wide_Value (X) into

      --    typ'Value
      --      (Wide_String_To_String (X, Wide_Character_Encoding_Method))

      --  Wide_String_To_String is a runtime function that converts its wide
      --  string argument to String, converting any non-translatable characters
      --  into appropriate escape sequences. This preserves the required
      --  semantics of Wide_Value in all cases, and results in a very simple
      --  implementation approach.

      --  Note: for this approach to be fully standard compliant for the cases
      --  where typ is Wide_Character and Wide_Wide_Character, the encoding
      --  method must cover the entire character range (e.g. UTF-8). But that
      --  is a reasonable requirement when dealing with encoded character
      --  sequences. Presumably if one of the restrictive encoding mechanisms
      --  is in use such as Shift-JIS, then characters that cannot be
      --  represented using this encoding will not appear in any case.

      when Attribute_Wide_Value =>
         Rewrite (N,
           Make_Attribute_Reference (Loc,
             Prefix         => Pref,
             Attribute_Name => Name_Value,

             Expressions    => New_List (
               Make_Function_Call (Loc,
                 Name =>
                   New_Occurrence_Of
                     (RTE (if Is_User_Defined_Enumeration_Type (Typ)
                           then RE_Enum_Wide_String_To_String
                           else RE_Wide_String_To_String), Loc),

                 Parameter_Associations => New_List (
                   Relocate_Node (First (Exprs)),
                   Make_Integer_Literal (Loc,
                     Intval => Int (Wide_Character_Encoding_Method)))))));

         Analyze_And_Resolve (N, Typ);

      ---------------------
      -- Wide_Wide_Value --
      ---------------------

      --  We expand typ'Wide_Value_Value (X) into

      --    typ'Value
      --      (Wide_Wide_String_To_String (X, Wide_Character_Encoding_Method))

      --  See Wide_Value for more information. This is not quite right where
      --  typ = Wide_Wide_Character, because the encoding method may not cover
      --  the whole character type.

      when Attribute_Wide_Wide_Value =>
         Rewrite (N,
           Make_Attribute_Reference (Loc,
             Prefix         => Pref,
             Attribute_Name => Name_Value,

             Expressions    => New_List (
               Make_Function_Call (Loc,
                 Name                   =>
                   New_Occurrence_Of
                     (RTE (if Is_User_Defined_Enumeration_Type (Typ)
                           then RE_Enum_Wide_Wide_String_To_String
                           else RE_Wide_Wide_String_To_String), Loc),

                 Parameter_Associations => New_List (
                   Relocate_Node (First (Exprs)),
                   Make_Integer_Literal (Loc,
                     Intval => Int (Wide_Character_Encoding_Method)))))));

         Analyze_And_Resolve (N, Typ);

      ---------------------
      -- Wide_Wide_Width --
      ---------------------

      when Attribute_Wide_Wide_Width =>
         Exp_Imgv.Expand_Width_Attribute (N, Wide_Wide);

      ----------------
      -- Wide_Width --
      ----------------

      when Attribute_Wide_Width =>
         Exp_Imgv.Expand_Width_Attribute (N, Wide);

      -----------
      -- Width --
      -----------

      when Attribute_Width =>
         Exp_Imgv.Expand_Width_Attribute (N, Normal);

      -----------
      -- Write --
      -----------

      when Attribute_Write => Write : declare
         P_Type  : constant Entity_Id := Entity (Pref);
         U_Type  : constant Entity_Id := Underlying_Type (P_Type);
         Pname   : Entity_Id;
         Decl    : Node_Id;
         Prag    : Node_Id;
         Arg3    : Node_Id;
         Wfunc   : Node_Id;

      begin
         --  If no underlying type, we have an error that will be diagnosed
         --  elsewhere, so here we just completely ignore the expansion.

         if No (U_Type) then
            return;
         end if;

         --  Stream operations can appear in user code even if the restriction
         --  No_Streams is active (for example, when instantiating a predefined
         --  container). In that case rewrite the attribute as a Raise to
         --  prevent any run-time use.

         if Restriction_Active (No_Streams) then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Stream_Operation_Not_Allowed));
            Set_Etype (N, U_Type);
            return;
         end if;

         --  The simple case, if there is a TSS for Write, just call it

         Pname := Find_Stream_Subprogram (P_Type, TSS_Stream_Write, N);

         if No (Pname) then

            --  If there is a Stream_Convert pragma, use it, we rewrite

            --     sourcetyp'Output (stream, Item)

            --  as

            --     strmtyp'Output (Stream, strmwrite (acttyp (Item)));

            --  where strmwrite is the given Write function that converts an
            --  argument of type sourcetyp or a type acctyp, from which it is
            --  derived to type strmtyp. The conversion to acttyp is required
            --  for the derived case.

            Prag := Get_Stream_Convert_Pragma (P_Type);

            if Present (Prag) then
               Arg3 :=
                 Next (Next (First (Pragma_Argument_Associations (Prag))));
               Wfunc := Entity (Expression (Arg3));

               Rewrite (N,
                 Make_Attribute_Reference (Loc,
                   Prefix => New_Occurrence_Of (Etype (Wfunc), Loc),
                   Attribute_Name => Name_Output,
                   Expressions => New_List (
                     Relocate_Node (First (Exprs)),
                     Make_Function_Call (Loc,
                       Name => New_Occurrence_Of (Wfunc, Loc),
                       Parameter_Associations => New_List (
                         OK_Convert_To (Etype (First_Formal (Wfunc)),
                           Relocate_Node (Next (First (Exprs)))))))));

               Analyze (N);
               return;

            --  Limited types

            elsif Default_Streaming_Unavailable (U_Type) then
               --  Do the same thing here as is done above in the
               --  case where a No_Streams restriction is active.

               Rewrite (N,
                 Make_Raise_Program_Error (Loc,
                   Reason => PE_Stream_Operation_Not_Allowed));
               Set_Etype (N, U_Type);
               return;

            --  For elementary types, we call the W_xxx routine directly

            elsif Is_Elementary_Type (U_Type) then
               Rewrite (N, Build_Elementary_Write_Call (N));
               Analyze (N);
               return;

            --  Array type case

            elsif Is_Array_Type (U_Type) then
               declare
                  procedure Build_And_Insert_Array_Write_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Array_Write_Procedure);
               begin
                  Build_And_Insert_Array_Write_Proc
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;

            --  Tagged type case, use the primitive Write function. Note that
            --  this will dispatch in the class-wide case which is what we want

            elsif Is_Tagged_Type (U_Type) then

               --  If T'Class is mutably tagged, then the external tag
               --  is written out by T'Class'Write, not by T'Class'Output.

               if Is_Mutably_Tagged_Type (U_Type) then
                  Write_Controlling_Tag (P_Type);
               end if;

               Pname := Find_Prim_Op (U_Type, TSS_Stream_Write);

            --  All other record type cases, including protected records.
            --  The latter only arise for expander generated code for
            --  handling shared passive partition access.

            else
               pragma Assert
                 (Is_Record_Type (U_Type) or else Is_Protected_Type (U_Type));

               --  Ada 2005 (AI-216): Program_Error is raised when executing
               --  the default implementation of the Write attribute of an
               --  Unchecked_Union type. However, if the 'Write reference is
               --  within the generated Output stream procedure, Write outputs
               --  the components, and the default values of the discriminant
               --  are streamed by the Output procedure itself. If there are
               --  no default values this is also erroneous.

               if Is_Unchecked_Union (Base_Type (U_Type)) then
                  if (not Is_TSS (Current_Scope, TSS_Stream_Output)
                       and not Is_TSS (Current_Scope, TSS_Stream_Write))
                    or else No (Discriminant_Default_Value
                                 (First_Discriminant (U_Type)))
                  then
                     Rewrite (N,
                       Make_Raise_Program_Error (Loc,
                         Reason => PE_Unchecked_Union_Restriction));
                     Set_Etype (N, U_Type);
                     return;
                  end if;
               end if;

               declare
                  procedure Build_Record_Write_Proc
                    (Typ  : Entity_Id;
                     Decl : out Node_Id;
                     Subp : out Entity_Id);

                  procedure Build_Record_Write_Proc
                    (Typ  : Entity_Id;
                     Decl : out Node_Id;
                     Subp : out Entity_Id) is
                  begin
                     if Has_Defaulted_Discriminants (Typ) then
                        Build_Mutable_Record_Write_Procedure
                          (Typ, Decl, Subp);
                     else
                        Build_Record_Write_Procedure
                          (Typ, Decl, Subp);
                     end if;
                  end Build_Record_Write_Proc;

                  procedure Build_And_Insert_Record_Write_Proc is
                    new Build_And_Insert_Type_Attr_Subp
                          (Build_Record_Write_Proc);
               begin
                  Build_And_Insert_Record_Write_Proc
                    (Typ      => Full_Base (U_Type),
                     Decl     => Decl,
                     Subp     => Pname,
                     Attr_Ref => N);
               end;
            end if;
         end if;

         --  If we fall through, Pname is the procedure to be called

         Rewrite_Attribute_Proc_Call (Pname);

         if not Is_Tagged_Type (P_Type) then
            Cached_Attribute_Ops.Write_Map.Set (U_Type, Pname);
         end if;
      end Write;

      --  The following attributes are handled by the back end (except that
      --  static cases have already been evaluated during semantic processing,
      --  but in any case the back end should not count on this).

      when Attribute_Code_Address
         | Attribute_Deref
         | Attribute_Null_Parameter
         | Attribute_Passed_By_Reference
         | Attribute_Pool_Address
      =>
         null;

      --  The following attributes should not appear at this stage, since they
      --  have already been handled by the analyzer (and properly rewritten
      --  with corresponding values or entities to represent the right values).

      when Attribute_Abort_Signal
         | Attribute_Address_Size
         | Attribute_Aft
         | Attribute_Atomic_Always_Lock_Free
         | Attribute_Base
         | Attribute_Bit_Order
         | Attribute_Class
         | Attribute_Compiler_Version
         | Attribute_Default_Bit_Order
         | Attribute_Default_Scalar_Storage_Order
         | Attribute_Definite
         | Attribute_Delta
         | Attribute_Denorm
         | Attribute_Digits
         | Attribute_Emax
         | Attribute_Enabled
         | Attribute_Epsilon
         | Attribute_Fast_Math
         | Attribute_First_Valid
         | Attribute_Has_Access_Values
         | Attribute_Has_Discriminants
         | Attribute_Has_Tagged_Values
         | Attribute_Large
         | Attribute_Last_Valid
         | Attribute_Library_Level
         | Attribute_Machine_Emax
         | Attribute_Machine_Emin
         | Attribute_Machine_Mantissa
         | Attribute_Machine_Overflows
         | Attribute_Machine_Radix
         | Attribute_Machine_Rounds
         | Attribute_Max_Alignment_For_Allocation
         | Attribute_Max_Integer_Size
         | Attribute_Maximum_Alignment
         | Attribute_Model_Emin
         | Attribute_Model_Epsilon
         | Attribute_Model_Mantissa
         | Attribute_Model_Small
         | Attribute_Modulus
         | Attribute_Partition_ID
         | Attribute_Range
         | Attribute_Restriction_Set
         | Attribute_Safe_Emax
         | Attribute_Safe_First
         | Attribute_Safe_Large
         | Attribute_Safe_Last
         | Attribute_Safe_Small
         | Attribute_Scalar_Storage_Order
         | Attribute_Scale
         | Attribute_Signed_Zeros
         | Attribute_Small
         | Attribute_Small_Denominator
         | Attribute_Small_Numerator
         | Attribute_Storage_Unit
         | Attribute_Stub_Type
         | Attribute_Super
         | Attribute_System_Allocator_Alignment
         | Attribute_Target_Name
         | Attribute_Type_Class
         | Attribute_Type_Key
         | Attribute_Unconstrained_Array
         | Attribute_Universal_Literal_String
         | Attribute_Wchar_T_Size
         | Attribute_Word_Size
      =>
         raise Program_Error;
      end case;

   --  Note: as mentioned earlier, individual sections of the above case
   --  statement assume there is no code after the case statement, and are
   --  legitimately allowed to execute return statements if they have nothing
   --  more to do, so DO NOT add code at this point.

   exception
      when RE_Not_Available =>
         null;
   end Expand_N_Attribute_Reference;

   --------------------------------
   -- Expand_Pred_Succ_Attribute --
   --------------------------------

   --  For typ'Pred (exp), we generate the check

   --    [constraint_error when exp = typ'Base'First]

   --  Similarly, for typ'Succ (exp), we generate the check

   --    [constraint_error when exp = typ'Base'Last]

   --  These checks are not generated for modular types, since the proper
   --  semantics for Succ and Pred on modular types is to wrap, not raise CE.
   --  We also suppress these checks if we are the right side of an assignment
   --  statement or the expression of an object declaration, where the flag
   --  Suppress_Assignment_Checks is set for the assignment/declaration.

   procedure Expand_Pred_Succ_Attribute (N : Node_Id) is
      Loc  : constant Source_Ptr := Sloc (N);
      P    : constant Node_Id    := Parent (N);
      Cnam : Name_Id;

   begin
      if Attribute_Name (N) = Name_Pred then
         Cnam := Name_First;
      else
         Cnam := Name_Last;
      end if;

      if Nkind (P) not in N_Assignment_Statement | N_Object_Declaration
        or else not Suppress_Assignment_Checks (P)
      then
         Insert_Action (N,
           Make_Raise_Constraint_Error (Loc,
             Condition =>
               Make_Op_Eq (Loc,
                 Left_Opnd =>
                   Duplicate_Subexpr_Move_Checks (First (Expressions (N))),
                 Right_Opnd =>
                   Make_Attribute_Reference (Loc,
                     Prefix =>
                       New_Occurrence_Of (Base_Type (Etype (Prefix (N))), Loc),
                     Attribute_Name => Cnam)),
             Reason => CE_Overflow_Check_Failed));
      end if;
   end Expand_Pred_Succ_Attribute;

   ---------------------------
   -- Expand_Size_Attribute --
   ---------------------------

   procedure Expand_Size_Attribute (N : Node_Id) is
      Loc  : constant Source_Ptr   := Sloc (N);
      Typ  : constant Entity_Id    := Etype (N);
      Pref : constant Node_Id      := Prefix (N);
      Ptyp : constant Entity_Id    := Etype (Pref);
      Id   : constant Attribute_Id := Get_Attribute_Id (Attribute_Name (N));
      Siz  : Uint;

   begin
      --  Case of known RM_Size of a type

      if Id in Attribute_Size | Attribute_Value_Size
        and then Is_Entity_Name (Pref)
        and then Is_Type (Entity (Pref))
        and then Known_Static_RM_Size (Entity (Pref))
      then
         Siz := RM_Size (Entity (Pref));

      --  Case of known Esize of a type

      elsif Id = Attribute_Object_Size
        and then Is_Entity_Name (Pref)
        and then Is_Type (Entity (Pref))
        and then Known_Static_Esize (Entity (Pref))
      then
         Siz := Esize (Entity (Pref));

      --  Case of known size of object

      elsif Id = Attribute_Size
        and then Is_Entity_Name (Pref)
        and then Is_Object (Entity (Pref))
        and then Known_Static_Esize (Entity (Pref))
      then
         Siz := Esize (Entity (Pref));

      --  For an array component, we can do Size in the front end if the
      --  component_size of the array is set.

      elsif Nkind (Pref) = N_Indexed_Component then
         Siz := Component_Size (Etype (Prefix (Pref)));

      --  For a record component, we can do Size in the front end if there is a
      --  component clause, or if the record is packed and the component's size
      --  is known at compile time.

      elsif Nkind (Pref) = N_Selected_Component then
         declare
            Rec  : constant Entity_Id := Etype (Prefix (Pref));
            Comp : constant Entity_Id := Entity (Selector_Name (Pref));

         begin
            if Present (Component_Clause (Comp)) then
               Siz := Esize (Comp);

            elsif Is_Packed (Rec) then
               Siz := RM_Size (Ptyp);

            else
               Apply_Universal_Integer_Attribute_Checks (N);
               return;
            end if;
         end;

      --  All other cases are handled by the back end

      else
         --  If Size is applied to a formal parameter that is of a packed
         --  array subtype, then apply Size to the actual subtype.

         if Is_Entity_Name (Pref)
           and then Is_Formal (Entity (Pref))
           and then Is_Packed_Array (Ptyp)
         then
            Rewrite (N,
              Make_Attribute_Reference (Loc,
                Prefix         =>
                  New_Occurrence_Of (Get_Actual_Subtype (Pref), Loc),
                Attribute_Name => Name_Size));
            Analyze_And_Resolve (N, Typ);

         --  If Size is applied to a dereference of an access to unconstrained
         --  packed array, the back end needs to see its unconstrained nominal
         --  type, but also a hint to the actual constrained type.

         elsif Nkind (Pref) = N_Explicit_Dereference
           and then Is_Packed_Array (Ptyp)
           and then not Is_Constrained (Ptyp)
         then
            Set_Actual_Designated_Subtype (Pref, Get_Actual_Subtype (Pref));

         --  If Size was applied to a slice of a bit-packed array, we rewrite
         --  it into the product of Length and Component_Size. We need to do so
         --  because bit-packed arrays are represented internally as arrays of
         --  System.Unsigned_Types.Packed_Byte for code generation purposes so
         --  the size is always rounded up in the back end.

         elsif Nkind (Pref) = N_Slice and then Is_Bit_Packed_Array (Ptyp) then
            Rewrite (N,
              Make_Op_Multiply (Loc,
                Make_Attribute_Reference (Loc,
                  Prefix         => Duplicate_Subexpr (Pref, Name_Req => True),
                  Attribute_Name => Name_Length),
                Make_Attribute_Reference (Loc,
                  Prefix         => Duplicate_Subexpr (Pref, Name_Req => True),
                  Attribute_Name => Name_Component_Size)));
            Analyze_And_Resolve (N, Typ);
         end if;

         --  Apply the required checks last, after rewriting has taken place

         Apply_Universal_Integer_Attribute_Checks (N);
         return;
      end if;

      --  Common processing for record and array component case

      if Present (Siz) and then Siz /= 0 then
         declare
            CS : constant Boolean := Comes_From_Source (N);

         begin
            Rewrite (N, Make_Integer_Literal (Loc, Siz));

            --  This integer literal is not a static expression. We do not
            --  call Analyze_And_Resolve here, because this would activate
            --  the circuit for deciding that a static value was out of range,
            --  and we don't want that.

            --  So just manually set the type, mark the expression as
            --  nonstatic, and then ensure that the result is checked
            --  properly if the attribute comes from source (if it was
            --  internally generated, we never need a constraint check).

            Set_Etype (N, Typ);
            Set_Is_Static_Expression (N, False);

            if CS then
               Apply_Constraint_Check (N, Typ);
            end if;
         end;
      end if;
   end Expand_Size_Attribute;

   -----------------------------
   -- Expand_Update_Attribute --
   -----------------------------

   procedure Expand_Update_Attribute (N : Node_Id) is
      procedure Process_Component_Or_Element_Update
        (Temp : Entity_Id;
         Comp : Node_Id;
         Expr : Node_Id;
         Typ  : Entity_Id);
      --  Generate the statements necessary to update a single component or an
      --  element of the prefix. The code is inserted before the attribute N.
      --  Temp denotes the entity of the anonymous object created to reflect
      --  the changes in values. Comp is the component/index expression to be
      --  updated. Expr is an expression yielding the new value of Comp. Typ
      --  is the type of the prefix of attribute Update.

      procedure Process_Range_Update
        (Temp : Entity_Id;
         Comp : Node_Id;
         Expr : Node_Id;
         Typ  : Entity_Id);
      --  Generate the statements necessary to update a slice of the prefix.
      --  The code is inserted before the attribute N. Temp denotes the entity
      --  of the anonymous object created to reflect the changes in values.
      --  Comp is range of the slice to be updated. Expr is an expression
      --  yielding the new value of Comp. Typ is the type of the prefix of
      --  attribute Update.

      -----------------------------------------
      -- Process_Component_Or_Element_Update --
      -----------------------------------------

      procedure Process_Component_Or_Element_Update
        (Temp : Entity_Id;
         Comp : Node_Id;
         Expr : Node_Id;
         Typ  : Entity_Id)
      is
         Loc   : constant Source_Ptr := Sloc (Comp);
         Exprs : List_Id;
         LHS   : Node_Id;

      begin
         --  An array element may be modified by the following relations
         --  depending on the number of dimensions:

         --     1 => Expr           --  one dimensional update
         --    (1, ..., N) => Expr  --  multi dimensional update

         --  The above forms are converted in assignment statements where the
         --  left hand side is an indexed component:

         --    Temp (1) := Expr;          --  one dimensional update
         --    Temp (1, ..., N) := Expr;  --  multi dimensional update

         if Is_Array_Type (Typ) then

            --  The index expressions of a multi dimensional array update
            --  appear as an aggregate.

            if Nkind (Comp) = N_Aggregate then
               Exprs := New_Copy_List_Tree (Expressions (Comp));
            else
               Exprs := New_List (Relocate_Node (Comp));
            end if;

            LHS :=
              Make_Indexed_Component (Loc,
                Prefix      => New_Occurrence_Of (Temp, Loc),
                Expressions => Exprs);

         --  A record component update appears in the following form:

         --    Comp => Expr

         --  The above relation is transformed into an assignment statement
         --  where the left hand side is a selected component:

         --    Temp.Comp := Expr;

         else pragma Assert (Is_Record_Type (Typ));
            LHS :=
              Make_Selected_Component (Loc,
                Prefix        => New_Occurrence_Of (Temp, Loc),
                Selector_Name => Relocate_Node (Comp));
         end if;

         Insert_Action (N,
           Make_Assignment_Statement (Loc,
             Name       => LHS,
             Expression => Relocate_Node (Expr)));
      end Process_Component_Or_Element_Update;

      --------------------------
      -- Process_Range_Update --
      --------------------------

      procedure Process_Range_Update
        (Temp : Entity_Id;
         Comp : Node_Id;
         Expr : Node_Id;
         Typ  : Entity_Id)
      is
         Index_Typ : constant Entity_Id  := Etype (First_Index (Typ));
         Loc       : constant Source_Ptr := Sloc (Comp);
         Index     : Entity_Id;

      begin
         --  A range update appears as

         --    (Low .. High => Expr)

         --  The above construct is transformed into a loop that iterates over
         --  the given range and modifies the corresponding array values to the
         --  value of Expr:

         --    for Index in Low .. High loop
         --       Temp (<Index_Typ> (Index)) := Expr;
         --    end loop;

         Index := Make_Temporary (Loc, 'I');

         Insert_Action (N,
           Make_Loop_Statement (Loc,
             Iteration_Scheme =>
               Make_Iteration_Scheme (Loc,
                 Loop_Parameter_Specification =>
                   Make_Loop_Parameter_Specification (Loc,
                     Defining_Identifier         => Index,
                     Discrete_Subtype_Definition => Relocate_Node (Comp))),

             Statements       => New_List (
               Make_Assignment_Statement (Loc,
                 Name       =>
                   Make_Indexed_Component (Loc,
                     Prefix      => New_Occurrence_Of (Temp, Loc),
                     Expressions => New_List (
                       Convert_To (Index_Typ,
                         New_Occurrence_Of (Index, Loc)))),
                 Expression => Relocate_Node (Expr))),

             End_Label        => Empty));
      end Process_Range_Update;

      --  Local variables

      Aggr    : constant Node_Id    := First (Expressions (N));
      Loc     : constant Source_Ptr := Sloc (N);
      Pref    : constant Node_Id    := Prefix (N);
      Typ     : constant Entity_Id  := Etype (Pref);
      Assoc   : Node_Id;
      Comp    : Node_Id;
      CW_Temp : Entity_Id;
      CW_Typ  : Entity_Id;
      Expr    : Node_Id;
      Temp    : Entity_Id;

   --  Start of processing for Expand_Update_Attribute

   begin
      --  Create the anonymous object to store the value of the prefix and
      --  capture subsequent changes in value.

      Temp := Make_Temporary (Loc, 'T', Pref);

      --  Preserve the tag of the prefix by offering a specific view of the
      --  class-wide version of the prefix.

      if Is_Tagged_Type (Typ) then

         --  Generate:
         --    CW_Temp : Typ'Class := Typ'Class (Pref);

         CW_Temp := Make_Temporary (Loc, 'T');
         CW_Typ  := Class_Wide_Type (Typ);

         Insert_Action (N,
           Make_Object_Declaration (Loc,
             Defining_Identifier => CW_Temp,
             Object_Definition   => New_Occurrence_Of (CW_Typ, Loc),
             Expression          =>
               Convert_To (CW_Typ, Relocate_Node (Pref))));

         --  Generate:
         --    Temp : Typ renames Typ (CW_Temp);

         Insert_Action (N,
           Make_Object_Renaming_Declaration (Loc,
             Defining_Identifier => Temp,
             Subtype_Mark        => New_Occurrence_Of (Typ, Loc),
             Name                =>
               Convert_To (Typ, New_Occurrence_Of (CW_Temp, Loc))));

      --  Non-tagged case

      else
         --  Generate:
         --    Temp : Typ := Pref;

         Insert_Action (N,
           Make_Object_Declaration (Loc,
             Defining_Identifier => Temp,
             Object_Definition   => New_Occurrence_Of (Typ, Loc),
             Expression          => Relocate_Node (Pref)));
      end if;

      --  Process the update aggregate

      Assoc := First (Component_Associations (Aggr));
      while Present (Assoc) loop
         Comp := First (Choices (Assoc));
         Expr := Expression (Assoc);
         while Present (Comp) loop
            if Nkind (Comp) = N_Range then
               Process_Range_Update (Temp, Comp, Expr, Typ);
            elsif Nkind (Comp) = N_Subtype_Indication then
               Process_Range_Update
                 (Temp, Range_Expression (Constraint (Comp)), Expr, Typ);
            else
               Process_Component_Or_Element_Update (Temp, Comp, Expr, Typ);
            end if;

            Next (Comp);
         end loop;

         Next (Assoc);
      end loop;

      --  The attribute is replaced by a reference to the anonymous object

      Rewrite (N, New_Occurrence_Of (Temp, Loc));
      Analyze (N);
   end Expand_Update_Attribute;

   -------------------
   -- Find_Fat_Info --
   -------------------

   procedure Find_Fat_Info
     (T        : Entity_Id;
      Fat_Type : out Entity_Id;
      Fat_Pkg  : out RE_Id)
   is
      Rtyp : constant Entity_Id := Root_Type (T);

   begin
      --  All we do is use the root type (historically this dealt with
      --  VAX-float .. to be cleaned up further later ???)

      if Rtyp = Standard_Short_Float or else Rtyp = Standard_Float then
         Fat_Type := Standard_Float;
         Fat_Pkg  := RE_Attr_Float;

      elsif Rtyp = Standard_Long_Float then
         Fat_Type := Standard_Long_Float;
         Fat_Pkg  := RE_Attr_Long_Float;

      elsif Rtyp = Standard_Long_Long_Float then
         Fat_Type := Standard_Long_Long_Float;
         Fat_Pkg  := RE_Attr_Long_Long_Float;

         --  Universal real (which is its own root type) is treated as being
         --  equivalent to Standard.Long_Long_Float, since it is defined to
         --  have the same precision as the longest Float type.

      elsif Rtyp = Universal_Real then
         Fat_Type := Standard_Long_Long_Float;
         Fat_Pkg  := RE_Attr_Long_Long_Float;

      else
         raise Program_Error;
      end if;
   end Find_Fat_Info;

   ----------------------------
   -- Find_Stream_Subprogram --
   ----------------------------

   function Find_Stream_Subprogram
     (Typ      : Entity_Id;
      Nam      : TSS_Name_Type;
      Attr_Ref : Node_Id) return Entity_Id
   is
      --  Local declarations

      Base_Typ : constant Entity_Id := Base_Type (Typ);
      Ent      : Entity_Id := TSS (Typ, Nam);

   --  Start of processing for Find_Stream_Subprogram

   begin
      if Present (Ent) then
         return Ent;
      end if;

      --  Everything after this point is an optimization. In other words,
      --  there should be no *correctness* problems if we were to
      --  unconditionally return Empty here.

      if Is_Unchecked_Union (Base_Typ) then
         --  Conservatively avoid possible problems (e.g., Write behaves
         --  differently for a U_U type when called by Output vs. when
         --  called from elsewhere).

         return Empty;
      end if;

      declare
         function U_Base return Entity_Id is
           (Underlying_Type (Base_Type (Typ)));
         --  Return the right type node for use in a C_A_O map lookup.
         --  In particular, we do not want the entity for a subtype.
      begin
         if Nam = TSS_Stream_Read then
            Ent := Cached_Attribute_Ops.Read_Map.Get (U_Base);
         elsif Nam = TSS_Stream_Write then
            Ent := Cached_Attribute_Ops.Write_Map.Get (U_Base);
         elsif Nam = TSS_Stream_Input then
            Ent := Cached_Attribute_Ops.Input_Map.Get (U_Base);
         elsif Nam = TSS_Stream_Output then
            Ent := Cached_Attribute_Ops.Output_Map.Get (U_Base);
         end if;
      end;

      Cached_Attribute_Ops.Validate_Cached_Candidate
        (Subp => Ent, Attr_Ref => Attr_Ref);

      if Present (Ent) then
         return Ent;
      end if;

      --  Stream attributes for strings are expanded into library calls. The
      --  following checks are disabled when the run-time is not available or
      --  when compiling predefined types due to bootstrap issues. As a result,
      --  the compiler will generate in-place stream routines for string types
      --  that appear in GNAT's library, but will generate calls via rtsfind
      --  to library routines for user code.

      --  Note: In the case of using a configurable run time, it is very likely
      --  that stream routines for string types are not present (they require
      --  file system support). In this case, the specific stream routines for
      --  strings are not used, relying on the regular stream mechanism
      --  instead. That is why we include the test RTE_Available when dealing
      --  with these cases.

      if not Is_Predefined_Unit (Current_Sem_Unit) then
         --  Storage_Array as defined in package System.Storage_Elements

         if Is_RTE (Base_Typ, RE_Storage_Array) then

            --  Case of No_Stream_Optimizations restriction active

            if Restriction_Active (No_Stream_Optimizations) then
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Storage_Array_Input)
               then
                  return RTE (RE_Storage_Array_Input);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Storage_Array_Output)
               then
                  return RTE (RE_Storage_Array_Output);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Storage_Array_Read)
               then
                  return RTE (RE_Storage_Array_Read);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Storage_Array_Write)
               then
                  return RTE (RE_Storage_Array_Write);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;

            --  Restriction No_Stream_Optimizations is not set, so we can go
            --  ahead and optimize using the block IO forms of the routines.

            else
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Storage_Array_Input_Blk_IO)
               then
                  return RTE (RE_Storage_Array_Input_Blk_IO);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Storage_Array_Output_Blk_IO)
               then
                  return RTE (RE_Storage_Array_Output_Blk_IO);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Storage_Array_Read_Blk_IO)
               then
                  return RTE (RE_Storage_Array_Read_Blk_IO);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Storage_Array_Write_Blk_IO)
               then
                  return RTE (RE_Storage_Array_Write_Blk_IO);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;
            end if;

         --  Stream_Element_Array as defined in package Ada.Streams

         elsif Is_RTE (Base_Typ, RE_Stream_Element_Array) then

            --  Case of No_Stream_Optimizations restriction active

            if Restriction_Active (No_Stream_Optimizations) then
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Stream_Element_Array_Input)
               then
                  return RTE (RE_Stream_Element_Array_Input);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Stream_Element_Array_Output)
               then
                  return RTE (RE_Stream_Element_Array_Output);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Stream_Element_Array_Read)
               then
                  return RTE (RE_Stream_Element_Array_Read);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Stream_Element_Array_Write)
               then
                  return RTE (RE_Stream_Element_Array_Write);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;

            --  Restriction No_Stream_Optimizations is not set, so we can go
            --  ahead and optimize using the block IO forms of the routines.

            else
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Stream_Element_Array_Input_Blk_IO)
               then
                  return RTE (RE_Stream_Element_Array_Input_Blk_IO);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Stream_Element_Array_Output_Blk_IO)
               then
                  return RTE (RE_Stream_Element_Array_Output_Blk_IO);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Stream_Element_Array_Read_Blk_IO)
               then
                  return RTE (RE_Stream_Element_Array_Read_Blk_IO);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Stream_Element_Array_Write_Blk_IO)
               then
                  return RTE (RE_Stream_Element_Array_Write_Blk_IO);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;
            end if;

         --  String as defined in package Ada

         elsif Base_Typ = Standard_String then

            --  Case of No_Stream_Optimizations restriction active

            if Restriction_Active (No_Stream_Optimizations) then
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_String_Input)
               then
                  return RTE (RE_String_Input);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_String_Output)
               then
                  return RTE (RE_String_Output);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_String_Read)
               then
                  return RTE (RE_String_Read);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_String_Write)
               then
                  return RTE (RE_String_Write);

               elsif Nam /= TSS_Stream_Input and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;

            --  Restriction No_Stream_Optimizations is not set, so we can go
            --  ahead and optimize using the block IO forms of the routines.

            else
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_String_Input_Blk_IO)
               then
                  return RTE (RE_String_Input_Blk_IO);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_String_Output_Blk_IO)
               then
                  return RTE (RE_String_Output_Blk_IO);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_String_Read_Blk_IO)
               then
                  return RTE (RE_String_Read_Blk_IO);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_String_Write_Blk_IO)
               then
                  return RTE (RE_String_Write_Blk_IO);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;
            end if;

         --  Wide_String as defined in package Ada

         elsif Base_Typ = Standard_Wide_String then

            --  Case of No_Stream_Optimizations restriction active

            if Restriction_Active (No_Stream_Optimizations) then
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Wide_String_Input)
               then
                  return RTE (RE_Wide_String_Input);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Wide_String_Output)
               then
                  return RTE (RE_Wide_String_Output);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Wide_String_Read)
               then
                  return RTE (RE_Wide_String_Read);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Wide_String_Write)
               then
                  return RTE (RE_Wide_String_Write);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;

            --  Restriction No_Stream_Optimizations is not set, so we can go
            --  ahead and optimize using the block IO forms of the routines.

            else
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Wide_String_Input_Blk_IO)
               then
                  return RTE (RE_Wide_String_Input_Blk_IO);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Wide_String_Output_Blk_IO)
               then
                  return RTE (RE_Wide_String_Output_Blk_IO);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Wide_String_Read_Blk_IO)
               then
                  return RTE (RE_Wide_String_Read_Blk_IO);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Wide_String_Write_Blk_IO)
               then
                  return RTE (RE_Wide_String_Write_Blk_IO);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;
            end if;

         --  Wide_Wide_String as defined in package Ada

         elsif Base_Typ = Standard_Wide_Wide_String then

            --  Case of No_Stream_Optimizations restriction active

            if Restriction_Active (No_Stream_Optimizations) then
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Wide_Wide_String_Input)
               then
                  return RTE (RE_Wide_Wide_String_Input);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Wide_Wide_String_Output)
               then
                  return RTE (RE_Wide_Wide_String_Output);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Wide_Wide_String_Read)
               then
                  return RTE (RE_Wide_Wide_String_Read);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Wide_Wide_String_Write)
               then
                  return RTE (RE_Wide_Wide_String_Write);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;

            --  Restriction No_Stream_Optimizations is not set, so we can go
            --  ahead and optimize using the block IO forms of the routines.

            else
               if Nam = TSS_Stream_Input
                 and then RTE_Available (RE_Wide_Wide_String_Input_Blk_IO)
               then
                  return RTE (RE_Wide_Wide_String_Input_Blk_IO);

               elsif Nam = TSS_Stream_Output
                 and then RTE_Available (RE_Wide_Wide_String_Output_Blk_IO)
               then
                  return RTE (RE_Wide_Wide_String_Output_Blk_IO);

               elsif Nam = TSS_Stream_Read
                 and then RTE_Available (RE_Wide_Wide_String_Read_Blk_IO)
               then
                  return RTE (RE_Wide_Wide_String_Read_Blk_IO);

               elsif Nam = TSS_Stream_Write
                 and then RTE_Available (RE_Wide_Wide_String_Write_Blk_IO)
               then
                  return RTE (RE_Wide_Wide_String_Write_Blk_IO);

               elsif Nam /= TSS_Stream_Input  and then
                     Nam /= TSS_Stream_Output and then
                     Nam /= TSS_Stream_Read   and then
                     Nam /= TSS_Stream_Write
               then
                  raise Program_Error;
               end if;
            end if;
         end if;
      end if;

      if Is_Tagged_Type (Typ) and then Is_Derived_Type (Typ) then
         return Find_Prim_Op (Typ, Nam);
      else
         return Find_Inherited_TSS (Typ, Nam);
      end if;
   end Find_Stream_Subprogram;

   ---------------
   -- Full_Base --
   ---------------

   function Full_Base (T : Entity_Id) return Entity_Id is
      BT : Entity_Id;

   begin
      BT := Base_Type (T);

      if Is_Private_Type (BT)
        and then Present (Full_View (BT))
      then
         BT := Full_View (BT);
      end if;

      return BT;
   end Full_Base;

   -------------------------------
   -- Get_Stream_Convert_Pragma --
   -------------------------------

   function Get_Stream_Convert_Pragma (T : Entity_Id) return Node_Id is
      Typ : Entity_Id;
      N   : Node_Id;

   begin
      --  Note: we cannot use Get_Rep_Pragma here because of the peculiarity
      --  that a stream convert pragma for a tagged type is not inherited from
      --  its parent. Probably what is wrong here is that it is basically
      --  incorrect to consider a stream convert pragma to be a representation
      --  pragma at all ???

      N := First_Rep_Item (Implementation_Base_Type (T));
      while Present (N) loop
         if Nkind (N) = N_Pragma
           and then Pragma_Name (N) = Name_Stream_Convert
         then
            --  For tagged types this pragma is not inherited, so we
            --  must verify that it is defined for the given type and
            --  not an ancestor.

            Typ :=
              Entity (Expression (First (Pragma_Argument_Associations (N))));

            if not Is_Tagged_Type (T)
              or else T = Typ
              or else (Is_Private_Type (Typ) and then T = Full_View (Typ))
            then
               return N;
            end if;
         end if;

         Next_Rep_Item (N);
      end loop;

      return Empty;
   end Get_Stream_Convert_Pragma;

   ---------------------------------
   -- Is_Constrained_Packed_Array --
   ---------------------------------

   function Is_Constrained_Packed_Array (Typ : Entity_Id) return Boolean is
      Arr : Entity_Id := Typ;

   begin
      if Is_Access_Type (Arr) then
         Arr := Designated_Type (Arr);
      end if;

      return Is_Array_Type (Arr)
        and then Is_Constrained (Arr)
        and then Present (Packed_Array_Impl_Type (Arr));
   end Is_Constrained_Packed_Array;

   ----------------------------------------
   -- Is_Inline_Floating_Point_Attribute --
   ----------------------------------------

   function Is_Inline_Floating_Point_Attribute (N : Node_Id) return Boolean is
      Id : constant Attribute_Id := Get_Attribute_Id (Attribute_Name (N));

      function Is_GCC_Target return Boolean;
      --  Return True if we are using a GCC target/back-end
      --  ??? Note: the implementation is kludgy/fragile

      -------------------
      -- Is_GCC_Target --
      -------------------

      function Is_GCC_Target return Boolean is
      begin
         return not CodePeer_Mode;
      end Is_GCC_Target;

   --  Start of processing for Is_Inline_Floating_Point_Attribute

   begin
      --  Machine and Model can be expanded by the GCC back end only

      if Id = Attribute_Machine or else Id = Attribute_Model then
         return Is_GCC_Target;

      --  Remaining cases handled by all back ends are Rounding and Truncation
      --  when appearing as the operand of a conversion to some integer type.

      elsif Nkind (Parent (N)) /= N_Type_Conversion
        or else not Is_Integer_Type (Etype (Parent (N)))
      then
         return False;
      end if;

      --  Here we are in the integer conversion context. We reuse Rounding for
      --  Machine_Rounding as System.Fat_Gen, which is a permissible behavior.

      return
        Id = Attribute_Rounding
          or else Id = Attribute_Machine_Rounding
          or else Id = Attribute_Truncation;
   end Is_Inline_Floating_Point_Attribute;

   --------------------------------------
   -- Is_User_Defined_Enumeration_Type --
   --------------------------------------

   function Is_User_Defined_Enumeration_Type (Typ : Entity_Id) return Boolean
   is
      Rtyp : constant Entity_Id := Root_Type (Base_Type (Typ));

   begin
      return Is_Enumeration_Type (Rtyp)
        and then Rtyp not in Standard_Boolean
                           | Standard_Character
                           | Standard_Wide_Character
                           | Standard_Wide_Wide_Character;
   end Is_User_Defined_Enumeration_Type;

end Exp_Attr;
