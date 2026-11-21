modded class SCR_BaseGameMode
{
 override bool CanPlayerSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, out SCR_ESpawnResult result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED)
 {
  if (!super.CanPlayerSpawn_S(requestComponent, handlerComponent, data, result)) return false;
  BLCA_RushGameModeComponent rush = BLCA_RushGameModeComponent.Cast(FindComponent(BLCA_RushGameModeComponent));
  if (!rush) return true;
  if (requestComponent && rush.CanDeploy(requestComponent.GetPlayerId(), data)) return true;
  result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
  return false;
 }
}

modded class SCR_SpawnPointSpawnHandlerComponent
{
 protected override SCR_ESpawnResult SpawnEntity_S(SCR_SpawnRequestComponent requestComponent, notnull SCR_SpawnData data, out IEntity spawnedEntity)
 {
  BLCA_RushGameModeComponent rush = BLCA_RushGameModeComponent.Current();
  if (!rush) return super.SpawnEntity_S(requestComponent, data, spawnedEntity);

  int playerId = requestComponent.GetPlayerId();
  // The engine has already run CanHandleRequest_S. Reserve once, before spawning.
  if (!rush.Reserve(playerId, data)) return SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
  SCR_ESpawnResult result = super.SpawnEntity_S(requestComponent, data, spawnedEntity);
  if (result != SCR_ESpawnResult.OK || !spawnedEntity) rush.Release(playerId, false);
  return result;
 }

 override SCR_ESpawnResult FinalizeRequest_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
 {
  SCR_ESpawnResult result = super.FinalizeRequest_S(requestComponent, data, entity);
  BLCA_RushGameModeComponent rush = BLCA_RushGameModeComponent.Current();
  if (rush && result != SCR_ESpawnResult.OK)
  {
   int playerId = requestComponent.GetPlayerId();
   rush.Release(playerId, false);
   SCR_SpawnPointSpawnData pointData = SCR_SpawnPointSpawnData.Cast(data);
   if (pointData && pointData.GetSpawnPoint()) pointData.GetSpawnPoint().ClearReservationFor_S(playerId);
  }
  return result;
 }
}

modded class SCR_RespawnTimerComponent
{
 override void SetRespawnTime(int playerID, float respawnTime)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (mode && respawnTime > 0) respawnTime = mode.RespawnDelay(playerID);
  super.SetRespawnTime(playerID, respawnTime);
 }
}

modded class SCR_SpawnPoint
{
 bool BLCA_IsSectorPoint()
 {
  if (!BLCA_RushGameModeComponent.Current()) return false;
  string name = GetName();
  return name && name.StartsWith("BLCA_") && (name.EndsWith("_Attack") || name.EndsWith("_Defend") || name.EndsWith("_DefendA") || name.EndsWith("_DefendB"));
 }

 override bool IsSpawnPointVisibleForPlayer(int pid)
 {
  if (BLCA_IsSectorPoint() && !IsSpawnPointEnabled()) return false;
  return super.IsSpawnPointVisibleForPlayer(pid);
 }

 override protected void OnSetEnabled()
 {
  super.OnSetEnabled();
  if (BLCA_IsSectorPoint()) Event_SpawnPointFactionAssigned.Invoke(this);
 }

 override bool PrepareSpawnedEntity_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  string name = GetName();
  if (!mode || !name || !name.StartsWith("BLCA_") || (!name.EndsWith("_DefendA") && !name.EndsWith("_DefendB")))
   return super.PrepareSpawnedEntity_S(requestComponent, data, entity);

  if (!entity || !requestComponent) return false;
  vector position;
  if (!mode.FindObjectiveSpawnPosition(requestComponent.GetPlayerId(), this, position)) return false;
  entity.SetOrigin(position);
  entity.SetAngles(GetAngles());
  return true;
 }

 override bool CanFinalizeSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
 {
  // A sector may close after the character is prepared but before the engine
  // transfers control. Finish that request so the player's spawn lock clears.
  if (BLCA_IsSectorPoint()) return entity != null;
  return super.CanFinalizeSpawn_S(requestComponent, data, entity);
 }
}
