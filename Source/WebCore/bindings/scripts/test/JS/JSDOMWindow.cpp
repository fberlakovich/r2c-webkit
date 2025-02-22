/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSDOMWindow.h"

#include "ActiveDOMObject.h"
#include "DOMIsoSubspaces.h"
#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMWindow.h"
#include "JSDOMWrapperCache.h"
#include "JSExposedToWorkerAndWindow.h"
#include "JSTestConditionalIncludes.h"
#include "JSTestConditionallyReadWrite.h"
#include "JSTestDefaultToJSON.h"
#include "JSTestDefaultToJSONFilteredByExposed.h"
#include "JSTestEnabledBySetting.h"
#include "JSTestEnabledForContext.h"
#include "JSTestNode.h"
#include "JSTestObj.h"
#include "JSTestPromiseRejectionEvent.h"
#include "JSWindowProxy.h"
#include "ScriptExecutionContext.h"
#include "WebCoreJSClientData.h"
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/JSDestructibleObjectHeapCellType.h>
#include <JavaScriptCore/SubspaceInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>

#if ENABLE(Condition1) || ENABLE(Condition2)
#include "JSTestInterface.h"
#endif


namespace WebCore {
using namespace JSC;

// Attributes

JSC_DECLARE_CUSTOM_GETTER(jsDOMWindowConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_DOMWindowConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_DOMWindowConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_ExposedToWorkerAndWindowConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_ExposedToWorkerAndWindowConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestConditionalIncludesConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestConditionalIncludesConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestConditionallyReadWriteConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestConditionallyReadWriteConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestDefaultToJSONConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestDefaultToJSONConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestDefaultToJSONFilteredByExposedConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestDefaultToJSONFilteredByExposedConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestEnabledBySettingConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestEnabledBySettingConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestEnabledForContextConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestEnabledForContextConstructor);
#if ENABLE(Condition1) || ENABLE(Condition2)
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestInterfaceConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestInterfaceConstructor);
#endif
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestNodeConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestNodeConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestObjectConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestObjectConstructor);
JSC_DECLARE_CUSTOM_GETTER(jsDOMWindow_TestPromiseRejectionEventConstructor);
JSC_DECLARE_CUSTOM_SETTER(setJSDOMWindow_TestPromiseRejectionEventConstructor);

using JSDOMWindowDOMConstructor = JSDOMConstructorNotConstructable<JSDOMWindow>;

/* Hash table */

static const struct CompactHashIndex JSDOMWindowTableIndex[37] = {
    { -1, -1 },
    { 5, 35 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 4, 33 },
    { -1, -1 },
    { 2, -1 },
    { 1, 32 },
    { 0, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 9, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 3, 34 },
    { 6, 36 },
    { 7, -1 },
    { 8, -1 },
    { 10, -1 },
};


static const HashTableValue JSDOMWindowTableValues[] =
{
    { "DOMWindow", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_DOMWindowConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_DOMWindowConstructor) } },
    { "ExposedToWorkerAndWindow", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_ExposedToWorkerAndWindowConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_ExposedToWorkerAndWindowConstructor) } },
    { "TestConditionalIncludes", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestConditionalIncludesConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestConditionalIncludesConstructor) } },
    { "TestConditionallyReadWrite", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestConditionallyReadWriteConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestConditionallyReadWriteConstructor) } },
    { "TestDefaultToJSON", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestDefaultToJSONConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestDefaultToJSONConstructor) } },
    { "TestDefaultToJSONFilteredByExposed", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestDefaultToJSONFilteredByExposedConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestDefaultToJSONFilteredByExposedConstructor) } },
    { "TestEnabledBySetting", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestEnabledBySettingConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestEnabledBySettingConstructor) } },
#if ENABLE(Condition1) || ENABLE(Condition2)
    { "TestInterface", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestInterfaceConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestInterfaceConstructor) } },
#else
    { 0, 0, NoIntrinsic, { 0, 0 } },
