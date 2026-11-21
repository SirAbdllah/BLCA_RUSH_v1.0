class BLCA_RushSquadSpawnPointClass : SCR_SpawnPointClass {}

class BLCA_RushSquadSpawnPoint : SCR_SpawnPoint
{
 [RplProp(onRplName: "OnSquadChanged")] protected int m_GroupId = -1;
 [RplProp(onRplName: "OnSquadChanged")] protected int m_AnchorId;
 [RplProp(onRplName: "OnPositionChanged")] protected vector m_SpawnPosition;

 static SCR_AIGroup PlayerGroup(int playerId)
 {
  BaseGameMode mode = GetGame().GetGameMode();
  if (!mode || playerId <= 0) return null;
  SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.Cast(mode.FindComponent(SCR_GroupsManagerComponent));
  if (!groups) return null;
  return groups.GetPlayerGroup(playerId);
 }

 override bool IsSpawnPointVisibleForPlayer(int pid)
 {
  if (!IsSpawnPointEnabled() || pid == m_AnchorId || m_AnchorId <= 0 || m_GroupId < 0) return false;
  SCR_AIGroup group = PlayerGroup(pid);
  return group && group.GetGroupID() == m_GroupId && group.GetPlayerIDs().Contains(m_AnchorId);
 }

 protected void OnSquadChanged()
 {
  Event_SpawnPointFactionAssigned.Invoke(this);
 }

 protected void OnPositionChanged()
 {
  SetOrigin(m_SpawnPosition);
  Update();
 }

 override protected void OnSetEnabled()
 {
  super.OnSetEnabled();
  OnSquadChanged();
 }

 void SetLocation_S(SCR_AIGroup group, int anchorId, vector position)
 {
  Faction faction = group.GetFaction();
  if (faction && GetFactionKey() != faction.GetFactionKey()) SetFaction(faction);
  bool changed = m_GroupId != group.GetGroupID() || m_AnchorId != anchorId;
  m_GroupId = group.GetGroupID();
  m_AnchorId = anchorId;
  if (m_SpawnPosition != position)
  {
   m_SpawnPosition = position;
   OnPositionChanged();
   changed = true;
  }
  if (changed) Replication.BumpMe();
  SetSpawnPointEnabled_S(true);
  if (changed) OnSquadChanged();
 }

 int GetAnchorId()
 {
  return m_AnchorId;
 }

 bool CanUse_S(BLCA_RushGameModeComponent mode, int playerId)
 {
  if (!mode || !IsSpawnPointVisibleForPlayer(playerId)) return false;
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  if (!sector || !mode.IsActiveSector(sector.Index)) return false;
  int team = mode.TeamOfPlayer(playerId);
  SCR_AIGroup group = PlayerGroup(playerId);
  if (team < 0 || !group || PlayerGroup(m_AnchorId) != group || IsSquadEngaged(group)) return false;
  if (!IsEligibleAnchor(mode, sector, m_AnchorId, team)) return false;
  IEntity anchor = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_AnchorId);
  return vector.DistanceSq(anchor.GetOrigin(), GetOrigin()) <= 22 * 22 && mode.IsSpawnSafe(GetOrigin(), team, 50);
 }

 static bool IsSquadEngaged(SCR_AIGroup group)
 {
  PlayerManager players = GetGame().GetPlayerManager();
  float now = BLCA_RushGameModeComponent.Now();
  foreach (int playerId : group.GetPlayerIDs())
  {
   IEntity body = players.GetPlayerControlledEntity(playerId);
   SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.BLCA_RushCombatManager(body);
   if (damage && damage.BLCA_RushCombatUntil() > now) return true;
  }
  return false;
 }

 static bool IsEligibleAnchor(BLCA_RushGameModeComponent mode, BLCA_RushSectorComponent sector, int anchorId, int team)
 {
  if (anchorId <= 0 || mode.TeamOfPlayer(anchorId) != team) return false;
  ChimeraCharacter anchor = ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(anchorId));
  if (!BLCA_RushGameModeComponent.CanInteract(anchor) || anchor.IsInVehicle()) return false;
  vector position = anchor.GetOrigin();
  if (!sector.Contains(position, 20)) return false;
  return mode.IsSpawnSafe(position, team, 50);
 }

 override bool PrepareSpawnedEntity_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  int playerId = requestComponent.GetPlayerId();
  if (!entity || !CanUse_S(mode, playerId)) return false;
  vector position;
  if (!SCR_WorldTools.FindEmptyTerrainPosition(position, GetOrigin(), 3)) return false;
  if (!mode.IsSpawnSafe(position, mode.TeamOfPlayer(playerId), 50)) return false;
  entity.SetOrigin(position);
  return true;
 }

 override bool CanFinalizeSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
 {
  // Availability was checked while preparing the body. The anchor may move
  // while the prepared character is being handed over to the client.
  return entity != null;
 }
}

class BLCA_RushSquadSpawns
{
 static const ResourceName PREFAB = "{97550BAC861E518F}Prefabs/BLCA_Rush/BLCA_SquadSpawn.et";
 protected ref map<int, BLCA_RushSquadSpawnPoint> m_Points = new map<int, BLCA_RushSquadSpawnPoint>();
 protected float m_NextUpdate;

