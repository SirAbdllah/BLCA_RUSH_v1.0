modded class SCR_PlayerListMenu
{
 protected SCR_BaseGameMode m_BLCA_ListMode;

 override void OnMenuOpen()
 {
  super.OnMenuOpen();
  BLCA_RushGameModeComponent rush = BLCA_RushGameModeComponent.Current();
  if (!rush) return;
  m_BLCA_ListMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
  if (!m_BLCA_ListMode) return;
  m_BLCA_ListMode.GetOnPlayerDisconnected().Insert(BLCA_RemoveDisconnectedPlayer);
  DeleteDisconnectedEntries();
 }

 override void OnMenuClose()
 {
  if (m_BLCA_ListMode)
  {
   m_BLCA_ListMode.GetOnPlayerDisconnected().Remove(BLCA_RemoveDisconnectedPlayer);
   m_BLCA_ListMode.GetOnPlayerRegistered().Remove(OnPlayerConnected);
   GetGame().GetCallqueue().Remove(UpdatePlayerList);
   m_BLCA_ListMode = null;
  }
  super.OnMenuClose();
 }

 override void UpdatePlayerList(bool addPlayer, int id)
 {
  if (addPlayer && BLCA_RushGameModeComponent.Current())
  {
   if (!m_BLCA_ListMode || !m_wTable) return;
   array<int> connected = {};
   GetGame().GetPlayerManager().GetPlayers(connected);
   if (!connected.Contains(id)) return;
  }
  super.UpdatePlayerList(addPlayer, id);
 }

 override void DeleteDisconnectedEntries()
 {
  if (!m_BLCA_ListMode)
  {
   super.DeleteDisconnectedEntries();
   return;
  }
  array<int> connected = {};
  GetGame().GetPlayerManager().GetPlayers(connected);
  for (int i = m_aEntries.Count() - 1; i >= 0; i--)
  {
   SCR_PlayerListEntry entry = m_aEntries[i];
   if (entry && !connected.Contains(entry.m_iID)) BLCA_RemovePlayerRow(entry.m_iID);
  }
 }

 protected void BLCA_RemoveDisconnectedPlayer(int playerId, KickCauseCode cause, int timeout)
 {
  BLCA_RemovePlayerRow(playerId);
 }

 protected void BLCA_RemovePlayerRow(int playerId)
 {
  bool wasSelected = m_SelectedEntry && m_SelectedEntry.m_iID == playerId;
  m_aAllPlayersInfo.Remove(playerId);
  super.UpdatePlayerList(false, playerId);
  if (wasSelected)
  {
   if (m_SelectedEntry && m_SelectedEntry.m_iID == playerId) m_SelectedEntry = null;
   FocusFirstItem();
  }
 }
}

modded class SCR_GroupPlayerList
{
 override void HandlerAttached(Widget w)
 {
  super.HandlerAttached(w);
  SCR_AIGroup.GetOnPlayerRemoved().Insert(BLCA_GroupPlayerRemoved);
 }

 override void HandlerDeattached(Widget w)
 {
  SCR_AIGroup.GetOnPlayerRemoved().Remove(BLCA_GroupPlayerRemoved);
  super.HandlerDeattached(w);
 }

 protected void BLCA_GroupPlayerRemoved(SCR_AIGroup group, int playerId)
 {
  if (group == m_Group && BLCA_RushGameModeComponent.Current()) UpdatePlayerList();
 }
}
