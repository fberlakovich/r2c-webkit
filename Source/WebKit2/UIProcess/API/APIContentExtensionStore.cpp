/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "APIContentExtensionStore.h"

#if ENABLE(CONTENT_EXTENSIONS)

#include "APIContentExtension.h"
#include "NetworkCacheData.h"
#include "NetworkCacheFileSystem.h"
#include "SharedMemory.h"
#include "WebCompiledContentExtension.h"
#include <WebCore/ContentExtensionCompiler.h>
#include <WebCore/ContentExtensionError.h>
#include <string>
#include <wtf/NeverDestroyed.h>
#include <wtf/RunLoop.h>
#include <wtf/WorkQueue.h>
#include <wtf/persistence/Decoder.h>
#include <wtf/persistence/Encoder.h>

using namespace WebKit::NetworkCache;

namespace API {

ContentExtensionStore& ContentExtensionStore::defaultStore()
{
    static ContentExtensionStore* defaultStore = adoptRef(new ContentExtensionStore).leakRef();
    return *defaultStore;
}

Ref<ContentExtensionStore> ContentExtensionStore::storeWithPath(const WTF::String& storePath)
{
    return adoptRef(*new ContentExtensionStore(storePath));
}

ContentExtensionStore::ContentExtensionStore()
    : ContentExtensionStore(defaultStorePath())
{
}

ContentExtensionStore::ContentExtensionStore(const WTF::String& storePath)
    : m_storePath(storePath)
    , m_compileQueue(WorkQueue::create("ContentExtensionStore Compile Queue", WorkQueue::Type::Concurrent))
    , m_readQueue(WorkQueue::create("ContentExtensionStore Read Queue"))
    , m_removeQueue(WorkQueue::create("ContentExtensionStore Remove Queue"))
{
    WebCore::makeAllDirectories(storePath);
}

ContentExtensionStore::~ContentExtensionStore()
{
}

static const String& constructedPathPrefix()
{
    static NeverDestroyed<String> prefix("ContentExtension-");
    return prefix;
}

static const String constructedPathFilter()
{
    return makeString(constructedPathPrefix(), '*');
}

static String constructedPath(const String& base, const String& identifier)
{
    return WebCore::pathByAppendingComponent(base, makeString(constructedPathPrefix(), WebCore::encodeForFileName(identifier)));
}

// The size and offset of the densely packed bytes in the file, not sizeof and offsetof, which would
// represent the size and offset of the structure in memory, possibly with compiler-added padding.
const size_t ContentExtensionFileHeaderSize = 2 * sizeof(uint32_t) + 5 * sizeof(uint64_t);
const size_t ConditionsApplyOnlyToDomainOffset = sizeof(uint32_t) + 5 * sizeof(uint64_t);

struct ContentExtensionMetaData {
    uint32_t version { ContentExtensionStore::CurrentContentExtensionFileVersion };
    uint64_t sourceSize { 0 };
    uint64_t actionsSize { 0 };
    uint64_t filtersWithoutConditionsBytecodeSize { 0 };
    uint64_t filtersWithConditionsBytecodeSize { 0 };
    uint64_t conditionedFiltersBytecodeSize { 0 };
    uint32_t conditionsApplyOnlyToDomain { false };
    
