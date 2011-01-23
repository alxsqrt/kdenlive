/***************************************************************************
 *   Copyright (C) 2007 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#ifndef PROJECTLIST_H
#define PROJECTLIST_H

#include <QDomNodeList>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QUndoStack>
#include <QTimer>
#include <QApplication>

#include <KTreeWidgetSearchLine>
#include <KUrl>
#include <KIcon>

#ifdef NEPOMUK
#include <nepomuk/kratingpainter.h>
#include <nepomuk/resource.h>
#endif

#include "definitions.h"
#include "timecode.h"
#include "kdenlivesettings.h"
#include "folderprojectitem.h"
#include "subprojectitem.h"

namespace Mlt
{
class Producer;
};

class ProjectItem;
class ProjectListView;
class Render;
class KdenliveDoc;
class DocClipBase;

const int NameRole = Qt::UserRole;
const int DurationRole = NameRole + 1;
const int UsageRole = NameRole + 2;

class ItemDelegate: public QStyledItemDelegate
{
public:
    ItemDelegate(QAbstractItemView* parent = 0): QStyledItemDelegate(parent) {
    }

    /*void drawFocus(QPainter *, const QStyleOptionViewItem &, const QRect &) const {
    }*/

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        if (index.column() == 0 && !index.data(DurationRole).isNull()) {
            QRect r1 = option.rect;
            painter->save();
            QStyleOptionViewItemV4 opt(option);
            QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

            if (option.state & QStyle::State_Selected) {
                painter->setPen(option.palette.highlightedText().color());
            }
            const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
            QPixmap pixmap = qVariantValue<QPixmap>(index.data(Qt::DecorationRole));
            if ((index.flags() & (Qt::ItemIsDragEnabled)) == false) {
                KIcon icon("dialog-close");
                QPainter p(&pixmap);
                p.drawPixmap(1, 1, icon.pixmap(16, 16));
                p.end();
            }

            painter->drawPixmap(r1.left() + textMargin, r1.top() + (r1.height() - pixmap.height()) / 2, pixmap);
            int decoWidth = pixmap.width() + 2 * textMargin;

            QFont font = painter->font();
            font.setBold(true);
            painter->setFont(font);
            int mid = (int)((r1.height() / 2));
            r1.adjust(decoWidth, 0, 0, -mid);
            QRect r2 = option.rect;
            r2.adjust(decoWidth, mid, 0, 0);
            painter->drawText(r1, Qt::AlignLeft | Qt::AlignBottom , index.data().toString());
            font.setBold(false);
            painter->setFont(font);
            QString subText = index.data(DurationRole).toString();
            int usage = index.data(UsageRole).toInt();
            if (usage != 0) subText.append(QString(" (%1)").arg(usage));
            if (option.state & (QStyle::State_Selected)) painter->setPen(option.palette.color(QPalette::Mid));
            QRectF bounding;
            painter->drawText(r2, Qt::AlignLeft | Qt::AlignVCenter , subText, &bounding);
            
            int proxy = index.data(Qt::UserRole + 5).toInt();
            if (proxy > 0) {
                QRectF txtBounding;
                QString proxyText;
                QBrush brush;
                QColor color;
                if (proxy == 1) {
                    proxyText = i18n("Generating proxy...");
                    brush = option.palette.highlight();
                    color = option.palette.color(QPalette::HighlightedText);
                }
                else {
                    proxyText = i18n("Proxy");
                    brush = option.palette.mid();
                    color = option.palette.color(QPalette::WindowText);
                }
                txtBounding = painter->boundingRect(r2, Qt::AlignRight | Qt::AlignVCenter, " " + proxyText + " ");
                painter->setPen(Qt::NoPen);
                painter->setBrush(brush);
                painter->drawRoundedRect(txtBounding, 2, 2);
                painter->setPen(option.palette.highlightedText().color());
                painter->drawText(txtBounding, Qt::AlignHCenter | Qt::AlignVCenter , proxyText);
            }
            
            painter->restore();
        } else if (index.column() == 2 && KdenliveSettings::activate_nepomuk()) {
            if (index.data().toString().isEmpty()) {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }
            QRect r1 = option.rect;
            if (option.state & (QStyle::State_Selected)) {
                painter->fillRect(r1, option.palette.highlight());
            }
#ifdef NEPOMUK
            KRatingPainter::paintRating(painter, r1, Qt::AlignCenter, index.data().toInt());
#endif
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
};

