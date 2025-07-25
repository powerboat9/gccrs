------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                         G E N _ I L . F I E L D S                        --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2020-2025, Free Software Foundation, Inc.         --
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

package Gen_IL.Fields is

   --  The following is "optional field enumeration" -- i.e. it is Field_Enum
   --  (declared below) plus the special null value No_Field. See the spec of
   --  Gen_IL.Gen for how to modify this. (Of course, in Ada we have to define
   --  this backwards from the above conceptual description.)

   --  Note that there are various subranges of this type declared below,
   --  which might need to be kept in sync when modifying this.

   --  Be sure to put new fields in the appropriate subrange (Field_Enum,
   --  Node_Field, Entity_Field -- search for comments below).

   type Opt_Field_Enum is
     (No_Field,

      --  Start of node fields:

      Nkind,
      Sloc,
      In_List,
      Rewrite_Ins,
      Comes_From_Source,
      Analyzed,
      Error_Posted,
      Small_Paren_Count,
      Check_Actuals,
      Is_Ignored_Ghost_Node,
      Link,

      Abort_Present,
      Abortable_Part,
      Abstract_Present,
      Accept_Statement,
      Access_Definition,
      Access_To_Subprogram_Definition,
      Access_Types_To_Process,
      Actions,
      Activation_Chain_Entity,
      Acts_As_Spec,
      Actual_Designated_Subtype,
      Aggregate_Bounds_Or_Ancestor_Type,
      Aliased_Present,
      All_Others,
      All_Present,
      Alternatives,
      Ancestor_Part,
      Atomic_Sync_Required,
      Array_Aggregate,
      Aspect_On_Partial_View,
      Aspect_Rep_Item,
      Aspect_Specifications,
      Aspect_Subprograms,
      Assignment_OK,
      Attribute_Name,
      At_End_Proc,
      Aux_Decls_Node,
      Backwards_OK,
      Bad_Is_Detected,
      Binding_Chars,
      Body_Required,
      Body_To_Inline,
      Box_Present,
      Cannot_Be_Superflat,
      Char_Literal_Value,
      Chars,
      Check_Address_Alignment,
      Choice_Parameter,
      Choices,
      Class_Present,
      Classifications,
      Cleanup_Actions,
      Comes_From_Check_Or_Contract,
      Comes_From_Extended_Return_Statement,
      Comes_From_Iterator,
      Compare_Type,
      Compile_Time_Known_Aggregate,
      Component_Associations,
      Component_Clauses,
      Component_Definition,
      Component_Items,
      Component_List,
      Component_Name,
      Componentwise_Assignment,
      Condition,
      Condition_Actions,
      Config_Pragmas,
      Constant_Present,
      Constraint,
      Constraints,
      Context_Installed,
      Context_Items,
      Context_Pending,
      Contract_Test_Cases,
      Controlling_Argument,
      Conversion_OK,
      Corresponding_Aspect,
      Corresponding_Body,
      Corresponding_Entry_Body,
      Corresponding_Formal_Spec,
      Corresponding_Generic_Association,
      Corresponding_Integer_Value,
      Corresponding_Spec,
      Corresponding_Spec_Of_Stub,
      Corresponding_Stub,
      Dcheck_Function,
      Declarations,
      Default_Expression,
      Default_Storage_Pool,
      Default_Name,
      Default_Subtype_Mark,
      Defining_Identifier,
      Defining_Unit_Name,
      Delay_Alternative,
      Delay_Statement,
      Delta_Expression,
      Digits_Expression,
      Discr_Check_Funcs_Built,
      Discrete_Choices,
      Discrete_Range,
      Discrete_Subtype_Definition,
      Discrete_Subtype_Definitions,
      Discriminant_Specifications,
      Discriminant_Type,
      Do_Discriminant_Check,
      Do_Division_Check,
      Do_Length_Check,
      Do_Overflow_Check,
      Do_Range_Check,
      Do_Storage_Check,
      Elaborate_All_Desirable,
      Elaborate_All_Present,
      Elaborate_Desirable,
      Elaborate_Present,
      Else_Actions,
      Else_Statements,
      Elsif_Parts,
      Enclosing_Variant,
      End_Label,
      End_Span,
      Entity_Or_Associated_Node,
      Entry_Body_Formal_Part,
      Entry_Call_Alternative,
      Entry_Call_Statement,
      Entry_Direct_Name,
      Entry_Index,
      Entry_Index_Specification,
      Etype,
      Exception_Choices,
      Exception_Handlers,
      Exception_Junk,
      Exception_Label,
      Expansion_Delayed,
      Explicit_Actual_Parameter,
      Explicit_Generic_Actual_Parameter,
      Expression,
      Expression_Copy,
      Expressions,
      File_Index,
      Finally_Statements,
      First_Bit,
      First_Inlined_Subprogram,
      First_Name,
      First_Named_Actual,
      First_Subtype_Link,
      Float_Truncate,
      Formal_Type_Definition,
      Forwards_OK,
      For_Allocator,
      For_Special_Return_Object,
      From_Aspect_Specification,
      From_At_Mod,
      From_Conditional_Expression,
      From_Default,
      Generalized_Indexing,
      Generic_Associations,
      Generic_Formal_Declarations,
      Generic_Parent,
      Generic_Parent_Type,
      Handled_Statement_Sequence,
      Handler_List_Entry,
      Has_Created_Identifier,
      Has_Dereference_Action,
      Has_Dynamic_Length_Check,
      Has_Init_Expression,
      Has_Local_Raise,
      Has_No_Elaboration_Code,
      Has_Pragma_Suppress_All,
      Has_Private_View,
      Has_Relative_Deadline_Pragma,
      Has_Secondary_Private_View,
      Has_Self_Reference,
      Has_SP_Choice,
      Has_Storage_Size_Pragma,
      Has_Target_Names,
      Has_Wide_Character,
      Has_Wide_Wide_Character,
      Header_Size_Added,
      Hidden_By_Use_Clause,
      High_Bound,
      Identifier,
      Interface_List,
      Interface_Present,
      Import_Interface_Present,
      In_Present,
      Includes_Infinities,
      Inherited_Discriminant,
      Instance_Spec,
      Intval,
      Is_Abort_Block,
      Is_Accessibility_Actual,
      Is_Analyzed_Pragma,
      Is_Asynchronous_Call_Block,
      Is_Boolean_Aspect,
      Is_Checked,
      Is_Checked_Ghost_Pragma,
      Is_Component_Left_Opnd,
      Is_Component_Right_Opnd,
      Is_Controlling_Actual,
      Is_Declaration_Level_Node,
      Is_Delayed_Aspect,
      Is_Disabled,
      Is_Dispatching_Call,
      Is_Dynamic_Coextension,
      Is_Effective_Use_Clause,
      Is_Elaboration_Checks_OK_Node,
      Is_Elaboration_Code,
      Is_Elaboration_Warnings_OK_Node,
      Is_Elsif,
      Is_Entry_Barrier_Function,
      Is_Expanded_Build_In_Place_Call,
      Is_Expanded_Constructor_Call,
      Is_Expanded_Dispatching_Call,
      Is_Expanded_Prefixed_Call,
      Is_Folded_In_Parser,
      Is_Generic_Contract_Pragma,
      Is_Homogeneous_Aggregate,
      Is_Parenthesis_Aggregate,
      Is_Ignored,
      Is_Ignored_Ghost_Pragma,
      Is_Implicit_With,
      Is_In_Discriminant_Check,
      Is_Initialization_Block,
      Is_Interpolated_String_Literal,
      Is_Known_Guaranteed_ABE,
      Is_Machine_Number,
      Is_Null_Loop,
      Is_Overloaded,
      Is_Power_Of_2_For_Shift,
      Is_Preelaborable_Call,
      Is_Prefixed_Call,
      Is_Protected_Subprogram_Body,
      Is_Qualified_Universal_Literal,
      Is_Read,
      Is_Source_Call,
      Is_SPARK_Mode_On_Node,
      Is_Static_Coextension,
      Is_Static_Expression,
      Is_Subprogram_Descriptor,
      Is_Task_Allocation_Block,
      Is_Task_Body_Procedure,
      Is_Task_Master,
      Is_Write,
      Iterator_Filter,
      Iteration_Scheme,
      Iterator_Specification,
      Itype,
      Key_Expression,
      Kill_Range_Check,
      Last_Bit,
      Last_Name,
      Library_Unit,
      Label_Construct,
      Left_Opnd,
      Limited_View_Installed,
      Limited_Present,
      Literals,
      Local_Raise_Not_OK,
      Local_Raise_Statements,
      Loop_Actions,
      Loop_Parameter_Specification,
      Low_Bound,
      Mod_Clause,
      More_Ids,
      Multidefined_Bindings,
      Must_Be_Byte_Aligned,
      Must_Not_Freeze,
      Must_Not_Override,
      Must_Override,
      Name,
      Names,
      Next_Entity,
      Next_Exit_Statement,
      Next_Implicit_With,
      Next_Named_Actual,
      Next_Pragma,
      Next_Rep_Item,
      Next_Use_Clause,
      No_Ctrl_Actions,
      No_Elaboration_Check,
      No_Entities_Ref_In_Spec,
      No_Finalize_Actions,
      No_Initialization,
      No_Minimize_Eliminate,
      No_Truncation,
      Null_Excluding_Subtype,
      Null_Exclusion_Present,
      Null_Exclusion_In_Return_Present,
      Null_Present,
      Null_Record_Present,
      Null_Statement,
      Object_Definition,
      Of_Present,
      Original_Discriminant,
      Original_Entity,
      Others_Discrete_Choices,
      Out_Present,
      Parameter_Associations,
      Parameter_Specifications,
      Parameter_Type,
      Parent_Spec,
      Parent_With,
      Position,
      Pragma_Argument_Associations,
      Pragma_Identifier,
      Pragmas_After,
      Pragmas_Before,
      Pre_Post_Conditions,
      Prefix,
      Premature_Use,
      Present_Expr,
      Prev_Ids,
      Prev_Use_Clause,
      Print_In_Hex,
      Private_Declarations,
      Private_Present,
      Procedure_To_Call,
      Proper_Body,
      Protected_Definition,
      Protected_Present,
      Raises_Constraint_Error,
      Range_Constraint,
      Range_Expression,
      Real_Range_Specification,
      Realval,
      Reason,
      Record_Extension_Part,
      Redundant_Use,
      Renaming_Exception,
      Result_Definition,
      Return_Object_Declarations,
      Return_Statement_Entity,
      Reverse_Present,
      Right_Opnd,
      Rounded_Result,
      Save_Invocation_Graph_Of_Body,
      SCIL_Controlling_Tag,
      SCIL_Entity,
      SCIL_Tag_Value,
      SCIL_Target_Prim,
      Scope,
      Select_Alternatives,
      Selector_Name,
      Selector_Names,
      Shift_Count_OK,
      Source_Type,
      Specification,
      Statements,
      Storage_Pool,
      Subpool_Handle_Name,
      Strval,
      Subtype_Indication,
      Subtype_Mark,
      Subtype_Marks,
      Suppress_Assignment_Checks,
      Suppress_Loop_Warnings,
      Synchronized_Present,
      Tagged_Present,
      Tag_Propagated,
      Target,
      Call_Or_Target_Loop,
      Target_Type,
      Task_Definition,
      Task_Present,
      Then_Actions,
      Then_Statements,
      Triggering_Alternative,
      Triggering_Statement,
      TSS_Elist,
      Type_Definition,
      Uneval_Old_Accept,
      Uneval_Old_Warn,
      Unit,
      Unknown_Discriminants_Present,
      Unreferenced_In_Spec,
      Variant_Part,
      Variants,
      Visible_Declarations,
      Uninitialized_Variable,
      Used_Operations,
      Was_Attribute_Reference,
      Was_Default_Init_Box_Association,
      Was_Expression_Function,
      Was_Originally_Stub,

      --  End of node fields.

      Between_Node_And_Entity_Fields,

      --  Start of entity fields:

      Ekind,
      Basic_Convention,
      Abstract_States,
      Accept_Address,
      Access_Disp_Table,
      Access_Disp_Table_Elab_Flag,
      Access_Subprogram_Wrapper,
      Activation_Record_Component,
      Actual_Subtype,
      Address_Taken,
      Alignment,
      Anonymous_Collections,
      Anonymous_Designated_Type,
      Anonymous_Object,
      Associated_Entity,
      Associated_Formal_Package,
      Associated_Node_For_Itype,
      Associated_Storage_Pool,
      Barrier_Function,
      BIP_Initialization_Call,
      Block_Node,
      Body_Entity,
      Body_Needed_For_Inlining,
      Body_Needed_For_SAL,
      Body_References,
      C_Pass_By_Copy,
      Can_Never_Be_Null,
      Can_Use_Internal_Rep,
      Checks_May_Be_Suppressed,
      Class_Postconditions,
      Class_Preconditions,
      Class_Preconditions_Subprogram,
      Class_Wide_Equivalent_Type,
      Class_Wide_Type,
      Cloned_Subtype,
      Component_Alignment,
      Component_Bit_Offset,
      Component_Clause,
      Component_Size,
      Component_Type,
      Constructor_List,
      Constructor_Name,
      Continue_Mark,
      Contract,
      Contract_Wrapper,
      Corresponding_Concurrent_Type,
      Corresponding_Discriminant,
      Corresponding_Equality,
      Corresponding_Record_Component,
      Corresponding_Record_Type,
      Corresponding_Remote_Type,
      CR_Discriminant,
      Current_Use_Clause,
      Current_Value,
      Debug_Info_Off,
      Debug_Renaming_Link,
      Default_Aspect_Component_Value,
      Default_Aspect_Value,
      Default_Expressions_Processed,
      Default_Value,
      Delay_Cleanups,
      Delta_Value,
      Depends_On_Private,
      Derived_Type_Link,
      Digits_Value,
      Predicated_Parent,
      Predicates_Ignored,
      Direct_Primitive_Operations,
      Directly_Designated_Type,
      Disable_Controlled,
      Discard_Names,
      Discriminal,
      Discriminal_Link,
      Discriminant_Checking_Func,
      Discriminant_Constraint,
      Discriminant_Default_Value,
      Discriminant_Number,
      Dispatch_Table_Wrappers,
      Dynamic_Call_Helper,
      DT_Entry_Count,
      DT_Offset_To_Top_Func,
      DT_Position,
      DTC_Entity,
      Elaborate_Body_Desirable,
      Elaboration_Entity,
      Elaboration_Entity_Required,
      Encapsulating_State,
      Enclosing_Scope,
      Entry_Accepted,
      Entry_Bodies_Array,
      Entry_Cancel_Parameter,
      Entry_Component,
      Entry_Formal,
      Entry_Index_Constant,
      Entry_Max_Queue_Lengths_Array,
      Entry_Parameters_Type,
      Enum_Pos_To_Rep,
      Enumeration_Pos,
      Enumeration_Rep,
      Enumeration_Rep_Expr,
      Equivalent_Type,
      Esize,
      Extra_Accessibility,
      Extra_Accessibility_Of_Result,
      Extra_Constrained,
      Extra_Formal,
      Extra_Formals,
      Extra_Formals_Known,
      Finalization_Collection,
      Finalization_Master_Node,
      Finalize_Storage_Only,
      Finalizer,
      First_Entity,
      First_Exit_Statement,
      First_Index,
      First_Literal,
      First_Private_Entity,
      First_Rep_Item,
      Freeze_Node,
      From_Limited_With,
      Full_View,
      Generic_Homonym,
      Generic_Renamings,
      Has_Aliased_Components,
      Has_Alignment_Clause,
      Has_All_Calls_Remote,
      Has_Atomic_Components,
      Has_Biased_Representation,
      Has_Completion,
      Has_Completion_In_Body,
      Has_Complex_Representation,
      Has_Component_Size_Clause,
      Has_Constrained_Partial_View,
      Has_Contiguous_Rep,
      Has_Controlled_Component,
      Has_Controlling_Result,
      Has_Convention_Pragma,
      Has_Default_Aspect,
      Has_Delayed_Aspects,
      Has_Delayed_Freeze,
      Has_Delayed_Rep_Aspects,
      Has_Destructor,
      Has_Discriminants,
      Has_Dispatch_Table,
      Has_Dynamic_Predicate_Aspect,
      Has_Enumeration_Rep_Clause,
      Has_Exit,
      Has_Expanded_Contract,
      Has_First_Controlling_Parameter_Aspect,
      Has_Forward_Instantiation,
      Has_Fully_Qualified_Name,
      Has_Ghost_Predicate_Aspect,
      Has_Gigi_Rep_Item,
      Has_Homonym,
      Has_Implicit_Dereference,
      Has_Independent_Components,
      Has_Inheritable_Invariants,
      Has_Inherited_DIC,
      Has_Inherited_Invariants,
      Has_Initial_Value,
      Has_Loop_Entry_Attributes,
      Has_Machine_Radix_Clause,
      Has_Master_Entity,
      Has_Missing_Return,
      Has_Nested_Block_With_Handler,
      Has_Nested_Subprogram,
      Has_Non_Standard_Rep,
      Has_Object_Size_Clause,
      Has_Out_Or_In_Out_Parameter,
      Has_Own_DIC,
      Has_Own_Invariants,
      Has_Partial_Visible_Refinement,
      Has_Per_Object_Constraint,
      Has_Pragma_Controlled,
      Has_Pragma_Elaborate_Body,
      Has_Pragma_Inline,
      Has_Pragma_Inline_Always,
      Has_Pragma_No_Inline,
      Has_Pragma_Ordered,
      Has_Pragma_Pack,
      Has_Pragma_Preelab_Init,
      Has_Pragma_Pure,
      Has_Pragma_Pure_Function,
      Has_Pragma_Thread_Local_Storage,
      Has_Pragma_Unmodified,
      Has_Pragma_Unreferenced,
      Has_Pragma_Unreferenced_Objects,
      Has_Pragma_Unused,
      Has_Predicates,
      Has_Primitive_Operations,
      Has_Private_Ancestor,
      Has_Private_Declaration,
      Has_Private_Extension,
      Has_Protected,
      Has_Qualified_Name,
      Has_RACW,
      Has_Record_Rep_Clause,
      Has_Recursive_Call,
      Has_Relaxed_Finalization,
      Has_Shift_Operator,
      Has_Size_Clause,
      Has_Small_Clause,
      Has_Specified_Layout,
      Has_Specified_Stream_Input,
      Has_Specified_Stream_Output,
      Has_Specified_Stream_Read,
      Has_Specified_Stream_Write,
      Has_Static_Discriminants,
      Has_Static_Predicate,
      Has_Static_Predicate_Aspect,
      Has_Storage_Size_Clause,
      Has_Stream_Size_Clause,
      Has_Task,
      Has_Timing_Event,
      Has_Thunks,
      Has_Unchecked_Union,
      Has_Unknown_Discriminants,
      Has_Visible_Refinement,
      Has_Volatile_Components,
      Has_Xref_Entry,
      Has_Yield_Aspect,
      Hiding_Loop_Variable,
      Hidden_In_Formal_Instance,
      Homonym,
      Ignored_Class_Postconditions,
      Ignored_Class_Preconditions,
      Ignore_SPARK_Mode_Pragmas,
      Import_Pragma,
      Incomplete_Actuals,
      Incomplete_View,
      Indirect_Call_Wrapper,
      In_Package_Body,
      In_Private_Part,
      In_Use,
      Initialization_Statements,
      Inner_Instances,
      Interface_Alias,
      Interface_Name,
      Interfaces,
      Is_Abstract_Subprogram,
      Is_Abstract_Type,
      Is_Access_Constant,
      Is_Activation_Record,
      Is_Actual_Subtype,
      Is_Ada_2005_Only,
      Is_Ada_2012_Only,
      Is_Ada_2022_Only,
      Is_Aliased,
      Is_Asynchronous,
      Is_Atomic,
      Is_Bit_Packed_Array,
      Is_Called,
      Is_Character_Type,
      Is_Checked_Ghost_Entity,
      Is_Child_Unit,
      Is_Class_Wide_Equivalent_Type,
      Is_Class_Wide_Wrapper,
      Is_Compilation_Unit,
      Is_Completely_Hidden,
      Is_Concurrent_Record_Type,
      Is_Constr_Array_Subt_With_Bounds,
      Is_Constr_Subt_For_U_Nominal,
      Is_Constrained,
      Is_Constructor,
      Is_Controlled_Active,
      Is_Controlling_Formal,
      Is_CPP_Class,
      Is_CUDA_Kernel,
      Is_Descendant_Of_Address,
      Is_Destructor,
      Is_DIC_Procedure,
      Is_Discrim_SO_Function,
      Is_Discriminant_Check_Function,
      Is_Dispatch_Table_Entity,
      Is_Dispatch_Table_Wrapper,
      Is_Dispatching_Operation,
      Is_Elaboration_Checks_OK_Id,
      Is_Elaboration_Warnings_OK_Id,
      Is_Eliminated,
      Is_Entry_Formal,
      Is_Entry_Wrapper,
      Is_Exception_Handler,
      Is_Exported,
      Is_Finalized_Transient,
      Is_Finalizer,
      Is_First_Subtype,
      Is_Fixed_Lower_Bound_Array_Subtype,
      Is_Fixed_Lower_Bound_Index_Subtype,
      Is_Formal_Subprogram,
      Is_Frozen,
      Is_Generic_Actual_Subprogram,
      Is_Generic_Actual_Type,
      Is_Generic_Instance,
      Is_Generic_Type,
      Is_Hidden,
      Is_Hidden_Non_Overridden_Subpgm,
      Is_Hidden_Open_Scope,
      Is_Ignored_For_Finalization,
      Is_Ignored_Ghost_Entity,
      Is_Immediately_Visible,
      Is_Implementation_Defined,
      Is_Implicit_Full_View,
      Is_Imported,
      Is_Independent,
      Is_Initial_Condition_Procedure,
      Is_Inlined,
      Is_Inlined_Always,
      Is_Instantiated,
      Is_Interface,
      Is_Internal,
      Is_Interrupt_Handler,
      Is_Intrinsic_Subprogram,
      Is_Invariant_Procedure,
      Is_Itype,
      Is_Known_Non_Null,
      Is_Known_Null,
      Is_Known_Valid,
      Is_Large_Unconstrained_Definite,
      Is_Limited_Composite,
      Is_Limited_Interface,
      Is_Limited_Record,
      Is_Local_Anonymous_Access,
      Is_Loop_Parameter,
      Is_Machine_Code_Subprogram,
      Is_Mutably_Tagged_Type,
      Is_Non_Static_Subtype,
      Is_Null_Init_Proc,
      Is_Obsolescent,
      Is_Only_Out_Parameter,
      Is_Package_Body_Entity,
      Is_Packed,
      Is_Packed_Array_Impl_Type,
      Is_Not_Self_Hidden,
      Is_Param_Block_Component_Type,
      Is_Partial_Invariant_Procedure,
      Is_Potentially_Use_Visible,
      Is_Predicate_Function,
      Is_Preelaborated,
      Is_Primitive,
      Is_Primitive_Wrapper,
      Is_Private_Composite,
      Is_Private_Descendant,
      Is_Private_Primitive,
      Is_Public,
      Is_Pure,
      Is_Pure_Unit_Access_Type,
      Is_RACW_Stub_Type,
      Is_Raised,
      Is_Remote_Call_Interface,
      Is_Remote_Types,
      Is_Renaming_Of_Object,
      Is_Return_Object,
      Is_Safe_To_Reevaluate,
      Is_Shared_Passive,
      Is_Static_Type,
      Is_Statically_Allocated,
      Is_Tag,
      Is_Tagged_Type,
      Is_Thunk,
      Is_Trivial_Subprogram,
      Is_True_Constant,
      Is_Unchecked_Union,
      Is_Underlying_Full_View,
      Is_Underlying_Record_View,
      Is_Unimplemented,
      Is_Unsigned_Type,
      Is_Uplevel_Referenced_Entity,
      Is_Valued_Procedure,
      Is_Visible_Formal,
      Is_Visible_Lib_Unit,
      Is_Volatile_Type,
      Is_Volatile_Object,
      Is_Volatile_Full_Access,
      Is_Wrapper,
      Itype_Printed,
      Kill_Elaboration_Checks,
      Known_To_Have_Preelab_Init,
      Last_Aggregate_Assignment,
      Last_Assignment,
      Last_Entity,
      Limited_View,
      Linker_Section_Pragma,
      Lit_Hash,
      Lit_Indexes,
      Lit_Strings,
      Low_Bound_Tested,
      LSP_Subprogram,
      Machine_Radix_10,
      Master_Id,
      Materialize_Entity,
      May_Inherit_Delayed_Rep_Aspects,
      Mechanism,
      Minimum_Accessibility,
      Modulus,
      Must_Be_On_Byte_Boundary,
      Must_Have_Preelab_Init,
      Needs_Construction,
      Needs_Debug_Info,
      Needs_No_Actuals,
      Never_Set_In_Source,
      Next_Inlined_Subprogram,
      No_Dynamic_Predicate_On_Actual,
      No_Pool_Assigned,
      No_Predicate_On_Actual,
      No_Raise,
      No_Reordering,
      No_Return,
      No_Strict_Aliasing,
      No_Tagged_Streams_Pragma,
      Non_Binary_Modulus,
      Non_Limited_View,
      Nonzero_Is_True,
      Normalized_First_Bit,
      Normalized_Position,
      OK_To_Rename,
      Optimize_Alignment_Space,
      Optimize_Alignment_Time,
      Original_Access_Type,
      Original_Array_Type,
      Original_Protected_Subprogram,
      Original_Record_Component,
      Overlays_Constant,
      Overridden_Inherited_Operation,
      Overridden_Operation,
      Package_Instantiation,
      Packed_Array_Impl_Type,
      Parent_Subtype,
      Part_Of_Constituents,
      Part_Of_References,
      Partial_View_Has_Unknown_Discr,
      Predicate_Expression,
      Prev_Entity,
      Prival,
      Prival_Link,
      Private_Dependents,
      Protected_Body_Subprogram,
      Protected_Formal,
      Protected_Subprogram,
      Protection_Object,
      Reachable,
      Receiving_Entry,
      Referenced,
      Referenced_As_LHS,
      Referenced_As_Out_Parameter,
      Refinement_Constituents,
      Related_Array_Object,
      Related_Expression,
      Related_Instance,
      Related_Type,
      Relative_Deadline_Variable,
      Renamed_In_Spec,
      Renamed_Or_Alias, -- Shared among Alias, Renamed_Entity, Renamed_Object
      Renames_Limited_View,
      Requires_Overriding,
      Return_Applies_To,
      Return_Present,
      Return_Statement,
      Returns_By_Ref,
      Reverse_Bit_Order,
      Reverse_Storage_Order,
      RM_Size,
      Scalar_Range,
      Scale_Value,
      Scope_Depth_Value,
      Sec_Stack_Needed_For_Return,
      Shared_Var_Procs_Instance,
      Size_Depends_On_Discriminant,
      Size_Known_At_Compile_Time,
      Small_Value,
      SPARK_Aux_Pragma,
      SPARK_Aux_Pragma_Inherited,
      SPARK_Pragma,
      SPARK_Pragma_Inherited,
      Spec_Entity,
      SSO_Set_High_By_Default,
      SSO_Set_Low_By_Default,
      Static_Call_Helper,
      Static_Discrete_Predicate,
      Static_Elaboration_Desired,
      Static_Initialization,
      Static_Real_Or_String_Predicate,
      Storage_Size_Variable,
      Stored_Constraint,
      Stores_Attribute_Old_Prefix,
      Strict_Alignment,
      String_Literal_Length,
      String_Literal_Low_Bound,
      Subprograms_For_Type,
      Subps_Index,
      Suppress_Elaboration_Warnings,
      Suppress_Initialization,
      Suppress_Style_Checks,
      Suppress_Value_Tracking_On_Call,
      Task_Body_Procedure,
      Thunk_Entity,
      Treat_As_Volatile,
      Underlying_Full_View,
      Underlying_Record_View,
      Universal_Aliasing,
      Unset_Reference,
      Used_As_Generic_Actual,
      Uses_Lock_Free,
      Uses_Sec_Stack,
      Validated_Object,
      Warnings_Off,
      Warnings_Off_Used,
      Warnings_Off_Used_Unmodified,
      Warnings_Off_Used_Unreferenced,
      Was_Hidden,
      Wrapped_Entity,
      Wrapped_Statements

      --  End of entity fields.
     ); -- Opt_Field_Enum

   subtype Field_Enum is Opt_Field_Enum
     range Opt_Field_Enum'Succ (No_Field) .. Opt_Field_Enum'Last;
   --  Enumeration of fields -- Opt_Field_Enum without the special null value
   --  No_Field.

end Gen_IL.Fields;
