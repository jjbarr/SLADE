#pragma once

#include "ActionSpecial.h"
#include "Game.h"
#include "General/Defs.h"
#include "MapInfo.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "SpecialPreset.h"
#include "ThingType.h"
#include "UDMFProperty.h"
#include "Utility/PropertyList/PropertyList.h"

class ParseTreeNode;
class ArchiveEntry;
class Archive;
class MapLine;
class MapThing;
namespace ZScript
{
class Definitions;
}

namespace Game
{
// Feature Support
enum class Feature
{
	Boom,
	AnyMapName,
	MixTexFlats,
	TxTextures,
	LongNames,
};
enum class UDMFFeature
{
	Slopes,             // Slope support
	FlatLighting,       // Flat lighting independent from sector lighting
	FlatPanning,        // Flat panning
	FlatRotation,       // Flat rotation
	FlatScaling,        // Flat scaling
	LineTransparency,   // Line transparency
	SectorColor,        // Sector colour
	SectorFog,          // Sector fog
	SideLighting,       // Sidedef lighting independent from sector lighting
	SideMidtexWrapping, // Per-sidedef midtex wrapping
	SideScaling,        // Line scaling
	TextureScaling,     // Per-texture line scaling
	TextureOffsets,     // Per-texture offsets compared to per-sidedef
	ThingScaling,       // Per-thing scaling
	ThingRotation,      // Per-thing pitch and yaw rotation
};

typedef std::map<std::string, UDMFProperty> UDMFPropMap;

class Configuration
{
public:
	struct Flag
	{
		int         flag;
		std::string name;
		std::string udmf;
		bool        activation;
	};

	struct MapConf
	{
		std::string mapname;
		std::string sky1;
		std::string sky2;
	};

	Configuration();
	~Configuration() = default;

	void               setDefaults();
	const std::string& currentGame() const { return current_game_; }
	const std::string& currentPort() const { return current_port_; }
	bool               supportsSectorFlags() const { return boom_sector_flag_start_ > 0; }
	std::string        udmfNamespace() const;
	const std::string& skyFlat() const { return sky_flat_; }
	const std::string& scriptLanguage() const { return script_language_; }
	int                lightLevelInterval();

	unsigned           nMapNames() const { return maps_.size(); }
	const std::string& mapName(unsigned index);
	MapConf            mapInfo(std::string_view mapname);

	// General Accessors
	const std::map<int, ActionSpecial>& allActionSpecials() const { return action_specials_; }
	const std::map<int, ThingType>&     allThingTypes() const { return thing_types_; }
	const std::map<int, std::string>&   allSectorTypes() const { return sector_types_; }

	// Feature Support
	bool featureSupported(Feature feature) { return supported_features_[feature]; }
	bool featureSupported(UDMFFeature feature) { return udmf_features_[feature]; }

	// Configuration reading
	void readActionSpecials(ParseTreeNode* node, Arg::SpecialMap& shared_args, ActionSpecial* group_defaults = nullptr);
	void readThingTypes(ParseTreeNode* node, const ThingType& group_defaults = ThingType::unknown());
	void readUDMFProperties(ParseTreeNode* block, UDMFPropMap& plist) const;
	void readGameSection(ParseTreeNode* node_game, bool port_section = false);
	bool readConfiguration(
		std::string_view cfg,
		std::string_view source      = "",
		MapFormat        format      = MapFormat::Unknown,
		bool             ignore_game = false,
		bool             clear       = true);
	bool openConfig(const std::string& game, const std::string& port = "", MapFormat format = MapFormat::Unknown);

	// Action specials
	const ActionSpecial& actionSpecial(unsigned id);
	std::string          actionSpecialName(int special);

	// Thing types
	const ThingType& thingType(unsigned type);
	const ThingType& thingTypeGroupDefaults(const std::string& group);

	// Thing flags
	int         nThingFlags() const { return flags_thing_.size(); }
	std::string thingFlag(unsigned flag_index);
	bool        thingFlagSet(unsigned flag_index, MapThing* thing);
	bool        thingFlagSet(std::string_view udmf_name, MapThing* thing, MapFormat map_format);
	bool        thingBasicFlagSet(std::string_view flag, MapThing* thing, MapFormat map_format);
	std::string thingFlagsString(int flags);
	void        setThingFlag(unsigned flag_index, MapThing* thing, bool set = true);
	void        setThingFlag(std::string_view udmf_name, MapThing* thing, MapFormat map_format, bool set = true);
	void        setThingBasicFlag(std::string_view flag, MapThing* thing, MapFormat map_format, bool set = true);

	// DECORATE
	bool parseDecorateDefs(Archive* archive);
	void clearDecorateDefs();

	// ZScript
	void importZScriptDefs(ZScript::Definitions& defs);

