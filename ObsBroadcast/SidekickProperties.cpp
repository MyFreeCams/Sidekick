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

#include <mutex>
#include <string>

#include <QDesktopServices>
#include <QFile>
#include <QFont>
#include <QMetaObject>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#include <libobs/obs-module.h>

#ifndef _WIN32
#include <ctime>
#else
#include <libfcs/UtilCommon.h>
#endif

// solution
#include <libPlugins/build_version.h>
#include <libPlugins/ObsUtil.h>
#include <libPlugins/SidekickModelConfig.h>

// project
#include "SidekickProperties.h"
#include "ObsBroadcast.h"

extern CBroadcastCtx g_ctx;

#define DLGSZ_HEIGHT            420
#define CONSOLESZ_HEIGHT        200
#define BUTTONSZ_HEIGHT         32
#define OFFSET_Y_LINK_LBL       (DLGSZ_HEIGHT - 130)
#define OFFSET_Y_LINK_BTN       (DLGSZ_HEIGHT - 90)
#define OFFSET_Y_UNLINK_BTN     (DLGSZ_HEIGHT - 90)
#define OFFSET_Y_CLOSE_BTN      (DLGSZ_HEIGHT - 50)


void Ui_SKProps::setupUi(QDialog* pDlg)
{
    QFont font;

    // Set default with to 40% of screen width,
    // Default height to be (width * 72) / 100,
    // or proportionate to original size of 1000x720
    //
    QSize appSize = qApp->screens()[0]->size();

    m_nWidth  = (int)(((double)appSize.width()) * 0.40);
    m_nHeight = (m_nWidth * 72) / 100;

    // Max out size to 1400x900 to prevent background image
    // from repeating on very high res desktops
    if (m_nWidth > 1400)    m_nWidth = 1400;
    if (m_nHeight > 800)    m_nHeight = 800;

    // _MESG("(1) appSize: %d x %d, ratio: %.01f, dpi: %.01f  >> new dlg sz: %d x %d",
    //       appSize.width(), appSize.height(), appRatio, appDpi, m_nWidth, m_nHeight);

    if (pDlg->objectName().isEmpty())
        pDlg->setObjectName(QStringLiteral("SidekickProperties"));

    pDlg->resize(m_nWidth, m_nHeight);
    pDlg->setWindowOpacity(1);

    font.setPointSize(12);
    //font.setPointSize(10);

    // Calculate X,Y and width,height offsets, to pad controls
    // within dialog. Initial nOffsetX is calculated for the linked_label
    // control, will be changed for the link and unlink buttons which
    // take up closer to half the dialog size, rather than almost the
    // whole width of the dialog the way the linked label control does.
    //
    int nOffsetX = (int)((double)m_nWidth * 0.03);
    int nOffsetH = ((m_nHeight * 51) / 720);    //DLGSZ_HEIGHT);
    int nOffsetW = (int)((double)m_nWidth * 0.94);
    int nOffsetY = ((m_nHeight * 33) / DLGSZ_HEIGHT);   // 20;   //((m_nHeight * 60) / DLGSZ_HEIGHT);

    // calculate the width for link/unlink buttons, then the Y pos
    // for where to place link/unlink buttons
    int nLinkOffsetW = (int)((double)m_nWidth * 0.45);
    int nLinkBtnY   = nOffsetY + ( ( m_nHeight * OFFSET_Y_LINK_BTN   ) / DLGSZ_HEIGHT );
    int nUnlinkBtnY = nOffsetY + ( ( m_nHeight * OFFSET_Y_UNLINK_BTN ) / DLGSZ_HEIGHT );

    // old dimensions for close btn
    // closeBtn->setGeometry(QRect(770, 660, 200, 48));
    // if it was 1000x720, then rect of 200x48 @ 770,660 becomes
    // QRect(770, 660, 200 [20% of 1000x width, or 1000 * .20], 48)
    int nBtnWidth   = (int)((double)m_nWidth * .20);
    int nBtnHeight  = ((BUTTONSZ_HEIGHT * m_nHeight) / DLGSZ_HEIGHT);
    int nCloseBtnX  = (nOffsetX + (nLinkOffsetW * 2) + 18) - nBtnWidth;
    int nCloseBtnY  = nOffsetY + ((OFFSET_Y_CLOSE_BTN * m_nHeight) / DLGSZ_HEIGHT);
    int nHelpBtnX   = nOffsetX + 12;
    int nHelpBtnY   = nCloseBtnY;

    int nLabelY = nOffsetY + ((m_nHeight * OFFSET_Y_LINK_LBL) / DLGSZ_HEIGHT);

    closeBtn = new QPushButton(pDlg);
    //helpBtn = new QPushButton(pDlg);
    linked_label = new QLabel(pDlg);
    link_sidekick = new QPushButton(pDlg);
    unlink_sidekick = new QPushButton(pDlg);
    //useWebRTC = new QCheckBox(pDlg);
    //useNormal = new QRadioButton(pDlg);

    //useWebRTC->setObjectName(QStringLiteral("useWebRTC"));
    //useNormal->setObjectName(QStringLiteral("useNormal"));
    linked_label->setObjectName(QStringLiteral("linked_label"));
    link_sidekick->setObjectName(QStringLiteral("link_sidekick"));
    unlink_sidekick->setObjectName(QStringLiteral("unlink_sidekick"));

    closeBtn->setMinimumWidth(nBtnWidth);
    closeBtn->setMinimumHeight(nBtnHeight);

    //helpBtn->setMinimumWidth(nBtnWidth);
    //helpBtn->setMinimumHeight(nBtnHeight);

    // Adjust Y-pos of buttons if we would go off the bottom of the dialog
    if (nCloseBtnY + nBtnHeight > m_nHeight - 8)
    {
        //_MESG("ADJUST! closeBtn from %d => %d", nCloseBtnY, (m_nHeight - (nBtnHeight + 4)));
        nCloseBtnY = m_nHeight - (nBtnHeight + 4);
    }

    if (nUnlinkBtnY + nOffsetH > nCloseBtnY - 8)
    {
        //_MESG("ADJUST! unlinkBtn Y from %d => %d", nUnlinkBtnY, (nCloseBtnY - (nOffsetH + 4)));
        nUnlinkBtnY = nCloseBtnY - (nOffsetH + 8);
        nLinkBtnY = nUnlinkBtnY;
    }

    if (nLabelY + nOffsetH > nUnlinkBtnY - 8)
    {
        //_MESG("ADJUST! labelY from %d => %d", nLabelY, (nUnlinkBtnY - (nOffsetH + 4)));
        nLabelY = nUnlinkBtnY - (nOffsetH + 8);
    }

    // set link/unlink buttons position & sz
    closeBtn->setGeometry(          QRect(nCloseBtnX,                   nCloseBtnY,     nBtnWidth,      nBtnHeight  ) );
    //helpBtn->setGeometry(           QRect(nHelpBtnX,                    nCloseBtnY,     nBtnWidth,      nBtnHeight  ) );
    linked_label->setGeometry(      QRect(nOffsetX,                     nLabelY,        nOffsetW,       nOffsetH    ) );
    link_sidekick->setGeometry(     QRect(nOffsetX                + 12, nLinkBtnY,      nLinkOffsetW,   nOffsetH    ) );
    unlink_sidekick->setGeometry(   QRect(nOffsetX + nLinkOffsetW + 18, nUnlinkBtnY,    nLinkOffsetW,   nOffsetH    ) );
    //useWebRTC->setGeometry(         QRect(nOffsetX + nLinkOffsetW + 40, nLabelY + 2,    nLinkOffsetW,   nOffsetH    ) );

    //_MESG("useWebRTC @ %d,%d  for sz %dx%d", nOffsetX + nLinkOffsetW + 40,  nLabelY + 2,            nLinkOffsetW, nOffsetH  );
    //_MESG("linkLabel @ %d,%d  for sz %dx%d (dlg %dx%d)", nOffsetX, nLabelY, nOffsetW, nOffsetH, m_nWidth, m_nHeight);
    //_MESG("closeBtn  @ %d,%d  for sz %dx%d", nCloseBtnX, nCloseBtnY, nBtnWidth, nBtnHeight);
    //_MESG("linkBtn   @ %d,%d  for sz %dx%d", nOffsetX + 12, nLinkBtnY, nLinkOffsetW, nOffsetH);
    //_MESG("unlinkBtn @ %d,%d  for sz %dx%d", nOffsetX + nLinkOffsetW + 18, nUnlinkBtnY, nLinkOffsetW, nOffsetH);

    closeBtn->setFont(font);
    //helpBtn->setFont(font);
    linked_label->setFont(font);
    link_sidekick->setFont(font);
    unlink_sidekick->setFont(font);
    //useWebRTC->setFont(font);

    unlink_sidekick->setEnabled(false);
    closeBtn->setText("Close");
    //helpBtn->setText("Help");

    QTextEdit* pConsole = new QTextEdit( pDlg );

    constexpr int nNewHeader    = 188;
    int nConsoleHeight          = (nLabelY - nNewHeader) - 4;   //(m_nHeight * CONSOLESZ_HEIGHT) / DLGSZ_HEIGHT;
    int nConsoleWidth           = m_nWidth;
    int nConsoleX               = 0;
    int nVerAdjust              = 70;

    // If our detected height is too small, we can expect that means we are in scaling mode
    // and the background image is going to be scaled up, so move the version up only 60px
    // from the nNewHeader position instead of 70. This keeps thge version label from being
    // positioned over the webcam logo image in the background.
    //
    // On windows this doesnt seem to be an issue since it doesnt scale the background images
    // based on DPI settings appearently(?)
    //
    if (m_nHeight < 1300)
        nVerAdjust = 60;

    pConsole->setMaximumHeight( nConsoleHeight );
    pConsole->setGeometry( QRect(nConsoleX, nNewHeader, nConsoleWidth, nConsoleHeight) );
    pConsole->setReadOnly(true);

    //QFont f("Arial", 14);
    QFont f("Arial", 12);
    pConsole->setFont(f);

    // Version label
    QLabel* versionLabel = new QLabel(QStringLiteral("SidekickVersion"), pDlg);  //sidekick_prop);

    string sVer, sObsVer( obs_get_version_string() );
    stdprintf(sVer,
              "Sidekick v%s (%s, %s @ %s); obs %s",
              SIDEKICK_VERSION_STR,
              SIDEKICK_VERSION_GITBRANCH,
              SIDEKICK_VERSION_GITCOMMIT,
              SIDEKICK_VERSION_BUILDTM,
              sObsVer.c_str());
    versionLabel->setText( sVer.c_str() );

    // label position on bottom-left corner of the header graphic
    versionLabel->setGeometry( QRect(36, nNewHeader - nVerAdjust, m_nWidth - 72, 48) );
    versionLabel->setAttribute(Qt::WA_TranslucentBackground);       // transparent background
    versionLabel->setStyleSheet("color:rgba(255,255,255,0.75);");   // white @ 75% translucency

    pConsole->setReadOnly(true);

    retranslateUi(pDlg);

    QObject::connect(closeBtn,          SIGNAL(released()),         pDlg,   SLOT(onClose()));
    //QObject::connect(helpBtn,           SIGNAL(released()),         pDlg,   SLOT(onHelp()));
    QObject::connect(link_sidekick,     SIGNAL(released()),         pDlg,   SLOT(onLink()));
    QObject::connect(unlink_sidekick,   SIGNAL(released()),         pDlg,   SLOT(onUnlink()));
    //QObject::connect(useWebRTC,         SIGNAL(stateChanged(int)),  pDlg,   SLOT(onWebRTC(int)));

    QMetaObject::connectSlotsByName(pDlg);

    m_pConsole = pConsole;
}


