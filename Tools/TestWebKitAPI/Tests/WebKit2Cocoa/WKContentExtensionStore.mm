/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#import "config.h"
#import <WebKit/WKFoundation.h>

#if WK_API_ENABLED

#import "PlatformUtilities.h"
#import "Test.h"
#import <WebKit/WKContentExtension.h>
#import <WebKit/WKContentExtensionStorePrivate.h>
#import <wtf/RetainPtr.h>

class WKContentExtensionStoreTest : public testing::Test {
public:
    virtual void SetUp()
    {
        [[WKContentExtensionStore defaultStore] _removeAllContentExtensions];
    }
};

static NSString *basicFilter = @"[{\"action\":{\"type\":\"block\"},\"trigger\":{\"url-filter\":\".*webkit.org\"}}]";

TEST_F(WKContentExtensionStoreTest, Compile)
{
    __block bool doneCompiling = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
    
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);

        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);
}

static NSString *invalidFilter = @"[";

static void checkDomain(NSError *error)
{
    EXPECT_STREQ([[error domain] UTF8String], [WKErrorDomain UTF8String]);
}

TEST_F(WKContentExtensionStoreTest, InvalidExtension)
{
    __block bool doneCompiling = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestExtension" encodedContentExtension:invalidFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
    
        EXPECT_NULL(filter);
        EXPECT_NOT_NULL(error);
        checkDomain(error);
        EXPECT_EQ(error.code, WKErrorContentExtensionStoreCompileFailed);
        EXPECT_STREQ("Extension compilation failed: Failed to parse the JSON String.", [[error helpAnchor] UTF8String]);

        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);
}

TEST_F(WKContentExtensionStoreTest, Lookup)
{
    __block bool doneCompiling = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
    
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);

        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);

    __block bool doneLookingUp = false;
    [[WKContentExtensionStore defaultStore] lookUpContentExtensionForIdentifier:@"TestExtension" completionHandler:^(WKContentExtension *filter, NSError *error) {

        EXPECT_STREQ(filter.identifier.UTF8String, "TestExtension");
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);

        doneLookingUp = true;
    }];
    TestWebKitAPI::Util::run(&doneLookingUp);
}

TEST_F(WKContentExtensionStoreTest, EncodedIdentifier)
{
    // FIXME: U+00C4 causes problems here. Using the output of encodeForFileName with
    // the filesystem changes it to U+0041 followed by U+0308
    NSString *identifier = @":;%25%+25И😍";
    __block bool done = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:identifier encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {

        EXPECT_STREQ(filter.identifier.UTF8String, identifier.UTF8String);

        [[WKContentExtensionStore defaultStore] getAvailableContentExtensionIdentifiers:^(NSArray<NSString *> *identifiers) {
            EXPECT_EQ(identifiers.count, 1u);
            EXPECT_EQ(identifiers[0].length, identifier.length);
            EXPECT_STREQ(identifiers[0].UTF8String, identifier.UTF8String);

            done = true;
        }];
    }];
    TestWebKitAPI::Util::run(&done);
}

TEST_F(WKContentExtensionStoreTest, NonExistingIdentifierLookup)
{
    __block bool doneLookingUp = false;
    [[WKContentExtensionStore defaultStore] lookUpContentExtensionForIdentifier:@"DoesNotExist" completionHandler:^(WKContentExtension *filter, NSError *error) {
    
        EXPECT_NULL(filter);
        EXPECT_NOT_NULL(error);
        checkDomain(error);
        EXPECT_EQ(error.code, WKErrorContentExtensionStoreLookUpFailed);
        EXPECT_STREQ("Extension lookup failed: Unspecified error during lookup.", [[error helpAnchor] UTF8String]);
        
        doneLookingUp = true;
    }];
    TestWebKitAPI::Util::run(&doneLookingUp);
}

