[ComponentEditorProps(category: "BLCA Rush", description: "Authoritative Rush round, objectives, reinforcement tickets and deployment.")]
class BLCA_RushGameModeComponentClass : SCR_BaseGameModeComponentClass {}

class BLCA_RushGameModeComponent : SCR_BaseGameModeComponent
{
 [RplProp()] protected BLCA_RushPhase m_Phase;
 [RplProp()] protected int m_SectorIndex;
 [RplProp()] protected int m_SectorCount;
 [RplProp()] protected int m_AttackerTickets;
 [RplProp()] protected int m_Seconds;
 [RplProp()] protected bool m_FinaleMusic;
 [RplProp()] protected float m_PlantDuration = 4;
 [RplProp()] protected float m_DefuseDuration = 5;
 [RplProp()] protected int m_ASeconds;
 [RplProp()] protected int m_BSeconds;
 [RplProp()] protected BLCA_ObjectiveState m_StateA;
 [RplProp()] protected BLCA_ObjectiveState m_StateB;
 [RplProp()] protected int m_Winner = -1;
 [RplProp()] protected string m_Event;
 [RplProp()] protected int m_EventSerial;
 [RplProp()] protected string m_Summary;

 protected ref BLCA_RushSettings m_Settings;
 protected ref array<BLCA_RushSectorComponent> m_Sectors = {};
 protected ref array<ref BLCA_RushObjectiveRound> m_Objectives = {};
 protected ref map<string, ref BLCA_RushLife> m_Lives = new map<string, ref BLCA_RushLife>();
 protected ref map<int, string> m_PlayerKeys = new map<int, string>();
 protected int m_PlayerKeySequence;
 protected int m_ReservedAttackers;
 protected bool m_AttackerTicketsExhaustedBeforeDeadline;
 protected float m_PhaseUntil;
 protected float m_RoundUntil;
 protected float m_NextSectorTimeMs;
 protected ref BLCA_RushMapMarkers m_MapMarkers = new BLCA_RushMapMarkers();
 protected ref BLCA_RushSquadSpawns m_SquadSpawns = new BLCA_RushSquadSpawns();
 protected bool m_Started;
 protected bool m_Ready;
 protected bool m_NativeEnded;

 static BLCA_RushGameModeComponent Current()
 {
  if (!GetGame()) return null;
  BaseGameMode mode = GetGame().GetGameMode();
  if (!mode) return null;
  return BLCA_RushGameModeComponent.Cast(mode.FindComponent(BLCA_RushGameModeComponent));
 }

 bool SpawnProtectionEnabled() { return m_Settings && m_Settings.EnforceSpawnSafety; }

 static float Now() { return GetGame().GetWorld().GetWorldTime(); }
 BLCA_RushPhase Phase() { return m_Phase; }
 int SectorIndex() { return m_SectorIndex; }
 int SectorCount() { return m_SectorCount; }
 int AttackerTickets() { return m_AttackerTickets; }

 float HoldSeconds(bool defuse)
 {
  if (defuse) return m_DefuseDuration;
  return m_PlantDuration;
 }
 int PhaseSeconds() { return m_Seconds; }
 bool FinaleMusicStarted() { return m_FinaleMusic; }
 int ObjectiveSeconds(int letter)
 {
  if (letter == 0) return m_ASeconds;
  return m_BSeconds;
 }
 int Winner() { return m_Winner; }
 int EventSerial() { return m_EventSerial; }
 string EventText() { return m_Event; }
 string Summary() { return m_Summary; }

 BLCA_ObjectiveState ObjectiveState(int letter)
 {
  if (letter == 0) return m_StateA;
  return m_StateB;
 }

 bool IsOvertime()
 {
  return m_Phase == BLCA_RushPhase.ROUND_ACTIVE && m_Seconds == 0 && HasArmedObjective();
 }

 protected bool HasArmedObjective()
 {
  return BLCA_RushObjectiveRound.IsArmedState(m_StateA) || BLCA_RushObjectiveRound.IsArmedState(m_StateB);
 }

 BLCA_RushSectorComponent ActiveSector() { return BLCA_SectorAt(m_SectorIndex); }

 BLCA_RushSectorComponent BLCA_SectorAt(int index)
 {
  foreach (BLCA_RushSectorComponent sector : BLCA_RushSectorComponent.s_Sectors)
  {
   if (!sector || sector.Index != index) continue;
   if (!sector.A && !sector.Resolve()) return null;
   return sector;
  }
  return null;
 }

 bool IsActiveSector(int index) { return m_Phase == BLCA_RushPhase.ROUND_ACTIVE && index == m_SectorIndex; }
 bool IsFactionLockActive()
 {
  return m_Phase < BLCA_RushPhase.MATCH_FINISHED && GetGameMode().GetState() != SCR_EGameModeState.POSTGAME;
 }

 static bool CanInteract(IEntity body)
 {
  ChimeraCharacter character = ChimeraCharacter.Cast(body);
  if (!character) return false;
  CharacterControllerComponent controller = character.GetCharacterController();
  return controller && !controller.IsDead() && !controller.IsUnconscious();
 }

 int TeamOfEntity(IEntity entity)
 {
  if (!entity) return -1;
  FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
  if (!affiliation) return -1;
  return TeamOfFaction(affiliation.GetAffiliatedFaction());
 }

 int TeamOfPlayer(int playerId)
 {
  PlayerController player = GetGame().GetPlayerManager().GetPlayerController(playerId);
  if (!player) return -1;
  SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(player.FindComponent(SCR_PlayerFactionAffiliationComponent));
  if (!affiliation) return -1;
  return TeamOfFaction(affiliation.GetAffiliatedFaction());
 }

 protected int TeamOfFaction(Faction faction)
 {
  if (!faction) return -1;
  string key = faction.GetFactionKey();
  if (key == "US") return 0;
  if (key == "USSR") return 1;
  return -1;
 }

 override void OnGameModeStart()
 {
  super.OnGameModeStart();
  if (GetGameMode().IsMaster()) GetGame().GetCallqueue().CallLater(Initialize, 300, false);
 }

 override void OnPlayerRegistered(int playerId)
 {
  super.OnPlayerRegistered(playerId);
  if (!GetGameMode().IsMaster()) return;
  Initialize();
  QueueConnectionRestore(playerId);
 }

 override void OnPlayerAuditSuccess(int playerId)
 {
  super.OnPlayerAuditSuccess(playerId);
  if (!GetGameMode().IsMaster()) return;
  QueueConnectionRestore(playerId);
 }