    size_t fileSize() const
    {
        return ContentExtensionFileHeaderSize
            + sourceSize
            + actionsSize
            + filtersWithoutConditionsBytecodeSize
            + filtersWithConditionsBytecodeSize
            + conditionedFiltersBytecodeSize;
    }
};

static Data encodeContentExtensionMetaData(const ContentExtensionMetaData& metaData)
{
    WTF::Persistence::Encoder encoder;

    encoder << metaData.version;
    encoder << metaData.sourceSize;
    encoder << metaData.actionsSize;
    encoder << metaData.filtersWithoutConditionsBytecodeSize;
    encoder << metaData.filtersWithConditionsBytecodeSize;
    encoder << metaData.conditionedFiltersBytecodeSize;
    encoder << metaData.conditionsApplyOnlyToDomain;

    ASSERT(encoder.bufferSize() == ContentExtensionFileHeaderSize);
    return Data(encoder.buffer(), encoder.bufferSize());
}

static bool decodeContentExtensionMetaData(ContentExtensionMetaData& metaData, const Data& fileData)
{
    bool success = false;
    fileData.apply([&metaData, &success, &fileData](const uint8_t* data, size_t size) {
        // The file data should be mapped into one continuous memory segment so the size
        // passed to the applier should always equal the data size.
        if (size != fileData.size())
            return false;

        WTF::Persistence::Decoder decoder(data, size);
        if (!decoder.decode(metaData.version))
            return false;
        if (!decoder.decode(metaData.sourceSize))
            return false;
        if (!decoder.decode(metaData.actionsSize))
            return false;
        if (!decoder.decode(metaData.filtersWithoutConditionsBytecodeSize))
            return false;
        if (!decoder.decode(metaData.filtersWithConditionsBytecodeSize))
            return false;
        if (!decoder.decode(metaData.conditionedFiltersBytecodeSize))
            return false;
        if (!decoder.decode(metaData.conditionsApplyOnlyToDomain))
            return false;
        success = true;
        return false;
    });
    return success;
}

static bool openAndMapContentExtension(const String& path, ContentExtensionMetaData& metaData, Data& fileData)
{
    fileData = mapFile(WebCore::fileSystemRepresentation(path).data());
    if (fileData.isNull())
        return false;

    if (!decodeContentExtensionMetaData(metaData, fileData))
        return false;

    return true;
}

static bool writeDataToFile(const Data& fileData, WebCore::PlatformFileHandle fd)
{
    bool success = true;
    fileData.apply([fd, &success](const uint8_t* data, size_t size) {
        if (WebCore::writeToFile(fd, (const char*)data, size) == -1) {
            success = false;
            return false;
        }
        return true;
    });
    
    return success;
}

static std::error_code compiledToFile(String&& json, const String& finalFilePath, ContentExtensionMetaData& metaData, Data& mappedData)
{
    using namespace WebCore::ContentExtensions;

    class CompilationClient final : public ContentExtensionCompilationClient {
    public:
        CompilationClient(WebCore::PlatformFileHandle fileHandle, ContentExtensionMetaData& metaData)
            : m_fileHandle(fileHandle)
            , m_metaData(metaData)
        {
            ASSERT(!metaData.sourceSize);
            ASSERT(!metaData.actionsSize);
            ASSERT(!metaData.filtersWithoutConditionsBytecodeSize);
            ASSERT(!metaData.filtersWithConditionsBytecodeSize);
            ASSERT(!metaData.conditionedFiltersBytecodeSize);
            ASSERT(!metaData.conditionsApplyOnlyToDomain);
        }
        
        void writeSource(const String& sourceJSON) final {
            ASSERT(!m_filtersWithoutConditionsBytecodeWritten);
            ASSERT(!m_filtersWithConditionBytecodeWritten);
            ASSERT(!m_conditionFiltersBytecodeWritten);
            ASSERT(!m_actionsWritten);
            ASSERT(!m_sourceWritten);
            writeToFile(sourceJSON.is8Bit());
            m_sourceWritten += sizeof(bool);
            if (sourceJSON.is8Bit()) {
                size_t serializedLength = sourceJSON.length() * sizeof(LChar);
                writeToFile(Data(sourceJSON.characters8(), serializedLength));
                m_sourceWritten += serializedLength;
            } else {
                size_t serializedLength = sourceJSON.length() * sizeof(UChar);
                writeToFile(Data(reinterpret_cast<const uint8_t*>(sourceJSON.characters16()), serializedLength));
                m_sourceWritten += serializedLength;
            }
        }
        
        void writeFiltersWithoutConditionsBytecode(Vector<DFABytecode>&& bytecode) final
        {
            ASSERT(!m_filtersWithConditionBytecodeWritten);
            ASSERT(!m_conditionFiltersBytecodeWritten);
            m_filtersWithoutConditionsBytecodeWritten += bytecode.size();
            writeToFile(Data(bytecode.data(), bytecode.size()));
        }
        
        void writeFiltersWithConditionsBytecode(Vector<DFABytecode>&& bytecode) final
        {
            ASSERT(!m_conditionFiltersBytecodeWritten);
            m_filtersWithConditionBytecodeWritten += bytecode.size();
            writeToFile(Data(bytecode.data(), bytecode.size()));
        }
        
        void writeTopURLFiltersBytecode(Vector<DFABytecode>&& bytecode) final
        {
            m_conditionFiltersBytecodeWritten += bytecode.size();
            writeToFile(Data(bytecode.data(), bytecode.size()));
        }

        void writeActions(Vector<SerializedActionByte>&& actions, bool conditionsApplyOnlyToDomain) final
        {
            ASSERT(!m_filtersWithoutConditionsBytecodeWritten);
            ASSERT(!m_filtersWithConditionBytecodeWritten);
            ASSERT(!m_conditionFiltersBytecodeWritten);
            ASSERT(!m_actionsWritten);
            m_actionsWritten += actions.size();
            m_conditionsApplyOnlyToDomain = conditionsApplyOnlyToDomain;
            writeToFile(Data(actions.data(), actions.size()));
        }
        
        void finalize() final
        {
            m_metaData.sourceSize = m_sourceWritten;
            m_metaData.actionsSize = m_actionsWritten;
            m_metaData.filtersWithoutConditionsBytecodeSize = m_filtersWithoutConditionsBytecodeWritten;
            m_metaData.filtersWithConditionsBytecodeSize = m_filtersWithConditionBytecodeWritten;
            m_metaData.conditionedFiltersBytecodeSize = m_conditionFiltersBytecodeWritten;
            m_metaData.conditionsApplyOnlyToDomain = m_conditionsApplyOnlyToDomain;
            
            Data header = encodeContentExtensionMetaData(m_metaData);
            if (!m_fileError && WebCore::seekFile(m_fileHandle, 0ll, WebCore::FileSeekOrigin::SeekFromBeginning) == -1) {
                WebCore::closeFile(m_fileHandle);
                m_fileError = true;
            }
            writeToFile(header);
        }
        
        bool hadErrorWhileWritingToFile() { return m_fileError; }

    private:
        void writeToFile(bool value)
        {
            writeToFile(Data(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
        }
        void writeToFile(const Data& data)
        {
            if (!m_fileError && !writeDataToFile(data, m_fileHandle)) {
                WebCore::closeFile(m_fileHandle);
                m_fileError = true;
            }
        }
        
        WebCore::PlatformFileHandle m_fileHandle;
        ContentExtensionMetaData& m_metaData;
        size_t m_filtersWithoutConditionsBytecodeWritten { 0 };
        size_t m_filtersWithConditionBytecodeWritten { 0 };
        size_t m_conditionFiltersBytecodeWritten { 0 };
        size_t m_actionsWritten { 0 };
        size_t m_sourceWritten { 0 };
        bool m_conditionsApplyOnlyToDomain { false };
        bool m_fileError { false };
    };

    auto temporaryFileHandle = WebCore::invalidPlatformFileHandle;
    String temporaryFilePath = WebCore::openTemporaryFile("ContentExtension", temporaryFileHandle);
    if (temporaryFileHandle == WebCore::invalidPlatformFileHandle)
        return ContentExtensionStore::Error::CompileFailed;
    
    char invalidHeader[ContentExtensionFileHeaderSize];
    memset(invalidHeader, 0xFF, sizeof(invalidHeader));
    // This header will be rewritten in CompilationClient::finalize.
    if (WebCore::writeToFile(temporaryFileHandle, invalidHeader, sizeof(invalidHeader)) == -1) {
        WebCore::closeFile(temporaryFileHandle);
        return ContentExtensionStore::Error::CompileFailed;
    }

    CompilationClient compilationClient(temporaryFileHandle, metaData);
    
    if (auto compilerError = compileRuleList(compilationClient, WTFMove(json))) {
        WebCore::closeFile(temporaryFileHandle);
        return compilerError;
    }
    if (compilationClient.hadErrorWhileWritingToFile()) {
        WebCore::closeFile(temporaryFileHandle);
        return ContentExtensionStore::Error::CompileFailed;
    }
    
    mappedData = adoptAndMapFile(temporaryFileHandle, 0, metaData.fileSize());
    if (mappedData.isNull())
        return ContentExtensionStore::Error::CompileFailed;

    if (!WebCore::moveFile(temporaryFilePath, finalFilePath))
        return ContentExtensionStore::Error::CompileFailed;

    return { };
}

static RefPtr<API::ContentExtension> createExtension(const String& identifier, const ContentExtensionMetaData& metaData, const Data& fileData)
{
    auto sharedMemory = WebKit::SharedMemory::create(const_cast<uint8_t*>(fileData.data()), fileData.size(), WebKit::SharedMemory::Protection::ReadOnly);
    const size_t headerAndSourceSize = ContentExtensionFileHeaderSize + metaData.sourceSize;
    auto compiledContentExtensionData = WebKit::WebCompiledContentExtensionData(
        WTFMove(sharedMemory),
        fileData,
        ConditionsApplyOnlyToDomainOffset,
        headerAndSourceSize,
        metaData.actionsSize,
        headerAndSourceSize
            + metaData.actionsSize,
        metaData.filtersWithoutConditionsBytecodeSize,
        headerAndSourceSize
            + metaData.actionsSize
            + metaData.filtersWithoutConditionsBytecodeSize,
        metaData.filtersWithConditionsBytecodeSize,
        headerAndSourceSize
            + metaData.actionsSize
            + metaData.filtersWithoutConditionsBytecodeSize
            + metaData.filtersWithConditionsBytecodeSize,
        metaData.conditionedFiltersBytecodeSize
    );
    auto compiledContentExtension = WebKit::WebCompiledContentExtension::create(WTFMove(compiledContentExtensionData));
    return API::ContentExtension::create(identifier, WTFMove(compiledContentExtension));
}

void ContentExtensionStore::lookupContentExtension(const WTF::String& identifier, Function<void(RefPtr<API::ContentExtension>, std::error_code)> completionHandler)
{
    m_readQueue->dispatch([protectedThis = makeRef(*this), identifier = identifier.isolatedCopy(), storePath = m_storePath.isolatedCopy(), completionHandler = WTFMove(completionHandler)]() mutable {
        auto path = constructedPath(storePath, identifier);
        
        ContentExtensionMetaData metaData;
        Data fileData;
        if (!openAndMapContentExtension(path, metaData, fileData)) {
            RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler)] {
                completionHandler(nullptr, Error::LookupFailed);
            });
            return;
        }
        
        if (metaData.version != ContentExtensionStore::CurrentContentExtensionFileVersion) {
            RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler)] {
                completionHandler(nullptr, Error::VersionMismatch);
            });
            return;
        }
        
        RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), identifier = WTFMove(identifier), fileData = WTFMove(fileData), metaData = WTFMove(metaData), completionHandler = WTFMove(completionHandler)] {
            RefPtr<API::ContentExtension> contentExtension = createExtension(identifier, metaData, fileData);
            completionHandler(contentExtension, { });
        });
    });
}

