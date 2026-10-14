#include "graphics/shader/recompiler/backend/spirv/spirvEmitterInternal.h"

namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter {

uint32_t EmitAndConstant(EmitterState& state, uint32_t value, uint32_t mask) {
	const auto ret = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpBitwiseAnd, TypeU32(state), ret, value,
	                          ConstantU32(state, mask));
	return ret;
}

uint32_t EmitShiftRightConstant(EmitterState& state, uint32_t value, uint32_t shift) {
	const auto ret = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpShiftRightLogical, TypeU32(state), ret, value,
	                          ConstantU32(state, shift));
	return ret;
}

uint32_t EmitCompareU32Constant(EmitterState& state, spv::Op opcode, uint32_t value,
                                uint32_t constant) {
	const auto ret = state.builder.AllocateId();
	state.builder.AddFunction(opcode, TypeBool(state), ret, value, ConstantU32(state, constant));
	return ret;
}

uint32_t EmitSubConstantMinusU32(EmitterState& state, uint32_t constant, uint32_t value) {
	const auto ret = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpISub, TypeU32(state), ret, ConstantU32(state, constant),
	                          value);
	return ret;
}

uint32_t EmitF32ToF16RtzBits(EmitterState& state, uint32_t f32) {
	const auto bits = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpBitcast, TypeU32(state), bits, f32);

	const auto sign = EmitAndConstant(state, EmitShiftRightConstant(state, bits, 16), 0x8000u);
	const auto exp  = EmitAndConstant(state, EmitShiftRightConstant(state, bits, 23), 0xffu);
	const auto mant = EmitAndConstant(state, bits, 0x007fffffu);

	const auto half_exp       = state.builder.AllocateId();
	const auto normal_exp     = state.builder.AllocateId();
	const auto normal_mant    = EmitShiftRightConstant(state, mant, 13);
	const auto normal_payload = state.builder.AllocateId();
	const auto normal         = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpISub, TypeU32(state), half_exp, exp, ConstantU32(state, 112));
	state.builder.AddFunction(spv::OpShiftLeftLogical, TypeU32(state), normal_exp, half_exp,
	                          ConstantU32(state, 10));
	state.builder.AddFunction(spv::OpBitwiseOr, TypeU32(state), normal_payload, normal_exp,
	                          normal_mant);
	state.builder.AddFunction(spv::OpBitwiseOr, TypeU32(state), normal, sign, normal_payload);

	const auto mant_with_hidden = EmitOrU32(state, mant, ConstantU32(state, 0x00800000u));
	const auto raw_sub_shift    = EmitSubConstantMinusU32(state, 126, exp);
	const auto exp_lt_103       = EmitCompareU32Constant(state, spv::OpULessThan, exp, 103);
	const auto exp_gt_112       = EmitCompareU32Constant(state, spv::OpUGreaterThan, exp, 112);
	const auto sub_shift_low =
	    EmitSelectValueU32(state, exp_lt_103, ConstantU32(state, 31), raw_sub_shift);
	const auto sub_shift =
	    EmitSelectValueU32(state, exp_gt_112, ConstantU32(state, 14), sub_shift_low);
	const auto sub_mant  = state.builder.AllocateId();
	const auto subnormal = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpShiftRightLogical, TypeU32(state), sub_mant, mant_with_hidden,
	                          sub_shift);
	state.builder.AddFunction(spv::OpBitwiseOr, TypeU32(state), subnormal, sign, sub_mant);

	const auto nan_payload =
	    EmitOrU32(state, EmitShiftRightConstant(state, mant, 13), ConstantU32(state, 0x0200u));
	const auto nan =
	    EmitOrU32(state, sign, EmitOrU32(state, ConstantU32(state, 0x7c00u), nan_payload));
	const auto inf        = EmitOrU32(state, sign, ConstantU32(state, 0x7c00u));
	const auto max_finite = EmitOrU32(state, sign, ConstantU32(state, 0x7bffu));
	const auto mant_zero  = EmitCompareU32Constant(state, spv::OpIEqual, mant, 0);
	const auto special    = EmitSelectValueU32(state, mant_zero, inf, nan);

	const auto exp_le_112 = EmitCompareU32Constant(state, spv::OpULessThanEqual, exp, 112);
	const auto exp_ge_143 = EmitCompareU32Constant(state, spv::OpUGreaterThanEqual, exp, 143);
	const auto exp_eq_255 = EmitCompareU32Constant(state, spv::OpIEqual, exp, 255);
	const auto finite0    = EmitSelectValueU32(state, exp_le_112, subnormal, normal);
	const auto finite1    = EmitSelectValueU32(state, exp_lt_103, sign, finite0);
	const auto finite2    = EmitSelectValueU32(state, exp_ge_143, max_finite, finite1);
	return EmitAndConstant(state, EmitSelectValueU32(state, exp_eq_255, special, finite2), 0xffffu);
}

