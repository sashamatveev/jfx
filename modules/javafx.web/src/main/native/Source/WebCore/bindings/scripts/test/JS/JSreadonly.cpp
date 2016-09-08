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
#include "JSreadonly.h"

#include "JSDOMBinding.h"
#include "readonly.h"
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

// Attributes

JSC::EncodedJSValue jsreadonlyConstructor(JSC::ExecState*, JSC::JSObject*, JSC::EncodedJSValue, JSC::PropertyName);

class JSreadonlyPrototype : public JSC::JSNonFinalObject {
public:
    typedef JSC::JSNonFinalObject Base;
    static JSreadonlyPrototype* create(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSreadonlyPrototype* ptr = new (NotNull, JSC::allocateCell<JSreadonlyPrototype>(vm.heap)) JSreadonlyPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSreadonlyPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

class JSreadonlyConstructor : public DOMConstructorObject {
private:
    JSreadonlyConstructor(JSC::Structure*, JSDOMGlobalObject*);
    void finishCreation(JSC::VM&, JSDOMGlobalObject*);

public:
    typedef DOMConstructorObject Base;
    static JSreadonlyConstructor* create(JSC::VM& vm, JSC::Structure* structure, JSDOMGlobalObject* globalObject)
    {
        JSreadonlyConstructor* ptr = new (NotNull, JSC::allocateCell<JSreadonlyConstructor>(vm.heap)) JSreadonlyConstructor(structure, globalObject);
        ptr->finishCreation(vm, globalObject);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }
};

const ClassInfo JSreadonlyConstructor::s_info = { "readonlyConstructor", &Base::s_info, 0, CREATE_METHOD_TABLE(JSreadonlyConstructor) };

JSreadonlyConstructor::JSreadonlyConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorObject(structure, globalObject)
{
}

void JSreadonlyConstructor::finishCreation(VM& vm, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    putDirect(vm, vm.propertyNames->prototype, JSreadonly::getPrototype(vm, globalObject), DontDelete | ReadOnly);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontDelete | DontEnum);
}

/* Hash table for prototype */

static const HashTableValue JSreadonlyPrototypeTableValues[] =
{
    { "constructor", DontEnum | ReadOnly, NoIntrinsic, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsreadonlyConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) },
};

const ClassInfo JSreadonlyPrototype::s_info = { "readonlyPrototype", &Base::s_info, 0, CREATE_METHOD_TABLE(JSreadonlyPrototype) };

void JSreadonlyPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSreadonlyPrototypeTableValues, *this);
}

const ClassInfo JSreadonly::s_info = { "readonly", &Base::s_info, 0, CREATE_METHOD_TABLE(JSreadonly) };

JSreadonly::JSreadonly(Structure* structure, JSDOMGlobalObject* globalObject, Ref<readonly>&& impl)
    : JSDOMWrapper(structure, globalObject)
    , m_impl(&impl.leakRef())
{
}

JSObject* JSreadonly::createPrototype(VM& vm, JSGlobalObject* globalObject)
{
    return JSreadonlyPrototype::create(vm, globalObject, JSreadonlyPrototype::createStructure(vm, globalObject, globalObject->objectPrototype()));
}

JSObject* JSreadonly::getPrototype(VM& vm, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSreadonly>(vm, globalObject);
}

void JSreadonly::destroy(JSC::JSCell* cell)
{
    JSreadonly* thisObject = static_cast<JSreadonly*>(cell);
    thisObject->JSreadonly::~JSreadonly();
}

JSreadonly::~JSreadonly()
{
    releaseImpl();
}

EncodedJSValue jsreadonlyConstructor(ExecState* exec, JSObject* baseValue, EncodedJSValue, PropertyName)
{
    JSreadonlyPrototype* domObject = jsDynamicCast<JSreadonlyPrototype*>(baseValue);
    if (!domObject)
        return throwVMTypeError(exec);
    return JSValue::encode(JSreadonly::getConstructor(exec->vm(), domObject->globalObject()));
}

JSValue JSreadonly::getConstructor(VM& vm, JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSreadonlyConstructor>(vm, jsCast<JSDOMGlobalObject*>(globalObject));
}

bool JSreadonlyOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSreadonlyOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsreadonly = jsCast<JSreadonly*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsreadonly->impl(), jsreadonly);
}

JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject* globalObject, readonly* impl)
{
    if (!impl)
        return jsNull();
    if (JSValue result = getExistingWrapper<JSreadonly>(globalObject, impl))
        return result;
#if COMPILER(CLANG)
    // If you hit this failure the interface definition has the ImplementationLacksVTable
    // attribute. You should remove that attribute. If the class has subclasses
    // that may be passed through this toJS() function you should use the SkipVTableValidation
    // attribute to readonly.
    COMPILE_ASSERT(!__is_polymorphic(readonly), readonly_is_polymorphic_but_idl_claims_not_to_be);
#endif
    return createNewWrapper<JSreadonly>(globalObject, impl);
}

readonly* JSreadonly::toWrapped(JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSreadonly*>(value))
        return &wrapper->impl();
    return nullptr;
}

}