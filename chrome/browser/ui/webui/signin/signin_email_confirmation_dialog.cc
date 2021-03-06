// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/signin_email_confirmation_dialog.h"

#include <vector>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/notreached.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/signin_view_controller.h"
#include "chrome/browser/ui/webui/signin/signin_email_confirmation_ui.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

// Dialog size.
const int kSigninEmailConfirmationDialogWidth = 512;
const int kSigninEmailConfirmationDialogMinHeight = 200;
const int kSigninEmailConfirmationDialogMaxHeight = 700;

// Dialog action key;
const char kSigninEmailConfirmationActionKey[] = "action";

// Dialog action values.
const char kSigninEmailConfirmationActionCancel[] = "cancel";
const char kSigninEmailConfirmationActionCreateNewUser[] = "createNewUser";
const char kSigninEmailConfirmationActionStartSync[] = "startSync";

}  // namespace

class SigninEmailConfirmationDialog::DialogWebContentsObserver
    : public content::WebContentsObserver {
 public:
  DialogWebContentsObserver(content::WebContents* web_contents,
                            SigninEmailConfirmationDialog* dialog)
      : content::WebContentsObserver(web_contents),
        signin_email_confirmation_dialog_(dialog) {}
  ~DialogWebContentsObserver() override {}

 private:
  void WebContentsDestroyed() override {
    // The dialog is already closed. No need to call CloseDialog() again.
    // NOTE: |this| is deleted after |ResetDialogObserver| returns.
    signin_email_confirmation_dialog_->ResetDialogObserver();
  }

  void RenderProcessGone(base::TerminationStatus status) override {
    signin_email_confirmation_dialog_->CloseDialog();
  }

  SigninEmailConfirmationDialog* const signin_email_confirmation_dialog_;

  DISALLOW_COPY_AND_ASSIGN(DialogWebContentsObserver);
};

SigninEmailConfirmationDialog::SigninEmailConfirmationDialog(
    SigninViewController* signin_view_controller,
    content::WebContents* contents,
    Profile* profile,
    const std::string& last_email,
    const std::string& new_email,
    Callback callback)
    : signin_view_controller_(signin_view_controller),
      web_contents_(contents),
      profile_(profile),
      last_email_(last_email),
      new_email_(new_email),
      callback_(std::move(callback)) {}

SigninEmailConfirmationDialog::~SigninEmailConfirmationDialog() {}

// static
SigninEmailConfirmationDialog*
SigninEmailConfirmationDialog::AskForConfirmation(
    SigninViewController* signin_view_controller,
    content::WebContents* contents,
    Profile* profile,
    const std::string& last_email,
    const std::string& email,
    Callback callback) {
  base::RecordAction(base::UserMetricsAction("Signin_Show_ImportDataPrompt"));
  // ShowDialog() will take care of ownership.
  SigninEmailConfirmationDialog* dialog = new SigninEmailConfirmationDialog(
      signin_view_controller, contents, profile, last_email, email,
      std::move(callback));
  dialog->ShowDialog();
  return dialog;
}

void SigninEmailConfirmationDialog::ShowDialog() {
  gfx::Size min_size(kSigninEmailConfirmationDialogWidth,
                     kSigninEmailConfirmationDialogMinHeight);
  gfx::Size max_size(kSigninEmailConfirmationDialogWidth,
                     kSigninEmailConfirmationDialogMaxHeight);
  ConstrainedWebDialogDelegate* dialog_delegate =
      ShowConstrainedWebDialogWithAutoResize(profile_, base::WrapUnique(this),
                                             web_contents_, min_size, max_size);

  content::WebContents* dialog_web_contents = dialog_delegate->GetWebContents();

  // Clear the zoom level for the dialog so that it is not affected by the page
  // zoom setting.
  const GURL dialog_url = GetDialogContentURL();
  content::HostZoomMap::Get(dialog_web_contents->GetSiteInstance())
      ->SetZoomLevelForHostAndScheme(dialog_url.scheme(), dialog_url.host(), 0);

  dialog_observer_ =
      std::make_unique<DialogWebContentsObserver>(dialog_web_contents, this);
}

