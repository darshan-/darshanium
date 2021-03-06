// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_NAVIGATION_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_NAVIGATION_COMMANDS_H_

// Commands related to the Page Info navigation inside the page info view
// controller.
@protocol PageInfoNavigationCommands

// Shows the site security information.
- (void)showSiteSecurityInfo;

// Shows the cookies information.
- (void)showCookiesInfo;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_NAVIGATION_COMMANDS_H_
