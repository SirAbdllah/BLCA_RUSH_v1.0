enum BLCA_RushPhase
{
 WAITING_FOR_PLAYERS,
 WARMUP,
 ROUND_STARTING,
 ROUND_ACTIVE,
 SECTOR_TRANSITION,
 ROUND_ENDING,
 SCOREBOARD,
 NEXT_ROUND,
 MATCH_FINISHED
}

enum BLCA_ObjectiveState
{
 INACTIVE,
 ACTIVE,
 ARMING,
 ARMED,
 DEFUSING,
 DEFUSED,
 DESTROYED
}

class BLCA_RushSettings
{
 int MinimumPlayers = 2;
 int WarmupSeconds = 20;
 static const int DEFAULT_ATTACKER_TICKETS = 100;
 int AttackerTickets = DEFAULT_ATTACKER_TICKETS;
 int DetonationSeconds = 30;
 float PlantSeconds = 4;
 float DefuseSeconds = 5;
 int TransitionSeconds = 15;
 int AttackerRespawnSeconds = 8;
 int DefenderRespawnSeconds = 10;
 int RoundMinutes = 45;
 int RetreatGraceSeconds = 20;
 int ScoreboardSeconds = 16;
 int KillScore = 100;
 int PlantScore = 200;
 int DefuseScore = 200;
 int DestroyScore = 300;
 int SectorScore = 150;
 int CorpseSeconds = 15;
 bool EnforceSpawnSafety = true;
 bool BalanceTeams = true;
 bool AutoAssignSquads = true;
 int SquadSize = 4;
 int SpawnSafetyMeters = 25;

 void Validate()
 {
  SquadSize = Math.ClampInt(SquadSize, 4, 6);
  CorpseSeconds = Math.ClampInt(CorpseSeconds, 5, 300);
  MinimumPlayers = 2;
  WarmupSeconds = Math.ClampInt(WarmupSeconds, 0, 120);
  if (AttackerTickets <= 0) AttackerTickets = DEFAULT_ATTACKER_TICKETS;
  AttackerTickets = Math.Min(AttackerTickets, 999);
  DetonationSeconds = Math.ClampInt(DetonationSeconds, 10, 120);
  PlantSeconds = Math.Clamp(PlantSeconds, 2, 15);
  DefuseSeconds = Math.Clamp(DefuseSeconds, 2, 15);
  TransitionSeconds = Math.ClampInt(TransitionSeconds, 5, 90);
  AttackerRespawnSeconds = Math.ClampInt(AttackerRespawnSeconds, 0, 90);
  DefenderRespawnSeconds = Math.ClampInt(DefenderRespawnSeconds, 0, 90);
  RoundMinutes = Math.ClampInt(RoundMinutes, 5, 180);
  RetreatGraceSeconds = Math.ClampInt(RetreatGraceSeconds, 5, 120);
  ScoreboardSeconds = Math.ClampInt(ScoreboardSeconds, 8, 60);
  SpawnSafetyMeters = Math.ClampInt(SpawnSafetyMeters, 5, 80);
  KillScore = Math.ClampInt(KillScore, 0, 10000);
  PlantScore = Math.ClampInt(PlantScore, 0, 10000);
  DefuseScore = Math.ClampInt(DefuseScore, 0, 10000);
  DestroyScore = Math.ClampInt(DestroyScore, 0, 10000);
  SectorScore = Math.ClampInt(SectorScore, 0, 10000);
 }

 static BLCA_RushSettings Load()
 {
  BLCA_RushSettings settings = new BLCA_RushSettings();
  string path = "$profile:BLCARush/Rush.json";
  FileIO.MakeDirectory("$profile:BLCARush");
  if (FileIO.FileExists(path))
  {
   JsonLoadContext load = new JsonLoadContext();
   if (!load.LoadFromFile(path) || !load.ReadValue("rush", settings) || !settings)
   {
    Print("[BLCA_RUSH][MATCH] Invalid Rush.json; using defaults.", LogLevel.WARNING);
    settings = new BLCA_RushSettings();
   }
   else if (load.StartObject("rush"))
   {
    if (!load.DoesKeyExist("AttackerTickets") && load.DoesKeyExist("DefenderTickets"))
    {
     // Preserve the starting count in older server profiles.
     int legacyTickets;
     if (load.ReadValue("DefenderTickets", legacyTickets))
      settings.AttackerTickets = legacyTickets;
    }
    load.EndObject();
   }
  }
  else
  {
   JsonSaveContext save = new JsonSaveContext();
   save.WriteValue("rush", settings);
   save.SaveToFile(path);
  }
  settings.Validate();
  return settings;
 }
}

class BLCA_RushLife
{
 IEntity Body;
 IEntity BoundaryDeathBody;
 bool FreeBoundaryRedeploy;
 bool Deployed;
 int Team = -1;
 int LockedTeam = -1;
 bool Reserved;
 bool ChargedReservation;
 float EligibleAt;
 int ConnectedPlayer;
 string Name;
 int Score;
 int Kills;
 int Deaths;
 int Plants;
 int Defuses;
}

class BLCA_RushCorpse
{
 EntityID ID;
 float RemoveAt;
}

class BLCA_RushScoreboardRow
{
 int PlayerID;
 int Team;
 string Name;
 int Score;
 int Kills;
 int Deaths;
 int Plants;
 int Defuses;
}

class BLCA_RushScoreboardSnapshot
{
 ref array<ref BLCA_RushScoreboardRow> Rows = {};
 int AttackScore;
 int DefendScore;
 int AttackCount;
 int DefendCount;
}

class BLCA_RushTicketRules
{
 static bool HasTicket(int balance, int reservations)
 {
  return balance > reservations;
 }

 static bool IsExhausted(int balance, int reservations)
 {
  return balance == 0 && reservations == 0;
 }
}