TEST_F(WKContentExtensionStoreTest, VersionMismatch)
{
    __block bool doneCompiling = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error)
    {
        
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);
        
        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);

    [[WKContentExtensionStore defaultStore] _invalidateContentExtensionVersionForIdentifier:@"TestExtension"];
    
    __block bool doneLookingUp = false;
    [[WKContentExtensionStore defaultStore] lookUpContentExtensionForIdentifier:@"TestExtension" completionHandler:^(WKContentExtension *filter, NSError *error)
    {
        
        EXPECT_NULL(filter);
        EXPECT_NOT_NULL(error);
        checkDomain(error);
        EXPECT_EQ(error.code, WKErrorContentExtensionStoreVersionMismatch);
        EXPECT_STREQ("Extension lookup failed: Version of file does not match version of interpreter.", [[error helpAnchor] UTF8String]);
        
        doneLookingUp = true;
    }];
    TestWebKitAPI::Util::run(&doneLookingUp);

    __block bool doneGettingSource = false;
    [[WKContentExtensionStore defaultStore] _getContentExtensionSourceForIdentifier:@"TestExtension" completionHandler:^(NSString* source) {
        EXPECT_NULL(source);
        doneGettingSource = true;
    }];
    TestWebKitAPI::Util::run(&doneGettingSource);
}

TEST_F(WKContentExtensionStoreTest, Removal)
{
    __block bool doneCompiling = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
    
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);

        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);

    __block bool doneRemoving = false;
    [[WKContentExtensionStore defaultStore] removeContentExtensionForIdentifier:@"TestExtension" completionHandler:^(NSError *error) {
        EXPECT_NULL(error);

        doneRemoving = true;
    }];
    TestWebKitAPI::Util::run(&doneRemoving);
}

TEST_F(WKContentExtensionStoreTest, NonExistingIdentifierRemove)
{
    __block bool doneRemoving = false;
    [[WKContentExtensionStore defaultStore] removeContentExtensionForIdentifier:@"DoesNotExist" completionHandler:^(NSError *error) {
        EXPECT_NOT_NULL(error);
        checkDomain(error);
        EXPECT_EQ(error.code, WKErrorContentExtensionStoreRemoveFailed);
        EXPECT_STREQ("Extension removal failed: Unspecified error during remove.", [[error helpAnchor] UTF8String]);

        doneRemoving = true;
    }];
    TestWebKitAPI::Util::run(&doneRemoving);
}

TEST_F(WKContentExtensionStoreTest, NonDefaultStore)
{
    NSURL *tempDir = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"ContentExtensionTest"] isDirectory:YES];
    WKContentExtensionStore *store = [WKContentExtensionStore storeWithURL:tempDir];
    NSString *identifier = @"TestExtension";
    NSString *fileName = @"ContentExtension-TestExtension";

    __block bool doneGettingAvailableIdentifiers = false;
    [store getAvailableContentExtensionIdentifiers:^(NSArray<NSString *> *identifiers) {
        EXPECT_NOT_NULL(identifiers);
        EXPECT_EQ(identifiers.count, 0u);
        doneGettingAvailableIdentifiers = true;
    }];
    TestWebKitAPI::Util::run(&doneGettingAvailableIdentifiers);
    
    __block bool doneCompiling = false;
    [store compileContentExtensionForIdentifier:identifier encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);
        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);

    doneGettingAvailableIdentifiers = false;
    [store getAvailableContentExtensionIdentifiers:^(NSArray<NSString *> *identifiers) {
        EXPECT_NOT_NULL(identifiers);
        EXPECT_EQ(identifiers.count, 1u);
        EXPECT_STREQ(identifiers[0].UTF8String, "TestExtension");
        doneGettingAvailableIdentifiers = true;
    }];
    TestWebKitAPI::Util::run(&doneGettingAvailableIdentifiers);

    NSData *data = [NSData dataWithContentsOfURL:[tempDir URLByAppendingPathComponent:fileName]];
    EXPECT_NOT_NULL(data);
    EXPECT_EQ(data.length, 228u);
    
    __block bool doneCheckingSource = false;
    [store _getContentExtensionSourceForIdentifier:identifier completionHandler:^(NSString *source) {
        EXPECT_NOT_NULL(source);
        EXPECT_STREQ(basicFilter.UTF8String, source.UTF8String);
        doneCheckingSource = true;
    }];
    TestWebKitAPI::Util::run(&doneCheckingSource);
    
    __block bool doneRemoving = false;
    [store removeContentExtensionForIdentifier:identifier completionHandler:^(NSError *error) {
        EXPECT_NULL(error);
        doneRemoving = true;
    }];
    TestWebKitAPI::Util::run(&doneRemoving);

    NSData *dataAfterRemoving = [NSData dataWithContentsOfURL:[tempDir URLByAppendingPathComponent:fileName]];
    EXPECT_NULL(dataAfterRemoving);
}