F32Class EmitClassifyF32Bits(EmitterState& state, uint32_t bits) {
	F32Class cls;
	cls.bits            = bits;
	const auto abs_bits = EmitAndConstant(state, cls.bits, 0x7fffffffu);
	cls.nan  = EmitCompareU32Constant(state, spv::OpUGreaterThan, abs_bits, 0x7f800000u);
	cls.zero = EmitCompareU32Constant(state, spv::OpIEqual, abs_bits, 0);
	return cls;
}

F32Class EmitClassifyF32(EmitterState& state, uint32_t value) {
	return EmitClassifyF32Bits(state, EmitBitcastF32ToU32(state, value));
}

uint32_t EmitClassMaskBitMatch(EmitterState& state, uint32_t mask, uint32_t bit,
                               uint32_t class_match) {
	const auto selected =
	    EmitCompareU32Constant(state, spv::OpINotEqual, EmitAndConstant(state, mask, 1u << bit), 0);
	return EmitLogicalAndBool(state, selected, class_match);
}

static uint32_t EmitClassMaskBits(EmitterState& state, uint32_t bits, uint32_t mask, bool half) {
	const uint32_t sign_mask     = half ? 0x8000u : 0x80000000u;
	const uint32_t exponent_mask = half ? 0x7c00u : 0x7f800000u;
	const uint32_t mantissa_mask = half ? 0x03ffu : 0x007fffffu;
	const uint32_t quiet_mask    = half ? 0x0200u : 0x00400000u;
	const auto     sign_bits     = EmitAndConstant(state, bits, sign_mask);
	const auto     abs_bits      = EmitAndConstant(state, bits, sign_mask - 1u);
	const auto     exponent_bits = EmitAndConstant(state, abs_bits, exponent_mask);
	const auto     mantissa_bits = EmitAndConstant(state, abs_bits, mantissa_mask);
	const auto     quiet_bits    = EmitAndConstant(state, mantissa_bits, quiet_mask);

	const auto sign             = EmitCompareU32Constant(state, spv::OpINotEqual, sign_bits, 0);
	const auto positive         = EmitLogicalNotBool(state, sign);
	const auto exponent_zero    = EmitCompareU32Constant(state, spv::OpIEqual, exponent_bits, 0);
	const auto exponent_nonzero = EmitLogicalNotBool(state, exponent_zero);
	const auto exponent_inf =
	    EmitCompareU32Constant(state, spv::OpIEqual, exponent_bits, exponent_mask);
	const auto finite_exponent  = EmitLogicalNotBool(state, exponent_inf);
	const auto mantissa_zero    = EmitCompareU32Constant(state, spv::OpIEqual, mantissa_bits, 0);
	const auto mantissa_nonzero = EmitLogicalNotBool(state, mantissa_zero);
	const auto quiet            = EmitCompareU32Constant(state, spv::OpINotEqual, quiet_bits, 0);

	const auto nan_common = EmitLogicalAndBool(state, exponent_inf, mantissa_nonzero);
	const auto snan       = EmitLogicalAndBool(state, nan_common, EmitLogicalNotBool(state, quiet));
	const auto qnan       = EmitLogicalAndBool(state, nan_common, quiet);
	const auto inf        = EmitLogicalAndBool(state, exponent_inf, mantissa_zero);
	const auto normal     = EmitLogicalAndBool(state, exponent_nonzero, finite_exponent);
	const auto denorm     = EmitLogicalAndBool(state, exponent_zero, mantissa_nonzero);
	const auto zero       = EmitCompareU32Constant(state, spv::OpIEqual, abs_bits, 0);

	uint32_t match = EmitClassMaskBitMatch(state, mask, 0, snan);
	match          = EmitLogicalOrBool(state, match, EmitClassMaskBitMatch(state, mask, 1, qnan));
	match          = EmitLogicalOrBool(
	    state, match, EmitClassMaskBitMatch(state, mask, 2, EmitLogicalAndBool(state, inf, sign)));
	match = EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 3, EmitLogicalAndBool(state, normal, sign)));
	match = EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 4, EmitLogicalAndBool(state, denorm, sign)));
	match = EmitLogicalOrBool(
	    state, match, EmitClassMaskBitMatch(state, mask, 5, EmitLogicalAndBool(state, zero, sign)));
	match = EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 6, EmitLogicalAndBool(state, zero, positive)));
	match = EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 7, EmitLogicalAndBool(state, denorm, positive)));
	match = EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 8, EmitLogicalAndBool(state, normal, positive)));
	return EmitLogicalOrBool(
	    state, match,
	    EmitClassMaskBitMatch(state, mask, 9, EmitLogicalAndBool(state, inf, positive)));
}

