// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/inoreader/gui/formeditinoreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/inoreaderserviceroot.h"

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent) : QDialog(parent),
  m_network(nullptr), m_editableRoot(nullptr) {
  m_ui.setupUi(this);
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("inoreader")));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("Not tested yet."),
                                  tr("Not tested yet."));
  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("User-visible username"));

  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_btnTestSetup);
  setTabOrder(m_ui.m_btnTestSetup, m_ui.m_buttonBox);

  connect(m_ui.m_spinLimitMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int value) {
    if (value <= 0) {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else {
      m_ui.m_spinLimitMessages->setSuffix(QSL(" ") + tr("messages"));
    }
  });
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkUsername);
  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditInoreaderAccount::testSetup);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditInoreaderAccount::onClickedOk);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditInoreaderAccount::onClickedCancel);

  m_ui.m_spinLimitMessages->setValue(INOREADER_DEFAULT_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMinimum(INOREADER_UNLIMITED_BATCH_SIZE);

  checkUsername(m_ui.m_txtUsername->lineEdit()->text());
}

FormEditInoreaderAccount::~FormEditInoreaderAccount() {}

void FormEditInoreaderAccount::testSetup() {
  if (m_network->oauth()->login()) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("You are already logged in."),
                                    tr("Access granted."));
  }
  else {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Progress,
                                    tr("Requested access approval. Respond to it, please."),
                                    tr("Access approval was requested via OAuth 2.0 protocol."));
  }
}

void FormEditInoreaderAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new InoreaderServiceRoot(m_network);
    editing_account = false;
  }

  m_editableRoot->network()->setUsername(m_ui.m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setBatchSize(m_ui.m_spinLimitMessages->value());
  m_editableRoot->saveAccountDataToDatabase();
  accept();

  if (editing_account) {
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditInoreaderAccount::onClickedCancel() {
  reject();
}

void FormEditInoreaderAccount::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered."));
  }
}

void FormEditInoreaderAccount::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

void FormEditInoreaderAccount::hookNetwork() {
  connect(m_network->oauth(), &OAuth2Service::tokensReceived, this, &FormEditInoreaderAccount::onAuthGranted);
  connect(m_network->oauth(), &OAuth2Service::tokensRetrieveError, this, &FormEditInoreaderAccount::onAuthError);
  connect(m_network->oauth(), &OAuth2Service::authFailed, this, &FormEditInoreaderAccount::onAuthFailed);
}

void FormEditInoreaderAccount::unhookNetwork() {
  disconnect(m_network->oauth(), &OAuth2Service::tokensReceived, this, &FormEditInoreaderAccount::onAuthGranted);
  disconnect(m_network->oauth(), &OAuth2Service::tokensRetrieveError, this, &FormEditInoreaderAccount::onAuthError);
  disconnect(m_network->oauth(), &OAuth2Service::authFailed, this, &FormEditInoreaderAccount::onAuthFailed);
}

InoreaderServiceRoot* FormEditInoreaderAccount::execForCreate() {
  setWindowTitle(tr("Add new Inoreader account"));
  m_network = new InoreaderNetworkFactory(this);
  hookNetwork();
  exec();
  unhookNetwork();
  return m_editableRoot;
}

void FormEditInoreaderAccount::execForEdit(InoreaderServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Inoreader account"));
  m_editableRoot = existing_root;
  m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->userName());
  m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());

  m_network = existing_root->network();
  hookNetwork();
  exec();
  unhookNetwork();
}