class ProjectList : public QWidget
{
    Q_OBJECT

public:
    ProjectList(QWidget *parent = 0);
    virtual ~ProjectList();

    QDomElement producersList();
    void setRenderer(Render *projectRender);
    void slotUpdateClipProperties(const QString &id, QMap <QString, QString> properties);
    QByteArray headerInfo() const;
    void setHeaderInfo(const QByteArray &state);
    void updateProjectFormat(Timecode t);
    void setupMenu(QMenu *addMenu, QAction *defaultAction);
    void setupGeneratorMenu(QMenu *addMenu, QMenu *transcodeMenu, QMenu *inTimelineMenu);
    QString currentClipUrl() const;
    KUrl::List getConditionalUrls(const QString &condition) const;
    void reloadClipThumbnails();
    QDomDocument generateTemplateXml(QString data, const QString &replaceString);
    void cleanup();
    void trashUnusedClips();
    QList <DocClipBase*> documentClipList() const;
    void addClipCut(const QString &id, int in, int out, const QString desc, bool newItem);
    void removeClipCut(const QString &id, int in, int out);
    void focusTree() const;
    SubProjectItem *getSubItem(ProjectItem *clip, QPoint zone);
    void doUpdateClipCut(const QString &id, const QPoint oldzone, const QPoint zone, const QString &comment);
    bool hasMissingClips();
    void deleteProjectFolder(QMap <QString, QString> map);
    void selectItemById(const QString &clipId);

    /** @brief Returns a string list of all supported mime extensions. */
    static QString getExtensions();
    /** @brief Returns a list of urls containing original and proxy urls. */
    QMap <QString, QString> getProxies();
    /** @brief Enable / disable proxies. */
    void updateProxyConfig();

public slots:
    void setDocument(KdenliveDoc *doc);
    void updateAllClips();
    void slotReplyGetImage(const QString &clipId, const QPixmap &pix);
    void slotReplyGetFileProperties(const QString &clipId, Mlt::Producer *producer, const QMap < QString, QString > &properties, const QMap < QString, QString > &metadata, bool replace);
    void slotAddClip(DocClipBase *clip, bool getProperties);
    void slotDeleteClip(const QString &clipId);
    void slotUpdateClip(const QString &id);
    void slotRefreshClipThumbnail(const QString &clipId, bool update = true);
    void slotRefreshClipThumbnail(QTreeWidgetItem *item, bool update = true);
    void slotRemoveInvalidClip(const QString &id, bool replace);
    void slotSelectClip(const QString &ix);

    /** @brief Prepares removing the selected items. */
    void slotRemoveClip();
    void slotAddClip(const QList <QUrl> givenList = QList <QUrl> (), const QString &groupName = QString(), const QString &groupId = QString());

    /** @brief Adds, edits or deletes a folder item.
    *
    * This is triggered by AddFolderCommand and EditFolderCommand. */
    void slotAddFolder(const QString foldername, const QString &clipId, bool remove, bool edit = false);
    void slotResetProjectList();
    void slotOpenClip();
    void slotEditClip();
    void slotReloadClip(const QString &id = QString());

