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
#include "MfcOauthApiCtx.h"
#include "ObsBroadcast.h"

extern CBroadcastCtx g_ctx;      // part of MFCLibPlugins.lib::MfcPluginAPI.obj (libPlugins/MFCPluginAPI.cpp)
extern MfcOauthApiCtx g_apiCtx;  // part of MFCLibPlugins.lib::MfcPluginAPI.obj (libPlugins/MFCPluginAPI.cpp)

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
    //int nHelpBtnX   = nOffsetX + 12;
    //int nHelpBtnY   = nCloseBtnY;

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
    QLabel* versionLabel = new QLabel(QStringLiteral("SidekickVersion"), pDlg);  //s_pSidekickProperties);

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
    , m_pCheck(std::make_unique<CheckIfLinked>(&g_apiCtx))
{
    QFile fileDark(":/css/QtDarkOrange.stylesheet");
    fileDark.open(QFile::ReadOnly);
    QString fileData = fileDark.readAll();

    setStyleSheet(fileData);
    ui->setupUi(this);

    setSizeGripEnabled(false);

    QFont f("Arial", 12);
    setFont(f);

    QObject::connect(m_pCheck.get(), SIGNAL(finished()), this, SLOT(onLinked()));

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
    _MESG(__FUNCTION__);
    std::string linkCode, linkUrl, clientId, queryUrlRoot;
    int refId;
    QString qstr;

    {
        auto lk = g_apiCtx.sharedLock();

        if (!g_apiCtx.Link())
            return;

        linkCode = g_apiCtx.linkCode();
        linkUrl = g_apiCtx.linkUrl();
        clientId = g_apiCtx.api.clientId();
        refId = g_apiCtx.api.refId();
        queryUrlRoot = g_apiCtx.api.queryUrlRoot();
    }

    std::stringstream ss;
    ss << queryUrlRoot << "?clientId=" << clientId << "&linkCode=" << linkCode << "&refId=" << refId;

    std::string codeLabel =
        "<span style=\"font-size:18px;\">Link Code:</span><br/><span style=\"font-size:54px;\">" + linkCode
            + "</span><br/><span style=\"font-size:12px;\">Link URL: " + linkUrl
            + "</span><br/><span style=\"font-size:12px;\">Query URL: " + ss.str() + "</span>";
    ui->m_pConsole->moveCursor(QTextCursor::End);
    ui->m_pConsole->setHtml(QString::fromUtf8(codeLabel.c_str()));

    qstr = QString::fromUtf8(linkUrl.c_str());
    QDesktopServices::openUrl(QUrl(qstr));
    m_pCheck->start();
}


void SidekickPropertiesUI::onLinked(void)
{
    _MESG(__FUNCTION__);
    std::string queryResponse;

    {
        auto lk = g_apiCtx.sharedLock();

        g_apiCtx.m_bIsLinked = true;
        bool ret = g_apiCtx.api.FetchStreamingCredentials();

        g_apiCtx.m_bHaveCredentials = true;
        g_apiCtx.m_sQueryResponse = g_apiCtx.api.queryResponse();

        g_apiCtx.m_sVideoCodec  = g_apiCtx.api.codec();
        g_apiCtx.m_sProtocol    = g_apiCtx.api.prot();
        g_apiCtx.m_sRegion      = g_apiCtx.api.region();
        g_apiCtx.m_sUsername    = g_apiCtx.api.username();
        g_apiCtx.m_sPassword    = g_apiCtx.api.pwd();
        g_apiCtx.m_fCamScore    = g_apiCtx.api.camscore();
        g_apiCtx.m_nSid         = g_apiCtx.api.sid();
        g_apiCtx.m_nUid         = g_apiCtx.api.uid();
        g_apiCtx.m_nRoomId      = g_apiCtx.api.room();
        g_apiCtx.m_sStreamKey   = g_apiCtx.api.streamkey();
        g_apiCtx.m_sCtx         = g_apiCtx.api.ctx();
        g_apiCtx.m_sVidCtx      = g_apiCtx.api.vidctx();
        g_apiCtx.m_sVideoServer = g_apiCtx.api.videoserver();

        queryResponse = g_apiCtx.m_sQueryResponse;
    }

    ui->m_pConsole->moveCursor(QTextCursor::End);
    ui->m_pConsole->setHtml(QString::fromUtf8(queryResponse.c_str()));
    //ui->m_pConsole->moveCursor(QTextCursor::End);

    relabelPropertiesText();
}