void ContentExtensionStore::getAvailableContentExtensionIdentifiers(Function<void(WTF::Vector<WTF::String>)> completionHandler)
{
    m_readQueue->dispatch([protectedThis = makeRef(*this), storePath = m_storePath.isolatedCopy(), completionHandler = WTFMove(completionHandler)]() mutable {

        Vector<String> fullPaths = WebCore::listDirectory(storePath, constructedPathFilter());
        Vector<String> identifiers;
        identifiers.reserveInitialCapacity(fullPaths.size());
        const auto prefixLength = constructedPathPrefix().length();
        for (const auto& path : fullPaths)
            identifiers.uncheckedAppend(path.substring(path.reverseFind('/') + 1 + prefixLength));

        RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler), identifiers = WTFMove(identifiers)]() mutable {
            completionHandler(WTFMove(identifiers));
        });
    });
}

void ContentExtensionStore::compileContentExtension(const WTF::String& identifier, WTF::String&& json, Function<void(RefPtr<API::ContentExtension>, std::error_code)> completionHandler)
{
    AtomicString::init();
    m_compileQueue->dispatch([protectedThis = makeRef(*this), identifier = identifier.isolatedCopy(), json = json.isolatedCopy(), storePath = m_storePath.isolatedCopy(), completionHandler = WTFMove(completionHandler)] () mutable {
        auto path = constructedPath(storePath, identifier);

        ContentExtensionMetaData metaData;
        Data fileData;
        auto error = compiledToFile(WTFMove(json), path, metaData, fileData);
        if (error) {
            RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), error = WTFMove(error), completionHandler = WTFMove(completionHandler)] {
                completionHandler(nullptr, error);
            });
            return;
        }

        RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), identifier = WTFMove(identifier), fileData = WTFMove(fileData), metaData = WTFMove(metaData), completionHandler = WTFMove(completionHandler)] {
            RefPtr<API::ContentExtension> contentExtension = createExtension(identifier, metaData, fileData);
            completionHandler(contentExtension, { });
        });
    });
}

