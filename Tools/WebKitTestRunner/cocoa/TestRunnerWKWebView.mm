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

#import "config.h"
#import "TestRunnerWKWebView.h"

#import "WebKitTestRunnerDraggingInfo.h"
#import <wtf/Assertions.h>
#import <wtf/RetainPtr.h>

#if PLATFORM(IOS)
@interface WKWebView ()

// FIXME: move these to WKWebView_Private.h
- (void)scrollViewWillBeginZooming:(UIScrollView *)scrollView withView:(UIView *)view;
- (void)scrollViewDidEndZooming:(UIScrollView *)scrollView withView:(UIView *)view atScale:(CGFloat)scale;

@end
#endif

#if WK_API_ENABLED

@interface TestRunnerWKWebView ()
@property (nonatomic, copy) void (^zoomToScaleCompletionHandler)(void);
@end

@implementation TestRunnerWKWebView

#if PLATFORM(MAC)
- (void)dragImage:(NSImage *)anImage at:(NSPoint)viewLocation offset:(NSSize)initialOffset event:(NSEvent *)event pasteboard:(NSPasteboard *)pboard source:(id)sourceObj slideBack:(BOOL)slideFlag
{
    RetainPtr<WebKitTestRunnerDraggingInfo> draggingInfo = adoptNS([[WebKitTestRunnerDraggingInfo alloc] initWithImage:anImage offset:initialOffset pasteboard:pboard source:sourceObj]);
    [self draggingUpdated:draggingInfo.get()];
}
#endif

#if PLATFORM(IOS)
- (void)zoomToScale:(double)scale animated:(BOOL)animated completionHandler:(void (^)(void))completionHandler
{
    ASSERT(!self.zoomToScaleCompletionHandler);
    self.zoomToScaleCompletionHandler = completionHandler;

    [self.scrollView setZoomScale:scale animated:animated];
}

- (void)scrollViewWillBeginZooming:(UIScrollView *)scrollView withView:(nullable UIView *)view
{
    [super scrollViewWillBeginZooming:scrollView withView:view];

    if (self.willBeginZoomingCallback)
        self.willBeginZoomingCallback();
}

- (void)scrollViewDidEndZooming:(UIScrollView *)scrollView withView:(UIView *)view atScale:(CGFloat)scale
{
    [super scrollViewDidEndZooming:scrollView withView:view atScale:scale];
    
    if (self.didEndZoomingCallback)
        self.didEndZoomingCallback();

    if (self.zoomToScaleCompletionHandler) {
        self.zoomToScaleCompletionHandler();
        self.zoomToScaleCompletionHandler = nullptr;
    }
}
#endif

@end

#endif // WK_API_ENABLED
