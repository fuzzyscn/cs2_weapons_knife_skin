#include <stdio.h>
#include "Skin.h"
#include "metamod_oslink.h"
#include "utils.hpp"
#include <utlstring.h>
#include <KeyValues.h>
#include "sdk/schemasystem.h"
#include "sdk/CBaseEntity.h"
#include "sdk/CGameRulesProxy.h"
#include "sdk/CBasePlayerPawn.h"
#include "sdk/CCSPlayerController.h"
#include "sdk/CCSPlayer_ItemServices.h"
#include "sdk/CSmokeGrenadeProjectile.h"
#include <map>
#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#else
#include "utils/module.h"
#endif
#include <string>

Skin g_Skin;
PLUGIN_EXPOSE(Skin, g_Skin);
IVEngineServer2* engine = nullptr;
IGameEventManager2* gameeventmanager = nullptr;
IGameResourceServiceServer* g_pGameResourceService = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CSchemaSystem* g_pCSchemaSystem = nullptr;
CCSGameRules* g_pGameRules = nullptr;
CPlayerSpawnEvent g_PlayerSpawnEvent;
CRoundPreStartEvent g_RoundPreStartEvent;
CEntityListener g_EntityListener;
bool g_bPistolRound;

typedef struct SkinParm
{
	int m_nFallbackPaintKit;
	int m_nFallbackSeed;
	float m_flFallbackWear;
}SkinParm;;

#ifdef _WIN32
typedef void*(FASTCALL* EntityRemove_t)(CGameEntitySystem*, void*, void*, uint64_t);
typedef void*(FASTCALL* SetMeshGroupMask_t)(uint64_t mask_id);
typedef void(FASTCALL* GiveNamedItem_t)(void* itemService,const char* pchName, void* iSubType,void* pScriptItem, void* a5,void* a6);
typedef void(FASTCALL* UTIL_ClientPrintAll_t)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4);
typedef void(FASTCALL *ClientPrint_t)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);

extern EntityRemove_t FnEntityRemove;
extern SetMeshGroupMask_t FnSetMeshGroupMask;
extern GiveNamedItem_t FnGiveNamedItem;
extern UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
extern ClientPrint_t FnUTIL_ClientPrint;
EntityRemove_t FnEntityRemove;
SetMeshGroupMask_t FnSetMeshGroupMask;
GiveNamedItem_t FnGiveNamedItem;
UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
ClientPrint_t FnUTIL_ClientPrint;
#else
void (*FnEntityRemove)(CGameEntitySystem*, void*, void*, uint64_t) = nullptr;
void (*FnSetMeshGroupMask)(uint64_t mask_id) = nullptr;
void (*FnGiveNamedItem)(void* itemService,const char* pchName, void* iSubType,void* pScriptItem, void* a5,void* a6) = nullptr;
void (*FnUTIL_ClientPrintAll)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) = nullptr;
void(*FnUTIL_ClientPrint)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);
#endif

std::map<int, std::string> g_WeaponsMap;
std::map<uint64_t, std::map<int, SkinParm>> g_PlayerSkins;

class GameSessionConfiguration_t { };
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

#ifdef _WIN32
inline void* FindSignature(const char* modname,const char* sig)
{
	DWORD64 hModule = (DWORD64)GetModuleHandle(modname);
	if (!hModule)
	{
		return NULL;
	}
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32 mod = {sizeof(MODULEENTRY32)};
	
	while (Module32Next(hSnap, &mod))
	{
		if (!strcmp(modname, mod.szModule))
		{
			if(!strstr(mod.szExePath,"metamod"))
				break;
		}
	}
	CloseHandle(hSnap);
	byte* b_sig = (byte*)sig;
	int sig_len = strlen(sig);
	byte* addr = (byte*)mod.modBaseAddr;
	for (int i = 0; i < mod.modBaseSize; i++)
	{
		int flag = 0;
		for (int n = 0; n < sig_len; n++)
		{
			if (i + n >= mod.modBaseSize)break;
			if (*(b_sig + n)=='\x3F' || *(b_sig + n) == *(addr + i+ n))
			{
				flag++;
			}
		}
		if (flag == sig_len)
		{
			return (void*)(addr + i);
		}
	}
	return NULL;
}
#endif