SidekickPropertiesUI::SidekickPropertiesUI(QWidget* parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui_SKProps>())
{
    QFile fileDark(":/css/QtDarkOrange.stylesheet");
    fileDark.open(QFile::ReadOnly);
    QString fileData = fileDark.readAll();

    setStyleSheet(fileData);
    ui->setupUi(this);

    setSizeGripEnabled(false);

    QFont f("Arial", 12);
    setFont(f);

    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}


void SidekickPropertiesUI::onClose(void)
{
    if (isVisible() )
        ShowHideDialog();
}


#if 0
void SidekickPropertiesUI::onHelp(void)
{
    QString qstr = QString::fromUtf8("https://wiki.myfreecams.com/wiki/Sidekick");
    QDesktopServices::openUrl(QUrl(qstr));
}
#endif


void SidekickPropertiesUI::onLink(void)
{
    CObsUtil::ExecMFCLogin();
}


void SidekickPropertiesUI::onUnlink(void)
{
    auto lk = g_ctx.sharedLock();
    g_ctx.clear(true);
    g_ctx.cfg.writePluginConfig();
    g_ctx.cfg.readPluginConfig();
    CBroadcastCtx::sendEvent(SkReadProfile, 0, 0);
}


void SidekickPropertiesUI::relabelPropertiesText(void)
{
    bool isWebRTC = false, isMfc = false, isLinked = false, isLoggedIn = false;
    const char* pszText = "";
    static std::string s_sText;
    std::string sUsername;

    {
        auto lk     = g_ctx.sharedLock();
        sUsername   = g_ctx.cfg.getString("username");
        isLoggedIn  = g_ctx.isLoggedIn;
        isWebRTC    = g_ctx.isWebRTC;
        isLinked    = g_ctx.isLinked;
        isMfc       = g_ctx.isMfc;
    }

    std::string sService = isWebRTC ? "WebRTC" : "RTMP";
    std::string sLoginStatus = isLoggedIn ? "YES" : "NO";

    // Reset logged in label text
    if (isMfc)
    {
        if (isLinked)
        {
            stdprintf(s_sText, "Sidekick (%s) is LINKED with %s.  ModelWeb Login: %s", sService.c_str(), sUsername.c_str(), sLoginStatus.c_str() );
            pszText = s_sText.c_str();
        }
        else
        {
            pszText = "Sidekick is NOT LINKED with any model account.";
        }
    }
    else
    {
        pszText = "Sidekick DISABLED: current profile is not using a MyFreeCams service.";
    }

    // set logged in label text
    ui->linked_label->setText( QString( pszText ) );
    ui->link_sidekick->setDisabled(isMfc ? isLinked : true);
    ui->unlink_sidekick->setDisabled(isMfc ? !isLinked : true);
}