void SidekickPropertiesUI::onUnlink(void)
{
    _MESG(__FUNCTION__);

    {
        auto lk = g_apiCtx.sharedLock();

        std::string url = g_apiCtx.logoutUrl();
        const QString qstr = QString::fromUtf8(url.c_str());
        QDesktopServices::openUrl(QUrl(qstr));
        g_apiCtx.Unlink();

        ui->m_pConsole->moveCursor(QTextCursor::End);
        ui->m_pConsole->setHtml(QString::fromUtf8(""));
        ui->m_pConsole->moveCursor(QTextCursor::End);
    }

    relabelPropertiesText();
}


void SidekickPropertiesUI::relabelPropertiesText(void)
{
    bool isWebRTC = false, isMfc = false, isLinked = false, isLoggedIn = false;
    const char* pszText = "";
    static string s_sText;
    string sUsername;

    {
        auto lk     = g_ctx.sharedLock();
        auto lock   = g_apiCtx.sharedLock();

        sUsername   = g_apiCtx.username();
        isLinked    = g_apiCtx.IsLinked();
        isLoggedIn  = g_apiCtx.sid() != 0;

        isWebRTC    = g_ctx.isWebRTC;
        isMfc       = g_ctx.isMfc;

        g_ctx.isLinked = isLinked;
        g_ctx.isLoggedIn = isLoggedIn;
        g_ctx.cfg.set("username", sUsername);
    }

    string sService     = isWebRTC      ? "WebRTC"  : "RTMP";
    string sLoginStatus = isLoggedIn    ? "YES"     : "NO";

    // Reset logged in label text
    if (isMfc)
    {
        if (isLinked)
        {
            if (isWebRTC)
            {
                stdprintf(s_sText, "Mode: WebRTC. Linked with %s.", sUsername.c_str() );
                pszText = s_sText.c_str();
            }
            else
            {
                stdprintf(s_sText, "Mode: RTMP. Linked with %s.", sUsername.c_str() );
                pszText = s_sText.c_str();
            }
        }
        else
        {
            pszText = "Not yet linked with any model account.";
        }
    }
    else
    {
        pszText = "Disabled, this profile is not using a MyFreeCams service.";
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
    relabelPropertiesText();
}


void MFCDock::onUnlink()
{
    relabelPropertiesText();
}


void MFCDock::relabelPropertiesText()
{
    bool isWebRTC = false, isMfc = false, isLinked = false, isOnline = false, isCustom = false, isLoggedIn = false;
    const char* pszText = "";
    string sUsername;

    {
        auto lk     = g_ctx.sharedLock();
        auto lock   = g_apiCtx.sharedLock();

        sUsername   = g_apiCtx.username();
        isLinked    = g_apiCtx.IsLinked();
        isLoggedIn  = g_apiCtx.sid() != 0;

        isOnline    = isLoggedIn;
        isWebRTC    = g_ctx.isWebRTC;
        isCustom    = g_ctx.isCustom;
        isMfc       = g_ctx.isMfc;

        g_ctx.isLinked = isLinked;
        g_ctx.isLoggedIn = isLoggedIn;
        g_ctx.cfg.set("username", sUsername);
    }

    string sService, sLoginLabel;
    bool mfcLogoVisible = false;

    // Reset logged in label text
    if (isMfc)
    {
        sLoginLabel = "Modelweb: ";
        sLoginLabel += (isOnline ? "<b><i>logged in</i></b>" : "<i>not logged in</i>");
        sService    = isWebRTC ? "WebRTC" : (isCustom ? "<b><i>Custom RTMP</i></b>" : "RTMP");

        if (isLinked)
        {
            pszText = "Account linked";
            mfcLogoVisible = isOnline;
        }
        else pszText = "Account not linked";

    }
    else pszText = "Non MFC Service";

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
