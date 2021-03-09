/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#ifndef PROFILE_TEMPLATE_DLG_H_
#define PROFILE_TEMPLATE_DLG_H_

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>

QT_BEGIN_NAMESPACE

/// Dialog for profile template selection
class Ui_profileTemplateDlg
{
public:
    QDialogButtonBox* buttonBox;
    QLabel* label;
    QComboBox* profile_template;
    QRadioButton* useWebRTC;
    QRadioButton* useNormal;
    QLabel* label_3;

    void setupUi(QDialog* profileTemplateDlg)
    {
        if (profileTemplateDlg->objectName().isEmpty())
            profileTemplateDlg->setObjectName(QStringLiteral("profileTemplateDlg"));
        profileTemplateDlg->resize(400, 203);
        buttonBox = new QDialogButtonBox(profileTemplateDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(30, 160, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        label = new QLabel(profileTemplateDlg);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(20, 90, 71, 16));
        profile_template = new QComboBox(profileTemplateDlg);
        profile_template->setObjectName(QStringLiteral("profile_template"));
        profile_template->setGeometry(QRect(110, 90, 271, 22));
        useWebRTC = new QRadioButton(profileTemplateDlg);
        useWebRTC->setObjectName(QStringLiteral("useWebRTC"));
        useWebRTC->setGeometry(QRect(110, 120, 131, 16));
        QFont font;
        font.setPointSize(8);
        useWebRTC->setFont(font);
        useNormal = new QRadioButton(profileTemplateDlg);
        useNormal->setObjectName(QStringLiteral("useNormal"));
        useNormal->setGeometry(QRect(110, 140, 131, 16));
        label_3 = new QLabel(profileTemplateDlg);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(20, 20, 351, 51));
        QFont font1;
        font1.setPointSize(10);
        label_3->setFont(font1);

        retranslateUi(profileTemplateDlg);
        QObject::connect(buttonBox, SIGNAL(accepted()), profileTemplateDlg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), profileTemplateDlg, SLOT(reject()));

        QMetaObject::connectSlotsByName(profileTemplateDlg);
    }

    void retranslateUi(QDialog* profileTemplateDlg)
    {
        profileTemplateDlg->setWindowTitle(QApplication::translate("profileTemplateDlg", "Dialog", nullptr));
        label->setText(QApplication::translate("profileTemplateDlg", "Profile Template", nullptr));
        useWebRTC->setText(QApplication::translate("profileTemplateDlg", "WebRTC (default)", nullptr));
        useNormal->setText(QApplication::translate("profileTemplateDlg", "Normal", nullptr));
        label_3->setText(QApplication::translate("profileTemplateDlg", "Select the OBS profile to use as the template.", nullptr));
    }
};

namespace Ui {
    class profileTemplateDlg : public Ui_profileTemplateDlg {};
}

QT_END_NAMESPACE

#endif  // PROFILE_TEMPLATE_DLG_H_