bool Skin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceService, IGameResourceServiceServer, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	// Get CSchemaSystem
	{
		HINSTANCE m_hModule = dlmount(WIN_LINUX("schemasystem.dll", "libschemasystem.so"));
		g_pCSchemaSystem = reinterpret_cast<CSchemaSystem*>(reinterpret_cast<CreateInterfaceFn>(dlsym(m_hModule, "CreateInterface"))(SCHEMASYSTEM_INTERFACE_VERSION, nullptr));
		dlclose(m_hModule);
	}

	SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);

	gameeventmanager = static_cast<IGameEventManager2*>(CallVFunc<IToolGameEventAPI*, 91>(g_pSource2Server));

	ConVar_Register(FCVAR_GAMEDLL);

	g_WeaponsMap = { {1,"weapon_deagle"},{2,"weapon_elite"},{3,"weapon_fiveseven"},{4,"weapon_glock"},{7,"weapon_ak47"},{8,"weapon_aug"},{9,"weapon_awp"},{10,"weapon_famas"},{11,"weapon_g3sg1"},{13,"weapon_galilar"},{14,"weapon_m249"},{16,"weapon_m4a1"},{17,"weapon_mac10"},{19,"weapon_p90"},{23,"weapon_mp5sd"},{24,"weapon_ump45"},{25,"weapon_xm1014"},{26,"weapon_bizon"},{27,"weapon_mag7"},{28,"weapon_negev"},{29,"weapon_sawedoff"},{30,"weapon_tec9"},{31,"weapon_taser"},{32,"weapon_hkp2000"},{33,"weapon_mp7"},{34,"weapon_mp9"},{35,"weapon_nova"},{36,"weapon_p250"},{37,"weapon_shield"},{38,"weapon_scar20"},{39,"weapon_sg556"},{40,"weapon_ssg08"},{42,"weapon_knife"},{59,"weapon_knife_t"},{60,"weapon_m4a1_silencer"},{61,"weapon_usp_silencer"},{63,"weapon_cz75a"},{64,"weapon_revolver"},{500,"weapon_bayonet"},{503,"weapon_knife_css"},{505,"weapon_knife_flip"},{506,"weapon_knife_gut"},{507,"weapon_knife_karambit"},{508,"weapon_knife_m9_bayonet"},{509,"weapon_knife_tactical"},{512,"weapon_knife_falchion"},{514,"weapon_knife_survival_bowie"},{515,"weapon_knife_butterfly"},{516,"weapon_knife_push"},{517,"weapon_knife_cord"},{518,"weapon_knife_canis"},{519,"weapon_knife_ursus"},{520,"weapon_knife_gypsy_jackknife"},{521,"weapon_knife_outdoor"},{522,"weapon_knife_stiletto"},{523,"weapon_knife_widowmaker"},{525,"weapon_knife_skeleton"},{526,"weapon_knife_kukri"} };
	#ifdef _WIN32	
	byte* vscript = (byte*)FindSignature("vscript.dll", "\xBE\x01\x3F\x3F\x3F\x2B\xD6\x74\x61\x3B\xD6");
	if(vscript)
	{
		DWORD pOld;
		VirtualProtect(vscript, 2, PAGE_EXECUTE_READWRITE, &pOld);
		*(vscript + 1) = 2;
		VirtualProtect(vscript, 2, pOld, &pOld);
	}
	#endif
	return true;
}

bool Skin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);
	SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);

	gameeventmanager->RemoveListener(&g_PlayerSpawnEvent);
	gameeventmanager->RemoveListener(&g_RoundPreStartEvent);

	g_pGameEntitySystem->RemoveListenerEntity(&g_EntityListener);

	ConVar_Unregister();
	
	return true;
}