void ContentExtensionStore::removeContentExtension(const WTF::String& identifier, Function<void(std::error_code)> completionHandler)
{
    m_removeQueue->dispatch([protectedThis = makeRef(*this), identifier = identifier.isolatedCopy(), storePath = m_storePath.isolatedCopy(), completionHandler = WTFMove(completionHandler)]() mutable {
        auto path = constructedPath(storePath, identifier);

        if (!WebCore::deleteFile(path)) {
            RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler)] {
                completionHandler(Error::RemoveFailed);
            });
            return;
        }

        RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler)] {
            completionHandler({ });
        });
    });
}

void ContentExtensionStore::synchronousRemoveAllContentExtensions()
{
    for (const auto& path : WebCore::listDirectory(m_storePath, "*"))
        WebCore::deleteFile(path);
}

void ContentExtensionStore::invalidateContentExtensionVersion(const WTF::String& identifier)
{
    auto file = WebCore::openFile(constructedPath(m_storePath, identifier), WebCore::OpenForWrite);
    if (file == WebCore::invalidPlatformFileHandle)
        return;
    ContentExtensionMetaData invalidHeader = {0, 0, 0, 0, 0, 0};
    auto bytesWritten = WebCore::writeToFile(file, reinterpret_cast<const char*>(&invalidHeader), sizeof(invalidHeader));
    ASSERT_UNUSED(bytesWritten, bytesWritten == sizeof(invalidHeader));
    WebCore::closeFile(file);
}

