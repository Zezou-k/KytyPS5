#include "graphics/shader/recompiler/frontend/translate/Translator.h"

#include <bit>
#include <utility>

namespace Libs::Graphics::ShaderRecompiler::Frontend {

void Translator::EmitCompareResult(const Decoder::Instruction& inst, IR::U1 value, bool scalar,
                                   bool cmpx) {
	if (scalar) {
		ir.SetScc(value);
		return;
	}
	const auto masked = ir.LogicalAnd(ir.GetExec(), value);
	if (cmpx) {
		const auto mask = BallotMask(masked);
		ir.SetExec(masked);
		ir.SetExecLo(mask[0]);
		ir.SetExecHi(mask[1]);
		return;
	}
	WriteMask(inst.dst, masked);
}

void Translator::EmitCompareConstant(const Decoder::Instruction& inst, bool value, bool scalar,
                                     bool cmpx) {
	EmitCompareResult(inst, IR::U1(IR::Value(value)), scalar, cmpx);
}

void Translator::EmitIntegerCompare(const Decoder::Instruction& inst, IR::ValueOpcode opcode,
                                    IR::Type type, bool scalar, bool cmpx, bool swap_operands) {
	// The equality compares share their IR opcode with the unsigned ones.
	const bool signed_64 = opcode == IR::ValueOpcode::SLessThan64 ||
	                       opcode == IR::ValueOpcode::SLessThanEqual64 ||
	                       inst.opcode == Decoder::Opcode::V_CMP_EQ_I64 ||
	                       inst.opcode == Decoder::Opcode::V_CMPX_EQ_I64 ||
	                       inst.opcode == Decoder::Opcode::V_CMP_NE_I64 ||
	                       inst.opcode == Decoder::Opcode::V_CMPX_NE_I64;
	const auto read = [&](const Decoder::Operand& operand) {
		// RDNA2 expands signed 64-bit integer literals by sign extension.
		if (signed_64 && operand.kind == Decoder::OperandKind::LiteralConstant) {
			return IR::Value(static_cast<uint64_t>(
			    static_cast<int64_t>(std::bit_cast<int32_t>(operand.value))));
		}
		return ReadOperand(operand, type);
	};
	auto lhs = read(inst.src0);
	auto rhs = read(inst.src1);
	if (swap_operands) {
		std::swap(lhs, rhs);
	}
	EmitCompareResult(inst, IR::U1(ir.Emit(opcode, {lhs, rhs})), scalar, cmpx);
}

void Translator::EmitInteger16Compare(const Decoder::Instruction& inst, IR::ValueOpcode opcode,
                                      bool signed_value, bool cmpx) {
	const auto lhs = ReadU16AsU32(inst.src0, signed_value);
	const auto rhs = ReadU16AsU32(inst.src1, signed_value);
	EmitCompareResult(inst, IR::U1(ir.Emit(opcode, {lhs, rhs})), false, cmpx);
}

void Translator::EmitFloatCompare(const Decoder::Instruction& inst, IR::ValueOpcode opcode,
                                  bool half, bool cmpx) {
	const auto type = IR::ArgTypeOf(opcode, 0);
	const auto lhs =
	    half ? IR::Value(ReadF16AsF32(inst.src0)) : ReadOperand(inst.src0, type);
	const auto rhs =
	    half ? IR::Value(ReadF16AsF32(inst.src1)) : ReadOperand(inst.src1, type);
	const IR::FPCompareFlags flags{
	    .flush_input_denorms = !half && type == IR::Type::F32 && flush_f32_inputs};
	EmitCompareResult(inst, IR::U1(ir.Emit(opcode, {lhs, rhs}, flags)), false, cmpx);
}

// Raw-bit helper (declared in Translator.h): every OpFOrd*/OpFUnord* F64 compare
// miscompiles on some GPUs (NaN pairs can still compare as equal, +Inf pairs as
// unequal), while 64-bit integer compares behave; judge F64 edge cases on the
// raw bits with integer logic instead. The operand stays two U32 dwords: some
// drivers also miscompile the F64<->U64 bitcast round-trip itself.
Translator::F64OperandBits Translator::ReadF64OperandBits(const Decoder::Operand& operand) {
	F64OperandBits out{};
	if (operand.kind == Decoder::OperandKind::LiteralConstant) {
		// FP64 literals only supply the high word; the low word is zero.
		out.low  = IR::U32(IR::Value(0u));
		out.high = IR::U32(IR::Value(operand.value));
	} else {
		const auto pair = ReadU32Pair(operand);
		out.low         = pair[0];
		out.high        = pair[1];
	}
	if (operand.absolute) {
		out.high = ir.BitwiseAnd(out.high, IR::U32(IR::Value(0x7fffffffu)));
	}
	if (operand.negate) {
		out.high = ir.BitwiseXor(out.high, IR::U32(IR::Value(0x80000000u)));
	}
	// exponent==0x7ff and (top mantissa bits OR low mantissa bits) non-zero covers
	// every NaN payload, including signaling NaNs that only set a low-dword bit.
	const auto exponent = ir.BitwiseAnd(out.high, IR::U32(IR::Value(0x7ff00000u)));
	const auto is_exponent = IR::U1(ir.Emit(IR::ValueOpcode::IEqual32,
	    {exponent, IR::U32(IR::Value(0x7ff00000u))}));
	const auto mantissa = ir.BitwiseOr(
	    ir.BitwiseAnd(out.high, IR::U32(IR::Value(0x000fffffu))), out.low);
	const auto not_zero = ir.INotEqual(mantissa, IR::U32(IR::Value(0u)));
	out.is_nan = ir.LogicalAnd(is_exponent, not_zero);
	// +0 and -0 (exactly) both have a zero exponent, zero mantissa and zero low
	// dword; +Inf has a zero low dword too but the 0x7ff exponent, and a NaN can
	// hide bits in the low dword, so check the low dword explicitly.
	const auto magnitude_zero = IR::U1(ir.Emit(IR::ValueOpcode::IEqual32,
	    {ir.BitwiseAnd(out.high, IR::U32(IR::Value(0x7fffffffu))),
	     IR::U32(IR::Value(0u))}));
	const auto low_zero = IR::U1(ir.Emit(IR::ValueOpcode::IEqual32,
	    {out.low, IR::U32(IR::Value(0u))}));
	out.is_zero = ir.LogicalAnd(magnitude_zero, low_zero);
	return out;
}

// Equality on F64 values: NaN never equal, +0 == -0, otherwise the raw bits
// match. Compare the two 32-bit dword pairs: some drivers also miscompile
// 64-bit OpIEqual on bitcast round-trips.
IR::U1 Translator::IsFloatNumericEqual64(const F64OperandBits& lhs, const F64OperandBits& rhs) {
	const auto low_equal = IR::U1(ir.Emit(IR::ValueOpcode::IEqual32, {lhs.low, rhs.low}));
	const auto high_equal = IR::U1(ir.Emit(IR::ValueOpcode::IEqual32, {lhs.high, rhs.high}));
	const auto bits_equal = ir.LogicalAnd(low_equal, high_equal);
	return ir.LogicalOr(bits_equal, ir.LogicalAnd(lhs.is_zero, rhs.is_zero));
}

void Translator::EmitFloatEqual(const Decoder::Instruction& inst, bool cmpx, bool is64) {
	// Every OpFOrd*/OpFUnord* F64 compare miscompiles on some GPUs, while 32-bit
	// integer compares behave. GCN's EQ from integer logic: ordered AND not-equal.
	const auto lhs       = ReadF64OperandBits(inst.src0);
	const auto rhs       = ReadF64OperandBits(inst.src1);
	const auto ordered   = ir.LogicalNot(ir.LogicalOr(lhs.is_nan, rhs.is_nan));
	EmitCompareResult(inst, ir.LogicalAnd(ordered, IsFloatNumericEqual64(lhs, rhs)), false,
	                  cmpx);
}

void Translator::EmitFloatUnordEqual(const Decoder::Instruction& inst, bool cmpx) {
	// GCN's NLG (unordered OR equal) from integer logic:
	// NLG = (lhs is NaN OR rhs is NaN) OR numeric-equal.
	const auto lhs        = ReadF64OperandBits(inst.src0);
	const auto rhs        = ReadF64OperandBits(inst.src1);
	const auto unordered  = ir.LogicalOr(lhs.is_nan, rhs.is_nan);
	EmitCompareResult(inst, ir.LogicalOr(unordered, IsFloatNumericEqual64(lhs, rhs)), false,
	                  cmpx);
}

void Translator::EmitFloatNotEqual(const Decoder::Instruction& inst, bool cmpx, bool is64) {
	// Every OpFOrd*/OpFUnord* F64 compare miscompiles on some GPUs, while the 64-bit
	// integer compares behave. Express GCN's ordered compare (LG) from the raw bits:
	// LG = ordered AND (bits differ).
	if (is64) {
		// GCN's ordered compare (LG): ordered AND not numerically-equal.
		const auto lhs      = ReadF64OperandBits(inst.src0);
		const auto rhs      = ReadF64OperandBits(inst.src1);
		const auto ordered  = ir.LogicalNot(ir.LogicalOr(lhs.is_nan, rhs.is_nan));
		const auto not_equal =
		    ir.LogicalNot(IsFloatNumericEqual64(lhs, rhs));
		EmitCompareResult(inst, ir.LogicalAnd(ordered, not_equal), false, cmpx);
		return;
	}
	EmitFloatCompare(inst, IR::ValueOpcode::FPOrdNotEqual32, false, cmpx);
}

void Translator::EmitFloatOrderedCompare(const Decoder::Instruction& inst, bool ordered, bool cmpx,
                                         bool is64) {
	const auto type      = is64 ? IR::Type::F64 : IR::Type::F32;
	const auto is_nan_op = is64 ? IR::ValueOpcode::FPIsNan64 : IR::ValueOpcode::FPIsNan32;
	const auto lhs       = ReadOperand(inst.src0, type);
	const auto rhs       = ReadOperand(inst.src1, type);
	const auto unordered = ir.LogicalOr(IR::U1(ir.Emit(is_nan_op, {lhs})),
	                                    IR::U1(ir.Emit(is_nan_op, {rhs})));
	EmitCompareResult(inst, ordered ? ir.LogicalNot(unordered) : unordered, false, cmpx);
}

void Translator::EmitFloatClassCompare(const Decoder::Instruction& inst, bool cmpx, bool half) {
	const auto value = half ? IR::Value(Read16LaneBits(inst.src0, false))
	                        : ReadOperand(inst.src0, IR::Type::F32);
	const auto mask = half ? IR::Value(Read16LaneBits(inst.src1, false))
	                      : ReadOperand(inst.src1, IR::Type::U32);
	const auto opcode = half ? IR::ValueOpcode::FPCmpClass16 : IR::ValueOpcode::FPCmpClass32;
	EmitCompareResult(inst, IR::U1(ir.Emit(opcode, {value, mask})), false, cmpx);
}

} // namespace Libs::Graphics::ShaderRecompiler::Frontend
