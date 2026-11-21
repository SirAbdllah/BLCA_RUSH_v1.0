modded class SCR_BaseGameMode
{
 override bool IsFactionChangeAllowed()
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Cast(FindComponent(BLCA_RushGameModeComponent));
  if (!mode) return super.IsFactionChangeAllowed();
  PlayerController player = GetGame().GetPlayerController();
  return !mode.IsFactionLockActive() || !player || mode.TeamOfPlayer(player.GetPlayerId()) < 0;
 }
}

modded class SCR_PlayerFactionAffiliationComponent
{
 override protected bool CanRequestFaction_S(Faction faction)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (mode && !mode.CanChooseTeam(GetPlayerId(), faction)) return false;
  return super.CanRequestFaction_S(faction);
 }

 override bool SetFaction_S(Faction faction)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (mode && !mode.CanChooseTeam(GetPlayerId(), faction)) return false;
  bool assigned = super.SetFaction_S(faction);
  if (assigned && mode && faction)
  {
   mode.RecordFactionAssignment_S(GetPlayerId(), faction);
   GetGame().GetCallqueue().CallLater(mode.AssignSquad, 250, false, GetPlayerId());
  }
  return assigned;
 }
}

modded class SCR_FactionManager
{
 override protected void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
 {
  if (!BLCA_RushGameModeComponent.Current())
  {
   super.OnPlayerDisconnected(playerId, cause, timeout);
   return;
  }
  if (!Replication.IsServer()) return;
  // The authority already knows the player left. Clear the live faction roster
  // without asking a departing client to pass the in-round faction lock.
  PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
  if (!controller) return;
  SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(controller.FindComponent(SCR_PlayerFactionAffiliationComponent));
  if (!affiliation || !affiliation.GetAffiliatedFaction()) return;
  affiliation.SetAffiliatedFaction(null);
  UpdatePlayerFaction_S(affiliation);
 }
}

modded class SCR_PlayerControllerGroupComponent
{
 void BLCA_RepairRushGroupId_S(SCR_AIGroup group)
 {
  if (!Replication.IsServer() || !BLCA_RushGameModeComponent.Current() || !group) return;
  int playerId = GetPlayerID();
  int groupId = group.GetGroupID();
  if (m_iGroupID == groupId || !group.IsPlayerInGroup(playerId)) return;
  if (group.GetFaction() != SCR_FactionManager.SGetPlayerFaction(playerId)) return;
  m_iPreviousGroupID = m_iGroupID;
  m_iGroupID = groupId;
  m_iUISelectedGroupID = groupId;
  Rpc(RPC_DoChangeGroupID, groupId);
 }
}

modded class SCR_GroupsManagerComponent
{
 override SCR_AIGroup CreateNewPlayableGroup(Faction faction, SCR_EGroupRole groupRole = SCR_EGroupRole.NONE)
 {
  SCR_AIGroup group = super.CreateNewPlayableGroup(faction, groupRole);
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode || !group || !Replication.IsServer()) return group;
  group.SetMaxMembers(mode.SquadCapacity());
  array<string> names = {"ALPHA", "BRAVO", "CHARLIE", "DELTA", "ECHO", "FOXTROT"};
  array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(faction);
  if (!groups || groups.IsEmpty()) return group;
  int index = groups.Count() - 1;
  string name = "SQUAD " + (index + 1);
  if (index >= 0 && index < names.Count()) name = names[index];
  group.SetCustomName(name, 0);
  return group;
 }
}

class BLCA_RushChatCommands
{
 protected ChatCommandInvoker m_Command;

 void Register()
 {
  SCR_ChatPanelManager chat = SCR_ChatPanelManager.GetInstance();
  if (!chat || m_Command) return;
  m_Command = chat.GetCommandInvoker("rush");
  if (m_Command) m_Command.Insert(OnCommand);
 }

 void Close()
 {
  if (m_Command) m_Command.Remove(OnCommand);
  m_Command = null;
 }

 protected void OnCommand(SCR_ChatPanel panel, string otherData)
 {
  SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
  if (!controller) return;
  array<string> args = {};
  otherData.Split(" ", args, true);
  string subcommand = "status";
  int value;
  if (args.Count() > 0) subcommand = args[0];
  if (args.Count() > 1) value = args[1].ToInt();
  controller.BLCA_RushAdminCommand(subcommand, value);
 }
}

modded class SCR_PlayerController
{
 void BLCA_RushAdminCommand(string command, int value)
 {
  if (Replication.IsServer()) BLCA_RushRpcAdmin(command, value);
  else Rpc(BLCA_RushRpcAdmin, command, value);
 }

 [RplRpc(RplChannel.Reliable, RplRcver.Server)]
 protected void BLCA_RushRpcAdmin(string command, int value)
 {
  if (!Replication.IsServer() || GetPlayerId() <= 0 || GetGame().GetPlayerManager().GetPlayerController(GetPlayerId()) != this) return;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode) return;
  string response = mode.Admin(GetPlayerId(), command, value);
  if (GetGame().GetPlayerController() == this) BLCA_RushRpcAdminResponse(response);
  else Rpc(BLCA_RushRpcAdminResponse, response);
 }

 [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
 protected void BLCA_RushRpcAdminResponse(string response)
 {
  SCR_ChatPanelManager chat = SCR_ChatPanelManager.GetInstance();
  if (chat) chat.ShowHelpMessage(response);
 }
}