uint32_t EmitClassMaskF32(EmitterState& state, uint32_t value, uint32_t mask) {
	return EmitClassMaskBits(state, EmitBitcastF32ToU32(state, value), mask, false);
}

uint32_t EmitClassMaskF16(EmitterState& state, uint32_t bits, uint32_t mask) {
	return EmitClassMaskBits(state, bits, mask, true);
}

uint32_t EmitMinMaxF32Value(EmitterState& state, uint32_t lhs, uint32_t rhs, bool max_value) {
	const auto lhs_class = EmitClassifyF32(state, lhs);
	const auto rhs_class = EmitClassifyF32(state, rhs);

	const auto numeric_cond = state.builder.AllocateId();
	state.builder.AddFunction(max_value ? spv::OpFOrdGreaterThanEqual : spv::OpFOrdLessThan,
	                          TypeBool(state), numeric_cond, lhs, rhs);
	const auto ordered_bits =
	    EmitSelectValueU32(state, numeric_cond, lhs_class.bits, rhs_class.bits);

	const auto both_zero    = EmitLogicalAndBool(state, lhs_class.zero, rhs_class.zero);
	const auto zero_bits    = max_value ? EmitAndU32(state, lhs_class.bits, rhs_class.bits)
	                                    : EmitOrU32(state, lhs_class.bits, rhs_class.bits);
	const auto numeric_bits = EmitSelectValueU32(state, both_zero, zero_bits, ordered_bits);

	const auto rhs_nan_bits =
	    EmitSelectValueU32(state, rhs_class.nan, lhs_class.bits, numeric_bits);
	const auto result_bits = EmitSelectValueU32(state, lhs_class.nan, rhs_class.bits, rhs_nan_bits);
	return EmitBitcastU32ToF32(state, result_bits);
}

uint32_t EmitFlushF32DenormToSignedZero(EmitterState& state, uint32_t value) {
	const auto bits      = state.builder.AllocateId();
	const auto abs_bits  = state.builder.AllocateId();
	const auto sign_bits = state.builder.AllocateId();
	const auto subnormal = state.builder.AllocateId();
	const auto selected  = state.builder.AllocateId();
	const auto ret       = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpBitcast, TypeU32(state), bits, value);
	state.builder.AddFunction(spv::OpBitwiseAnd, TypeU32(state), abs_bits, bits,
	                          ConstantU32(state, 0x7fffffffu));
	state.builder.AddFunction(spv::OpBitwiseAnd, TypeU32(state), sign_bits, bits,
	                          ConstantU32(state, 0x80000000u));
	state.builder.AddFunction(spv::OpULessThan, TypeBool(state), subnormal, abs_bits,
	                          ConstantU32(state, 0x00800000u));
	state.builder.AddFunction(spv::OpSelect, TypeU32(state), selected, subnormal, sign_bits, bits);
	state.builder.AddFunction(spv::OpBitcast, TypeF32(state), ret, selected);
	return ret;
}

