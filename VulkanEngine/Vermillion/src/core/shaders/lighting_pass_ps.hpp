#if 0
; SPIR-V
; Version: 1.0
; Generator: Google spiregg; 0
; Bound: 44
; Schema: 0
               OpCapability Shader
               OpCapability InputAttachment
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main" %out_var_SV_Target
               OpExecutionMode %main OriginUpperLeft
               OpSource HLSL 600
               OpName %type_subpass_image "type.subpass.image"
               OpName %gPos "gPos"
               OpName %gCol "gCol"
               OpName %gNorm "gNorm"
               OpName %out_var_SV_Target "out.var.SV_Target"
               OpName %main "main"
               OpDecorate %gPos InputAttachmentIndex 0
               OpDecorate %gCol InputAttachmentIndex 1
               OpDecorate %gNorm InputAttachmentIndex 2
               OpDecorate %out_var_SV_Target Location 0
               OpDecorate %gPos DescriptorSet 0
               OpDecorate %gPos Binding 0
               OpDecorate %gCol DescriptorSet 0
               OpDecorate %gCol Binding 1
               OpDecorate %gNorm DescriptorSet 0
               OpDecorate %gNorm Binding 2
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
      %v2int = OpTypeVector %int 2
         %11 = OpConstantComposite %v2int %int_0 %int_0
      %float = OpTypeFloat 32
    %float_5 = OpConstant %float 5
   %float_n5 = OpConstant %float -5
    %v3float = OpTypeVector %float 3
         %16 = OpConstantComposite %v3float %float_5 %float_n5 %float_n5
   %float_30 = OpConstant %float 30
%float_0_100000001 = OpConstant %float 0.100000001
%type_subpass_image = OpTypeImage %float SubpassData 2 0 0 2 Unknown
%_ptr_UniformConstant_type_subpass_image = OpTypePointer UniformConstant %type_subpass_image
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %void = OpTypeVoid
         %23 = OpTypeFunction %void
       %gPos = OpVariable %_ptr_UniformConstant_type_subpass_image UniformConstant
       %gCol = OpVariable %_ptr_UniformConstant_type_subpass_image UniformConstant
      %gNorm = OpVariable %_ptr_UniformConstant_type_subpass_image UniformConstant
%out_var_SV_Target = OpVariable %_ptr_Output_v4float Output
       %main = OpFunction %void None %23
         %24 = OpLabel
         %25 = OpLoad %type_subpass_image %gPos
         %26 = OpImageRead %v4float %25 %11 None
         %27 = OpLoad %type_subpass_image %gCol
         %28 = OpImageRead %v4float %27 %11 None
         %29 = OpLoad %type_subpass_image %gNorm
         %30 = OpImageRead %v4float %29 %11 None
         %31 = OpVectorShuffle %v3float %26 %26 0 1 2
         %32 = OpFSub %v3float %16 %31
         %33 = OpExtInst %float %1 Length %32
         %34 = OpFMul %float %33 %33
         %35 = OpFDiv %float %float_30 %34
         %36 = OpExtInst %v3float %1 Normalize %32
         %37 = OpVectorShuffle %v3float %30 %30 0 1 2
         %38 = OpDot %float %36 %37
         %39 = OpFMul %float %38 %35
         %40 = OpExtInst %float %1 FMax %39 %float_0_100000001
         %41 = OpVectorShuffle %v3float %28 %28 0 1 2
         %42 = OpVectorTimesScalar %v3float %41 %40
         %43 = OpVectorShuffle %v4float %28 %42 4 5 6 3
               OpStore %out_var_SV_Target %43
               OpReturn
               OpFunctionEnd

#endif

const unsigned char lighting_pass_ps[] = {
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x28, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c,
  0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00,
  0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x0f, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x10, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00,
  0x05, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65,
  0x2e, 0x73, 0x75, 0x62, 0x70, 0x61, 0x73, 0x73, 0x2e, 0x69, 0x6d, 0x61,
  0x67, 0x65, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x67, 0x50, 0x6f, 0x73, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x67, 0x43, 0x6f, 0x6c, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x67, 0x4e, 0x6f, 0x72,
  0x6d, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x6f, 0x75, 0x74, 0x2e, 0x76, 0x61, 0x72, 0x2e, 0x53, 0x56, 0x5f, 0x54,
  0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x16, 0x00, 0x03, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xa0, 0x40, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0xc0, 0x17, 0x00, 0x04, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x2c, 0x00, 0x06, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xf0, 0x41, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x12, 0x00, 0x00, 0x00, 0xcd, 0xcc, 0xcc, 0x3d, 0x19, 0x00, 0x09, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
  0x16, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x17, 0x00, 0x00, 0x00,
  0x16, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
  0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x62, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x1a, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x1b, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x62, 0x00, 0x06, 0x00,
  0x14, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x62, 0x00, 0x06, 0x00, 0x14, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x4f, 0x00, 0x08, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
  0x1a, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x83, 0x00, 0x05, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x1f, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x22, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
  0x88, 0x00, 0x05, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x11, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x06, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x45, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x94, 0x00, 0x05, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x26, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00,
  0x85, 0x00, 0x05, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
  0x26, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x07, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x28, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
  0x4f, 0x00, 0x08, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00,
  0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00,
  0x0f, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00,
  0x28, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x09, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};
