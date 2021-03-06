// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "extensions/browser/api/declarative_net_request/action_tracker.h"
#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_id.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class ExtensionPrefs;
class WarningService;

namespace api {
namespace declarative_net_request {
struct Rule;
}  // namespace declarative_net_request
}  // namespace api

namespace declarative_net_request {
class RulesetMatcher;
enum class DynamicRuleUpdateAction;
struct LoadRequestData;

// Observes loading and unloading of extensions to load and unload their
// rulesets for the Declarative Net Request API. Lives on the UI thread. Note: A
// separate instance of RulesMonitorService is not created for incognito. Both
// the incognito and normal contexts will share the same ruleset.
class RulesMonitorService : public BrowserContextKeyedAPI,
                            public ExtensionRegistryObserver {
 public:
  // An observer used in tests.
  class TestObserver {
   public:
    // Called when the ruleset load (in response to extension load) is complete
    // for |extension_id|,
    virtual void OnRulesetLoadComplete(const ExtensionId& extension_id) = 0;

   protected:
    virtual ~TestObserver() = default;
  };

  // This is public so that it can be deleted by tests.
  ~RulesMonitorService() override;

  // Returns the instance for |browser_context|. An instance is shared between
  // an incognito and a regular context.
  static RulesMonitorService* Get(content::BrowserContext* browser_context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<RulesMonitorService>*
  GetFactoryInstance();

  static std::unique_ptr<RulesMonitorService> CreateInstanceForTesting(
      content::BrowserContext* context);

  // Updates the dynamic rules for the |extension| and then invokes
  // |callback| with an optional error.
  using DynamicRuleUpdateUICallback =
      base::OnceCallback<void(base::Optional<std::string> error)>;
  void UpdateDynamicRules(
      const Extension& extension,
      std::vector<int> rule_ids_to_remove,
      std::vector<api::declarative_net_request::Rule> rules_to_add,
      DynamicRuleUpdateUICallback callback);

  RulesetManager* ruleset_manager() { return &ruleset_manager_; }

  const ActionTracker& action_tracker() const { return action_tracker_; }
  ActionTracker& action_tracker() { return action_tracker_; }

  void SetObserverForTest(TestObserver* observer) { test_observer_ = observer; }

 private:
  class FileSequenceBridge;

  friend class BrowserContextKeyedAPIFactory<RulesMonitorService>;

  struct DynamicRuleUpdate {
    DynamicRuleUpdate(
        std::vector<int> rule_ids_to_remove,
        std::vector<api::declarative_net_request::Rule> rules_to_add,
        DynamicRuleUpdateUICallback ui_callback);

    DynamicRuleUpdate(DynamicRuleUpdate&&);
    DynamicRuleUpdate& operator=(DynamicRuleUpdate&&);

    ~DynamicRuleUpdate();

    std::vector<int> rule_ids_to_remove;
    std::vector<api::declarative_net_request::Rule> rules_to_add;
    DynamicRuleUpdateUICallback ui_callback;
  };

  // The constructor is kept private since this should only be created by the
  // BrowserContextKeyedAPIFactory.
  explicit RulesMonitorService(content::BrowserContext* browser_context);

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "RulesMonitorService"; }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  // ExtensionRegistryObserver implementation.
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const Extension* extension,
                              UninstallReason reason) override;

  // Internal helper for UpdateDynamicRules.
  void UpdateDynamicRulesInternal(const ExtensionId& extension_id,
                                  DynamicRuleUpdate update);

  // Invoked when we have loaded the rulesets in |load_data| on
  // |file_task_runner_|.
  void OnRulesetsLoaded(LoadRequestData load_data);

  // Invoked when the dynamic rules for the extension have been updated.
  void OnDynamicRulesUpdated(DynamicRuleUpdateUICallback callback,
                             LoadRequestData load_data,
                             base::Optional<std::string> error);

  // Unloads all rulesets for the given |extension_id|.
  void UnloadRulesets(const ExtensionId& extension_id);

  // Loads the given |matcher| for the given |extension_id|.
  void LoadRulesets(const ExtensionId& extension_id,
                    std::unique_ptr<CompositeMatcher> matcher);

  // Adds the given ruleset for the given |extension_id|.
  void UpdateRuleset(const ExtensionId& extension_id,
                     std::unique_ptr<RulesetMatcher> ruleset_matcher);

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      registry_observer_{this};

  // Helper to bridge tasks to a sequence which allows file IO.
  std::unique_ptr<const FileSequenceBridge> file_sequence_bridge_;

  // Guaranteed to be valid through-out the lifetime of this instance.
  ExtensionPrefs* const prefs_;
  ExtensionRegistry* const extension_registry_;
  WarningService* const warning_service_;

  content::BrowserContext* const context_;

  declarative_net_request::RulesetManager ruleset_manager_;

  ActionTracker action_tracker_;

  // Non-owned pointer.
  TestObserver* test_observer_ = nullptr;

  // Stores the pending dynamic rule updates to be performed once ruleset
  // loading is done for an extension. This is only maintained for extensions
  // which are undergoing a ruleset load in response to OnExtensionLoaded.
  std::map<ExtensionId, std::vector<DynamicRuleUpdate>>
      pending_dynamic_rule_updates_;

  // Must be the last member variable. See WeakPtrFactory documentation for
  // details.
  base::WeakPtrFactory<RulesMonitorService> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(RulesMonitorService);
};

}  // namespace declarative_net_request

template <>
void BrowserContextKeyedAPIFactory<
    declarative_net_request::RulesMonitorService>::DeclareFactoryDependencies();

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULES_MONITOR_SERVICE_H_