uint32_t EmitTrigCycleF32(EmitterState& state, uint32_t src, bool preserve_signed_zero) {
	const auto fract        = state.builder.AllocateId();
	const auto bits         = state.builder.AllocateId();
	const auto abs_bits     = state.builder.AllocateId();
	const auto large        = state.builder.AllocateId();
	const auto finite       = state.builder.AllocateId();
	const auto large_finite = state.builder.AllocateId();
	const auto reduced      = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpExtInst, TypeF32(state), fract, GlslStd450(state),
	                          GLSLstd450Fract, src);
	state.builder.AddFunction(spv::OpBitcast, TypeU32(state), bits, src);
	state.builder.AddFunction(spv::OpBitwiseAnd, TypeU32(state), abs_bits, bits,
	                          ConstantU32(state, 0x7fffffffu));
	state.builder.AddFunction(spv::OpUGreaterThanEqual, TypeBool(state), large, abs_bits,
	                          ConstantU32(state, 0x4b000000u));
	state.builder.AddFunction(spv::OpULessThan, TypeBool(state), finite, abs_bits,
	                          ConstantU32(state, 0x7f800000u));
	state.builder.AddFunction(spv::OpLogicalAnd, TypeBool(state), large_finite, large, finite);
	state.builder.AddFunction(spv::OpSelect, TypeF32(state), reduced, large_finite,
	                          ConstantF32(state, 0), fract);
	if (!preserve_signed_zero) {
		return reduced;
	}
	const auto zero = state.builder.AllocateId();
	const auto ret  = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpIEqual, TypeBool(state), zero, abs_bits,
	                          ConstantU32(state, 0));
	state.builder.AddFunction(spv::OpSelect, TypeF32(state), ret, zero, src, reduced);
	return ret;
}

uint32_t EmitF16BitsToF32(EmitterState& state, uint32_t bits) {
	const auto unpacked = state.builder.AllocateId();
	const auto ret      = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpExtInst, TypeF32Vector(state, 2), unpacked, GlslStd450(state),
	                          GLSLstd450UnpackHalf2x16, bits);
	state.builder.AddFunction(spv::OpCompositeExtract, TypeF32(state), ret, unpacked, 0);
	return ret;
}

