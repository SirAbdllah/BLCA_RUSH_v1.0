class BLCA_RushCombatBoundary
{
 static bool RequiresReturn(BLCA_RushGameModeComponent mode, BLCA_RushSectorComponent sector, IEntity body)
 {
  return IsEnemySpawn(mode, sector, body) || !sector.Contains(body.GetOrigin());
 }

 static bool IsEnemySpawn(BLCA_RushGameModeComponent mode, BLCA_RushSectorComponent sector, IEntity body)
 {
  int team = mode.TeamOfEntity(body);
  return team >= 0 && mode.SpawnProtectionEnabled() && sector.ContainsSpawn(body.GetOrigin(), 1 - team);
 }

 static bool IsProtectedSpawn(vector position)
 {
  BLCA_RushSectorComponent sector = ProtectedSector(BLCA_RushGameModeComponent.Current());
  return sector && ContainsSpawn(sector, position);
 }

 protected static BLCA_RushSectorComponent ProtectedSector(BLCA_RushGameModeComponent mode)
 {
  if (!mode || mode.Phase() >= BLCA_RushPhase.SECTOR_TRANSITION || !mode.SpawnProtectionEnabled()) return null;
  return mode.ActiveSector();
 }

 protected static bool ContainsSpawn(BLCA_RushSectorComponent sector, vector position)
 {
  return sector.ContainsSpawn(position, 0) || sector.ContainsSpawn(position, 1);
 }

 static bool BlocksDamage(IEntity victim, notnull BaseDamageContext context)
 {
  if (!victim || context.damageValue <= 0 || context.damageType == EDamageType.TRUE || SCR_DamageManagerComponent.HEALING_DAMAGE_TYPES.Contains(context.damageType)) return false;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  BLCA_RushSectorComponent sector = ProtectedSector(mode);
  if (!sector) return false;
  int team = mode.TeamOfEntity(victim);
  vector position = victim.GetOrigin();
  if (team >= 0 && sector.ContainsSpawn(position, team)) return true;
  if (Vehicle.Cast(victim) && ContainsSpawn(sector, position)) return true;
  if (!context.instigator) return false;
  int playerId = context.instigator.GetInstigatorPlayerID();
  if (playerId <= 0) return false;
  IEntity attacker = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
  return attacker && ContainsSpawn(sector, attacker.GetOrigin());
 }

 static float Remaining(BLCA_RushGameModeComponent mode, IEntity body)
 {
  if (!mode || !BLCA_RushGameModeComponent.CanInteract(body) || mode.Phase() != BLCA_RushPhase.ROUND_ACTIVE) return -1;
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  if (!sector || !RequiresReturn(mode, sector, body)) return -1;

  SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
  if (!controller) return -1;
  return controller.BLCA_RetreatSeconds(mode.SectorIndex(), mode.BLCA_RetreatGrace());
 }
}

modded class SCR_PlayerController
{
 protected int m_BLCA_RetreatSector = -1;
 protected int m_BLCA_RetreatDuration;
 protected WorldTimestamp m_BLCA_RetreatIssued;
 protected IEntity m_BLCA_RetreatBody;

 void BLCA_SendRetreatDeadline(int sector, int duration)
 {
  if (!Replication.IsServer()) return;
  ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
  if (!world) return;
  WorldTimestamp issued = world.GetServerTimestamp();
  if (GetGame().GetPlayerController() == this) BLCA_RpcRetreatDeadline(sector, duration, issued);
  else Rpc(BLCA_RpcRetreatDeadline, sector, duration, issued);
 }

 [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
 protected void BLCA_RpcRetreatDeadline(int sector, int duration, WorldTimestamp issued)
 {
  if (GetGame().GetPlayerController() != this) return;
  m_BLCA_RetreatSector = sector;
  m_BLCA_RetreatDuration = duration;
  m_BLCA_RetreatIssued = issued;
  m_BLCA_RetreatBody = GetControlledEntity();
 }

 float BLCA_RetreatSeconds(int sector, int fallback)
 {
  // The warning appears on crossing while the authoritative deadline is in transit.
  if (m_BLCA_RetreatSector != sector || m_BLCA_RetreatBody != GetControlledEntity() || m_BLCA_RetreatDuration <= 0) return fallback;
  ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
  if (!world) return fallback;
  int age = Math.Max(0, world.GetServerTimestamp().DiffMilliseconds(m_BLCA_RetreatIssued));
  return Math.Max(0, m_BLCA_RetreatDuration - age) * 0.001;
 }
}

modded class SCR_BaseGameMode
{
 override protected void HandleOnGameModeEndSaveData()
 {
  if (!FindComponent(BLCA_RushGameModeComponent))
  {
   super.HandleOnGameModeEndSaveData();
   return;
  }

  if (!IsMaster()) return;
  SaveGameManager saves = GetGame().GetSaveGameManager();
  if (!saves) return;
  saves.SetSavingAllowed(false);

  PersistenceSystem persistence = PersistenceSystem.GetInstance();
  if (System.IsCLIParam("keepSessionSave") || (persistence && persistence.ShouldKeepSessionData())) return;
  #ifndef WORKBENCH
  if (RplSession.Mode() != RplMode.Dedicated) return;
  #endif

  // ClearStorage crashes on dedicated RUSH servers in 1.9. Purge the completed playthrough through the save manager.
  SaveGame activeSave = saves.GetActiveSave();
  ResourceName mission = SaveGameManager.GetCurrentMissionResource();
  int playthrough = saves.GetCurrentPlaythroughNumber();
  if (activeSave)
  {
   mission = activeSave.GetMissionResource();
   playthrough = activeSave.GetPlaythroughNumber();
  }
  if (!mission.IsEmpty() && playthrough >= 0) saves.Purge(mission, playthrough);
  Print("[BLCA_RUSH][SAVE] POSTGAME save cleanup finished.");
 }

 override float GetAutoReloadDelay()
 {
  if (FindComponent(BLCA_RushGameModeComponent)) return 90;
  return super.GetAutoReloadDelay();
 }
}