void ContentExtensionStore::getContentExtensionSource(const WTF::String& identifier, Function<void(WTF::String)> completionHandler)
{
    m_readQueue->dispatch([protectedThis = makeRef(*this), identifier = identifier.isolatedCopy(), storePath = m_storePath.isolatedCopy(), completionHandler = WTFMove(completionHandler)]() mutable {
        auto path = constructedPath(storePath, identifier);
        
        auto complete = [protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler)](String source) mutable {
            RunLoop::main().dispatch([protectedThis = WTFMove(protectedThis), completionHandler = WTFMove(completionHandler), source = source.isolatedCopy()] {
                completionHandler(source);
            });
        };
        
        ContentExtensionMetaData metaData;
        Data fileData;
        if (!openAndMapContentExtension(path, metaData, fileData)) {
            complete({ });
            return;
        }
        
        switch (metaData.version) {
        case 9:
            if (!metaData.sourceSize) {
                complete({ });
                return;
            }
            bool is8Bit = fileData.data()[ContentExtensionFileHeaderSize];
            size_t start = ContentExtensionFileHeaderSize + sizeof(bool);
            size_t length = metaData.sourceSize - sizeof(bool);
            if (is8Bit)
                complete(String(fileData.data() + start, length));
            else {
                ASSERT(!(length % sizeof(UChar)));
                complete(String(reinterpret_cast<const UChar*>(fileData.data() + start), length / sizeof(UChar)));
            }
            return;
        }

        // Older versions cannot recover the original JSON source from disk.
        complete({ });
    });
}

const std::error_category& contentExtensionStoreErrorCategory()
{
    class ContentExtensionStoreErrorCategory : public std::error_category {
        const char* name() const noexcept final
        {
            return "content extension store";
        }

        std::string message(int errorCode) const final
        {
            switch (static_cast<ContentExtensionStore::Error>(errorCode)) {
            case ContentExtensionStore::Error::LookupFailed:
                return "Unspecified error during lookup.";
            case ContentExtensionStore::Error::VersionMismatch:
                return "Version of file does not match version of interpreter.";
            case ContentExtensionStore::Error::CompileFailed:
                return "Unspecified error during compile.";
            case ContentExtensionStore::Error::RemoveFailed:
                return "Unspecified error during remove.";
            }

            return std::string();
        }
    };

    static NeverDestroyed<ContentExtensionStoreErrorCategory> contentExtensionErrorCategory;
    return contentExtensionErrorCategory;
}

} // namespace API

#endif // ENABLE(CONTENT_EXTENSIONS)