void Skin::NextFrame(std::function<void()> fn)
{
	m_nextFrame.push_back(fn);
}

void Skin::StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*)
{
	#ifdef _WIN32
	FnUTIL_ClientPrintAll = (UTIL_ClientPrintAll_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x81\xEC\x70\x01\x3F\x3F\x8B\xE9");
	FnUTIL_ClientPrint = (ClientPrint_t)FindSignature("server.dll", "\x48\x85\xC9\x0F\x84\x3F\x3F\x3F\x3F\x48\x8B\xC4\x48\x89\x58\x18");
	FnGiveNamedItem = (GiveNamedItem_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x18\x48\x89\x74\x24\x20\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8D\x6C\x24\xD9");
	FnEntityRemove = (EntityRemove_t)FindSignature("server.dll", "\x48\x85\xD2\x0F\x3F\x3F\x3F\x3F\x3F\x57\x48\x3F\x3F\x3F\x48\x89\x3F\x3F\x3F\x48\x8B\xF9\x48\x8B");
    FnSetMeshGroupMask = (SetMeshGroupMask_t)FindSignature("server.dll", "\xE8\x3F\x3F\x3F\x3F\x8B\x45\xD0\x48\x8B\x55\xD8");
	#else
	CModule libserver(g_pSource2Server);
	FnUTIL_ClientPrintAll = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 D7 41 56 49 89 F6 41 55 41 89 FD").RCast< decltype(FnUTIL_ClientPrintAll) >();
	FnUTIL_ClientPrint = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 CF 41 56 49 89 D6 41 55 41 89 F5 41 54 4C 8D A5 A0 FE FF FF").RCast<decltype(FnUTIL_ClientPrint)>();
	FnGiveNamedItem = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 49 89 CE 41 55 49 89 F5 41 54 49 89 D4 53 48 89").RCast<decltype(FnGiveNamedItem)>();
	FnEntityRemove = libserver.FindPatternSIMD("48 85 F6 74 0B 48 8B 76 10 E9 B2 FE FF FF").RCast<decltype(FnEntityRemove)>();
	FnSetMeshGroupMask = libserver.FindPatternSIMD("E8 ? ? ? ? 8B 45 D0 48 8B 55 D8").RCast<decltype(FnSetMeshGroupMask)>();
	#endif
	g_pGameRules = nullptr;

	static bool bDone = false;
	if (!bDone)
	{
		g_pGameEntitySystem = *reinterpret_cast<CGameEntitySystem**>(reinterpret_cast<uintptr_t>(g_pGameResourceService) + WIN_LINUX(0x58, 0x50));
		g_pEntitySystem = g_pGameEntitySystem;

		g_pGameEntitySystem->AddListenerEntity(&g_EntityListener);

		gameeventmanager->AddListener(&g_PlayerSpawnEvent, "player_spawn", true);
		gameeventmanager->AddListener(&g_RoundPreStartEvent, "round_prestart", true);

		bDone = true;
	}
}

void Skin::GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
	if (!g_pGameRules)
	{
		CCSGameRulesProxy* pGameRulesProxy = static_cast<CCSGameRulesProxy*>(UTIL_FindEntityByClassname(nullptr, "cs_gamerules"));
		if (pGameRulesProxy)
		{
			g_pGameRules = pGameRulesProxy->m_pGameRules();
		}
	}
	
	while (!m_nextFrame.empty())
	{
		m_nextFrame.front()();
		m_nextFrame.pop_front();
	}
}

void CPlayerSpawnEvent::FireGameEvent(IGameEvent* event)
{
	if (!g_pGameRules || g_pGameRules->m_bWarmupPeriod())
		return;
	CBasePlayerController* pPlayerController = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
	if (!pPlayerController || pPlayerController->m_steamID() == 0) // Ignore bots
		return;
}

void CRoundPreStartEvent::FireGameEvent(IGameEvent* event)
{
	if (g_pGameRules)
	{
		g_bPistolRound = g_pGameRules->m_totalRoundsPlayed() == 0 || (g_pGameRules->m_bSwitchingTeamsAtRoundReset() && g_pGameRules->m_nOvertimePlaying() == 0) || g_pGameRules->m_bGameRestart();
	}
}

void CEntityListener::OnEntitySpawned(CEntityInstance* pEntity)
{
	CBasePlayerWeapon* pBasePlayerWeapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
	if(!pBasePlayerWeapon)return;
	
	g_Skin.NextFrame([pBasePlayerWeapon = pBasePlayerWeapon]()
	{
		int64_t steamid = pBasePlayerWeapon->m_OriginalOwnerXuidLow();
		if(!steamid)return;
		int64_t weaponId = pBasePlayerWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex();

		auto weapon = g_PlayerSkins.find(steamid);
		if(weapon == g_PlayerSkins.end())return;
		auto skin_parm = weapon->second.find(weaponId);
		if(skin_parm == weapon->second.end())return;
		
		pBasePlayerWeapon->m_AttributeManager().m_Item().m_iItemIDHigh() = -1;
		
		pBasePlayerWeapon->m_nFallbackPaintKit() = skin_parm->second.m_nFallbackPaintKit;
		pBasePlayerWeapon->m_nFallbackSeed() = skin_parm->second.m_nFallbackSeed;
		pBasePlayerWeapon->m_flFallbackWear() = skin_parm->second.m_flFallbackWear;
		META_CONPRINTF( "--------Fuzzys Skin System: steamId: %lld itemId: %d\n", steamid, weaponId);
	});
}

CON_COMMAND_F(skin, "修改武器皮肤", FCVAR_CLIENT_CAN_EXECUTE)
{
    if (context.GetPlayerSlot() == -1) return;
    CCSPlayerController* pPlayerController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));
    CCSPlayerPawnBase* pPlayerPawn = pPlayerController->m_hPlayerPawn();
    if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE)
        return;
    char buf[255] = { 0 };
    if (args.ArgC() != 2 && args.ArgC() != 4)
    {
		sprintf(buf, "-------------------------------------------------");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 输入命令 \x06skin <skin_id> \x01到控制台!");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 搜索 \x06skin_id \x01网站地址 \x06 skin.fu.link !");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 如有其他问题请加QQ群询问: \x06 314498023!");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, "-------------------------------------------------");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
        return;
    }

    CPlayer_WeaponServices* pWeaponServices = pPlayerPawn->m_pWeaponServices();

    int64_t steamid = pPlayerController->m_steamID();
    int64_t weaponId = pWeaponServices->m_hActiveWeapon()->m_AttributeManager().m_Item().m_iItemDefinitionIndex();

    auto weapon_name = g_WeaponsMap.find(weaponId);
    if (weapon_name == g_WeaponsMap.end()) return;

    g_PlayerSkins[steamid][weaponId].m_nFallbackPaintKit = atoi(args.Arg(1));
    if (args.ArgC() == 4)
    {
        g_PlayerSkins[steamid][weaponId].m_nFallbackSeed = atoi(args.Arg(2));
        g_PlayerSkins[steamid][weaponId].m_flFallbackWear = atof(args.Arg(3));
    }
    else
    {
        g_PlayerSkins[steamid][weaponId].m_nFallbackSeed = 0;
        g_PlayerSkins[steamid][weaponId].m_flFallbackWear = 0.0f;
    }

    CBasePlayerWeapon* pPlayerWeapon = pWeaponServices->m_hActiveWeapon();
    
    pWeaponServices->RemoveWeapon(pPlayerWeapon);
    FnEntityRemove(g_pGameEntitySystem, pPlayerWeapon, nullptr, -1);
    FnGiveNamedItem(pPlayerPawn->m_pItemServices(), weapon_name->second.c_str(), nullptr, nullptr, nullptr, nullptr);
    pPlayerWeapon->m_AttributeManager().m_Item().m_iAccountID() = 271098320;
    int64_t skinMeshGroupMask = 2;
    FnSetMeshGroupMask(skinMeshGroupMask);
    
    META_CONPRINTF("--------Fuzzys Skin System: Skin called by %lld\n", steamid);
    sprintf(buf, " \x0E [皮肤系统] \x04 更换成功! 当前武器皮肤编号:%d 模板:%d 磨损:%f", g_PlayerSkins[steamid][weaponId].m_nFallbackPaintKit, g_PlayerSkins[steamid][weaponId].m_nFallbackSeed, g_PlayerSkins[steamid][weaponId].m_flFallbackWear);
    FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
}