    /** @brief Shows dialog for setting up a color clip. */
    void slotAddColorClip();
    void regenerateTemplate(const QString &id);
    void slotUpdateClipCut(QPoint p);
    void slotAddClipCut(const QString &id, int in, int out);
    void slotForceProcessing(const QString &id);

private:
    ProjectListView *m_listView;
    Render *m_render;
    Timecode m_timecode;
    double m_fps;
    QTimer m_queueTimer;
    QMenu *m_menu;
    QUndoStack *m_commandStack;
    ProjectItem *getItemById(const QString &id);
    QTreeWidgetItem *getAnyItemById(const QString &id);
    FolderProjectItem *getFolderItemById(const QString &id);
    QAction *m_openAction;
    QAction *m_reloadAction;
    QMenu *m_transcodeAction;
    KdenliveDoc *m_doc;
    ItemDelegate *m_listViewDelegate;
    bool m_refreshed;
    QToolButton *m_addButton;
    QToolButton *m_deleteButton;
    QToolButton *m_editButton;
    QMap <QString, QDomElement> m_infoQueue;
    QMap <QString, QDomElement> m_producerQueue;
    void requestClipInfo(const QDomElement xml, const QString id);
    QList <QString> m_thumbnailQueue;
    QAction *m_proxyAction;
    void requestClipThumbnail(const QString id);

    /** @brief Creates an EditFolderCommand to change the name of an folder item. */
    void editFolder(const QString folderName, const QString oldfolderName, const QString &clipId);

    /** @brief Gets the selected folder (or the folder of the selected item). */
    QStringList getGroup() const;
    void regenerateTemplate(ProjectItem *clip);
    void editClipSelection(QList<QTreeWidgetItem *> list);

    /** @brief Enables and disables transcode actions based on the selected clip's type. */
    void adjustTranscodeActions(ProjectItem *clip) const;
    /** @brief Enables and disables proxy action based on the selected clip. */
    void adjustProxyActions(ProjectItem *clip) const;

    /** @brief Sets the buttons enabled/disabled according to selected item. */
    void updateButtons() const;

private slots:
    void slotClipSelected();
    void slotAddSlideshowClip();
    void slotAddTitleClip();
    void slotAddTitleTemplateClip();

    /** @brief Shows the context menu after enabling and disabling actions based on the item's type.
    * @param pos The position where the menu should pop up
    * @param item The item for which the checks should be done */
    void slotContextMenu(const QPoint &pos, QTreeWidgetItem *item);

    /** @brief Creates an AddFolderCommand. */
    void slotAddFolder();

    /** @brief This is triggered when a clip description has been modified. */
    void slotItemEdited(QTreeWidgetItem *item, int column);
    void slotUpdateClipProperties(ProjectItem *item, QMap <QString, QString> properties);
    void slotProcessNextClipInQueue();
    void slotProcessNextThumbnail();
    void slotCheckForEmptyQueue();
    void slotPauseMonitor();
    /** A clip was modified externally, change icon so that user knows it */
    void slotModifiedClip(const QString &id);
    void slotMissingClip(const QString &id);
    void slotAvailableClip(const QString &id);
    /** @brief Try to find a matching profile for given item. */
    bool adjustProjectProfileToItem(ProjectItem *item = NULL);
    /** @brief Add a sequence from the stopmotion widget. */
    void slotAddOrUpdateSequence(const QString frameName);
    /** @brief A proxy clip was created, update display. */
    void slotGotProxy(const QString id, bool success);
    /** @brief Enable / disable proxy for current clip. */
    void slotProxyCurrentItem(bool doProxy);

signals:
    void clipSelected(DocClipBase *, QPoint zone = QPoint());
    void getFileProperties(const QDomElement, const QString &, int pixHeight, bool);
    void receivedClipDuration(const QString &);
    void showClipProperties(DocClipBase *);
    void showClipProperties(QList <DocClipBase *>, QMap<QString, QString> commonproperties);
    void projectModified();
    void loadingIsOver();
    void displayMessage(const QString, int progress);
    void clipNameChanged(const QString, const QString);
    void clipNeedsReload(const QString&, bool);
    /** @brief A property affecting display was changed, so we need to update monitors and thumbnails
     *  @param id: The clip's id string */
    void refreshClip(const QString &id);
    void updateRenderStatus();
    void deleteProjectClips(QStringList ids, QMap <QString, QString> folderids);
    void findInTimeline(const QString &clipId);
    /** @brief Request a profile change for current document. */
    void updateProfile(const QString &);
};

#endif

