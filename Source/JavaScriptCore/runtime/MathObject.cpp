/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007-2019 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "MathObject.h"

#include "JSCInlines.h"
#include "MathCommon.h"
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(MathObject);

JSC_DECLARE_HOST_FUNCTION(mathProtoFuncACos);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncACosh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncASin);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncASinh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncATan);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncATanh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncATan2);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncCbrt);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncCeil);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncClz32);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncCos);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncCosh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncExp);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncExpm1);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncFround);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncHypot);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncLog);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncLog1p);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncLog10);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncLog2);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncMax);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncPow);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncRandom);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncRound);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncSign);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncSin);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncSinh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncSqrt);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncTan);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncTanh);
JSC_DECLARE_HOST_FUNCTION(mathProtoFuncIMul);

const ClassInfo MathObject::s_info = { "Math", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(MathObject) };

MathObject::MathObject(VM& vm, Structure* structure)
    : JSNonFinalObject(vm, structure)
{
}

void MathObject::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

    putDirectWithoutTransition(vm, Identifier::fromString(vm, "E"), jsNumber(Math::exp(1.0)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "LN2"), jsNumber(Math::log(2.0)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "LN10"), jsNumber(Math::log(10.0)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "LOG2E"), jsNumber(1.0 / Math::log(2.0)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "LOG10E"), jsNumber(0.4342944819032518), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "PI"), jsNumber(piDouble), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "SQRT1_2"), jsNumber(sqrt(0.5)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    putDirectWithoutTransition(vm, Identifier::fromString(vm, "SQRT2"), jsNumber(sqrt(2.0)), PropertyAttribute::DontDelete | PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
    JSC_TO_STRING_TAG_WITHOUT_TRANSITION();

    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "abs"), 1, mathProtoFuncAbs, AbsIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "acos"), 1, mathProtoFuncACos, ACosIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "asin"), 1, mathProtoFuncASin, ASinIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "atan"), 1, mathProtoFuncATan, ATanIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "acosh"), 1, mathProtoFuncACosh, ACoshIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "asinh"), 1, mathProtoFuncASinh, ASinhIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "atanh"), 1, mathProtoFuncATanh, ATanhIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "atan2"), 2, mathProtoFuncATan2, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "cbrt"), 1, mathProtoFuncCbrt, CbrtIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "ceil"), 1, mathProtoFuncCeil, CeilIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "clz32"), 1, mathProtoFuncClz32, Clz32Intrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "cos"), 1, mathProtoFuncCos, CosIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "cosh"), 1, mathProtoFuncCosh, CoshIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "exp"), 1, mathProtoFuncExp, ExpIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "expm1"), 1, mathProtoFuncExpm1, Expm1Intrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "floor"), 1, mathProtoFuncFloor, FloorIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "fround"), 1, mathProtoFuncFround, FRoundIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "hypot"), 2, mathProtoFuncHypot, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "log"), 1, mathProtoFuncLog, LogIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "log10"), 1, mathProtoFuncLog10, Log10Intrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "log1p"), 1, mathProtoFuncLog1p, Log1pIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "log2"), 1, mathProtoFuncLog2, Log2Intrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "max"), 2, mathProtoFuncMax, MaxIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "min"), 2, mathProtoFuncMin, MinIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "pow"), 2, mathProtoFuncPow, PowIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "random"), 0, mathProtoFuncRandom, RandomIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "round"), 1, mathProtoFuncRound, RoundIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "sign"), 1, mathProtoFuncSign, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "sin"), 1, mathProtoFuncSin, SinIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "sinh"), 1, mathProtoFuncSinh, SinhIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "sqrt"), 1, mathProtoFuncSqrt, SqrtIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "tan"), 1, mathProtoFuncTan, TanIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "tanh"), 1, mathProtoFuncTanh, TanhIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "trunc"), 1, mathProtoFuncTrunc, TruncIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirectNativeFunctionWithoutTransition(vm, globalObject, Identifier::fromString(vm, "imul"), 2, mathProtoFuncIMul, IMulIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
}