 void Tick(BLCA_RushGameModeComponent mode, float now)
 {
  if (now < m_NextUpdate) return;
  m_NextUpdate = now + 1000;
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  if (!sector || !mode.IsActiveSector(sector.Index))
  {
   foreach (BLCA_RushSquadSpawnPoint point : m_Points)
   {
    if (point) point.SetSpawnPointEnabled_S(false);
   }
   return;
  }

  set<int> active = new set<int>();
  set<int> visited = new set<int>();
  array<int> players = {};
  GetGame().GetPlayerManager().GetPlayers(players);
  foreach (int playerId : players)
  {
   SCR_AIGroup group = BLCA_RushSquadSpawnPoint.PlayerGroup(playerId);
   if (!group || group.GetPlayerCount() < 2) continue;
   int team = mode.TeamOfPlayer(playerId);
   if (team < 0) continue;
   int id = group.GetGroupID();
   if (visited.Contains(id)) continue;
   visited.Insert(id);
   if (BLCA_RushSquadSpawnPoint.IsSquadEngaged(group)) continue;
   BLCA_RushSquadSpawnPoint point;
   m_Points.Find(id, point);

   int anchorId;
   vector position;
   if (!FindSquadLocation(mode, sector, group, team, point, anchorId, position)) continue;
   if (!point) point = CreatePoint(group, position);
   if (!point) continue;
   point.SetLocation_S(group, anchorId, position);
   active.Insert(id);
  }

  foreach (int id, BLCA_RushSquadSpawnPoint point : m_Points)
  {
   if (point && !active.Contains(id)) point.SetSpawnPointEnabled_S(false);
  }
 }

 protected bool FindSquadLocation(BLCA_RushGameModeComponent mode, BLCA_RushSectorComponent sector, SCR_AIGroup group, int team, BLCA_RushSquadSpawnPoint point, out int anchorId, out vector position)
 {
  anchorId = group.GetLeaderID();
  if (FindAnchorLocation(mode, sector, team, anchorId, point, position)) return true;
  foreach (int memberId : group.GetPlayerIDs())
  {
   if (memberId == group.GetLeaderID()) continue;
   if (!FindAnchorLocation(mode, sector, team, memberId, point, position)) continue;
   anchorId = memberId;
   return true;
  }
  return false;
 }

 protected bool FindAnchorLocation(BLCA_RushGameModeComponent mode, BLCA_RushSectorComponent sector, int team, int anchorId, BLCA_RushSquadSpawnPoint point, out vector position)
 {
  if (!BLCA_RushSquadSpawnPoint.IsEligibleAnchor(mode, sector, anchorId, team)) return false;
  IEntity anchor = GetGame().GetPlayerManager().GetPlayerControlledEntity(anchorId);
  vector origin = anchor.GetOrigin();
  if (point && point.IsSpawnPointEnabled() && point.GetAnchorId() == anchorId && vector.DistanceSq(origin, point.GetOrigin()) < 64 && mode.IsSpawnSafe(point.GetOrigin(), team, 50))
  {
   position = point.GetOrigin();
   return true;
  }
  return SCR_WorldTools.FindEmptyTerrainPosition(position, origin, 8) && mode.IsSpawnSafe(position, team, 50);
 }

 protected BLCA_RushSquadSpawnPoint CreatePoint(SCR_AIGroup group, vector position)
 {
  Resource resource = Resource.Load(PREFAB);
  if (!resource || !resource.IsValid()) return null;
  EntitySpawnParams params = new EntitySpawnParams();
  params.TransformMode = ETransformMode.WORLD;
  params.Transform[3] = position;
  BLCA_RushSquadSpawnPoint point = BLCA_RushSquadSpawnPoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
  if (!point) return null;
  point.SetSpawnPointName("SQUAD");
  m_Points.Set(group.GetGroupID(), point);
  return point;
 }

 void Clear()
 {
  foreach (BLCA_RushSquadSpawnPoint point : m_Points)
  {
   if (point) SCR_EntityHelper.DeleteEntityAndChildren(point);
  }
  m_Points.Clear();
 }
}

// The stock selector only adds options on a faction event and does not remove
// a same-faction option that became hidden.
modded class SCR_SpawnPointRequestUIComponent
{
 override protected void OnSpawnPointFactionChange(SCR_SpawnPoint spawnPoint)
 {
  if (!BLCA_RushSquadSpawnPoint.Cast(spawnPoint) && !spawnPoint.BLCA_IsSectorPoint())
  {
   super.OnSpawnPointFactionChange(spawnPoint);
   return;
  }
  if (!m_SpawnPointSelector) return;

  Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
  bool visible = faction && faction.GetFactionKey() == spawnPoint.GetFactionKey() && spawnPoint.IsSpawnPointVisibleForPlayer(SCR_PlayerController.GetLocalPlayerId());
  bool listed = m_SpawnPointSelector.GetItemId(spawnPoint.GetRplId()) >= 0;
  if (visible && !listed)
  {
   AddSpawnPoint(spawnPoint);
   if (!m_SelectedSpawnPointId.IsValid()) SetSpawnPoint(GetCurrentRplId());
  }
  else if (!visible && listed)
  {
   RemoveSpawnPoint(spawnPoint);
   if (m_SelectedSpawnPointId == spawnPoint.GetRplId()) SetSpawnPoint(GetCurrentRplId());
  }
 }
}