 protected void QueueConnectionRestore(int playerId)
 {
  PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
  if (controller) GetGame().GetCallqueue().Call(RestoreConnectedBody, playerId, controller);
 }

 protected void RestoreConnectedBody(int playerId, PlayerController controller)
 {
  PlayerManager players = GetGame().GetPlayerManager();
  if (!controller || !players.IsPlayerConnected(playerId) || controller != players.GetPlayerController(playerId)) return;
  int lockedTeam = LockedTeamOfPlayer_S(playerId);
  if (lockedTeam >= 0 && TeamOfPlayer(playerId) != lockedTeam)
  {
   string factionKey = "US";
   if (lockedTeam == 1) factionKey = "USSR";
   Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
   SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(controller.FindComponent(SCR_PlayerFactionAffiliationComponent));
   if (affiliation && faction) affiliation.SetFaction_S(faction);
  }
  IEntity body = controller.GetControlledEntity();
  int team = TeamOfPlayer(playerId);
  if (m_Winner >= 0 && m_Phase >= BLCA_RushPhase.ROUND_ENDING)
  {
   // A returning participant can reappear in results without spawning again.
   string key = Key(playerId);
   BLCA_RushLife resultLife;
   if (m_Lives.Find(key, resultLife) && resultLife.Deployed)
   {
    resultLife.ConnectedPlayer = playerId;
    resultLife.Name = players.GetPlayerName(playerId);
    RefreshSummary();
   }
   return;
  }
  if (team < 0 || !CanInteract(body)) return;
  BLCA_RushLife life = Life(playerId);
  if (!life.Deployed || life.Reserved || life.Body) return;
  life.Body = body;
  life.Team = team;
  SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.BLCA_RushCombatManager(body);
  if (damage) damage.BLCA_TrackRushShots();
  AssignSquad(playerId);
 }

 override void OnGameEnd()
 {
  GetGame().GetCallqueue().Remove(Initialize);
  GetGame().GetCallqueue().Remove(RestoreConnectedBody);
  GetGame().GetCallqueue().Remove(Tick);
  if (GetGameMode().IsMaster())
  {
   m_MapMarkers.Clear();
   m_SquadSpawns.Clear();
  }
  super.OnGameEnd();
 }

 protected void Initialize()
 {
  if (m_Started || !GetGameMode().IsMaster()) return;
  m_Started = true;
  if (!m_Settings) m_Settings = BLCA_RushSettings.Load();
  m_PlantDuration = m_Settings.PlantSeconds;
  m_RetreatGrace = m_Settings.RetreatGraceSeconds;
  m_DefuseDuration = m_Settings.DefuseSeconds;
  int count = BLCA_RushSectorComponent.s_Sectors.Count();
  for (int index = 0; index < count; index++)
  {
   BLCA_RushSectorComponent sector;
   foreach (BLCA_RushSectorComponent candidate : BLCA_RushSectorComponent.s_Sectors)
   {
    if (candidate.Index != index) continue;
    if (sector)
    {
     InvalidSetup("duplicate sector " + index);
     return;
    }
    sector = candidate;
   }
   if (!sector || !sector.Resolve() || sector.BoundaryRadius < 50)
   {
    InvalidSetup("invalid sector " + index);
    return;
   }
   m_Sectors.Insert(sector);
  }
  if (m_Sectors.IsEmpty())
  {
   InvalidSetup("no sectors");
   return;
  }
  m_Objectives.Insert(new BLCA_RushObjectiveRound());
  m_Objectives.Insert(new BLCA_RushObjectiveRound());
  m_SectorCount = m_Sectors.Count();
  ResetMatchState();
  m_Ready = true;
  SetSpawns(0, true);
  SetPhase(BLCA_RushPhase.WAITING_FOR_PLAYERS, 0, "Waiting for players");
  GetGame().GetCallqueue().CallLater(Tick, 250, true);
  Print("[BLCA_RUSH][GAME] Server authority ready with " + m_SectorCount + " sectors.");
 }

 protected void ResetMatchState()
 {
  // The postgame summary belongs to the previous world. Begin fresh on the new authority.
  m_SectorIndex = 0;
  m_Winner = -1;
  m_Summary = "";
  m_FinaleMusic = false;
  m_RoundUntil = 0;
  m_NextSectorTimeMs = 0;
  m_StateA = BLCA_ObjectiveState.INACTIVE;
  m_StateB = BLCA_ObjectiveState.INACTIVE;
  m_ASeconds = 0;
  m_BSeconds = 0;
  m_NativeEnded = false;
  m_AttackerTickets = m_Settings.AttackerTickets;
  m_ReservedAttackers = 0;
  m_AttackerTicketsExhaustedBeforeDeadline = false;
  m_PlayerKeySequence = 0;
  m_Lives.Clear();
  m_PlayerKeys.Clear();
  m_BoundaryDeadline.Clear();
  m_NextBoundary = 0;

  SaveGameManager saves = GetGame().GetSaveGameManager();
  if (saves) saves.SetSavingAllowed(false);
  Print("[BLCA_RUSH][MATCH] Fresh round data initialized.");
 }

 protected void InvalidSetup(string reason)
 {
  m_Phase = BLCA_RushPhase.MATCH_FINISHED;
  m_Event = "Rush setup error: " + reason;
  Replication.BumpMe();
  Print("[BLCA_RUSH][GAME] " + m_Event, LogLevel.ERROR);
 }

 protected void SetSpawns(int index, bool enable, bool objectiveSpawns = false)
 {
  foreach (BLCA_RushSectorComponent sector : m_Sectors)
  {
   bool active = enable && sector.Index == index;
   sector.Attack.SetSpawnPointEnabled_S(active);
   sector.Defend.SetSpawnPointEnabled_S(active);
   if (sector.DefendA) sector.DefendA.SetSpawnPointEnabled_S(active && objectiveSpawns);
   if (sector.DefendB) sector.DefendB.SetSpawnPointEnabled_S(active && objectiveSpawns);
  }
 }

 protected void SetPhase(BLCA_RushPhase phase, int seconds, string message)
 {
  m_Phase = phase;
  m_PhaseUntil = Now() + seconds * 1000;
  m_Seconds = seconds;
  Notify(message);
  Print("[BLCA_RUSH][MATCH] phase=" + phase + " sector=" + m_SectorIndex + " attackerTickets=" + m_AttackerTickets);
 }

 protected void Notify(string message)
 {
  m_Event = message;
  m_EventSerial++;
  Replication.BumpMe();
 }