// GLSLstd450PackHalf2x16 misrounds on some GPUs: the low lane (and only the low
// lane on the observed driver) truncates F32->F16 instead of round-to-nearest-
// even, breaking the half-precision 1-ulp edge cases. Lower the pack to the
// bitwise RTZ conversion (already exact) plus a mid-round increment. The width
// of the dropped mantissa region depends on the input exponent band, so the
// increment decision is computed per band:
//   exp 113..142 : normal half output; 13-bit dropped region, tie at 0x1000.
//   exp 103..112 : subnormal half output; shift width 126-exp, tie at 1<<shift.
//   exp     102  : rounds to 0x0001 unless the input is exactly 2^-25 (tie -> zero).
//   exp    >=143 : every finite input sits above the 65520 Inf tie; +1 turns the
//                  saturated 0x7bff into 0x7c00 (Inf), the architectural result.
//   exp    ==255 : Inf/NaN keep their RTZ bit pattern (no payload drift).
// An incremented half mantissa carries as exponents do (0x7bff+1 -> 0x7c00,
// subnormal 0x3ff+1 -> 0x0400 wraps into the smallest normal), which is IEEE
// RNE into the rounding-up direction. Both packed lanes use the same math.
uint32_t EmitF32ToF16RneBits(EmitterState& state, uint32_t f32) {
	const auto rtz_bits = EmitF32ToF16RtzBits(state, f32);
	const auto raw      = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpBitcast, TypeU32(state), raw, f32);
	const auto mant     = EmitAndConstant(state, raw, 0x007fffffu);
	const auto exponent = EmitAndConstant(state, EmitShiftRightConstant(state, raw, 23), 0xffu);

	// Packed half rounded away from zero in every band; its low bit doubles as
	// the kept-LSB test for both the normal and the subnormal bands.
	const auto kept_lsb =
	    EmitCompareU32Constant(state, spv::OpINotEqual, EmitAndConstant(state, rtz_bits, 1u), 0u);

	// Normal band: dropped 13-bit region, fixed tie at halfway.
	const auto dropped    = EmitAndConstant(state, mant, 0x00001fffu);
	const auto tie        = EmitCompareU32Constant(state, spv::OpIEqual, dropped, 0x1000u);
	const auto above_half = EmitCompareU32Constant(state, spv::OpUGreaterThan, dropped, 0x1000u);
	const auto up_normal  = EmitLogicalOrBool(state, above_half,
	                                          EmitLogicalAndBool(state, tie, kept_lsb));

	// Subnormal-output band: the conversion shifts by 126-exp (14..23 inside the
	// band), so the dropped region is mant & (2^shift - 1) and the tie sits at
	// 2^shift / 2. Keep the shift in range even outside the band (14) so the
	// SPIR-V shift ops never see an undefined operand count.
	const auto exp_le_112 =
	    EmitCompareU32Constant(state, spv::OpULessThanEqual, exponent, 112);
	const auto exp_lt_103 = EmitCompareU32Constant(state, spv::OpULessThan, exponent, 103);
	const auto raw_sub_shift = EmitSubConstantMinusU32(state, 126, exponent);
	const auto sub_shift =
	    EmitSelectValueU32(state, exp_le_112, raw_sub_shift, ConstantU32(state, 14));
	const auto step = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpShiftLeftLogical, TypeU32(state), step,
	                          ConstantU32(state, 1), sub_shift);
	const auto mask = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpISub, TypeU32(state), mask, step, ConstantU32(state, 1));
	const auto dropped_sub = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpBitwiseAnd, TypeU32(state), dropped_sub, mant, mask);
	const auto halfway = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpShiftRightLogical, TypeU32(state), halfway, step,
	                          ConstantU32(state, 1));
	const auto sub_above = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpUGreaterThan, TypeBool(state), sub_above, dropped_sub,
	                          halfway);
	const auto sub_tie = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpIEqual, TypeBool(state), sub_tie, dropped_sub, halfway);
	const auto up_subnormal =
	    EmitLogicalOrBool(state, sub_above, EmitLogicalAndBool(state, sub_tie, kept_lsb));

	// Band predicates and the special exponent cases.
	const auto exp_ge_143 =
	    EmitCompareU32Constant(state, spv::OpUGreaterThanEqual, exponent, 143);
	const auto exp_eq_255 = EmitCompareU32Constant(state, spv::OpIEqual, exponent, 255);
	const auto exp_eq_102 = EmitCompareU32Constant(state, spv::OpIEqual, exponent, 102);
	const auto mant_not_zero =
	    EmitCompareU32Constant(state, spv::OpINotEqual, mant, 0);
	const auto band_normal = EmitLogicalAndBool(state, EmitLogicalNotBool(state, exp_le_112),
	                                            EmitLogicalNotBool(state, exp_ge_143));
	const auto band_sub    = EmitLogicalAndBool(state, exp_le_112,
	                                            EmitLogicalNotBool(state, exp_lt_103));
	const auto over_cond   = EmitLogicalAndBool(state, exp_ge_143,
	                                            EmitLogicalNotBool(state, exp_eq_255));
	const auto up_small    = EmitLogicalAndBool(state, exp_eq_102, mant_not_zero);

	// Combine: exp==255 -> no increment; overflow -> always; normal band ->
	// 13-bit rule; subnormal band -> shift-width rule; exp==102 -> mant!=0.
	const auto one  = ConstantU32(state, 1);
	const auto zero = ConstantU32(state, 0);
	const auto as_num =
	    [](EmitterState& st, uint32_t cond) { return EmitSelectValueU32(st, cond, ConstantU32(st, 1), ConstantU32(st, 0)); };
	const auto sel_102   = as_num(state, up_small);
	const auto sel_small = EmitSelectValueU32(state, band_sub, as_num(state, up_subnormal), sel_102);
	const auto sel_normal =
	    EmitSelectValueU32(state, band_normal, as_num(state, up_normal), sel_small);
	const auto incr_num = EmitSelectValueU32(state, over_cond, one, sel_normal);

	const auto do_add = EmitCompareU32Constant(state, spv::OpINotEqual, incr_num, 0);
	return EmitSelectValueU32(
	    state, do_add, EmitAddU32(state, rtz_bits, ConstantU32(state, 1)), rtz_bits);
}

uint32_t EmitPackHalf2x16(EmitterState& state, uint32_t pair) {
	const auto low  = state.builder.AllocateId();
	const auto high = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpCompositeExtract, TypeF32(state), low, pair, 0);
	state.builder.AddFunction(spv::OpCompositeExtract, TypeF32(state), high, pair, 1);
	const auto low_bits  = EmitF32ToF16RneBits(state, low);
	const auto high_bits = EmitF32ToF16RneBits(state, high);
	const auto high_move = state.builder.AllocateId();
	state.builder.AddFunction(spv::OpShiftLeftLogical, TypeU32(state), high_move, high_bits,
	                          ConstantU32(state, 16));
	return EmitOrU32(state, low_bits, high_move);
}

} // namespace Libs::Graphics::ShaderRecompiler::Spirv::Emitter
