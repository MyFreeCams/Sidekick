/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#ifndef SIDEKICK_PROPERTIES_H_
#define SIDEKICK_PROPERTIES_H_

#include <memory>
#include <string>

#include <QApplication>
#include <QObject>
#include <QTimer>
#include <QtWidgets>
//#include <QtWidgets/QApplication>
//#include <QtWidgets/QComboBox>
//#include <QtWidgets/QDockWidget>
//#include <QtWidgets/QDialog>
//#include <QtWidgets/QDialogButtonBox>
//#include <QtWidgets/QLabel>
//#include <QtWidgets/QPushButton>
//#include <QtWidgets/QRadioButton>
//#include <QtWidgets/QSpacerItem>
//#include <QtWidgets/QTextEdit>

//#include "ProfileTemplateDlg.h"

QT_BEGIN_NAMESPACE


//---/ Dialog for SidekickPropertiesUI class /------------------------------------------------------
//
// Originally generated with QtDesigner, but managed and edited by hand now.
//
class Ui_SKProps
{
public:
    QPushButton* closeBtn;
    //QPushButton* helpBtn;
    QPushButton* link_sidekick;
    QPushButton* unlink_sidekick;
    QLabel* linked_label;
    int m_nWidth;
    int m_nHeight;

    void setupUi(QDialog* pDlg);

    void retranslateUi(QDialog* pDlg)
    {
        pDlg->setWindowTitle(QApplication::translate("SidekickProperties", "Sidekick Setup", nullptr));
        link_sidekick->setText(QApplication::translate("SidekickProperties", "Link Sidekick to your Account", nullptr));
        unlink_sidekick->setText(QApplication::translate("SidekickProperties", "Unlink Sidekick from your Account", nullptr));
        linked_label->setText(QApplication::translate("SidekickProperties", "", nullptr));
        //useWebRTC->setText(QApplication::translate("SidekickProperties", "Use WebRTC (default)", nullptr));
        //useNormal->setText(QApplication::translate("SidekickProperties", "Normal", nullptr));
    }

    QTextEdit* m_pConsole;
};


class Ui_MFCDock
{
public:
    QAction* toggleMfc;
    QWidget* mfcDockContents;
    QVBoxLayout* buttonsVLayout;
    QPushButton* linkMfcButton;
    QPushButton* unlinkMfcButton;
    QLabel* linkedLabel;
    QLabel* loginLabel;
    QLabel* modeLabel;
    QLabel* usernameLabel;
    QSpacerItem* expVSpacer;

    static QPushButton* CreateButton(QWidget* parent, const std::string& name)
    {
        QPushButton* pButton = new QPushButton(parent);
        pButton->setObjectName(QString(name.c_str()));

        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        //QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pButton->sizePolicy().hasHeightForWidth());
        pButton->setSizePolicy(sizePolicy);

        pButton->setMinimumSize(QSize(100, 0));
        pButton->setCheckable(true);

        return pButton;
    }

    void setupUi(QDockWidget* MFCDock)
    {
        MFCDock->setObjectName(QStringLiteral("MFCDock"));
        MFCDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);

        QSizePolicy sizePolicyMfcDock(QSizePolicy::Ignored, QSizePolicy::Fixed);
        sizePolicyMfcDock.setHorizontalStretch(0);
        sizePolicyMfcDock.setVerticalStretch(0);

        toggleMfc = new QAction(MFCDock);
        toggleMfc->setObjectName(QStringLiteral("toggleMfc"));
        toggleMfc->setCheckable(true);
        toggleMfc->setChecked(true);

        mfcDockContents = new QWidget();
        mfcDockContents->setObjectName(QStringLiteral("mfcDockContents"));

        buttonsVLayout = new QVBoxLayout(mfcDockContents);
        buttonsVLayout->setSpacing(2);
        buttonsVLayout->setObjectName(QStringLiteral("buttonsVLayout"));
        buttonsVLayout->setContentsMargins(4, 4, 4, 4);

        linkMfcButton = CreateButton(mfcDockContents, "linkMfcButton");
        linkMfcButton->setEnabled(true);
        buttonsVLayout->addWidget(linkMfcButton);

        unlinkMfcButton = CreateButton(mfcDockContents, "unlinkMfcButton");
        unlinkMfcButton->setEnabled(false);
        buttonsVLayout->addWidget(unlinkMfcButton);