#endif
    { "TestNode", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestNodeConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestNodeConstructor) } },
    { "TestObject", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestObjectConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestObjectConstructor) } },
    { "TestPromiseRejectionEvent", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindow_TestPromiseRejectionEventConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSDOMWindow_TestPromiseRejectionEventConstructor) } },
};

static const HashTable JSDOMWindowTable = { 11, 31, true, JSDOMWindow::info(), JSDOMWindowTableValues, JSDOMWindowTableIndex };
template<> JSValue JSDOMWindowDOMConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    return JSEventTarget::getConstructor(vm, &globalObject);
}

template<> void JSDOMWindowDOMConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, globalObject.getPrototypeDirect(vm), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, "DOMWindow"_s), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSDOMWindowDOMConstructor::s_info = { "DOMWindow", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSDOMWindowDOMConstructor) };

/* Hash table for prototype */

static const struct CompactHashIndex JSDOMWindowPrototypeTableIndex[2] = {
    { -1, -1 },
    { 0, -1 },
};


static const HashTableValue JSDOMWindowPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsDOMWindowConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
};

static const HashTable JSDOMWindowPrototypeTable = { 1, 1, true, JSDOMWindow::info(), JSDOMWindowPrototypeTableValues, JSDOMWindowPrototypeTableIndex };
const ClassInfo JSDOMWindowPrototype::s_info = { "DOMWindow", &Base::s_info, &JSDOMWindowPrototypeTable, nullptr, CREATE_METHOD_TABLE(JSDOMWindowPrototype) };

void JSDOMWindowPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSDOMWindow::info(), JSDOMWindowPrototypeTableValues, *this);
    JSC_TO_STRING_TAG_WITHOUT_TRANSITION();
}

const ClassInfo JSDOMWindow::s_info = { "DOMWindow", &Base::s_info, &JSDOMWindowTable, nullptr, CREATE_METHOD_TABLE(JSDOMWindow) };

JSDOMWindow::JSDOMWindow(VM& vm, Structure* structure, Ref<DOMWindow>&& impl, JSWindowProxy* proxy)
    : JSEventTarget(vm, structure, WTFMove(impl), proxy)
{
}

void JSDOMWindow::finishCreation(VM& vm, JSWindowProxy* proxy)
{
    Base::finishCreation(vm, proxy);

    static_assert(!std::is_base_of<ActiveDOMObject, DOMWindow>::value, "Interface is not marked as [ActiveDOMObject] even though implementation class subclasses ActiveDOMObject.");

    if ((jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext() && TestEnabledForContext::enabledForContext(*jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext())))
        putDirectCustomAccessor(vm, static_cast<JSVMClientData*>(vm.clientData)->builtinNames().TestEnabledForContextPublicName(), CustomGetterSetter::create(vm, jsDOMWindow_TestEnabledForContextConstructor, setJSDOMWindow_TestEnabledForContextConstructor), attributesForStructure(static_cast<unsigned>(JSC::PropertyAttribute::DontEnum)));
}

JSValue JSDOMWindow::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSDOMWindowDOMConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

template<> inline JSDOMWindow* IDLAttribute<JSDOMWindow>::cast(JSGlobalObject& lexicalGlobalObject, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSDOMWindow*>(JSC::getVM(&lexicalGlobalObject), JSValue::decode(thisValue));
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindowConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSDOMWindowPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSDOMWindow::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

static inline JSValue jsDOMWindow_DOMWindowConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSDOMWindow::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_DOMWindowConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_DOMWindowConstructorGetter>(*lexicalGlobalObject, thisValue, "DOMWindow");
}

static inline bool setJSDOMWindow_DOMWindowConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("DOMWindow"), strlen("DOMWindow")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_DOMWindowConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_DOMWindowConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "DOMWindow");
}

static inline JSValue jsDOMWindow_ExposedToWorkerAndWindowConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSExposedToWorkerAndWindow::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_ExposedToWorkerAndWindowConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_ExposedToWorkerAndWindowConstructorGetter>(*lexicalGlobalObject, thisValue, "ExposedToWorkerAndWindow");
}

