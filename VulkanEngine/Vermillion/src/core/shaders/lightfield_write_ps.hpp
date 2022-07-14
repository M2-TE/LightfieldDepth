#if 0
; SPIR-V
; Version: 1.0
; Generator: Google spiregg; 0
; Bound: 41
; Schema: 0
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main" %in_var_WorldPos %in_var_Color %in_var_Normal %out_var_SV_Target
               OpExecutionMode %main OriginUpperLeft
               OpSource HLSL 600
               OpName %in_var_WorldPos "in.var.WorldPos"
               OpName %in_var_Color "in.var.Color"
               OpName %in_var_Normal "in.var.Normal"
               OpName %out_var_SV_Target "out.var.SV_Target"
               OpName %main "main"
               OpDecorate %in_var_WorldPos Location 0
               OpDecorate %in_var_Color Location 1
               OpDecorate %in_var_Normal Location 2
               OpDecorate %out_var_SV_Target Location 0
      %float = OpTypeFloat 32
    %float_0 = OpConstant %float 0
  %float_n10 = OpConstant %float -10
    %v3float = OpTypeVector %float 3
         %11 = OpConstantComposite %v3float %float_0 %float_0 %float_n10
    %float_1 = OpConstant %float 1
%float_0_300000012 = OpConstant %float 0.300000012
%float_0_0500000007 = OpConstant %float 0.0500000007
    %v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %void = OpTypeVoid
         %19 = OpTypeFunction %void
%in_var_WorldPos = OpVariable %_ptr_Input_v4float Input
%in_var_Color = OpVariable %_ptr_Input_v4float Input
%in_var_Normal = OpVariable %_ptr_Input_v4float Input
%out_var_SV_Target = OpVariable %_ptr_Output_v4float Output
       %main = OpFunction %void None %19
         %20 = OpLabel
         %21 = OpLoad %v4float %in_var_WorldPos
         %22 = OpLoad %v4float %in_var_Color
         %23 = OpLoad %v4float %in_var_Normal
         %24 = OpVectorShuffle %v3float %21 %21 0 1 2
         %25 = OpFSub %v3float %11 %24
         %26 = OpExtInst %float %1 Length %25
         %27 = OpExtInst %float %1 Pow %26 %float_0_300000012
         %28 = OpFDiv %float %float_1 %27
         %29 = OpVectorShuffle %v3float %22 %22 0 1 2
         %30 = OpExtInst %v3float %1 Normalize %25
         %31 = OpVectorShuffle %v3float %23 %23 0 1 2
         %32 = OpDot %float %30 %31
         %33 = OpFMul %float %32 %28
         %34 = OpExtInst %float %1 FMax %33 %float_0_0500000007
         %35 = OpVectorTimesScalar %v3float %29 %34
         %36 = OpCompositeExtract %float %22 3
         %37 = OpCompositeExtract %float %35 0
         %38 = OpCompositeExtract %float %35 1
         %39 = OpCompositeExtract %float %35 2
         %40 = OpCompositeConstruct %v4float %37 %38 %39 %36
               OpStore %out_var_SV_Target %40
               OpReturn
               OpFunctionEnd

#endif

const unsigned char lightfield_write_ps[] = {
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
  0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x09, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x58, 0x02, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x57, 0x6f, 0x72, 0x6c, 0x64,
  0x50, 0x6f, 0x73, 0x00, 0x05, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x43, 0x6f, 0x6c, 0x6f, 0x72,
  0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x69, 0x6e, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x4e, 0x6f, 0x72, 0x6d, 0x61,
  0x6c, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x6f, 0x75, 0x74, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x53, 0x56, 0x5f, 0x54,
  0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xc1,
  0x17, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2b, 0x00, 0x04, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x9a, 0x99, 0x99, 0x3e,
  0x2b, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0xcd, 0xcc, 0x4c, 0x3d, 0x17, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x03, 0x00, 0x13, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0xf8, 0x00, 0x02, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
  0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x83, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x19, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x88, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x1c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
  0x4f, 0x00, 0x08, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x16, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x06, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x45, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x94, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
  0x85, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x07, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x28, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x8e, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x25, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x51, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
  0x23, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x00, 0x00,
  0x28, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
  0x27, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
  0x38, 0x00, 0x01, 0x00
};
