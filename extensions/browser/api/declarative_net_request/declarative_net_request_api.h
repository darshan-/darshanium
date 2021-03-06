// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_

#include <string>

#include "base/macros.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

namespace declarative_net_request {
struct ReadJSONRulesResult;
}  // namespace declarative_net_request

class DeclarativeNetRequestUpdateDynamicRulesFunction
    : public ExtensionFunction {
 public:
  DeclarativeNetRequestUpdateDynamicRulesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.updateDynamicRules",
                             DECLARATIVENETREQUEST_UPDATEDYNAMICRULES)

 protected:
  ~DeclarativeNetRequestUpdateDynamicRulesFunction() override;

  // ExtensionFunction override:
  ExtensionFunction::ResponseAction Run() override;

 private:
  void OnDynamicRulesUpdated(base::Optional<std::string> error);

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestUpdateDynamicRulesFunction);
};

class DeclarativeNetRequestGetDynamicRulesFunction : public ExtensionFunction {
 public:
  DeclarativeNetRequestGetDynamicRulesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.getDynamicRules",
                             DECLARATIVENETREQUEST_GETDYNAMICRULES)

 protected:
  ~DeclarativeNetRequestGetDynamicRulesFunction() override;

  // ExtensionFunction override:
  ExtensionFunction::ResponseAction Run() override;

 private:
  void OnDynamicRulesFetched(
      declarative_net_request::ReadJSONRulesResult read_json_result);

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestGetDynamicRulesFunction);
};

class DeclarativeNetRequestGetMatchedRulesFunction : public ExtensionFunction {
 public:
  DeclarativeNetRequestGetMatchedRulesFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.getMatchedRules",
                             DECLARATIVENETREQUEST_GETMATCHEDRULES)

  static void set_disable_throttling_for_tests(
      bool disable_throttling_for_test) {
    disable_throttling_for_test_ = disable_throttling_for_test;
  }

 protected:
  ~DeclarativeNetRequestGetMatchedRulesFunction() override;

  // ExtensionFunction override:
  ExtensionFunction::ResponseAction Run() override;
  void GetQuotaLimitHeuristics(QuotaLimitHeuristics* heuristics) const override;
  bool ShouldSkipQuotaLimiting() const override;

 private:
  static bool disable_throttling_for_test_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestGetMatchedRulesFunction);
};

class DeclarativeNetRequestSetActionCountAsBadgeTextFunction
    : public ExtensionFunction {
 public:
  DeclarativeNetRequestSetActionCountAsBadgeTextFunction();
  DECLARE_EXTENSION_FUNCTION("declarativeNetRequest.setActionCountAsBadgeText",
                             DECLARATIVENETREQUEST_SETACTIONCOUNTASBADGETEXT)

 protected:
  ~DeclarativeNetRequestSetActionCountAsBadgeTextFunction() override;

  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(
      DeclarativeNetRequestSetActionCountAsBadgeTextFunction);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_DECLARATIVE_NET_REQUEST_API_H_