 protected void Tick()
 {
  if (!GetGameMode().IsMaster() || m_NativeEnded || !m_Settings) return;
  CleanupCorpses();
  float now = Now();
  int remaining = Math.Max(0, Math.Ceil((m_PhaseUntil - now) * 0.001));
  if (m_Phase == BLCA_RushPhase.ROUND_ACTIVE) remaining = Math.Max(0, Math.Ceil((m_RoundUntil - now) / 1000.0));
  if (remaining != m_Seconds)
  {
   m_Seconds = remaining;
   Replication.BumpMe();
  }
  if (!m_FinaleMusic && (m_Phase == BLCA_RushPhase.ROUND_ACTIVE || m_Phase == BLCA_RushPhase.SECTOR_TRANSITION))
  {
   if (m_AttackerTickets <= 15 || (m_RoundUntil > 0 && m_RoundUntil - now <= 600000))
   {
    m_FinaleMusic = true;
    Replication.BumpMe();
   }
  }
  m_MapMarkers.Update(this);
  m_SquadSpawns.Tick(this, now);
  switch (m_Phase)
  {
   case BLCA_RushPhase.WAITING_FOR_PLAYERS:
    if (EnoughPlayers()) SetPhase(BLCA_RushPhase.WARMUP, m_Settings.WarmupSeconds, "Warm-up");
    break;
   case BLCA_RushPhase.WARMUP:
    if (!EnoughPlayers()) SetPhase(BLCA_RushPhase.WAITING_FOR_PLAYERS, 0, "Waiting for players");
    else if (remaining == 0) SetPhase(BLCA_RushPhase.ROUND_STARTING, 3, "Round starting");
    break;
   case BLCA_RushPhase.ROUND_STARTING:
    if (!EnoughPlayers()) SetPhase(BLCA_RushPhase.WAITING_FOR_PLAYERS, 0, "Waiting for players");
    else if (remaining == 0)
    {
     m_RoundUntil = now + m_Settings.RoundMinutes * 60000;
     OpenSector(0);
    }
    break;
   case BLCA_RushPhase.ROUND_ACTIVE:
    if (m_AttackerTicketsExhaustedBeforeDeadline || (now < m_RoundUntil && BLCA_RushTicketRules.IsExhausted(m_AttackerTickets, m_ReservedAttackers)))
    {
     FinishRound(1);
     break;
    }
    UpdateObjectives(now);
    EvaluateRound(now);
    UpdateBoundary();
    break;
   case BLCA_RushPhase.SECTOR_TRANSITION:
    if (remaining == 0) OpenSector(m_SectorIndex + 1);
    break;
   case BLCA_RushPhase.ROUND_ENDING:
    if (remaining == 0) SetPhase(BLCA_RushPhase.SCOREBOARD, m_Settings.ScoreboardSeconds, m_Event);
    break;
   case BLCA_RushPhase.SCOREBOARD:
    if (remaining == 0) SetPhase(BLCA_RushPhase.NEXT_ROUND, 1, "Finalizing round");
    break;
   case BLCA_RushPhase.NEXT_ROUND:
    if (remaining == 0) EndNative();
    break;
  }
 }

 protected bool EnoughPlayers()
 {
  PlayerManager manager = GetGame().GetPlayerManager();
  if (manager.GetPlayerCount() < m_Settings.MinimumPlayers) return false;
  if (m_Settings.MinimumPlayers == 1) return true;
  array<int> players = {};
  manager.GetPlayers(players);
  bool attackers;
  bool defenders;
  foreach (int player : players)
  {
   int team = TeamOfPlayer(player);
   if (team == 0) attackers = true;
   if (team == 1) defenders = true;
   if (attackers && defenders) return true;
  }
  return false;
 }

 protected void OpenSector(int index)
 {
  if (index < 0 || index >= m_Sectors.Count()) return;
  if (m_Phase != BLCA_RushPhase.ROUND_STARTING && m_Phase != BLCA_RushPhase.SECTOR_TRANSITION) return;
  float now = Now();
  if (index > 0)
  {
   if (m_NextSectorTimeMs > 0) m_RoundUntil = now + m_NextSectorTimeMs;
   m_NextSectorTimeMs = 0;
  }
  m_SectorIndex = index;
  m_BoundaryDeadline.Clear();
  m_NextBoundary = 0;
  foreach (BLCA_RushObjectiveRound objective : m_Objectives) objective.Reset(BLCA_ObjectiveState.ACTIVE);
  foreach (BLCA_RushSectorComponent visualSector : m_Sectors)
  {
   BLCA_ObjectiveState state = BLCA_ObjectiveState.INACTIVE;
   if (visualSector.Index == m_SectorIndex) state = BLCA_ObjectiveState.ACTIVE;
   if (visualSector.Index < m_SectorIndex) state = BLCA_ObjectiveState.DESTROYED;
   visualSector.A.SetVisualState_S(state);
   visualSector.B.SetVisualState_S(state);
  }
  SyncObjectives(now);
  SetSpawns(index, true, true);
  int seconds = Math.Max(0, Math.Ceil((m_RoundUntil - now) / 1000.0));
  SetPhase(BLCA_RushPhase.ROUND_ACTIVE, seconds, "Secure " + m_Sectors[index].Title);
  if (index > 0) UpdateBoundary(true);
 }

 protected static float NextSectorTimeMs(float remainingMs, bool finalSector)
 {
  if (!finalSector) return Math.Max(remainingMs, 1800000);
  float extendedMs = Math.Clamp(remainingMs + 600000, 900000, 1200000);
  return Math.Max(remainingMs, extendedMs);
 }