CON_COMMAND_F(knife, "给玩家发刀", FCVAR_CLIENT_CAN_EXECUTE)
{
    if (context.GetPlayerSlot() == -1) return;
    CCSPlayerController* pPlayerController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));
    CCSPlayerPawnBase* pPlayerPawn = pPlayerController->m_hPlayerPawn();
    if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE)
        return;
    char buf[255] = { 0 };
    if (args.ArgC() != 2)
    {

		sprintf(buf, "-------------------------------------------------");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 请输入命令 \x06knife <id> \x01到控制台!");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 可使用的刀id范围为 \x06 500 - 525 \x01 详细内容请访问网站: \x06 skin.fu.link !");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, " \x0E [皮肤系统] \x01 如有其他问题请加QQ群询问: \x06 314498023!");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);

		sprintf(buf, "-------------------------------------------------");
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
        return;
    }

	

    CPlayer_WeaponServices* pWeaponServices = pPlayerPawn->m_pWeaponServices();
    // Get the weapon currently in the player's hand
	CBasePlayerWeapon* pCurrentWeapon = pWeaponServices->m_hActiveWeapon();

	// Check if the player is currently holding a knife
	// Check if the player is currently holding a knife
	if (pCurrentWeapon && strstr(pCurrentWeapon->GetClassname(), "weapon_knife") != nullptr)
	{
		// Remove the player's current knife
		// pWeaponServices->RemoveWeapon(pCurrentWeapon);

		// Delete the knife entity
		// FnEntityRemove(g_pGameEntitySystem, pCurrentWeapon, nullptr, -1);
	}

    // Give the player the knife
    if (strcmp(args.Arg(1), "500") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_bayonet", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "503") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_css", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "505") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_flip", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "506") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_gut", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "507") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_karambit", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "508") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_m9_bayonet", nullptr, nullptr, nullptr, nullptr);
    }
    	else if (strcmp(args.Arg(1), "509") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_tactical", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "512") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_falchion", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "514") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_survival_bowie", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "515") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_butterfly", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "516") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_push", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "517") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_cord", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "518") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_canis", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "519") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_ursus", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "520") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_gypsy_jackknife", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "521") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_outdoor", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "522") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_stiletto", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "523") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_widowmaker", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "525") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_skeleton", nullptr, nullptr, nullptr, nullptr);
    }
	else if (strcmp(args.Arg(1), "526") == 0)
    {
        FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife_kukri", nullptr, nullptr, nullptr, nullptr);
    }
    else
    {
        sprintf(buf, " \x0E [皮肤系统] \x04 %s 你输入的刀id无效!", pPlayerController->m_iszPlayerName());
        FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
        return;
    }

    sprintf(buf, " \x0E [皮肤系统] \x01 先切换至刀再输入命令! 发刀成功 %s !", args.Arg(1));
    FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
}

const char* Skin::GetLicense()
{
	return "GPL";
}

const char* Skin::GetVersion()
{
	return "1.0.0";
}

const char* Skin::GetDate()
{
	return __DATE__;
}

const char* Skin::GetLogTag()
{
	return "skin";
}

const char* Skin::GetAuthor()
{
	return "yuzhou and Fuzzys";
}

const char* Skin::GetDescription()
{
	return "Weapon and knife skin changer";
}

const char* Skin::GetName()
{
	return "Weapon skin";
}

const char* Skin::GetURL()
{
	return "http://skin.fu.link";
}