	// MapInfo
	bool parseMapInfo(Archive* archive);
	void clearMapInfo() { map_info_.clear(); }
	void linkDoomEdNums();

	// Line flags
	unsigned    nLineFlags() const { return flags_line_.size(); }
	const Flag& lineFlag(unsigned flag_index);
	bool        lineFlagSet(unsigned flag_index, MapLine* line);
	bool        lineFlagSet(std::string_view udmf_name, MapLine* line, MapFormat map_format);
	bool        lineBasicFlagSet(std::string_view flag, MapLine* line, MapFormat map_format);
	std::string lineFlagsString(MapLine* line);
	void        setLineFlag(unsigned flag_index, MapLine* line, bool set = true);
	void        setLineFlag(std::string_view udmf_name, MapLine* line, MapFormat map_format, bool set = true);
	void        setLineBasicFlag(std::string_view flag, MapLine* line, MapFormat map_format, bool set = true);

	// Line action (SPAC) triggers
	std::string         spacTriggerString(MapLine* line, MapFormat map_format);
	int                 spacTriggerIndexHexen(MapLine* line);
	vector<std::string> allSpacTriggers();
	void                setLineSpacTrigger(unsigned trigger_index, MapLine* line);
	const std::string&  spacTriggerUDMFName(unsigned trigger_index);

	// UDMF properties
	UDMFProperty* getUDMFProperty(const std::string& name, MapObject::Type type);
	UDMFPropMap&  allUDMFProperties(MapObject::Type type);
	void          cleanObjectUDMFProps(MapObject* object);

	// Sector types
	std::string sectorTypeName(int type);
	int         sectorTypeByName(std::string_view name);
	int         baseSectorType(int type) const;
	int         sectorBoomDamage(int type) const;
	bool        sectorBoomSecret(int type) const;
	bool        sectorBoomFriction(int type) const;
	bool        sectorBoomPushPull(int type) const;
	int         boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull) const;

	// Defaults
	std::string defaultString(MapObject::Type type, const std::string& property);
	int         defaultInt(MapObject::Type type, const std::string& property);
	double      defaultFloat(MapObject::Type type, const std::string& property);
	bool        defaultBool(MapObject::Type type, const std::string& property);
	void        applyDefaults(MapObject* object, bool udmf = false);

	// Special Presets
	const vector<SpecialPreset>& specialPresets() const { return special_presets_; }

	// Misc
	void setLightLevelInterval(int interval);
	int  upLightLevel(int light_level);
	int  downLightLevel(int light_level);

	// Testing
	void dumpActionSpecials();
	void dumpThingTypes();
	void dumpValidMapNames();
	void dumpUDMFProperties();

private:
	std::string               current_game_;           // Current game name
	std::string               current_port_;           // Current port name (empty if none)
	std::map<MapFormat, bool> map_formats_;            // Supported map formats
	std::string               udmf_namespace_;         // Namespace to use for UDMF
	int                       boom_sector_flag_start_; // Beginning of Boom sector flags
	std::string               sky_flat_;               // Sky flat for 3d mode
	std::string               script_language_;        // Scripting language (should be extended to allow multiple)
	vector<int>               light_levels_;           // Light levels for up/down light in editor

	// Action specials
	std::map<int, ActionSpecial> action_specials_;

	// Thing types
	std::map<int, ThingType>         thing_types_;
	std::map<std::string, ThingType> tt_group_defaults_;
	vector<ThingType>                parsed_types_;
	// std::map<string, ThingType> parsed_types_;		// ThingTypes parsed from definitions
	// (DECORATE, ZScript etc.)

	// Flags
	vector<Flag> flags_thing_;
	vector<Flag> flags_line_;
	vector<Flag> triggers_line_;

	// Sector types
	std::map<int, std::string> sector_types_;

	// Map info
	vector<MapConf> maps_;
	MapInfo         map_info_;

	// UDMF properties
	UDMFPropMap udmf_vertex_props_;
	UDMFPropMap udmf_linedef_props_;
	UDMFPropMap udmf_sidedef_props_;
	UDMFPropMap udmf_sector_props_;
	UDMFPropMap udmf_thing_props_;

	// Defaults
	PropertyList defaults_line_;
	PropertyList defaults_line_udmf_;
	PropertyList defaults_side_;
	PropertyList defaults_side_udmf_;
	PropertyList defaults_sector_;
	PropertyList defaults_sector_udmf_;
	PropertyList defaults_thing_;
	PropertyList defaults_thing_udmf_;

	// Feature Support
	std::map<Feature, bool>     supported_features_;
	std::map<UDMFFeature, bool> udmf_features_;

	// Special Presets
	vector<SpecialPreset> special_presets_;
};
} // namespace Game