static inline bool setJSDOMWindow_ExposedToWorkerAndWindowConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("ExposedToWorkerAndWindow"), strlen("ExposedToWorkerAndWindow")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_ExposedToWorkerAndWindowConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_ExposedToWorkerAndWindowConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "ExposedToWorkerAndWindow");
}

static inline JSValue jsDOMWindow_TestConditionalIncludesConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestConditionalIncludes::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestConditionalIncludesConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestConditionalIncludesConstructorGetter>(*lexicalGlobalObject, thisValue, "TestConditionalIncludes");
}

static inline bool setJSDOMWindow_TestConditionalIncludesConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestConditionalIncludes"), strlen("TestConditionalIncludes")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestConditionalIncludesConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestConditionalIncludesConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestConditionalIncludes");
}

static inline JSValue jsDOMWindow_TestConditionallyReadWriteConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestConditionallyReadWrite::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestConditionallyReadWriteConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestConditionallyReadWriteConstructorGetter>(*lexicalGlobalObject, thisValue, "TestConditionallyReadWrite");
}

static inline bool setJSDOMWindow_TestConditionallyReadWriteConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestConditionallyReadWrite"), strlen("TestConditionallyReadWrite")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestConditionallyReadWriteConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestConditionallyReadWriteConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestConditionallyReadWrite");
}

static inline JSValue jsDOMWindow_TestDefaultToJSONConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestDefaultToJSON::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestDefaultToJSONConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestDefaultToJSONConstructorGetter>(*lexicalGlobalObject, thisValue, "TestDefaultToJSON");
}

static inline bool setJSDOMWindow_TestDefaultToJSONConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestDefaultToJSON"), strlen("TestDefaultToJSON")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestDefaultToJSONConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestDefaultToJSONConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestDefaultToJSON");
}

static inline JSValue jsDOMWindow_TestDefaultToJSONFilteredByExposedConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestDefaultToJSONFilteredByExposed::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestDefaultToJSONFilteredByExposedConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestDefaultToJSONFilteredByExposedConstructorGetter>(*lexicalGlobalObject, thisValue, "TestDefaultToJSONFilteredByExposed");
}

static inline bool setJSDOMWindow_TestDefaultToJSONFilteredByExposedConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestDefaultToJSONFilteredByExposed"), strlen("TestDefaultToJSONFilteredByExposed")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestDefaultToJSONFilteredByExposedConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestDefaultToJSONFilteredByExposedConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestDefaultToJSONFilteredByExposed");
}

static inline JSValue jsDOMWindow_TestEnabledBySettingConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestEnabledBySetting::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestEnabledBySettingConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestEnabledBySettingConstructorGetter>(*lexicalGlobalObject, thisValue, "TestEnabledBySetting");
}

static inline bool setJSDOMWindow_TestEnabledBySettingConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestEnabledBySetting"), strlen("TestEnabledBySetting")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestEnabledBySettingConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestEnabledBySettingConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestEnabledBySetting");
}

static inline JSValue jsDOMWindow_TestEnabledForContextConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestEnabledForContext::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestEnabledForContextConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestEnabledForContextConstructorGetter>(*lexicalGlobalObject, thisValue, "TestEnabledForContext");
}

static inline bool setJSDOMWindow_TestEnabledForContextConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestEnabledForContext"), strlen("TestEnabledForContext")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestEnabledForContextConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestEnabledForContextConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestEnabledForContext");
}

#if ENABLE(Condition1) || ENABLE(Condition2)
static inline JSValue jsDOMWindow_TestInterfaceConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestInterface::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestInterfaceConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestInterfaceConstructorGetter>(*lexicalGlobalObject, thisValue, "TestInterface");
}

#endif

#if ENABLE(Condition1) || ENABLE(Condition2)
static inline bool setJSDOMWindow_TestInterfaceConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestInterface"), strlen("TestInterface")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestInterfaceConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestInterfaceConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestInterface");
}

