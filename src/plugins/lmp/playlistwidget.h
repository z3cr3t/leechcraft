/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

#include <QWidget>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/core/icoreproxyfwd.h>
#include "ui_playlistwidget.h"
#include "player.h"

class QToolBar;
class QActionGroup;
class QUndoStack;
class QSortFilterProxyModel;

namespace LeechCraft
{
namespace LMP
{
	class Player;

	class PlaylistWidget : public QWidget
	{
		Q_OBJECT

		Ui::PlaylistWidget Ui_;
		QToolBar * const PlaylistToolbar_;
		QActionGroup *PlayModesGroup_ = nullptr;
		QSortFilterProxyModel * const PlaylistFilter_;

		QUndoStack * const UndoStack_;

		ICoreProxy_ptr Proxy_;
		Player *Player_ = nullptr;

		QAction *ActionDownloadTrack_ = nullptr;

		QAction *ActionRemoveSelected_ = nullptr;

		QAction *ActionStopAfterSelected_ = nullptr;
		QAction *ActionAddToOneShot_ = nullptr;
		QAction *ActionRemoveFromOneShot_ = nullptr;
		QAction *ActionMoveOneShotUp_ = nullptr;
		QAction *ActionMoveOneShotDown_ = nullptr;

		QAction *ActionShowTrackProps_ = nullptr;
		QAction *ActionShowAlbumArt_ = nullptr;
		QAction *ActionMoveTop_ = nullptr;
		QAction *ActionMoveUp_ = nullptr;
		QAction *ActionMoveDown_ = nullptr;
		QAction *ActionMoveBottom_ = nullptr;

		QMenu *TrackActions_ = nullptr;
		QMenu *ExistingTrackActions_ = nullptr;

		QAction *MoveUpButtonAction_ = nullptr;
		QAction *MoveDownButtonAction_ = nullptr;

		QAction *ActionToggleSearch_ = nullptr;

		QList<AudioSource> NextResetSelect_;
	public:
		PlaylistWidget (QWidget* = nullptr);

		void SetPlayer (Player*, const ICoreProxy_ptr&);
	private:
		void InitCommonActions ();
		void InitToolbarActions ();
		void SetPlayModeButton ();
		void SetSortOrderButton ();
		void InitViewActions ();

		void EnableMoveButtons (bool);

		QList<AudioSource> GetSelected () const;
		void SelectSources (const QList<AudioSource>&);
	public slots:
		void focusIndex (const QModelIndex&);
	private slots:
		void on_Playlist__customContextMenuRequested (const QPoint&);
		void handleChangePlayMode ();
		void handlePlayModeChanged (Player::PlayMode);

		void play (const QModelIndex&);
		void expand (const QModelIndex&);
		void expandAll ();
		void checkSelections ();

		void handleBufferStatus (int);
		void handleSongChanged (const MediaInfo&);

		void handleStdSort ();
		void handleCustomSort ();

		void savePlayScrollPosition ();

		void removeSelectedSongs ();

		void setStopAfterSelected ();
		void addToOneShot ();
		void removeFromOneShot ();
		void moveOneShotUp ();
		void moveOneShotDown ();

		void showTrackProps ();

		void showAlbumArt ();

		void initPerformAfterTrackStart ();
		void initPerformAfterTrackStop ();
		void handleExistingTrackAction (QAction*);

		void handleMoveUp ();
		void handleMoveTop ();
		void handleMoveDown ();
		void handleMoveBottom();

		void handleSavePlaylist ();
		void loadFromDisk ();
		void addURL ();

		bool updateDownloadAction ();
		void handleDownload ();

		void updateStatsLabel ();
	signals:
		void hookPlaylistContextMenuRequested (LeechCraft::IHookProxy_ptr,
				QMenu*,
				LeechCraft::LMP::MediaInfo);
	};
}
}
