/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Action.h"
#include "Table.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"


#define ART_BANNER		"gfx/shell/head_custom"

#define MAX_MODS		512	// engine limit

#define TYPE_LENGTH		16
#define NAME_SPACE		4
#define NAME_LENGTH		32+TYPE_LENGTH
#define VER_LENGTH		6+NAME_LENGTH
#define SIZE_LENGTH		10+VER_LENGTH

class CMenuModListModel : public CMenuBaseModel
{
public:
	void Update();
	int GetColumns() const { return 4; }
	int GetRows() const { return m_iNumItems; }
	const char *GetCellText( int line, int column )
	{
		return modsDescription[line][column];
	}

	char		modsDir[MAX_MODS][64];
	char		modsWebSites[MAX_MODS][256];
	char		modsDescription[MAX_MODS][5][32];

	int m_iNumItems;
};

class CMenuCustomGame: public CMenuFramework
{
public:
	CMenuCustomGame() : CMenuFramework("CMenuCustomGame") { }

private:
	static void ChangeGame( CMenuBaseItem *pSelf, void *pExtra );
	static void Go2Site( CMenuBaseItem *pSelf, void *pExtra );
	static void UpdateExtras(CMenuBaseItem *pSelf, void *pExtra);
	virtual void _Init( );
	virtual void _VidInit( );

	CMenuPicButton	load;
	CMenuPicButton	go2url;
	CMenuPicButton	done;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuTable	modList;
	CMenuModListModel modListModel;
};

static CMenuCustomGame	uiCustomGame;

void CMenuCustomGame::ChangeGame(CMenuBaseItem *pSelf, void *pExtra)
{
	char cmd[128];
	sprintf( cmd, "game %s\n", (const char*)pExtra );
	EngFuncs::ClientCmd( FALSE, cmd );
}

void CMenuCustomGame::Go2Site(CMenuBaseItem *pSelf, void *pExtra)
{
	const char *url = (const char *)pExtra;
	if( url[0] )
		EngFuncs::ShellExecute( url, NULL, false );
}

void CMenuCustomGame::UpdateExtras( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCustomGame *parent = (CMenuCustomGame*)pSelf->Parent();
	CMenuTable *self = (CMenuTable*)pSelf;

	int i = self->GetCurrentIndex();

	parent->load.onActivated.pExtra = parent->modListModel.modsDir[i];
	parent->load.SetGrayed( !stricmp( parent->modListModel.modsDir[i], gMenu.m_gameinfo.gamefolder ) );

	parent->go2url.onActivated.pExtra = parent->modListModel.modsWebSites[i];
	parent->go2url.SetGrayed( parent->modListModel.modsWebSites[i][0] == 0 );

	parent->msgBox.onPositive.pExtra = parent->modListModel.modsDir[i];
}

/*
=================
CMenuModListModel::Update
=================
*/
void CMenuModListModel::Update( void )
{
	int	numGames, i;
	GAMEINFO	**games;

	games = EngFuncs::GetGamesList( &numGames );

	for( i = 0; i < numGames; i++ )
	{
		Q_strncpy( modsDir[i], games[i]->gamefolder, sizeof( modsDir[i] ));
		Q_strncpy( modsWebSites[i], games[i]->game_url, sizeof( modsWebSites[i] ));

		Q_strncpy( modsDescription[i][0], games[i]->type, 32 );

		if( ColorStrlen( games[i]->title ) > 31 ) // NAME_LENGTH
		{
			Q_strncpy( modsDescription[i][1], games[i]->title, 32 - 4 );
			// I am lazy to put strncat here :(
			modsDescription[i][1][28] = modsDescription[i][1][29] = modsDescription[i][1][30] = '.';
			modsDescription[i][1][31] = 0;
		}
		else Q_strncpy( modsDescription[i][1], games[i]->title, 32 );

		Q_strncpy( modsDescription[i][2], games[i]->version, 32 );

		if( games[i]->size[0] && atoi( games[i]->size ) != 0 )
			Q_strncpy( modsDescription[i][3], games[i]->size, 32 );
		else Q_strncpy( modsDescription[i][3], "0.0 Mb", 32 );
	}

	m_iNumItems = numGames;
}

/*
=================
UI_CustomGame_Init
=================
*/
void CMenuCustomGame::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	load.SetNameAndStatus("Activate", "Activate selected custom game" );
	load.SetPicture( PC_ACTIVATE );
	load.onActivatedClActive = msgBox.MakeOpenEvent();
	load.onActivated = ChangeGame;

	go2url.SetNameAndStatus( "Visit web site", "Visit the web site of game developers" );
	go2url.SetPicture( PC_VISIT_WEB_SITE );
	go2url.onActivated = Go2Site;

	done.SetNameAndStatus( "Done", "Return to main menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = HideCb;

	modList.onChanged = UpdateExtras;
	modList.SetupColumn( 0, "Type", 0.20f );
	modList.SetupColumn( 1, "Name", 0.50f );
	modList.SetupColumn( 2, "Ver",  0.15f );
	modList.SetupColumn( 3, "Size", 0.15f );
	modList.SetModel( &modListModel );

	msgBox.SetMessage( "Leave current game?" );
	msgBox.onPositive = ChangeGame;
	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( load );
	AddItem( go2url );
	AddItem( done );
	AddItem( modList );

	for( int i = 0; i < modListModel.GetRows(); i++ )
	{
		if( !stricmp( modListModel.modsDir[i], gMenu.m_gameinfo.gamefolder ) )
		{
			modList.SetCurrentIndex( i );
			modList.onChanged( &modList );
			break;
		}
	}
}

void CMenuCustomGame::_VidInit()
{
	load.SetCoord( 72, 230 );
	go2url.SetCoord( 72, 280 );
	done.SetCoord( 72, 330 );
	modList.SetRect( 300, 255, 640, 440 );
}

/*
=================
UI_CustomGame_Precache
=================
*/
void UI_CustomGame_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_CustomGame_Menu
=================
*/
void UI_CustomGame_Menu( void )
{
	// current instance is not support game change
	if( !EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		return;

	UI_CustomGame_Precache();
	uiCustomGame.Show();
}
