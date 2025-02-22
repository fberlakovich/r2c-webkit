/*
 * Copyright (C) 2017-2020 Apple Inc. All rights reserved.
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

#import "Test.h"
#import "Utilities.h"
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/WKWebViewPrivateForTesting.h>
#import <WebKit/_WKInspector.h>
#import <WebKit/_WKInspectorDelegate.h>
#import <WebKit/_WKInspectorPrivateForTesting.h>
#import <wtf/RetainPtr.h>
#import <wtf/SetForScope.h>

#if PLATFORM(MAC)

@class InspectorDelegate;

static bool didAttachLocalInspectorCalled = false;
static bool willCloseLocalInspectorCalled = false;
static bool browserDomainEnabledForInspectorCalled = false;
static bool browserDomainDisabledForInspectorCalled = false;
static bool shouldCallInspectorCloseReentrantly = false;
static bool openURLExternallyCalled = false;
static RetainPtr<id <_WKInspectorDelegate>> sharedInspectorDelegate;
static RetainPtr<NSURL> urlToOpen;

@interface InspectorDelegate : NSObject <_WKInspectorDelegate>
@end

@implementation InspectorDelegate

- (void)inspectorDidEnableBrowserDomain:(_WKInspector *)inspector
{
    browserDomainEnabledForInspectorCalled = true;
}

- (void)inspectorDidDisableBrowserDomain:(_WKInspector *)inspector
{
    browserDomainDisabledForInspectorCalled = true;
}

- (void)inspector:(_WKInspector *)inspector openURLExternally:(NSURL *)url
{
    EXPECT_STREQ(url.absoluteString.UTF8String, urlToOpen.get().absoluteString.UTF8String);
    openURLExternallyCalled = true;
}

@end

@interface UIDelegate : NSObject <WKUIDelegate>
@end

@implementation UIDelegate

- (void)_webView:(WKWebView *)webView didAttachLocalInspector:(_WKInspector *)inspector
{
    didAttachLocalInspectorCalled = false;
    willCloseLocalInspectorCalled = false;
    browserDomainEnabledForInspectorCalled = false;
    browserDomainDisabledForInspectorCalled = false;

    EXPECT_EQ(webView._inspector, inspector);

    sharedInspectorDelegate = [InspectorDelegate new];
    [inspector setDelegate:sharedInspectorDelegate.get()];

    didAttachLocalInspectorCalled = true;
}

- (void)_webView:(WKWebView *)webView willCloseLocalInspector:(_WKInspector *)inspector
{
    EXPECT_EQ(webView._inspector, inspector);
    willCloseLocalInspectorCalled = true;

    if (shouldCallInspectorCloseReentrantly)
        [inspector close];
}

@end

TEST(WKInspectorDelegate, InspectorLifecycleCallbacks)
{
    auto webViewConfiguration = adoptNS([WKWebViewConfiguration new]);
    webViewConfiguration.get().preferences._developerExtrasEnabled = YES;
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto uiDelegate = adoptNS([UIDelegate new]);

    [webView setUIDelegate:uiDelegate.get()];
    [webView loadHTMLString:@"<head><title>Test page to be inspected</title></head><body><p>Filler content</p></body>" baseURL:[NSURL URLWithString:@"http://example.com/"]];

    EXPECT_FALSE(webView.get()._isBeingInspected);

    [[webView _inspector] show];

    TestWebKitAPI::Util::run(&didAttachLocalInspectorCalled);
    TestWebKitAPI::Util::run(&browserDomainEnabledForInspectorCalled);

    [[webView _inspector] close];
    TestWebKitAPI::Util::run(&browserDomainDisabledForInspectorCalled);
    TestWebKitAPI::Util::run(&willCloseLocalInspectorCalled);
}

TEST(WKInspectorDelegate, InspectorCloseCalledReentrantly)
{
    auto webViewConfiguration = adoptNS([WKWebViewConfiguration new]);
    webViewConfiguration.get().preferences._developerExtrasEnabled = YES;
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto uiDelegate = adoptNS([UIDelegate new]);

    [webView setUIDelegate:uiDelegate.get()];
    [webView loadHTMLString:@"<head><title>Test page to be inspected</title></head><body><p>Filler content</p></body>" baseURL:[NSURL URLWithString:@"http://example.com/"]];

    EXPECT_FALSE(webView.get()._isBeingInspected);

    [[webView _inspector] show];
    TestWebKitAPI::Util::run(&didAttachLocalInspectorCalled);

    {
        SetForScope<bool> closeReentrantlyFromDelegate(shouldCallInspectorCloseReentrantly, true);
        [[webView _inspector] close];
        TestWebKitAPI::Util::run(&willCloseLocalInspectorCalled);
    }
}

TEST(WKInspectorDelegate, ShowURLExternally)
{
    auto webViewConfiguration = adoptNS([WKWebViewConfiguration new]);
    webViewConfiguration.get().preferences._developerExtrasEnabled = YES;
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto uiDelegate = adoptNS([UIDelegate new]);

    [webView setUIDelegate:uiDelegate.get()];
    [webView loadHTMLString:@"<head><title>Test page to be inspected</title></head><body><p>Filler content</p></body>" baseURL:[NSURL URLWithString:@"http://example.com/"]];

    [[webView _inspector] show];
    TestWebKitAPI::Util::run(&didAttachLocalInspectorCalled);

    urlToOpen = [NSURL URLWithString:@"https://www.webkit.org/"];

    // Check the case where the load is intercepted by the navigation delegate.
    [[webView _inspector] _openURLExternallyForTesting:urlToOpen.get() useFrontendAPI:NO];
    TestWebKitAPI::Util::run(&openURLExternallyCalled);

    // Check the case where the frontend calls InspectorFrontendHost.openURLExternally().
    [[webView _inspector] _openURLExternallyForTesting:urlToOpen.get() useFrontendAPI:YES];
    TestWebKitAPI::Util::run(&openURLExternallyCalled);
}

#endif // PLATFORM(MAC)