 void RequestObjective(int playerId, IEntity body, BLCA_RushObjectiveComponent terminal, bool defuse, int step)
 {
  if (!GetGameMode().IsMaster() || !terminal || !IsActiveSector(terminal.Sector)) return;
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  if (terminal != sector.A && terminal != sector.B) return;
  BLCA_RushObjectiveRound round = m_Objectives[terminal.Letter];
  if (step == 2)
  {
   if (round.Actor == playerId) { round.Cancel(); SyncObjectives(Now()); }
   return;
  }
  if (step < 0 || step > 1 || !CanInteract(body) || !terminal.InRange(body)) return;
  int team = TeamOfPlayer(playerId);
  if (TeamOfEntity(body) != team || (defuse && team != 1) || (!defuse && team != 0)) return;
  float now = Now();
  if (m_AttackerTicketsExhaustedBeforeDeadline || (!defuse && now >= m_RoundUntil)) return;
  if (defuse && round.ExplodesAt <= now) return;
  if (step == 0)
  {
   if (round.Actor != 0) return;
   if (defuse && round.State != BLCA_ObjectiveState.ARMED) return;
   if (!defuse && round.State != BLCA_ObjectiveState.ACTIVE && round.State != BLCA_ObjectiveState.DEFUSED) return;
   round.Actor = playerId;
   round.Body = body;
   round.StartTime = now;
   round.State = BLCA_ObjectiveState.ARMING;
   if (defuse) round.State = BLCA_ObjectiveState.DEFUSING;
   SyncObjectives(now);
   return;
  }
  BLCA_ObjectiveState expected = BLCA_ObjectiveState.ARMING;
  float duration = HoldSeconds(defuse);
  if (defuse)
  {
   expected = BLCA_ObjectiveState.DEFUSING;
   // The same configured duration is replicated to the native interaction.
  }
  if (round.Actor != playerId || round.Body != body || round.State != expected || now < round.StartTime + duration * 1000 - 175) return;
  BLCA_RushLife life = Life(playerId);
  string label = "M-COM " + BLCA_RushObjectiveComponent.Label(terminal.Letter);
  if (defuse)
  {
   round.Reset(BLCA_ObjectiveState.DEFUSED);
   life.Defuses++;
   life.Score += m_Settings.DefuseScore;
   Notify(label + " defused");
  }
  else
  {
   round.Reset(BLCA_ObjectiveState.ARMED);
   round.ExplodesAt = now + m_Settings.DetonationSeconds * 1000;
   round.PlanterKey = Key(playerId);
   life.Plants++;
   life.Score += m_Settings.PlantScore;
   Notify(label + " armed");
  }
  SyncObjectives(now);
  Print("[BLCA_RUSH][OBJECTIVE] " + m_Event + " sector=" + m_SectorIndex);
 }

 protected void UpdateObjectives(float now)
 {
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  for (int i = 0; i < 2; i++)
  {
   BLCA_RushObjectiveRound objective = m_Objectives[i];
   BLCA_RushObjectiveComponent terminal = sector.A;
   if (i == 1) terminal = sector.B;
   if (objective.HasCharge() && now >= objective.ExplodesAt)
   {
    BLCA_RushLife planter;
    if (m_Lives.Find(objective.PlanterKey, planter)) planter.Score += m_Settings.DestroyScore;
    objective.Reset(BLCA_ObjectiveState.DESTROYED);
    Notify("M-COM " + BLCA_RushObjectiveComponent.Label(i) + " destroyed");
    DetonateObjective(terminal);
    Print("[BLCA_RUSH][OBJECTIVE] " + m_Event + " sector=" + m_SectorIndex);
    continue;
   }
   if (!objective.Actor) continue;
   int expectedTeam = 0;
   if (objective.State == BLCA_ObjectiveState.DEFUSING) expectedTeam = 1;
   IEntity actor = GetGame().GetPlayerManager().GetPlayerControlledEntity(objective.Actor);
   if (actor != objective.Body || !CanInteract(actor) || !terminal.InRange(actor) || TeamOfPlayer(objective.Actor) != expectedTeam || now - objective.StartTime > (HoldSeconds(expectedTeam == 1) + 3.5) * 1000)
    objective.Cancel();
  }
  SyncObjectives(now);
 }

 protected void DetonateObjective(BLCA_RushObjectiveComponent terminal)
 {
  if (!Replication.IsServer() || !GetGameMode().IsMaster() || !terminal || !IsActiveSector(terminal.Sector)) return;
  IEntity source = terminal.GetOwner();
  if (!source) return;
  vector position = source.GetOrigin() + "0 0.15 0";
  Rpc(BLCA_RpcObjectiveDetonation, position);
  PlayObjectiveDetonation(position);

  // Gather bodies before applying damage. Native death callbacks can update
  // the round's player records, and the same record must not be hit twice.
  array<IEntity> victims = {};
  PlayerManager players = GetGame().GetPlayerManager();
  foreach (BLCA_RushLife life : m_Lives)
  {
   if (!life.Deployed || life.Team < 0 || life.ConnectedPlayer <= 0 || !life.Body) continue;
   if (players.GetPlayerControlledEntity(life.ConnectedPlayer) != life.Body) continue;
   if (vector.DistanceSq(position, life.Body.GetOrigin()) > BLCA_RushObjectiveBlast.OUTER_RADIUS_SQ) continue;
   if (!victims.Contains(life.Body)) victims.Insert(life.Body);
  }
  foreach (IEntity victim : victims) BLCA_RushObjectiveBlast.Damage(source, position, victim);
 }

 [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
 protected void BLCA_RpcObjectiveDetonation(vector position)
 {
  PlayObjectiveDetonation(position);
 }

 protected void PlayObjectiveDetonation(vector position)
 {
  BLCA_RushObjectiveBlast.Play(position);
 }

 protected void SyncObjectives(float now)
 {
  if (m_SectorIndex >= 0 && m_SectorIndex < m_Sectors.Count())
  {
   BLCA_RushSectorComponent visualSector = m_Sectors[m_SectorIndex];
   if (visualSector.A) visualSector.A.SetVisualState_S(m_Objectives[0].State);
   if (visualSector.B) visualSector.B.SetVisualState_S(m_Objectives[1].State);
  }
  BLCA_RushObjectiveRound a = m_Objectives[0];
  BLCA_RushObjectiveRound b = m_Objectives[1];
  int secondsA = Math.Max(0, Math.Ceil((a.ExplodesAt - now) * 0.001));
  int secondsB = Math.Max(0, Math.Ceil((b.ExplodesAt - now) * 0.001));
  if (a.State == m_StateA && b.State == m_StateB && secondsA == m_ASeconds && secondsB == m_BSeconds) return;
  if (m_Phase == BLCA_RushPhase.ROUND_ACTIVE && (a.State != m_StateA || b.State != m_StateB))
  {
   BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
   if (sector.DefendA) sector.DefendA.SetSpawnPointEnabled_S(a.State != BLCA_ObjectiveState.DESTROYED);
   if (sector.DefendB) sector.DefendB.SetSpawnPointEnabled_S(b.State != BLCA_ObjectiveState.DESTROYED);
  }
  m_StateA = a.State;
  m_StateB = b.State;
  m_ASeconds = secondsA;
  m_BSeconds = secondsB;
  Replication.BumpMe();
 }

 protected void EvaluateRound(float now)
 {
  if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE) return;
  if (m_StateA == BLCA_ObjectiveState.DESTROYED && m_StateB == BLCA_ObjectiveState.DESTROYED)
  {
   foreach (string key, BLCA_RushLife life : m_Lives)
   {
    if (life.Body && CanInteract(life.Body) && life.Team == 0 && m_Sectors[m_SectorIndex].ContainsObjectiveArea(life.Body.GetOrigin()))
     life.Score += m_Settings.SectorScore;
   }
   SetSpawns(m_SectorIndex, false);
   int nextIndex = m_SectorIndex + 1;
   if (nextIndex >= m_Sectors.Count())
    FinishRound(0);
   else
   {
    float remainingMs = Math.Max(0, m_RoundUntil - now);
    m_NextSectorTimeMs = NextSectorTimeMs(remainingMs, nextIndex == m_Sectors.Count() - 1);
    SetPhase(BLCA_RushPhase.SECTOR_TRANSITION, m_Settings.TransitionSeconds, "Sector captured   /   Defenders fall back");
   }
   return;
  }
  if (now >= m_RoundUntil && !HasArmedObjective()) FinishRound(1);
 }

