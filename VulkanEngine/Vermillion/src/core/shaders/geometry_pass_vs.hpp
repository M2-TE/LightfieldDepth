#if 0
; SPIR-V
; Version: 1.0
; Generator: Google spiregg; 0
; Bound: 29
; Schema: 0
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Vertex %main "main" %in_var_Position %in_var_Color %in_var_Normal %gl_Position %out_var_WorldPos %out_var_Color %out_var_Normal
               OpSource HLSL 600
               OpName %type_ViewProjectionBuffer "type.ViewProjectionBuffer"
               OpMemberName %type_ViewProjectionBuffer 0 "view"
               OpMemberName %type_ViewProjectionBuffer 1 "proj"
               OpMemberName %type_ViewProjectionBuffer 2 "viewProj"
               OpName %ViewProjectionBuffer "ViewProjectionBuffer"
               OpName %in_var_Position "in.var.Position"
               OpName %in_var_Color "in.var.Color"
               OpName %in_var_Normal "in.var.Normal"
               OpName %out_var_WorldPos "out.var.WorldPos"
               OpName %out_var_Color "out.var.Color"
               OpName %out_var_Normal "out.var.Normal"
               OpName %main "main"
               OpDecorate %gl_Position BuiltIn Position
               OpDecorate %in_var_Position Location 0
               OpDecorate %in_var_Color Location 1
               OpDecorate %in_var_Normal Location 2
               OpDecorate %out_var_WorldPos Location 0
               OpDecorate %out_var_Color Location 1
               OpDecorate %out_var_Normal Location 2
               OpDecorate %ViewProjectionBuffer DescriptorSet 0
               OpDecorate %ViewProjectionBuffer Binding 1
               OpMemberDecorate %type_ViewProjectionBuffer 0 Offset 0
               OpMemberDecorate %type_ViewProjectionBuffer 0 MatrixStride 16
               OpMemberDecorate %type_ViewProjectionBuffer 0 RowMajor
               OpMemberDecorate %type_ViewProjectionBuffer 1 Offset 64
               OpMemberDecorate %type_ViewProjectionBuffer 1 MatrixStride 16
               OpMemberDecorate %type_ViewProjectionBuffer 1 RowMajor
               OpMemberDecorate %type_ViewProjectionBuffer 2 Offset 128
               OpMemberDecorate %type_ViewProjectionBuffer 2 MatrixStride 16
               OpMemberDecorate %type_ViewProjectionBuffer 2 RowMajor
               OpDecorate %type_ViewProjectionBuffer Block
        %int = OpTypeInt 32 1
      %int_2 = OpConstant %int 2
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%mat4v4float = OpTypeMatrix %v4float 4
%type_ViewProjectionBuffer = OpTypeStruct %mat4v4float %mat4v4float %mat4v4float
%_ptr_Uniform_type_ViewProjectionBuffer = OpTypePointer Uniform %type_ViewProjectionBuffer
%_ptr_Input_v4float = OpTypePointer Input %v4float
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %void = OpTypeVoid
         %20 = OpTypeFunction %void
%_ptr_Uniform_mat4v4float = OpTypePointer Uniform %mat4v4float
%ViewProjectionBuffer = OpVariable %_ptr_Uniform_type_ViewProjectionBuffer Uniform
%in_var_Position = OpVariable %_ptr_Input_v4float Input
%in_var_Color = OpVariable %_ptr_Input_v4float Input
%in_var_Normal = OpVariable %_ptr_Input_v4float Input
%gl_Position = OpVariable %_ptr_Output_v4float Output
%out_var_WorldPos = OpVariable %_ptr_Output_v4float Output
%out_var_Color = OpVariable %_ptr_Output_v4float Output
%out_var_Normal = OpVariable %_ptr_Output_v4float Output
       %main = OpFunction %void None %20
         %22 = OpLabel
         %23 = OpLoad %v4float %in_var_Position
         %24 = OpLoad %v4float %in_var_Color
         %25 = OpLoad %v4float %in_var_Normal
         %26 = OpAccessChain %_ptr_Uniform_mat4v4float %ViewProjectionBuffer %int_2
         %27 = OpLoad %mat4v4float %26
         %28 = OpVectorTimesMatrix %v4float %23 %27
               OpStore %gl_Position %28
               OpStore %out_var_WorldPos %23
               OpStore %out_var_Color %24
               OpStore %out_var_Normal %25
               OpReturn
               OpFunctionEnd

#endif

const unsigned char geometry_pass_vs[] = {
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x58, 0x02, 0x00, 0x00, 0x05, 0x00, 0x09, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x74, 0x79, 0x70, 0x65, 0x2e, 0x56, 0x69, 0x65, 0x77, 0x50, 0x72, 0x6f,
  0x6a, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x42, 0x75, 0x66, 0x66, 0x65,
  0x72, 0x00, 0x00, 0x00, 0x06, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x76, 0x69, 0x65, 0x77, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x70, 0x72, 0x6f, 0x6a, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x76, 0x69, 0x65, 0x77,
  0x50, 0x72, 0x6f, 0x6a, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x08, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x56, 0x69, 0x65, 0x77, 0x50, 0x72, 0x6f, 0x6a,
  0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x42, 0x75, 0x66, 0x66, 0x65, 0x72,
  0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x50, 0x6f, 0x73, 0x69, 0x74,
  0x69, 0x6f, 0x6e, 0x00, 0x05, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x43, 0x6f, 0x6c, 0x6f, 0x72,
  0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x4e, 0x6f, 0x72, 0x6d, 0x61,
  0x6c, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x6f, 0x75, 0x74, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x57, 0x6f, 0x72, 0x6c,
  0x64, 0x50, 0x6f, 0x73, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x2e, 0x76, 0x61, 0x72, 0x2e,
  0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x2e, 0x76, 0x61, 0x72, 0x2e,
  0x4e, 0x6f, 0x72, 0x6d, 0x61, 0x6c, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x48, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x48, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x40, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x48, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
  0x48, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x48, 0x00, 0x04, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x15, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x18, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x12, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x02, 0x00, 0x13, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
  0x14, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x15, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x11, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x12, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x14, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x18, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x41, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
  0x90, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
  0x38, 0x00, 0x01, 0x00
};
