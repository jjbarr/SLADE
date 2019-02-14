
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapBackupManager.cpp
// Description: MapBackupManager class - creates/manages map backups
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "MapBackupManager.h"
#include "App.h"
#include "Archive/Formats/ZipArchive.h"
#include "General/Misc.h"
#include "MapEditor.h"
#include "UI/MapBackupPanel.h"
#include "UI/SDialog.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, max_map_backups, 25, CVar::Flag::Save)
namespace
{
// List of entry names to be ignored for backups
wxString mb_ignore_entries[] = { "NODES",    "SSECTORS", "ZNODES",  "SEGS",     "REJECT",
								 "BLOCKMAP", "GL_VERT",  "GL_SEGS", "GL_SSECT", "GL_NODES" };
} // namespace


// -----------------------------------------------------------------------------
//
// MapBackupManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Writes a backup for [map_name] in [archive_name], with the map data entries
// in [map_data]
// -----------------------------------------------------------------------------
bool MapBackupManager::writeBackup(vector<ArchiveEntry::UPtr>& map_data, wxString archive_name, wxString map_name) const
{
	// Create backup directory if needed
	auto backup_dir = App::path("backups", App::Dir::User);
	if (!wxDirExists(backup_dir))
		wxMkdir(backup_dir);

	// Open or create backup zip
	ZipArchive backup;
	archive_name.Replace(".", "_");
	auto backup_file = backup_dir + "/" + archive_name.ToStdString() + "_backup.zip";
	if (!backup.open(backup_file))
		backup.setFilename(backup_file);

	// Filter ignored entries
	vector<ArchiveEntry*> backup_entries;
	for (unsigned a = 0; a < map_data.size(); a++)
	{
		// Check for ignored entry
		bool ignored = false;
		for (unsigned b = 0; b < 10; b++)
		{
			if (S_CMPNOCASE(mb_ignore_entries[b], map_data[a]->name()))
			{
				ignored = true;
				break;
			}
		}

		if (!ignored)
			backup_entries.push_back(map_data[a].get());
	}

	// Compare with last backup (if any)
	auto map_dir = backup.dir(map_name.ToStdString());
	if (map_dir && map_dir->nChildren() > 0)
	{
		auto last_backup = dynamic_cast<ArchiveTreeNode*>(map_dir->child(map_dir->nChildren() - 1));
		bool same        = true;
		if (last_backup->numEntries() != backup_entries.size())
			same = false;
		else
		{
			for (unsigned a = 0; a < last_backup->numEntries(); a++)
			{
				auto e1 = backup_entries[a];
				auto e2 = last_backup->entryAt(a);
				if (e1->size() != e2->size())
				{
					same = false;
					break;
				}

				uint32_t crc1 = Misc::crc(e1->rawData(), e1->size());
				uint32_t crc2 = Misc::crc(e2->rawData(), e2->size());
				if (crc1 != crc2)
				{
					same = false;
					break;
				}
			}
		}

		if (same)
		{
			Log::info(2, "Same data as previous backup - ignoring");
			return true;
		}
	}

	// Add map data to backup
	wxString timestamp = wxDateTime::Now().FormatISOCombined('_');
	timestamp.Replace(":", "");
	wxString dir = map_name + "/" + timestamp;
	for (unsigned a = 0; a < backup_entries.size(); a++)
		backup.addEntry(backup_entries[a], dir.ToStdString(), true);

	// Check for max backups & remove old ones if over
	map_dir = backup.dir(map_name.ToStdString());
	while ((int)map_dir->nChildren() > max_map_backups)
		backup.removeDir(map_dir->child(0)->name(), map_dir);

	// Save backup file
	Archive::save_backup = false;
	bool ok              = backup.save();
	Archive::save_backup = true;

	return ok;
}

// -----------------------------------------------------------------------------
// Shows the map backups for [map_name] in [archive_name], returns the selected
// map backup data in a WadArchive
// -----------------------------------------------------------------------------
Archive* MapBackupManager::openBackup(wxString archive_name, wxString map_name) const
{
	SDialog dlg(MapEditor::windowWx(), wxString::Format("Restore %s backup", CHR(map_name)), "map_backup", 500, 400);
	auto    sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);
	auto panel_backup = new MapBackupPanel(&dlg);
	sizer->Add(panel_backup, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	sizer->AddSpacer(4);
	sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
	sizer->AddSpacer(10);

	if (panel_backup->loadBackups(archive_name, map_name))
	{
		if (dlg.ShowModal() == wxID_OK)
			return panel_backup->selectedMapData();
	}
	else
		wxMessageBox(
			wxString::Format("No backups exist for %s of %s", CHR(map_name), CHR(archive_name)),
			"Restore Backup",
			wxICON_INFORMATION,
			MapEditor::windowWx());

	return nullptr;
}