 protected void FinishRound(int winner)
 {
  if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE) return;
  m_Winner = winner;
  SetSpawns(m_SectorIndex, false);
  m_Summary = BuildSummary();
  string message = "Defenders win";
  if (winner == 0) message = "Attackers win";
  SetPhase(BLCA_RushPhase.ROUND_ENDING, 3, message);
  EndNative();
 }

 protected void EndNative()
 {
  if (m_NativeEnded) return;
  m_NativeEnded = true;
  SetPhase(BLCA_RushPhase.MATCH_FINISHED, 0, m_Event);
  string key = "USSR";
  if (m_Winner == 0) key = "US";
  FactionManager factions = GetGame().GetFactionManager();
  int index = factions.GetFactionIndex(factions.GetFactionByKey(key));
  GetGameMode().EndGameMode(SCR_GameModeEndData.CreateSimple(EGameOverTypes.ENDREASON_UNDEFINED, -1, index));
 }

 protected void RefreshSummary()
 {
  if (m_Winner < 0 || m_Phase < BLCA_RushPhase.ROUND_ENDING) return;
  string summary = BuildSummary();
  if (summary == m_Summary) return;
  m_Summary = summary;
  Replication.BumpMe();
 }

 protected string BuildSummary()
 {
  BLCA_RushScoreboardSnapshot snapshot = new BLCA_RushScoreboardSnapshot();
  array<int> players = {};
  GetGame().GetPlayerManager().GetPlayers(players);
  array<BLCA_RushLife> ranked = {};
  foreach (int playerId : players)
  {
   string key;
   BLCA_RushLife entry;
   if (!m_PlayerKeys.Find(playerId, key) || !m_Lives.Find(key, entry) || !entry || entry.ConnectedPlayer != playerId || !entry.Deployed) continue;
   if (entry.Team == 0)
   {
    snapshot.AttackScore += entry.Score;
    snapshot.AttackCount++;
   }
   else if (entry.Team == 1)
   {
    snapshot.DefendScore += entry.Score;
    snapshot.DefendCount++;
   }
   else continue;
   int i;
   for (i = 0; i < ranked.Count(); i++)
   {
    if (entry.Score > ranked[i].Score) break;
    if (entry.Score == ranked[i].Score && playerId < ranked[i].ConnectedPlayer) break;
   }
   ranked.InsertAt(entry, i);
  }
  int attackRows;
  int defendRows;
  foreach (BLCA_RushLife entry : ranked)
  {
   if ((entry.Team == 0 && attackRows >= 16) || (entry.Team == 1 && defendRows >= 16)) continue;
   if (entry.Team == 0) attackRows++;
   else defendRows++;
   BLCA_RushScoreboardRow row = new BLCA_RushScoreboardRow();
   row.PlayerID = entry.ConnectedPlayer;
   row.Team = entry.Team;
   row.Name = entry.Name;
   if (row.Name.Length() > 48) row.Name = row.Name.Substring(0, 48);
   row.Score = entry.Score;
   row.Kills = entry.Kills;
   row.Deaths = entry.Deaths;
   row.Plants = entry.Plants;
   row.Defuses = entry.Defuses;
   snapshot.Rows.Insert(row);
  }
  JsonSaveContext save = new JsonSaveContext();
  save.WriteValue("scoreboard", snapshot);
  return save.SaveToString();
 }

 protected string ReadPlayerIdentity(int playerId)
 {
  return SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
 }

 protected string Key(int playerId)
 {
  string cached;
  if (m_PlayerKeys.Find(playerId, cached) && !cached.StartsWith("session-")) return cached;
  string identity = ReadPlayerIdentity(playerId);
  if (identity.IsEmpty() || identity == "00000000-0000-0000-0000-000000000000")
  {
   if (!cached.IsEmpty()) return cached;
   m_PlayerKeySequence++;
   cached = "session-" + m_PlayerKeySequence;
   m_PlayerKeys.Set(playerId, cached);
   return cached;
  }
  // Some clients acquire an identity after registration. Preserve an initial selection when the key becomes stable.
  BLCA_RushLife temporary, existing;
  if (!cached.IsEmpty() && m_Lives.Find(cached, temporary))
  {
   if (!m_Lives.Find(identity, existing) || existing.LockedTeam < 0) m_Lives.Set(identity, temporary);
   else Release(playerId, false);
   m_Lives.Remove(cached);
   foreach (BLCA_RushObjectiveRound objective : m_Objectives)
    if (objective.PlanterKey == cached) objective.PlanterKey = identity;
  }
  m_PlayerKeys.Set(playerId, identity);
  return identity;
 }

 protected BLCA_RushLife Life(int playerId)
 {
  string key = Key(playerId);
  BLCA_RushLife life;
  if (!m_Lives.Find(key, life))
  {
   life = new BLCA_RushLife();
   m_Lives.Set(key, life);
  }
  life.ConnectedPlayer = playerId;
  life.Name = GetGame().GetPlayerManager().GetPlayerName(playerId);
  return life;
 }

 int RespawnDelay(int playerId)
 {
  if (!m_Settings) return 0;
  if (TeamOfPlayer(playerId) == 0) return m_Settings.AttackerRespawnSeconds;
  return m_Settings.DefenderRespawnSeconds;
 }

 bool CanDeploy(int playerId, SCR_SpawnData data)
 {
  if (!m_Ready || playerId <= 0) return false;
  if (m_Phase >= BLCA_RushPhase.SECTOR_TRANSITION) return false;
  if (m_Phase == BLCA_RushPhase.ROUND_ACTIVE && (Now() >= m_RoundUntil || m_AttackerTicketsExhaustedBeforeDeadline)) return false;
  int team = TeamOfPlayer(playerId);
  if (team < 0) return false;
  SCR_SpawnPointSpawnData pointData = SCR_SpawnPointSpawnData.Cast(data);
  if (!pointData || !IsLoadoutAllowed(team, data.GetPrefab())) return false;
  SCR_SpawnPoint point = pointData.GetSpawnPoint();
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  if (!point || !point.IsSpawnPointEnabled()) return false;
  bool objectiveSpawn;
  BLCA_RushSquadSpawnPoint squadPoint = BLCA_RushSquadSpawnPoint.Cast(point);
  if (squadPoint)
  {
   if (!squadPoint.CanUse_S(this, playerId)) return false;
  }
  else if ((team == 0 && point != sector.Attack) || (team == 1 && point != sector.Defend))
  {
   objectiveSpawn = CanUseObjectiveSpawn(playerId, point);
   if (!objectiveSpawn) return false;
  }
  BLCA_RushLife life = Life(playerId);
  if (life.LockedTeam >= 0 && life.LockedTeam != team) return false;
  if (life.Body || life.EligibleAt > Now() || life.Reserved) return false;
  if (ShouldChargeRedeploy(life, team) && !BLCA_RushTicketRules.HasTicket(m_AttackerTickets, m_ReservedAttackers)) return false;
  return objectiveSpawn || IsSpawnSafe(point.GetOrigin(), team);
 }

 bool CanUseObjectiveSpawn(int playerId, SCR_SpawnPoint point)
 {
  if (!point || !point.IsSpawnPointEnabled() || m_Phase != BLCA_RushPhase.ROUND_ACTIVE || TeamOfPlayer(playerId) != 1) return false;
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  if (point == sector.DefendA) return m_StateA != BLCA_ObjectiveState.DESTROYED;
  if (point == sector.DefendB) return m_StateB != BLCA_ObjectiveState.DESTROYED;
  return false;
 }

 bool FindObjectiveSpawnPosition(int playerId, SCR_SpawnPoint point, out vector position)
 {
  if (!CanUseObjectiveSpawn(playerId, point)) return false;
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  BLCA_RushObjectiveComponent terminal = sector.A;
  if (point == sector.DefendB) terminal = sector.B;
  if (!terminal || !terminal.GetOwner()) return false;

  vector objective = terminal.GetOwner().GetOrigin();
  BaseWorld world = GetGame().GetWorld();
  if (!world) return false;
  const float distance = 100;
  const int attempts = 12;
  float firstAngle = Math.RandomFloat(0, Math.PI2);
  for (int attempt = 0; attempt < attempts; attempt++)
  {
   float angle = firstAngle + Math.PI2 * attempt / attempts;
   vector search = objective + Vector(Math.Cos(angle) * distance, 0, Math.Sin(angle) * distance);
   if (!sector.ContainsObjectiveArea(search) || sector.ContainsSpawn(search, 0) || !IsSpawnSafe(search, 1)) continue;
   search[1] = world.GetSurfaceY(search[0], search[2]) + 1;
   if (!SCR_WorldTools.FindEmptyTerrainPosition(position, search, 7, 0.65, 2.2)) continue;
   if (sector.ContainsObjectiveArea(position) && !sector.ContainsSpawn(position, 0) && IsSpawnSafe(position, 1))
    return true;
  }
  return false;
 }

 bool IsSpawnSafe(vector position, int team, float minimumMeters = 0)
 {
  float meters = minimumMeters;
  if (m_Settings.EnforceSpawnSafety) meters = Math.Max(meters, m_Settings.SpawnSafetyMeters);
  if (meters <= 0) return true;
  float radiusSquared = meters * meters;
  foreach (BLCA_RushLife entry : m_Lives)
  {
   if (!entry.ConnectedPlayer || entry.Team == team || !CanInteract(entry.Body)) continue;
   if (vector.DistanceSqXZ(position, entry.Body.GetOrigin()) < radiusSquared) return false;
  }
  return true;
 }

 protected bool ShouldChargeRedeploy(BLCA_RushLife life, int team)
 {
  return team == 0 && m_Phase == BLCA_RushPhase.ROUND_ACTIVE && life.Deployed && !life.FreeBoundaryRedeploy;
 }

 bool Reserve(int playerId, SCR_SpawnData data)
 {
  if (!CanDeploy(playerId, data)) return false;
  BLCA_RushLife life = Life(playerId);
  life.Team = TeamOfPlayer(playerId);
  life.Reserved = true;
  life.ChargedReservation = ShouldChargeRedeploy(life, life.Team);
  if (life.ChargedReservation) m_ReservedAttackers++;
  return true;
 }

 void Release(int playerId, bool commit)
 {
  string key;
  BLCA_RushLife life;
  if (!m_PlayerKeys.Find(playerId, key) || !m_Lives.Find(key, life) || !life.Reserved) return;
  life.Reserved = false;
  if (!life.ChargedReservation) return;
  life.ChargedReservation = false;
  m_ReservedAttackers--;
  if (commit)
  {
   m_AttackerTickets = Math.Max(0, m_AttackerTickets - 1);
   // Keep the winner if the timer expires before the next round tick.
   if (BLCA_RushTicketRules.IsExhausted(m_AttackerTickets, m_ReservedAttackers) && m_Phase == BLCA_RushPhase.ROUND_ACTIVE && Now() < m_RoundUntil)
    m_AttackerTicketsExhaustedBeforeDeadline = true;
   Replication.BumpMe();
  }
 }

 override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
 {
  super.OnPlayerSpawnFinalize_S(requestComponent, handlerComponent, data, entity);
  if (!requestComponent || !entity || !m_Settings) return;
  int playerId = requestComponent.GetPlayerId();
  BLCA_RushLife life = Life(playerId);
  if (life.Body == entity) return;
  Release(playerId, true);
  life.FreeBoundaryRedeploy = false;
  life.BoundaryDeathBody = null;
  life.Team = TeamOfPlayer(playerId);
  life.Body = entity;
  life.Deployed = true;
  life.EligibleAt = 0;
  SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.BLCA_RushCombatManager(entity);
  if (damage) damage.BLCA_TrackRushShots();
  AssignSquad(playerId);
  SCR_SpawnPointSpawnData pointData = SCR_SpawnPointSpawnData.Cast(data);
  if (pointData && pointData.GetSpawnPoint())
   Print("[BLCA_RUSH][DEPLOY] player=" + playerId + " sector=" + m_SectorIndex + " point=" + pointData.GetSpawnPoint().GetSpawnPointName());
 }

 override void OnSpawnPlayerEntityFailure_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, IEntity entity, SCR_SpawnData data, SCR_ESpawnResult reason)
 {
  super.OnSpawnPlayerEntityFailure_S(requestComponent, handlerComponent, entity, data, reason);
  if (requestComponent) Release(requestComponent.GetPlayerId(), false);
 }

 override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
 {
  string key;
  if (GetGameMode().IsMaster() && m_PlayerKeys.Find(playerId, key)) Key(playerId);
  super.OnPlayerDisconnected(playerId, cause, timeout);
  if (!GetGameMode().IsMaster()) return;
  m_BoundaryDeadline.Remove(playerId);
  BLCA_RushLife life;
  if (!m_PlayerKeys.Find(playerId, key)) return;
  if (!m_Lives.Find(key, life) || life.ConnectedPlayer != playerId)
  {
   m_PlayerKeys.Remove(playerId);
   return;
  }
  Release(playerId, false);
  life.ConnectedPlayer = 0;
  life.Body = null;
  m_PlayerKeys.Remove(playerId);
  RefreshSummary();
 }

 override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
 {
  super.OnPlayerKilled(instigatorContextData);
  if (!GetGameMode().IsMaster() || !m_Settings) return;
  int victimId = instigatorContextData.GetVictimPlayerID();
  if (victimId <= 0) return;
  BLCA_RushLife victim = Life(victimId);
  if (!victim.Body || victim.Body != instigatorContextData.GetVictimEntity()) return;
  BLCA_RushCorpse corpse = new BLCA_RushCorpse();
  corpse.ID = victim.Body.GetID();
  corpse.RemoveAt = Now() + m_Settings.CorpseSeconds * 1000;
  m_Corpses.Insert(corpse);
  victim.FreeBoundaryRedeploy = victim.BoundaryDeathBody == victim.Body;
  victim.BoundaryDeathBody = null;
  victim.Body = null;
  victim.EligibleAt = Now() + RespawnDelay(victimId) * 1000;
  if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE) return;
  victim.Deaths++;
  int killerId = instigatorContextData.GetKillerPlayerID();
  if (killerId <= 0 || killerId == victimId) return;
  BLCA_RushLife killer = Life(killerId);
  if (killer.Team < 0 || killer.Team == victim.Team) return;
  killer.Kills++;
  killer.Score += m_Settings.KillScore;
 }

 protected ref array<ref BLCA_RushCorpse> m_Corpses = {};

 protected void CleanupCorpses()
 {
  if (!Replication.IsServer() || !GetGame() || !GetGame().GetWorld()) return;
  float now = Now();
  for (int i = m_Corpses.Count() - 1; i >= 0; i--)
  {
   if (now < m_Corpses[i].RemoveAt) continue;
   IEntity body = GetGame().GetWorld().FindEntityByID(m_Corpses[i].ID);
   m_Corpses.RemoveOrdered(i);
   ChimeraCharacter character = ChimeraCharacter.Cast(body);
   if (!character || !character.GetCharacterController() || !character.GetCharacterController().IsDead()) continue;
   SCR_EntityHelper.DeleteEntityAndChildren(body);
  }
 }

 protected static const int SECTOR_CHANGE_GRACE_SECONDS = 120;
 protected ref map<int, float> m_BoundaryDeadline = new map<int, float>();
 protected float m_NextBoundary;
 [RplProp()] protected int m_RetreatGrace = 20;
 int BLCA_RetreatGrace() { return m_RetreatGrace; }

 bool CanChooseTeam(int playerId, Faction faction)
 {
  if (playerId <= 0) return false;
  int team = TeamOfFaction(faction);
  if (faction && team < 0) return false;
  if (!IsFactionLockActive()) return true;
  int lockedTeam = LockedTeamOfPlayer_S(playerId);
  if (lockedTeam >= 0) return lockedTeam == team;
  int assignedTeam = TeamOfPlayer(playerId);
  if (assignedTeam >= 0) return assignedTeam == team;
  if (!faction) return true;
  if (!m_Settings) m_Settings = BLCA_RushSettings.Load();
  if (!m_Settings.BalanceTeams) return true;
  array<int> players = {};
  GetGame().GetPlayerManager().GetPlayers(players);
  int own, other;
  foreach (int id : players)
  {
   if (id == playerId) continue;
   int assigned = TeamOfPlayer(id);
   if (assigned == team) own++;
   else if (assigned >= 0) other++;
  }
  return own <= other;
 }

 int LockedTeamOfPlayer_S(int playerId)
 {
  if (!Replication.IsServer() || playerId <= 0 || !IsFactionLockActive()) return -1;
  BLCA_RushLife life;
  if (m_Lives.Find(Key(playerId), life)) return life.LockedTeam;
  return -1;
 }

 void RecordFactionAssignment_S(int playerId, Faction faction)
 {
  if (!Replication.IsServer() || playerId <= 0 || !IsFactionLockActive()) return;
  int team = TeamOfFaction(faction);
  if (team < 0) return;
  BLCA_RushLife life = Life(playerId);
  if (life.LockedTeam < 0) life.LockedTeam = team;
 }

 int SquadCapacity()
 {
  if (!m_Settings) return 4;
  return m_Settings.SquadSize;
 }

 void AssignSquad(int playerId)
 {
  if (!Replication.IsServer() || !m_Settings) return;
  PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
  if (!controller) return;
  SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(controller.FindComponent(SCR_PlayerFactionAffiliationComponent));
  if (!affiliation) return;
  Faction faction = affiliation.GetAffiliatedFaction();
  if (!faction || TeamOfFaction(faction) < 0) return;
  SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.Cast(GetGameMode().FindComponent(SCR_GroupsManagerComponent));
  SCR_PlayerControllerGroupComponent playerGroup = SCR_PlayerControllerGroupComponent.Cast(controller.FindComponent(SCR_PlayerControllerGroupComponent));
  if (!groups || !playerGroup) return;

  // Keep a squad choice available even for the first player to join this faction.
  array<SCR_AIGroup> available = groups.GetPlayableGroupsByFaction(faction);
  int groupCount;
  if (available) groupCount = available.Count();
  for (int i = 0; i < 2; i++)
  {
   SCR_AIGroup startingGroup;
   if (i < groupCount)
    startingGroup = available[i];
   else
    startingGroup = groups.CreateNewPlayableGroup(faction);
   if (!startingGroup) break;
   startingGroup.SetCanDeleteIfNoPlayer(false);
  }
  if (!m_Settings.AutoAssignSquads) return;

  SCR_AIGroup assigned = groups.GetPlayerGroup(playerId);
  if (assigned)
  {
   playerGroup.BLCA_RepairRushGroupId_S(assigned);
   return;
  }
  SCR_AIGroup chosen;
  available = groups.GetPlayableGroupsByFaction(faction);
  if (available)
  {
   foreach (SCR_AIGroup group : available)
   {
    if (!group || group.IsPrivate() || group.GetPlayerCount() >= group.GetMaxMembers()) continue;
    chosen = group;
    break;
   }
  }
  if (!chosen) chosen = groups.CreateNewPlayableGroup(faction);
  if (chosen) playerGroup.RPC_AskJoinGroup(chosen.GetGroupID());
 }

 bool IsLoadoutAllowed(int team, ResourceName prefab)
 {
  SCR_LoadoutManager manager = GetGame().GetLoadoutManager();
  if (!manager || prefab.IsEmpty()) return false;
  string key = "US";
  if (team == 1) key = "USSR";
  foreach (SCR_BasePlayerLoadout entry : manager.GetPlayerLoadouts())
  {
   SCR_FactionPlayerLoadout loadout = SCR_FactionPlayerLoadout.Cast(entry);
   if (loadout && loadout.GetFactionKey() == key && loadout.GetLoadoutResource() == prefab) return true;
  }
  return false;
 }

 protected void UpdateBoundary(bool sectorJustOpened = false)
 {
  if (!m_Settings || m_Phase != BLCA_RushPhase.ROUND_ACTIVE || !GetGameMode().IsMaster()) return;
  float now = Now();
  if (now < m_NextBoundary) return;
  m_NextBoundary = now + 250;
  BLCA_RushSectorComponent sector = m_Sectors[m_SectorIndex];
  foreach (string key, BLCA_RushLife life : m_Lives)
  {
   int id = life.ConnectedPlayer;
   if (id <= 0) continue;
   ChimeraCharacter character = ChimeraCharacter.Cast(life.Body);
   CharacterControllerComponent controller;
   if (character) controller = character.GetCharacterController();
   if (!controller || controller.IsDead() || !BLCA_RushCombatBoundary.RequiresReturn(this, sector, life.Body))
   {
    if (m_BoundaryDeadline.Contains(id))
    {
     m_BoundaryDeadline.Remove(id);
     BLCA_SendRetreatNotice(id, 0);
    }
    continue;
   }

   bool enemySpawn = BLCA_RushCombatBoundary.IsEnemySpawn(this, sector, life.Body);
   int normalDuration = m_Settings.RetreatGraceSeconds * 1000;
   float deadline;
   if (!m_BoundaryDeadline.Find(id, deadline))
   {
    int duration = normalDuration;
    if (sectorJustOpened && !enemySpawn) duration = SECTOR_CHANGE_GRACE_SECONDS * 1000;
    m_BoundaryDeadline.Set(id, now + duration);
    BLCA_SendRetreatNotice(id, duration);
    continue;
   }
   if (enemySpawn && deadline > now + normalDuration)
   {
    deadline = now + normalDuration;
    m_BoundaryDeadline.Set(id, deadline);
    BLCA_SendRetreatNotice(id, normalDuration);
   }
   if (now < deadline) continue;
   life.BoundaryDeathBody = life.Body;
   SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.BLCA_RushCombatManager(life.Body);
   if (damage) damage.Kill(Instigator.CreateInstigatorGM());
   if (life.Body && !controller.IsDead()) controller.ForceDeath();
   m_BoundaryDeadline.Remove(id);
   BLCA_SendRetreatNotice(id, 0);
  }
 }

 protected void BLCA_SendRetreatNotice(int playerId, int duration)
 {
  SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
  if (controller) controller.BLCA_SendRetreatDeadline(m_SectorIndex, duration);
 }

 string Admin(int playerId, string command, int value)
 {
  if (!GetGameMode().IsMaster() || !GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
   return "Rush: administrator access required.";
  if (!m_Settings || m_NativeEnded || m_Objectives.Count() != 2) return "Rush: scenario is unavailable.";
  switch (command)
  {
   case "status":
    return string.Format("Rush phase %1, sector %2/%3, US tickets %4, players %5.", m_Phase, m_SectorIndex + 1, m_SectorCount, m_AttackerTickets, GetGame().GetPlayerManager().GetPlayerCount());
   case "start":
    if (m_Phase != BLCA_RushPhase.WAITING_FOR_PLAYERS && m_Phase != BLCA_RushPhase.WARMUP) return "Rush: round already started.";
    if (!EnoughPlayers()) return "Rush: waiting for enough players on both teams.";
    m_RoundUntil = Now() + m_Settings.RoundMinutes * 60000;
    SetPhase(BLCA_RushPhase.ROUND_STARTING, 0, "Administrator started the round");
    OpenSector(0);
    break;
   case "stop":
    if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE) return "Rush: an active round is required.";
    FinishRound(1);
    break;
   case "tickets":
    if (m_AttackerTicketsExhaustedBeforeDeadline) return "Rush: the last US ticket was spent.";
    m_AttackerTickets = Math.ClampInt(value, 0, 999);
    m_AttackerTicketsExhaustedBeforeDeadline = m_Phase == BLCA_RushPhase.ROUND_ACTIVE && BLCA_RushTicketRules.IsExhausted(m_AttackerTickets, m_ReservedAttackers) && Now() < m_RoundUntil;
    Replication.BumpMe();
    break;
   case "resetobjectives":
    if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE) return "Rush: an active sector is required.";
    foreach (BLCA_RushObjectiveRound objective : m_Objectives)
    {
     if (objective.State == BLCA_ObjectiveState.DESTROYED) continue;
     objective.Reset(BLCA_ObjectiveState.ACTIVE);
    }
    SyncObjectives(Now());
    break;
   case "nextsector":
    if (m_Phase != BLCA_RushPhase.ROUND_ACTIVE || m_AttackerTicketsExhaustedBeforeDeadline || Now() >= m_RoundUntil || m_SectorIndex + 1 >= m_Sectors.Count()) return "Rush: there is no next sector.";
    SetSpawns(m_SectorIndex, false);
    SetPhase(BLCA_RushPhase.SECTOR_TRANSITION, m_Settings.TransitionSeconds, "Administrator advanced the sector");
    break;
   default:
    return "Rush: /rush status, start, stop, tickets N, resetobjectives, nextsector";
  }
  Print("[BLCA_RUSH][ADMIN] command=" + command + " value=" + value);
  return "Rush: " + command + " applied.";
 }
}