// ------------------------------ Functions --------------------------------

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncAbs, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsNumber(fabs(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncACos, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::acos(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncASin, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::asin(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncATan, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::atan(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncATan2, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    double arg0 = callFrame->argument(0).toNumber(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    scope.release();
    double arg1 = callFrame->argument(1).toNumber(globalObject);
    return JSValue::encode(jsDoubleNumber(atan2(arg0, arg1)));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncCeil, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsNumber(ceil(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncClz32, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    uint32_t value = callFrame->argument(0).toUInt32(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    return JSValue::encode(JSValue(clz(value)));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncCos, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::cos(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncExp, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::exp(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncFloor, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsNumber(floor(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncHypot, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    unsigned argsCount = callFrame->argumentCount();
    double max = 0;
    Vector<double, 8> args;
    args.reserveInitialCapacity(argsCount);
    for (unsigned i = 0; i < argsCount; ++i) {
        args.uncheckedAppend(callFrame->uncheckedArgument(i).toNumber(globalObject));
        RETURN_IF_EXCEPTION(scope, encodedJSValue());
        if (std::isinf(args[i]))
            return JSValue::encode(jsDoubleNumber(+std::numeric_limits<double>::infinity()));
        max = std::max(fabs(args[i]), max);
    }
    if (!max)
        max = 1;
    // Kahan summation algorithm significantly reduces the numerical error in the total obtained.
    double sum = 0;
    double compensation = 0;
    for (double argument : args) {
        double scaledArgument = argument / max;
        double summand = scaledArgument * scaledArgument - compensation;
        double preliminary = sum + summand;
        compensation = (preliminary - sum) - summand;
        sum = preliminary;
    }
    return JSValue::encode(jsDoubleNumber(sqrt(sum) * max));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncLog, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::log(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncMax, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    unsigned argsCount = callFrame->argumentCount();
    double result = -std::numeric_limits<double>::infinity();
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = callFrame->uncheckedArgument(k).toNumber(globalObject);
        RETURN_IF_EXCEPTION(scope, encodedJSValue());
        if (std::isnan(val)) {
            result = PNaN;
        } else if (val > result || (!val && !result && !std::signbit(val)))
            result = val;
    }
    return JSValue::encode(jsNumber(result));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncMin, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    unsigned argsCount = callFrame->argumentCount();
    double result = +std::numeric_limits<double>::infinity();
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = callFrame->uncheckedArgument(k).toNumber(globalObject);
        RETURN_IF_EXCEPTION(scope, encodedJSValue());
        if (std::isnan(val)) {
            result = PNaN;
        } else if (val < result || (!val && !result && std::signbit(val)))
            result = val;
    }
    return JSValue::encode(jsNumber(result));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncPow, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    // ECMA 15.8.2.1.13

    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    double arg = callFrame->argument(0).toNumber(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    scope.release();
    double arg2 = callFrame->argument(1).toNumber(globalObject);

    return JSValue::encode(JSValue(operationMathPow(arg, arg2)));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncRandom, (JSGlobalObject* globalObject, CallFrame*))
{
    return JSValue::encode(jsDoubleNumber(globalObject->weakRandomNumber()));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncRound, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsNumber(Math::roundDouble(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncSign, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    double arg = callFrame->argument(0).toNumber(globalObject);
    if (std::isnan(arg))
        return JSValue::encode(jsNaN());
    if (!arg)
        return JSValue::encode(std::signbit(arg) ? jsNumber(-0.0) : jsNumber(0));
    return JSValue::encode(jsNumber(std::signbit(arg) ? -1 : 1));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncSin, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::sin(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncSqrt, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(sqrt(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncTan, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::tan(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncIMul, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    int32_t left = callFrame->argument(0).toInt32(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    scope.release();
    int32_t right = callFrame->argument(1).toInt32(globalObject);
    return JSValue::encode(jsNumber(left * right));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncACosh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::acosh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncASinh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::asinh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncATanh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::atanh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncCbrt, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::cbrt(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncCosh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::cosh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncExpm1, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::expm1(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncFround, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(static_cast<float>(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncLog1p, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::log1p(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncLog10, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::log10(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncLog2, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::log2(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncSinh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::sinh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncTanh, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsDoubleNumber(Math::tanh(callFrame->argument(0).toNumber(globalObject))));
}

JSC_DEFINE_HOST_FUNCTION(mathProtoFuncTrunc, (JSGlobalObject* globalObject, CallFrame* callFrame))
{
    return JSValue::encode(jsNumber(callFrame->argument(0).toIntegerPreserveNaN(globalObject)));
}

} // namespace JSC
