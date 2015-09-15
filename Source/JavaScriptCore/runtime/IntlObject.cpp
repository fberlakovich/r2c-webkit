/*
 * Copyright (C) 2015 Andy VanWagoner (thetalecrafter@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IntlObject.h"

#if ENABLE(INTL)

#include "Error.h"
#include "FunctionPrototype.h"
#include "IntlCollator.h"
#include "IntlCollatorConstructor.h"
#include "IntlCollatorPrototype.h"
#include "IntlDateTimeFormat.h"
#include "IntlDateTimeFormatConstructor.h"
#include "IntlDateTimeFormatPrototype.h"
#include "IntlNumberFormat.h"
#include "IntlNumberFormatConstructor.h"
#include "IntlNumberFormatPrototype.h"
#include "JSCInlines.h"
#include "JSCJSValueInlines.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include <unicode/uloc.h>
#include <wtf/Assertions.h>

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(IntlObject);

}

namespace JSC {

const ClassInfo IntlObject::s_info = { "Object", &Base::s_info, 0, CREATE_METHOD_TABLE(IntlObject) };

IntlObject::IntlObject(VM& vm, Structure* structure)
    : JSNonFinalObject(vm, structure)
{
}

IntlObject* IntlObject::create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
{
    IntlObject* object = new (NotNull, allocateCell<IntlObject>(vm.heap)) IntlObject(vm, structure);
    object->finishCreation(vm, globalObject);
    return object;
}

void IntlObject::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));

    // Set up Collator.
    IntlCollatorPrototype* collatorPrototype = IntlCollatorPrototype::create(vm, globalObject, IntlCollatorPrototype::createStructure(vm, globalObject, globalObject->objectPrototype()));
    Structure* collatorStructure = IntlCollator::createStructure(vm, globalObject, collatorPrototype);
    IntlCollatorConstructor* collatorConstructor = IntlCollatorConstructor::create(vm, IntlCollatorConstructor::createStructure(vm, globalObject, globalObject->functionPrototype()), collatorPrototype, collatorStructure);

    collatorPrototype->putDirectWithoutTransition(vm, vm.propertyNames->constructor, collatorConstructor, DontEnum);

    // Set up NumberFormat.
    IntlNumberFormatPrototype* numberFormatPrototype = IntlNumberFormatPrototype::create(vm, globalObject, IntlNumberFormatPrototype::createStructure(vm, globalObject, globalObject->objectPrototype()));
    Structure* numberFormatStructure = IntlNumberFormat::createStructure(vm, globalObject, numberFormatPrototype);
    IntlNumberFormatConstructor* numberFormatConstructor = IntlNumberFormatConstructor::create(vm, IntlNumberFormatConstructor::createStructure(vm, globalObject, globalObject->functionPrototype()), numberFormatPrototype, numberFormatStructure);

    numberFormatPrototype->putDirectWithoutTransition(vm, vm.propertyNames->constructor, numberFormatConstructor, DontEnum);

    // Set up DateTimeFormat.
    IntlDateTimeFormatPrototype* dateTimeFormatPrototype = IntlDateTimeFormatPrototype::create(vm, globalObject, IntlDateTimeFormatPrototype::createStructure(vm, globalObject, globalObject->objectPrototype()));
    Structure* dateTimeFormatStructure = IntlDateTimeFormat::createStructure(vm, globalObject, dateTimeFormatPrototype);
    IntlDateTimeFormatConstructor* dateTimeFormatConstructor = IntlDateTimeFormatConstructor::create(vm, IntlDateTimeFormatConstructor::createStructure(vm, globalObject, globalObject->functionPrototype()), dateTimeFormatPrototype, dateTimeFormatStructure);

    dateTimeFormatPrototype->putDirectWithoutTransition(vm, vm.propertyNames->constructor, dateTimeFormatConstructor, DontEnum);

    // 8.1 Properties of the Intl Object (ECMA-402 2.0)
    putDirectWithoutTransition(vm, vm.propertyNames->Collator, collatorConstructor, DontEnum);
    putDirectWithoutTransition(vm, vm.propertyNames->NumberFormat, numberFormatConstructor, DontEnum);
    putDirectWithoutTransition(vm, vm.propertyNames->DateTimeFormat, dateTimeFormatConstructor, DontEnum);
}

Structure* IntlObject::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

static String getIntlStringOption(ExecState* exec, JSValue options, PropertyName property, const HashSet<String>& values, const char* notFound, String fallback)
{
    // 9.2.9 GetOption (options, property, type, values, fallback)
    // For type="string".

    // 1. Let opts be ToObject(options).
    JSObject* opts = options.toObject(exec);

    // 2. ReturnIfAbrupt(opts).
    if (exec->hadException())
        return String();

    // 3. Let value be Get(opts, property).
    JSValue value = opts->get(exec, property);

    // 4. ReturnIfAbrupt(value).
    if (exec->hadException())
        return String();

    // 5. If value is not undefined, then
    if (!value.isUndefined()) {
        // a. Assert: type is "boolean" or "string".
        // Function dedicated to "string".

        // c. If type is "string", then
        // i. Let value be ToString(value).
        JSString* stringValue = value.toString(exec);

        // ii. ReturnIfAbrupt(value).
        if (exec->hadException())
            return String();

        // d. If values is not undefined, then
        // i. If values does not contain an element equal to value, throw a RangeError exception.
        if (!values.isEmpty() && !values.contains(stringValue->value(exec))) {
            exec->vm().throwException(exec, createRangeError(exec, String(notFound)));
            return String();
        }

        // e. Return value.
        return stringValue->value(exec);
    }

    // 6. Else return fallback.
    return fallback;
}

static String getPrivateUseLangTag(const Vector<String>& parts, size_t startIndex)
{
    size_t numParts = parts.size();
    size_t currentIndex = startIndex;

    // Check for privateuse.
    // privateuse = "x" 1*("-" (2*8alphanum))
    StringBuilder privateuse;
    while (currentIndex < numParts) {
        const String& singleton = parts[currentIndex];
        unsigned singletonLength = singleton.length();
        bool isValid = (singletonLength == 1 && (singleton == "x" || singleton == "X"));
        if (!isValid)
            break;

        if (currentIndex != startIndex)
            privateuse.append('-');

        ++currentIndex;
        unsigned numExtParts = 0;
        privateuse.append('x');
        while (currentIndex < numParts) {
            const String& extPart = parts[currentIndex];
            unsigned extPartLength = extPart.length();

            bool isValid = (extPartLength >= 2 && extPartLength <= 8 && extPart.isAllSpecialCharacters<isASCIIAlphanumeric>());
            if (!isValid)
                break;

            ++currentIndex;
            ++numExtParts;
            privateuse.append('-');
            privateuse.append(extPart.convertToASCIILowercase());
        }

        // Requires at least one production.
        if (!numExtParts)
            return String();
    }

    // Leftovers makes it invalid.
    if (currentIndex < numParts)
        return String();

    return privateuse.toString();
}

static String getCanonicalLangTag(const Vector<String>& parts)
{
    ASSERT(!parts.isEmpty());

    // Follows the grammar at https://www.rfc-editor.org/rfc/bcp/bcp47.txt
    // langtag = language ["-" script] ["-" region] *("-" variant) *("-" extension) ["-" privateuse]

    size_t numParts = parts.size();
    // Check for language.
    // language = 2*3ALPHA ["-" extlang] / 4ALPHA / 5*8ALPHA
    size_t currentIndex = 0;
    const String& language = parts[currentIndex];
    unsigned languageLength = language.length();
    bool canHaveExtlang = languageLength >= 2 && languageLength <= 3;
    bool isValidLanguage = languageLength >= 2 && languageLength <= 8 && language.isAllSpecialCharacters<isASCIIAlpha>();
    if (!isValidLanguage)
        return String();

    ++currentIndex;
    StringBuilder canonical;
    canonical.append(language.convertToASCIILowercase());

    // Check for extlang.
    // extlang = 3ALPHA *2("-" 3ALPHA)
    if (canHaveExtlang) {
        for (unsigned times = 0; times < 3 && currentIndex < numParts; ++times) {
            const String& extlang = parts[currentIndex];
            unsigned extlangLength = extlang.length();
            if (extlangLength == 3 && extlang.isAllSpecialCharacters<isASCIIAlpha>()) {
                ++currentIndex;
                canonical.append('-');
                canonical.append(extlang.convertToASCIILowercase());
            } else
                break;
        }
    }

    // Check for script.
    // script = 4ALPHA
    if (currentIndex < numParts) {
        const String& script = parts[currentIndex];
        unsigned scriptLength = script.length();
        if (scriptLength == 4 && script.isAllSpecialCharacters<isASCIIAlpha>()) {
            ++currentIndex;
            canonical.append('-');
            canonical.append(toASCIIUpper(script[0]));
            canonical.append(script.substring(1, 3).convertToASCIILowercase());
        }
    }

    // Check for region.
    // region = 2ALPHA / 3DIGIT
    if (currentIndex < numParts) {
        const String& region = parts[currentIndex];
        unsigned regionLength = region.length();
        bool isValidRegion = (
            (regionLength == 2 && region.isAllSpecialCharacters<isASCIIAlpha>())
            || (regionLength == 3 && region.isAllSpecialCharacters<isASCIIDigit>())
        );
        if (isValidRegion) {
            ++currentIndex;
            canonical.append('-');
            canonical.append(region.upper());
        }
    }

    // Check for variant.
    // variant = 5*8alphanum / (DIGIT 3alphanum)
    HashSet<String> subtags;
    while (currentIndex < numParts) {
        const String& variant = parts[currentIndex];
        unsigned variantLength = variant.length();
        bool isValidVariant = (
            (variantLength >= 5 && variantLength <= 8 && variant.isAllSpecialCharacters<isASCIIAlphanumeric>())
            || (variantLength == 4 && isASCIIDigit(variant[0]) && variant.substring(1, 3).isAllSpecialCharacters<isASCIIAlphanumeric>())
        );
        if (!isValidVariant)
            break;

        // Cannot include duplicate subtags (case insensitive).
        String lowerVariant = variant.convertToASCIILowercase();
        if (!subtags.add(lowerVariant).isNewEntry)
            return String();

        ++currentIndex;

        // Reordering variant subtags is not required in the spec.
        canonical.append('-');
        canonical.append(lowerVariant);
    }

    // Check for extension.
    // extension = singleton 1*("-" (2*8alphanum))
    // singleton = alphanum except x or X
    subtags.clear();
    Vector<String> extensions;
    while (currentIndex < numParts) {
        const String& possibleSingleton = parts[currentIndex];
        unsigned singletonLength = possibleSingleton.length();
        bool isValidSingleton = (singletonLength == 1 && possibleSingleton != "x" && possibleSingleton != "X" && isASCIIAlphanumeric(possibleSingleton[0]));
        if (!isValidSingleton)
            break;

        // Cannot include duplicate singleton (case insensitive).
        String singleton = possibleSingleton.convertToASCIILowercase();
        if (!subtags.add(singleton).isNewEntry)
            return String();

        ++currentIndex;
        int numExtParts = 0;
        StringBuilder extension;
        extension.append(singleton);
        while (currentIndex < numParts) {
            const String& extPart = parts[currentIndex];
            unsigned extPartLength = extPart.length();

            bool isValid = (extPartLength >= 2 && extPartLength <= 8 && extPart.isAllSpecialCharacters<isASCIIAlphanumeric>());
            if (!isValid)
                break;

            ++currentIndex;
            ++numExtParts;
            extension.append('-');
            extension.append(extPart.convertToASCIILowercase());
        }

        // Requires at least one production.
        if (!numExtParts)
            return String();

        extensions.append(extension.toString());
    }

    // Add extensions to canonical sorted by singleton.
    std::sort(
        extensions.begin(),
        extensions.end(),
        [] (const String& a, const String& b) -> bool {
            return a[0] < b[0];
        }
    );
    size_t numExtenstions = extensions.size();
    for (size_t i = 0; i < numExtenstions; ++i) {
        canonical.append('-');
        canonical.append(extensions[i]);
    }

    // Check for privateuse.
    if (currentIndex < numParts) {
        String privateuse = getPrivateUseLangTag(parts, currentIndex);
        if (privateuse.isNull())
            return String();
        canonical.append('-');
        canonical.append(privateuse);
    }

    // FIXME: Replace subtags with their preferred values.

    return canonical.toString();
}

static String getGrandfatheredLangTag(const String& locale)
{
    // grandfathered = irregular / regular
    // FIXME: convert to a compile time hash table if this is causing performance issues.
    HashMap<String, String> tagMap = {
        // Irregular.
        { ASCIILiteral("en-gb-oed"), ASCIILiteral("en-GB-oed") },
        { ASCIILiteral("i-ami"), ASCIILiteral("ami") },
        { ASCIILiteral("i-bnn"), ASCIILiteral("bnn") },
        { ASCIILiteral("i-default"), ASCIILiteral("i-default") },
        { ASCIILiteral("i-enochian"), ASCIILiteral("i-enochian") },
        { ASCIILiteral("i-hak"), ASCIILiteral("hak") },
        { ASCIILiteral("i-klingon"), ASCIILiteral("tlh") },
        { ASCIILiteral("i-lux"), ASCIILiteral("lb") },
        { ASCIILiteral("i-mingo"), ASCIILiteral("i-mingo") },
        { ASCIILiteral("i-navajo"), ASCIILiteral("nv") },
        { ASCIILiteral("i-pwn"), ASCIILiteral("pwn") },
        { ASCIILiteral("i-tao"), ASCIILiteral("tao") },
        { ASCIILiteral("i-tay"), ASCIILiteral("tay") },
        { ASCIILiteral("i-tsu"), ASCIILiteral("tsu") },
        { ASCIILiteral("sgn-be-fr"), ASCIILiteral("sfb") },
        { ASCIILiteral("sgn-be-nl"), ASCIILiteral("vgt") },
        { ASCIILiteral("sgn-ch-de"), ASCIILiteral("sgg") },
        // Regular.
        { ASCIILiteral("art-lojban"), ASCIILiteral("jbo") },
        { ASCIILiteral("cel-gaulish"), ASCIILiteral("cel-gaulish") },
        { ASCIILiteral("no-bok"), ASCIILiteral("nb") },
        { ASCIILiteral("no-nyn"), ASCIILiteral("nn") },
        { ASCIILiteral("zh-guoyu"), ASCIILiteral("cmn") },
        { ASCIILiteral("zh-hakka"), ASCIILiteral("hak") },
        { ASCIILiteral("zh-min"), ASCIILiteral("zh-min") },
        { ASCIILiteral("zh-min-nan"), ASCIILiteral("nan") },
        { ASCIILiteral("zh-xiang"), ASCIILiteral("hsn") }
    };

    return tagMap.get(locale.convertToASCIILowercase());
}

static String canonicalizeLanguageTag(const String& locale)
{
    // 6.2.2 IsStructurallyValidLanguageTag (locale)
    // 6.2.3 CanonicalizeLanguageTag (locale)
    // These are done one after another in CanonicalizeLocaleList, so they are combined here to reduce duplication.
    // https://www.rfc-editor.org/rfc/bcp/bcp47.txt

    // Language-Tag = langtag / privateuse / grandfathered
    String grandfather = getGrandfatheredLangTag(locale);
    if (!grandfather.isNull())
        return grandfather;

    // FIXME: Replace redundant tags [RFC4647].

    Vector<String> parts;
    locale.split('-', true, parts);
    if (!parts.isEmpty()) {
        String langtag = getCanonicalLangTag(parts);
        if (!langtag.isNull())
            return langtag;

        String privateuse = getPrivateUseLangTag(parts, 0);
        if (!privateuse.isNull())
            return privateuse;
    }

    return String();
}

JSArray* canonicalizeLocaleList(ExecState* exec, JSValue locales)
{
    // 9.2.1 CanonicalizeLocaleList (locales)
    VM& vm = exec->vm();
    JSGlobalObject* globalObject = exec->callee()->globalObject();
    JSArray* seen = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithContiguous), 0);
    if (!seen) {
        throwOutOfMemoryError(exec);
        return nullptr;
    }

    // 1. If locales is undefined, then a. Return a new empty List.
    if (locales.isUndefined())
        return seen;

    // 2. Let seen be an empty List.
    // Done before to also return in step 1, if needed.

    // 3. If Type(locales) is String, then
    JSObject* localesObject;
    if (locales.isString()) {
        //  a. Let aLocales be CreateArrayFromList(«locales»).
        JSArray* localesArray = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithContiguous), 1);
        localesArray->initializeIndex(vm, 0, locales);
        // 4. Let O be ToObject(aLocales).
        localesObject = localesArray;
    } else {
        // 4. Let O be ToObject(aLocales).
        localesObject = locales.toObject(exec);
    }

    // 5. ReturnIfAbrupt(O).
    if (exec->hadException())
        return nullptr;

    // 6. Let len be ToLength(Get(O, "length")).
    JSValue lengthProperty = localesObject->get(exec, vm.propertyNames->length);
    if (exec->hadException())
        return nullptr;

    double length = lengthProperty.toLength(exec);
    if (exec->hadException())
        return nullptr;

    // Keep track of locales that have been added to the list.
    HashSet<String> seenSet;

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    for (double k = 0; k < length; ++k) {
        // a. Let Pk be ToString(k).
        // Not needed because hasProperty and get take an int for numeric key.

        // b. Let kPresent be HasProperty(O, Pk).
        bool kPresent = localesObject->hasProperty(exec, k);

        // c. ReturnIfAbrupt(kPresent).
        if (exec->hadException())
            return nullptr;

        // d. If kPresent is true, then
        if (kPresent) {
            // i. Let kValue be Get(O, Pk).
            JSValue kValue = localesObject->get(exec, k);

            // ii. ReturnIfAbrupt(kValue).
            if (exec->hadException())
                return nullptr;

            // iii. If Type(kValue) is not String or Object, throw a TypeError exception.
            if (!kValue.isString() && !kValue.isObject()) {
                throwTypeError(exec, ASCIILiteral("locale value must be a string or object"));
                return nullptr;
            }

            // iv. Let tag be ToString(kValue).
            JSString* tag = kValue.toString(exec);

            // v. ReturnIfAbrupt(tag).
            if (exec->hadException())
                return nullptr;

            // vi. If IsStructurallyValidLanguageTag(tag) is false, throw a RangeError exception.
            // vii. Let canonicalizedTag be CanonicalizeLanguageTag(tag).
            String canonicalizedTag = canonicalizeLanguageTag(tag->value(exec));
            if (canonicalizedTag.isNull()) {
                exec->vm().throwException(exec, createRangeError(exec, String::format("invalid language tag: %s", tag->value(exec).utf8().data())));
                return nullptr;
            }

            // viii. If canonicalizedTag is not an element of seen, append canonicalizedTag as the last element of seen.
            if (seenSet.add(canonicalizedTag).isNewEntry)
                seen->push(exec, jsString(exec, canonicalizedTag));
        }
        // e. Increase k by 1.
    }

    return seen;
}

static String bestAvailableLocale(const HashSet<String>& availableLocales, const String& locale)
{
    // 9.2.2 BestAvailableLocale (availableLocales, locale)
    // 1. Let candidate be locale.
    String candidate = locale;

    // 2. Repeat
    while (!candidate.isEmpty()) {
        // a. If availableLocales contains an element equal to candidate, then return candidate.
        if (availableLocales.contains(candidate))
            return candidate;

        // b. Let pos be the character index of the last occurrence of "-" (U+002D) within candidate. If that character does not occur, return undefined.
        size_t pos = candidate.reverseFind('-');
        if (pos == notFound)
            return String();

        // c. If pos ≥ 2 and the character "-" occurs at index pos-2 of candidate, then decrease pos by 2.
        if (pos >= 2 && candidate[pos - 2] == '-')
            pos -= 2;

        // d. Let candidate be the substring of candidate from position 0, inclusive, to position pos, exclusive.
        candidate = candidate.substring(0, pos);
    }

    return String();
}

static JSArray* lookupSupportedLocales(ExecState* exec, const HashSet<String>& availableLocales, JSArray* requestedLocales)
{
    // 9.2.6 LookupSupportedLocales (availableLocales, requestedLocales)

    // 1. Let rLocales be CreateArrayFromList(requestedLocales).
    // Already an array.

    // 2. Let len be ToLength(Get(rLocales, "length")).
    unsigned len = requestedLocales->length();

    // 3. Let subset be an empty List.
    VM& vm = exec->vm();
    JSGlobalObject* globalObject = exec->callee()->globalObject();
    JSArray* subset = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithUndecided), 0);
    if (!subset) {
        throwOutOfMemoryError(exec);
        return nullptr;
    }

    // 4. Let k be 0.
    // 5. Repeat while k < len
    for (unsigned k = 0; k < len; ++k) {
        // a. Let Pk be ToString(k).
        // b. Let locale be Get(rLocales, Pk).
        JSValue locale = requestedLocales->get(exec, k);

        // c. ReturnIfAbrupt(locale).
        if (exec->hadException())
            return nullptr;

        // d. Let noExtensionsLocale be the String value that is locale with all Unicode locale extension sequences removed.
        JSString* jsLocale = locale.toString(exec);
        if (exec->hadException())
            return nullptr;

        String sLocale = jsLocale->value(exec);
        Vector<String> parts;
        sLocale.split('-', parts);
        StringBuilder builder;
        size_t partsSize = parts.size();
        if (partsSize > 0)
            builder.append(parts[0]);
        for (size_t p = 1; p < partsSize; ++p) {
            if (parts[p] == "u") {
                // Skip the u- and anything that follows until another singleton.
                // While the next part is part of the unicode extension, skip it.
                while (p + 1 < partsSize && parts[p + 1].length() > 1)
                    ++p;
            } else {
                builder.append('-');
                builder.append(parts[p]);
            }
        }
        String noExtensionsLocale = builder.toString();

        // e. Let availableLocale be BestAvailableLocale(availableLocales, noExtensionsLocale).
        String availableLocale = bestAvailableLocale(availableLocales, noExtensionsLocale);

        // f. If availableLocale is not undefined, then append locale to the end of subset.
        if (!availableLocale.isNull())
            subset->push(exec, locale);

        // g. Increment k by 1.
    }

    // 6. Return subset.
    return subset;
}

static JSArray* bestFitSupportedLocales(ExecState* exec, const HashSet<String>& availableLocales, JSArray* requestedLocales)
{
    // 9.2.7 BestFitSupportedLocales (availableLocales, requestedLocales)
    // FIXME: Implement something better than lookup.
    return lookupSupportedLocales(exec, availableLocales, requestedLocales);
}

JSValue supportedLocales(ExecState* exec, const HashSet<String>& availableLocales, JSArray* requestedLocales, JSValue options)
{
    // 9.2.8 SupportedLocales (availableLocales, requestedLocales, options)
    VM& vm = exec->vm();
    String matcher;

    // 1. If options is not undefined, then
    if (!options.isUndefined()) {
        // a. Let matcher be GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
        const HashSet<String> matchers({ ASCIILiteral("lookup"), ASCIILiteral("best fit") });
        matcher = getIntlStringOption(exec, options, vm.propertyNames->localeMatcher, matchers, "localeMatcher must be either \"lookup\" or \"best fit\"", ASCIILiteral("best fit"));
        // b. ReturnIfAbrupt(matcher).
        if (exec->hadException())
            return jsUndefined();
    } else {
        // 2. Else, let matcher be "best fit".
        matcher = ASCIILiteral("best fit");
    }

    JSArray* supportedLocales;
    // 3. If matcher is "best fit",
    if (matcher == "best fit") {
        // a. Let MatcherOperation be the abstract operation BestFitSupportedLocales.
        // 5. Let supportedLocales be MatcherOperation(availableLocales, requestedLocales).
        supportedLocales = bestFitSupportedLocales(exec, availableLocales, requestedLocales);
    } else {
        // 4. Else
        // a. Let MatcherOperation be the abstract operation LookupSupportedLocales.
        // 5. Let supportedLocales be MatcherOperation(availableLocales, requestedLocales).
        supportedLocales = lookupSupportedLocales(exec, availableLocales, requestedLocales);
    }

    if (exec->hadException())
        return jsUndefined();

    // 6. Let subset be CreateArrayFromList(supportedLocales).
    // Already an array.

    // 7. Let keys be subset.[[OwnPropertyKeys]]().
    PropertyNameArray keys(exec, PropertyNameMode::Strings);
    supportedLocales->getOwnPropertyNames(supportedLocales, exec, keys, EnumerationMode());

    PropertyDescriptor desc;
    desc.setConfigurable(false);
    desc.setWritable(false);

    // 8. Repeat for each element P of keys in List order,
    size_t len = keys.size();
    for (size_t i = 0; i < len; ++i) {
        // a. Let desc be PropertyDescriptor { [[Configurable]]: false, [[Writable]]: false }.
        // Created above for reuse.

        // b. Let status be DefinePropertyOrThrow(subset, P, desc).
        supportedLocales->defineOwnProperty(supportedLocales, exec, keys[i], desc, true);

        // c. Assert: status is not abrupt completion.
        if (exec->hadException())
            return jsUndefined();
    }

    // 9. Return subset.
    return supportedLocales;
}

} // namespace JSC

#endif // ENABLE(INTL)