        QSizePolicy sizePolicyLinkLabel(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicyLinkLabel.setHorizontalStretch(0);
        sizePolicyLinkLabel.setVerticalStretch(0);

        linkedLabel = new QLabel(mfcDockContents);
        linkedLabel->setObjectName(QStringLiteral("linkedLabel"));
        sizePolicyLinkLabel.setHeightForWidth(linkedLabel->sizePolicy().hasHeightForWidth());
        linkedLabel->setSizePolicy(sizePolicyLinkLabel);
        linkedLabel->setAlignment(Qt::AlignCenter);
        buttonsVLayout->addWidget(linkedLabel);

        loginLabel = new QLabel(mfcDockContents);
        loginLabel->setObjectName(QStringLiteral("loginLabel"));
        sizePolicyLinkLabel.setHeightForWidth(loginLabel->sizePolicy().hasHeightForWidth());
        loginLabel->setSizePolicy(sizePolicyLinkLabel);
        loginLabel->setAlignment(Qt::AlignCenter);
        buttonsVLayout->addWidget(loginLabel);

        modeLabel = new QLabel(mfcDockContents);
        modeLabel->setObjectName(QStringLiteral("modeLabel"));
        sizePolicyLinkLabel.setHeightForWidth(modeLabel->sizePolicy().hasHeightForWidth());
        modeLabel->setSizePolicy(sizePolicyLinkLabel);
        modeLabel->setAlignment(Qt::AlignCenter);
        buttonsVLayout->addWidget(modeLabel);

        usernameLabel = new QLabel(mfcDockContents);
        usernameLabel->setObjectName(QStringLiteral("modeLabel"));
        sizePolicyLinkLabel.setHeightForWidth(usernameLabel->sizePolicy().hasHeightForWidth());
        usernameLabel->setSizePolicy(sizePolicyLinkLabel);
        usernameLabel->setAlignment(Qt::AlignCenter);
        buttonsVLayout->addWidget(usernameLabel);

        expVSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        buttonsVLayout->addItem(expVSpacer);

        MFCDock->setWidget(mfcDockContents);

        retranslateUi(MFCDock);

        QObject::connect(linkMfcButton,   SIGNAL(released()), MFCDock, SLOT(onLink()));
        QObject::connect(unlinkMfcButton, SIGNAL(released()), MFCDock, SLOT(onUnlink()));

        QMetaObject::connectSlotsByName(MFCDock);
    }

    void retranslateUi(QDockWidget* MFCDock)
    {
        //MFCDock->setWindowTitle("MFC");
        //toggleMfc->setText("MFC");
        //linkMfcButton->setText("Link");
        //unlinkMfcButton->setText("Unlink");
        //linkedLabel->setText("");

        MFCDock->setWindowTitle(QApplication::translate("MFCDock", "MFC", nullptr));
        linkedLabel->setText(QApplication::translate("MFCDock", "", nullptr));
        loginLabel->setText(QApplication::translate("MFCDock", "", nullptr));
        modeLabel->setText(QApplication::translate("MFCDock", "", nullptr));
        usernameLabel->setText(QApplication::translate("MFCDock", "", nullptr));
        linkMfcButton->setText(QApplication::translate("MFCDock", "Link", nullptr));
        unlinkMfcButton->setText(QApplication::translate("MFCDock", "Unlink", nullptr));
    }
};


namespace Ui {
class MFCDock : public Ui_MFCDock {};
}


QT_END_NAMESPACE


//---/ SidekickPropertiesUI /-----------------------------------------------------------------------
//
// Dialog implementing the SidekickProperties page loaded from the menubar Tools -> Sidekick Setup.
//
class SidekickPropertiesUI : public QDialog
{
    Q_OBJECT

public:
    explicit SidekickPropertiesUI(QWidget* parent);
    ~SidekickPropertiesUI() override = default;

    void ShowHideDialog();

    std::unique_ptr< Ui_SKProps > ui;
    //std::unique_ptr< Ui_profileTemplateDlg > ui2;

public slots:
    void onClose(void);
    //void onHelp(void);
    void onLink(void);
    void onUnlink(void);
    //void onWebRTC(int nState);

    void relabelPropertiesText(void);

    QTextEdit* createConsole(void)
    {
        return ui->m_pConsole;
    }
};


class MFCDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MFCDock(QWidget* parent = nullptr);
    ~MFCDock() override = default;

public slots:
    void onLink();
    void onUnlink();

    void relabelPropertiesText();

private:
    std::unique_ptr<Ui::MFCDock> ui;
};


//---/ SidekickTimer /-------------------------------------------------------------------------
//
// Timer Object for setting up main-thread/ui-thread callbacks from obs module exported
// functions (which are not methods of a QObject derived class).
//
class SidekickTimer : public QObject
{
    Q_OBJECT

public:
    SidekickTimer()
        : m_pTimer(std::make_unique<QTimer>(qApp))
    {
        m_pTimer->setSingleShot(false);
        QObject::connect(m_pTimer.get(), SIGNAL(timeout()), SLOT(onTimerEvent()));
        m_pTimer->start(250);
    }

    ~SidekickTimer() override = default;

    std::unique_ptr<QTimer> m_pTimer;

public slots:
    void onTimerEvent();
};

#endif  // SIDEKICK_PROPERTIES_H_
