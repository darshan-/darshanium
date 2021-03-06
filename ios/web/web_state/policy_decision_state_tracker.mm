// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/policy_decision_state_tracker.h"

#include "base/barrier_closure.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

PolicyDecisionStateTracker::PolicyDecisionStateTracker(
    base::OnceCallback<void(WebStatePolicyDecider::PolicyDecision)> callback)
    : callback_(std::move(callback)) {}

PolicyDecisionStateTracker::~PolicyDecisionStateTracker() = default;

void PolicyDecisionStateTracker::OnSinglePolicyDecisionReceived(
    WebStatePolicyDecider::PolicyDecision decision) {
  if (DeterminedFinalResult())
    return;
  if (decision.ShouldCancelNavigation() && !decision.ShouldDisplayError()) {
    result_ = decision;
    OnFinalResultDetermined();
    return;
  }
  num_decisions_received_++;
  if (decision.ShouldDisplayError() && result_.ShouldAllowNavigation()) {
    result_ = decision;
  }
  decision_closure_.Run();
}

bool PolicyDecisionStateTracker::DeterminedFinalResult() {
  return callback_.is_null();
}

void PolicyDecisionStateTracker::FinishedRequestingDecisions(
    int num_decisions_requested) {
  if (DeterminedFinalResult())
    return;
  decision_closure_ = base::BarrierClosure(
      num_decisions_requested - num_decisions_received_,
      base::Bind(&PolicyDecisionStateTracker::OnFinalResultDetermined,
                 AsWeakPtr()));
}

void PolicyDecisionStateTracker::OnFinalResultDetermined() {
  std::move(callback_).Run(result_);
}

}  // namespace web