TEST_F(WKContentExtensionStoreTest, MultipleExtensions)
{
    __block bool done = false;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"FirstExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);
        [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"SecondExtension" encodedContentExtension:basicFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
            EXPECT_NOT_NULL(filter);
            EXPECT_NULL(error);
            [[WKContentExtensionStore defaultStore] getAvailableContentExtensionIdentifiers:^(NSArray<NSString *> *identifiers) {
                EXPECT_NOT_NULL(identifiers);
                EXPECT_EQ(identifiers.count, 2u);
                EXPECT_STREQ(identifiers[0].UTF8String, "FirstExtension");
                EXPECT_STREQ(identifiers[1].UTF8String, "SecondExtension");
                done = true;
            }];
        }];
    }];
    TestWebKitAPI::Util::run(&done);
}

TEST_F(WKContentExtensionStoreTest, NonASCIISource)
{
    static NSString *nonASCIIFilter = @"[{\"action\":{\"type\":\"block\"},\"trigger\":{\"url-filter\":\".*webkit.org\"}, \"unused\":\"ðŸ’©\"}]";
    NSURL *tempDir = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"ContentExtensionTest"] isDirectory:YES];
    WKContentExtensionStore *store = [WKContentExtensionStore storeWithURL:tempDir];
    NSString *identifier = @"TestExtension";
    NSString *fileName = @"ContentExtension-TestExtension";
    
    __block bool done = false;
    [store compileContentExtensionForIdentifier:identifier encodedContentExtension:nonASCIIFilter completionHandler:^(WKContentExtension *filter, NSError *error) {
        EXPECT_NOT_NULL(filter);
        EXPECT_NULL(error);

        [store _getContentExtensionSourceForIdentifier:identifier completionHandler:^(NSString *source) {
            EXPECT_NOT_NULL(source);
            EXPECT_STREQ(nonASCIIFilter.UTF8String, source.UTF8String);

            [store _removeAllContentExtensions];
            NSData *dataAfterRemoving = [NSData dataWithContentsOfURL:[tempDir URLByAppendingPathComponent:fileName]];
            EXPECT_NULL(dataAfterRemoving);

            done = true;
        }];
    }];
    TestWebKitAPI::Util::run(&done);
}

static size_t alertCount { 0 };
static bool receivedAlert { false };

@interface ContentExtensionDelegate : NSObject <WKUIDelegate>
@end

@implementation ContentExtensionDelegate

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler
{
    switch (alertCount++) {
    case 0:
        // Default behavior.
        EXPECT_STREQ("content blockers enabled", message.UTF8String);
        break;
    case 1:
        // After having removed the content extension.
        EXPECT_STREQ("content blockers disabled", message.UTF8String);
        break;
    default:
        EXPECT_TRUE(false);
    }
    receivedAlert = true;
    completionHandler();
}

@end

TEST_F(WKContentExtensionStoreTest, AddRemove)
{
    [[WKContentExtensionStore defaultStore] _removeAllContentExtensions];

    __block bool doneCompiling = false;
    NSString* contentBlocker = @"[{\"action\":{\"type\":\"css-display-none\",\"selector\":\".hidden\"},\"trigger\":{\"url-filter\":\".*\"}}]";
    __block RetainPtr<WKContentExtension> extension;
    [[WKContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestAddRemove" encodedContentExtension:contentBlocker completionHandler:^(WKContentExtension *compiledExtension, NSError *error) {
        EXPECT_TRUE(error == nil);
        extension = compiledExtension;
        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);
    EXPECT_NOT_NULL(extension);

    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [[configuration userContentController] addContentExtension:extension.get()];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[ContentExtensionDelegate alloc] init]);
    [webView setUIDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"contentBlockerCheck" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    alertCount = 0;
    receivedAlert = false;
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedAlert);

    [[configuration userContentController] removeContentExtension:extension.get()];

    receivedAlert = false;
    [webView reload];
    TestWebKitAPI::Util::run(&receivedAlert);
}


#endif