#endif

static inline JSValue jsDOMWindow_TestNodeConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestNode::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestNodeConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestNodeConstructorGetter>(*lexicalGlobalObject, thisValue, "TestNode");
}

static inline bool setJSDOMWindow_TestNodeConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestNode"), strlen("TestNode")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestNodeConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestNodeConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestNode");
}

static inline JSValue jsDOMWindow_TestObjectConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestObj::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestObjectConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestObjectConstructorGetter>(*lexicalGlobalObject, thisValue, "TestObject");
}

static inline bool setJSDOMWindow_TestObjectConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestObject"), strlen("TestObject")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestObjectConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestObjectConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestObject");
}

static inline JSValue jsDOMWindow_TestPromiseRejectionEventConstructorGetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return JSTestPromiseRejectionEvent::getConstructor(JSC::getVM(&lexicalGlobalObject), &thisObject);
}

JSC_DEFINE_CUSTOM_GETTER(jsDOMWindow_TestPromiseRejectionEventConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSDOMWindow>::get<jsDOMWindow_TestPromiseRejectionEventConstructorGetter>(*lexicalGlobalObject, thisValue, "TestPromiseRejectionEvent");
}

static inline bool setJSDOMWindow_TestPromiseRejectionEventConstructorSetter(JSGlobalObject& lexicalGlobalObject, JSDOMWindow& thisObject, JSValue value)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    // Shadowing a built-in constructor.
    return thisObject.putDirect(vm, Identifier::fromString(vm, reinterpret_cast<const LChar*>("TestPromiseRejectionEvent"), strlen("TestPromiseRejectionEvent")), value);
}

JSC_DEFINE_CUSTOM_SETTER(setJSDOMWindow_TestPromiseRejectionEventConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue))
{
    return IDLAttribute<JSDOMWindow>::set<setJSDOMWindow_TestPromiseRejectionEventConstructorSetter>(*lexicalGlobalObject, thisValue, encodedValue, "TestPromiseRejectionEvent");
}

JSC::IsoSubspace* JSDOMWindow::subspaceForImpl(JSC::VM& vm)
{
    auto& clientData = *static_cast<JSVMClientData*>(vm.clientData);
    auto& spaces = clientData.subspaces();
    if (auto* space = spaces.m_subspaceForDOMWindow.get())
        return space;
    spaces.m_subspaceForDOMWindow = makeUnique<IsoSubspace> ISO_SUBSPACE_INIT(vm.heap, clientData.m_heapCellTypeForJSDOMWindow.get(), JSDOMWindow);
    auto* space = spaces.m_subspaceForDOMWindow.get();
IGNORE_WARNINGS_BEGIN("unreachable-code")
IGNORE_WARNINGS_BEGIN("tautological-compare")
    if (&JSDOMWindow::visitOutputConstraints != &JSC::JSCell::visitOutputConstraints)
        clientData.outputConstraintSpaces().append(space);
IGNORE_WARNINGS_END
IGNORE_WARNINGS_END
    return space;
}

void JSDOMWindow::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSDOMWindow*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, "url " + thisObject->scriptExecutionContext()->url().string());
    Base::analyzeHeap(cell, analyzer);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7DOMWindow@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore9DOMWindowE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<DOMWindow>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    const void* actualVTablePointer = getVTablePointer(impl.ptr());
#if PLATFORM(WIN)
    void* expectedVTablePointer = __identifier("??_7DOMWindow@WebCore@@6B@");
#else
    void* expectedVTablePointer = &_ZTVN7WebCore9DOMWindowE[2];
#endif

    // If this fails DOMWindow does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<DOMWindow>::value, "DOMWindow is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // DOMWindow has subclasses. If DOMWindow has subclasses that get passed
    // to toJS() we currently require DOMWindow you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<DOMWindow>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, DOMWindow& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}

DOMWindow* JSDOMWindow::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSDOMWindow*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
