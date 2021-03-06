// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/overlays/test/fake_overlay_presentation_context.h"

#include "base/bind.h"
#include "base/logging.h"
#include "ios/chrome/browser/overlays/public/overlay_presentation_context_observer.h"
#include "ios/chrome/browser/overlays/public/overlay_request_queue.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FakeOverlayPresentationContext::FakeOverlayPresentationContext() = default;
FakeOverlayPresentationContext::~FakeOverlayPresentationContext() = default;

FakeOverlayPresentationContext::PresentationState
FakeOverlayPresentationContext::GetPresentationState(OverlayRequest* request) {
  return states_[request].presentation_state;
}

void FakeOverlayPresentationContext::SimulateDismissalForRequest(
    OverlayRequest* request,
    OverlayDismissalReason reason) {
  FakeUIState& state = states_[request];
  DCHECK_EQ(PresentationState::kPresented, state.presentation_state);
  switch (reason) {
    case OverlayDismissalReason::kUserInteraction:
      state.presentation_state = PresentationState::kUserDismissed;
      break;
    case OverlayDismissalReason::kHiding:
      state.presentation_state = PresentationState::kHidden;
      break;
    case OverlayDismissalReason::kCancellation:
      state.presentation_state = PresentationState::kCancelled;
      break;
  }
  std::move(state.dismissal_callback).Run(reason);
}

void FakeOverlayPresentationContext::SetPresentationCapabilities(
    UIPresentationCapabilities capabilities) {
  if (capabilities_ == capabilities)
    return;

  for (auto& observer : observers_) {
    observer.OverlayPresentationContextWillChangePresentationCapabilities(
        this, capabilities);
  }
  capabilities_ = capabilities;
  for (auto& observer : observers_) {
    observer.OverlayPresentationContextDidChangePresentationCapabilities(this);
  }
}

void FakeOverlayPresentationContext::AddObserver(
    OverlayPresentationContextObserver* observer) {
  observers_.AddObserver(observer);
}

void FakeOverlayPresentationContext::RemoveObserver(
    OverlayPresentationContextObserver* observer) {
  observers_.RemoveObserver(observer);
}

OverlayPresentationContext::UIPresentationCapabilities
FakeOverlayPresentationContext::GetPresentationCapabilities() const {
  return capabilities_;
}

bool FakeOverlayPresentationContext::CanShowUIForRequest(
    OverlayRequest* request,
    UIPresentationCapabilities capabilities) const {
  return capabilities !=
         OverlayPresentationContext::UIPresentationCapabilities::kNone;
}

bool FakeOverlayPresentationContext::CanShowUIForRequest(
    OverlayRequest* request) const {
  return CanShowUIForRequest(request, capabilities_) && !IsShowingOverlayUI();
}

bool FakeOverlayPresentationContext::IsShowingOverlayUI() const {
  for (auto& request_ui_state_pair : states_) {
    if (request_ui_state_pair.second.presentation_state ==
        PresentationState::kPresented) {
      return true;
    }
  }
  return false;
}

void FakeOverlayPresentationContext::PrepareToShowOverlayUI(
    OverlayRequest* request) {}

void FakeOverlayPresentationContext::ShowOverlayUI(
    OverlayRequest* request,
    OverlayPresentationCallback presentation_callback,
    OverlayDismissalCallback dismissal_callback) {
  FakeUIState& state = states_[request];
  state.presentation_state = PresentationState::kPresented;
  state.dismissal_callback = std::move(dismissal_callback);
  std::move(presentation_callback).Run();
}

void FakeOverlayPresentationContext::HideOverlayUI(OverlayRequest* request) {
  SimulateDismissalForRequest(request, OverlayDismissalReason::kHiding);
}

void FakeOverlayPresentationContext::CancelOverlayUI(OverlayRequest* request) {
  FakeUIState& state = states_[request];
  if (state.presentation_state == PresentationState::kPresented) {
    SimulateDismissalForRequest(request, OverlayDismissalReason::kCancellation);
  } else {
    state.presentation_state = PresentationState::kCancelled;
  }
}

FakeOverlayPresentationContext::FakeUIState::FakeUIState() = default;

FakeOverlayPresentationContext::FakeUIState::~FakeUIState() = default;