void SigninEmailConfirmationDialog::CloseDialog() {
  content::WebContents* dialog_web_contents = GetDialogWebContents();
  if (!dialog_web_contents)
    return;

  content::WebUI* web_ui = dialog_web_contents->GetWebUI();
  if (web_ui) {
    SigninEmailConfirmationUI* signin_email_confirmation_ui =
        static_cast<SigninEmailConfirmationUI*>(web_ui->GetController());
    if (signin_email_confirmation_ui)
      signin_email_confirmation_ui->Close();
  }
}

void SigninEmailConfirmationDialog::ResetDialogObserver() {
  dialog_observer_.reset();
}

content::WebContents* SigninEmailConfirmationDialog::GetDialogWebContents()
    const {
  return dialog_observer_.get() ? dialog_observer_->web_contents() : nullptr;
}

// ui::WebDialogDelegate implementation

ui::ModalType SigninEmailConfirmationDialog::GetDialogModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 SigninEmailConfirmationDialog::GetDialogTitle() const {
  return base::string16();
}

GURL SigninEmailConfirmationDialog::GetDialogContentURL() const {
  return GURL(chrome::kChromeUISigninEmailConfirmationURL);
}

void SigninEmailConfirmationDialog::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handlers) const {}

void SigninEmailConfirmationDialog::GetDialogSize(gfx::Size* size) const {
  DCHECK(size);

  // Set the dialog width if it's not set, so that the dialog is center-aligned
  // horizontally when it appears. Avoid setting a dialog height in here as
  // this dialog auto-resizes.
  if (size->IsEmpty())
    size->set_width(kSigninEmailConfirmationDialogWidth);
}

std::string SigninEmailConfirmationDialog::GetDialogArgs() const {
  std::string data;
  base::DictionaryValue dialog_args;
  dialog_args.SetString("lastEmail", last_email_);
  dialog_args.SetString("newEmail", new_email_);
  base::JSONWriter::Write(dialog_args, &data);
  return data;
}

void SigninEmailConfirmationDialog::OnDialogClosed(
    const std::string& json_retval) {
  Action action = CLOSE;
  std::unique_ptr<base::DictionaryValue> ret_value(base::DictionaryValue::From(
      base::JSONReader::ReadDeprecated(json_retval)));
  if (ret_value) {
    std::string action_string;
    if (ret_value->GetString(kSigninEmailConfirmationActionKey,
                             &action_string)) {
      if (action_string == kSigninEmailConfirmationActionCancel) {
        action = CLOSE;
      } else if (action_string == kSigninEmailConfirmationActionCreateNewUser) {
        action = CREATE_NEW_USER;
      } else if (action_string == kSigninEmailConfirmationActionStartSync) {
        action = START_SYNC;
      } else {
        NOTREACHED() << "Unexpected action value [" << action_string << "]";
      }
    } else {
      NOTREACHED() << "No action in the dialog close return arguments";
    }
  } else {
    // If the dialog is dismissed without any return value, then simply close
    // the dialog. (see http://crbug.com/667690)
    action = CLOSE;
  }

  if (signin_view_controller_) {
    signin_view_controller_->ResetModalSigninDelegate();
    signin_view_controller_ = nullptr;
  }

  if (callback_)
    std::move(callback_).Run(action);
}

void SigninEmailConfirmationDialog::OnCloseContents(
    content::WebContents* source,
    bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool SigninEmailConfirmationDialog::ShouldShowDialogTitle() const {
  return false;
}

void SigninEmailConfirmationDialog::CloseModalSignin() {
  CloseDialog();
}

void SigninEmailConfirmationDialog::ResizeNativeView(int height) {
  NOTIMPLEMENTED();
}

content::WebContents* SigninEmailConfirmationDialog::GetWebContents() {
  return GetDialogWebContents();
}
