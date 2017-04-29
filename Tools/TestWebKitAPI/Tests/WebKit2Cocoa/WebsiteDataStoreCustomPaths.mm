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

#import "PlatformUtilities.h"
#import "Test.h"
#import <WebKit/WebKit.h>

#import <WebKit/WKProcessPoolPrivate.h>
#import <WebKit/WKUserContentControllerPrivate.h>
#import <WebKit/WKWebViewConfigurationPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/WKWebsiteDataStorePrivate.h>
#import <WebKit/_WKProcessPoolConfiguration.h>
#import <WebKit/_WKUserStyleSheet.h>
#import <WebKit/_WKWebsiteDataStoreConfiguration.h>
#import <wtf/Deque.h>
#import <wtf/RetainPtr.h>

#if WK_API_ENABLED

static bool receivedScriptMessage;
static Deque<RetainPtr<WKScriptMessage>> scriptMessages;

@interface WebsiteDataStoreCustomPathsMessageHandler : NSObject <WKScriptMessageHandler>
@end

@implementation WebsiteDataStoreCustomPathsMessageHandler

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    receivedScriptMessage = true;
    scriptMessages.append(message);
}

@end

static WKScriptMessage *getNextMessage()
{
    if (scriptMessages.isEmpty()) {
        receivedScriptMessage = false;
        TestWebKitAPI::Util::run(&receivedScriptMessage);
    }

    return [[scriptMessages.takeFirst() retain] autorelease];
}

TEST(WebKit2, WebsiteDataStoreCustomPaths)
{
    RetainPtr<WebsiteDataStoreCustomPathsMessageHandler> handler = adoptNS([[WebsiteDataStoreCustomPathsMessageHandler alloc] init]);
    RetainPtr<WKWebViewConfiguration> configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [[configuration userContentController] addScriptMessageHandler:handler.get() name:@"testHandler"];

    NSURL *sqlPath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/CustomWebsiteData/WebSQL/" stringByExpandingTildeInPath]];
    NSURL *idbPath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/CustomWebsiteData/IndexedDB/" stringByExpandingTildeInPath]];
    NSURL *localStoragePath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/CustomWebsiteData/LocalStorage/" stringByExpandingTildeInPath]];
    NSURL *cookieStoragePath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/CustomWebsiteData/CookieStorage/" stringByExpandingTildeInPath]];

    NSURL *defaultSQLPath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/WebsiteData/WebSQL/" stringByExpandingTildeInPath]];
    NSURL *defaultIDBPath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/WebsiteData/IndexedDB/" stringByExpandingTildeInPath]];
    NSURL *defaultLocalStoragePath = [NSURL fileURLWithPath:[@"~/Library/WebKit/TestWebKitAPI/WebsiteData/LocalStorage/" stringByExpandingTildeInPath]];

    [[NSFileManager defaultManager] removeItemAtURL:sqlPath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:idbPath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:localStoragePath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:cookieStoragePath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:defaultSQLPath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:defaultIDBPath error:nil];
    [[NSFileManager defaultManager] removeItemAtURL:defaultLocalStoragePath error:nil];

    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:sqlPath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:idbPath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:localStoragePath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:cookieStoragePath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:defaultSQLPath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:defaultIDBPath.path]);
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:defaultLocalStoragePath.path]);

    _WKWebsiteDataStoreConfiguration *websiteDataStoreConfiguration = [[_WKWebsiteDataStoreConfiguration alloc] init];
    websiteDataStoreConfiguration._webSQLDatabaseDirectory = sqlPath;
    websiteDataStoreConfiguration._indexedDBDatabaseDirectory = idbPath;
    websiteDataStoreConfiguration._webStorageDirectory = localStoragePath;
    websiteDataStoreConfiguration._cookieStorageDirectory = cookieStoragePath;

    configuration.get().websiteDataStore = [[WKWebsiteDataStore alloc] _initWithConfiguration:websiteDataStoreConfiguration];
    [websiteDataStoreConfiguration release];

    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"WebsiteDataStoreCustomPaths" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:request];

    // We expect 4 messages, 1 each for WebSQL, IndexedDB, cookies, and localStorage.
    EXPECT_STREQ([getNextMessage().body UTF8String], "localstorage written");
    EXPECT_STREQ([getNextMessage().body UTF8String], "cookie written");
    EXPECT_STREQ([getNextMessage().body UTF8String], "Exception: QuotaExceededError (DOM Exception 22): The quota has been exceeded.");
    EXPECT_STREQ([getNextMessage().body UTF8String], "Success opening indexed database");

    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:sqlPath.path]);
    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:localStoragePath.path]);

    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:idbPath.path]);
    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:defaultIDBPath.path]);

    // FIXME: This should be true, but comes up false. Possibly a CFNetwork issue. Being explored in <rdar://problem/31666275>
    EXPECT_FALSE([[NSFileManager defaultManager] fileExistsAtPath:cookieStoragePath.path]);

    // FIXME: The default SQL and LocalStorage paths are being used for something, but they shouldn't be. (theses should be false, not true)
    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:defaultSQLPath.path]);
    EXPECT_TRUE([[NSFileManager defaultManager] fileExistsAtPath:defaultLocalStoragePath.path]);
}

#endif