void SidekickPropertiesUI::ShowHideDialog()
{
    setVisible( !isVisible() );
}


MFCDock::MFCDock(QWidget* parent)
    : QDockWidget(parent)
    , ui(std::make_unique<Ui::MFCDock>())
{
    ui->setupUi(this);
}


void MFCDock::onLink()
{
    CObsUtil::ExecMFCLogin();
}


void MFCDock::onUnlink()
{
    auto lk = g_ctx.sharedLock();
    g_ctx.clear(true);
    g_ctx.cfg.writePluginConfig();
    g_ctx.cfg.readPluginConfig();
    CBroadcastCtx::sendEvent(SkReadProfile, 0, 0);
}


void MFCDock::relabelPropertiesText()
{
    bool isWebRTC = false, isMfc = false, isLinked = false, isLoggedIn = false;
    const char* pszText = "";
    static std::string s_sText;
    std::string sUsername;

    {
        auto lk     = g_ctx.sharedLock();
        sUsername   = g_ctx.cfg.getString("username");
        isLoggedIn  = g_ctx.isLoggedIn;
        isWebRTC    = g_ctx.isWebRTC;
        isLinked    = g_ctx.isLinked;
        isMfc       = g_ctx.isMfc;
    }

    std::string sService = isWebRTC ? "WebRTC" : "RTMP";
    std::string sLoginLabel = isLoggedIn ? "<i>ModelWeb logged in</i>" : "<i>ModelWeb not logged in</i>";
    bool mfcLogoVisible = false;

    // Reset logged in label text
    if (isMfc)
    {
        if (isLinked)
        {
            pszText = "Linked";
            if (isLoggedIn)
                mfcLogoVisible = true;
        }
        else
        {
            pszText = "Not Linked";
        }
    }
    else
    {
        pszText = "Non MFC Service";
        sLoginLabel.clear();
        sService.clear();
    }

    ui->linkedLabel->setText(pszText);
    ui->loginLabel->setText(sLoginLabel.c_str());
    ui->modeLabel->setText(sService.c_str());
    ui->usernameLabel->setText(sUsername.c_str());

    ui->linkMfcButton->setDisabled(isMfc ? isLinked : true);
    ui->unlinkMfcButton->setDisabled(isMfc ? !isLinked : true);

    ui->linkMfcButton->setVisible(isMfc ? !isLinked : false);
    ui->unlinkMfcButton->setVisible(isMfc ? isLinked : false);

    ui->logo->setVisible(mfcLogoVisible);
}
