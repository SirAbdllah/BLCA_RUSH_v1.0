class BLCA_RushClientSettings
{
 bool Minimap = true;
 bool AudioAlerts = true;
 bool SquadNames = true;
 int MapHalfRange = 80;

 static BLCA_RushClientSettings Load()
 {
  BLCA_RushClientSettings settings = new BLCA_RushClientSettings();
  string file = "$profile:BLCARush/HUD.json";
  if (FileIO.FileExists(file))
  {
   JsonLoadContext load = new JsonLoadContext();
   if (!load.LoadFromFile(file) || !load.ReadValue("hud", settings) || !settings)
    settings = new BLCA_RushClientSettings();
  }
  else
  {
   FileIO.MakeDirectory("$profile:BLCARush");
   JsonSaveContext save = new JsonSaveContext();
   save.WriteValue("hud", settings);
   save.SaveToFile(file);
  }
  settings.MapHalfRange = Math.ClampInt(settings.MapHalfRange, 50, 100);
  return settings;
 }
}

class BLCA_RushHUDController
{
 protected static const ResourceName LAYOUT = "{4DCA6B737E515B96}UI/layouts/BLCA_Rush/BLCA_HUD.layout";
 static const ResourceName MAP_TEXTURE = "{B48C0B29FBEBA2A1}UI/Textures/Map/worlds/Arland/Arland.edds";
 protected static const ResourceName MAP_DETAIL = "{F98F3D2CBA523091}UI/Textures/Map/worlds/Arland/ArlandRasterized.edds";
 protected static const float MAP_SIZE = 248;
 protected static const float MAP_GRID_METRES = 25;
 protected static const float OBJECTIVE_FADE_RANGE = 90;
 protected static const float ENEMY_AIM_DELAY_MS = 350;
 static const int TEAM_BLUE = 0xff55c9ff;
 static const int SQUAD_GREEN = 0xff7afe88;
 protected static const int PLAYER_RED = 0xffff4055;
 protected static const int DOT_OUTLINE = 0xff12212e;
 protected static const ref array<float> DOT_VERTICES = {0, -1, 0.7071, -0.7071, 1, 0, 0.7071, 0.7071, 0, 1, -0.7071, 0.7071, -1, 0, -0.7071, -0.7071};
 protected ref BLCA_RushChatCommands m_Chat = new BLCA_RushChatCommands();
 protected Widget m_Root;
 protected ImageWidget m_MapImage;
 protected ImageWidget m_MapDetail;
 protected CanvasWidget m_Map;
 protected ref map<string, TextWidget> m_Text = new map<string, TextWidget>();
 protected ref array<ref CanvasWidgetCommand> m_MapCommands = {};
 protected ref array<ref PolygonDrawCommand> m_MapRectangles = {};
 protected ref array<ref PolygonDrawCommand> m_MapDots = {};
 protected ref array<ref LineDrawCommand> m_MapLines = {};
 protected ref array<TextWidget> m_CompassLabels = {};
 protected ref array<string> m_CompassDirections = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
 protected ref array<int> m_MapPlayerIds = {};
 protected ref array<IEntity> m_MapTeamBodies = {};
 protected ref array<IEntity> m_MapSquadBodies = {};
 protected CanvasWidget m_SectorTrack;
 protected CanvasWidget m_HeadingTicks;
 protected Widget m_RushStatus;
 protected ref array<TextWidget> m_HeadingLabels = {};
 protected ref array<ref CanvasWidgetCommand> m_TrackCommands = {};
 protected ref array<ref CanvasWidgetCommand> m_HeadingCommands = {};
 protected float m_TrackScale;
 protected int m_TrackSector = -1;
 protected int m_TrackSectors = -1;
 protected int m_TrackTeam = -2;
 protected bool m_TrackVictory;
 protected float m_HeadingScale;
 protected float m_LastCompassBearing = -1000;
 protected ref array<CanvasWidget> m_StatusBadges = {};
 protected ref array<CanvasWidget> m_WaypointBadges = {};
 protected ref array<ref array<ref CanvasWidgetCommand>> m_BadgeCommands = {};
 protected ref array<int> m_BadgeColors = {};
 protected ref array<float> m_BadgeScales = {};
 protected IEntity m_MapRosterBody;
 protected ref BLCA_RushClientSettings m_Settings;
 protected float m_NextUpdate;
 protected float m_MapDrawScale = 1;
 protected int m_RectangleCount;
 protected int m_DotCount;
 protected int m_LineCount;
 protected int m_LastHeading = -1;
 protected IEntity m_MapBody;
 protected vector m_MapCentre;
 protected vector m_MapExtent;
 protected float m_MapYaw;
 protected float m_MapCos;
 protected float m_MapSin;
 protected BLCA_RushSectorComponent m_MapBoundarySector;
 protected ref array<vector> m_MapBoundary = {};
 protected ref array<vector> m_MapBoundaryProjected = {};
 protected ref BLCA_RushMapBoundary m_MapBoundaryShade = new BLCA_RushMapBoundary();
 protected vector m_MapRoadCentre;
 protected float m_MapRoadHalf;
 protected float m_MapRoadRefreshAt;
 protected ref array<vector> m_MapRoadPoints = {};
 protected ref array<float> m_MapRoadWidths = {};
 protected SCR_AIGroup m_Group;
 protected Widget m_WaypointA;
 protected Widget m_WaypointB;
 protected TextWidget m_EnemyName;
 protected ref TraceParam m_AimTrace;
 protected ref array<IEntity> m_AimExclude = {};
 protected int m_AimedPlayer;
 protected float m_AimedSince;
 protected float m_NextAimTrace;
 protected int m_EventSerial = -1;
 protected int m_ArmedSeconds;
 protected float m_EventExpires;
 protected Widget m_ResultRoot;
 protected ref BLCA_RushResultView m_ResultView;
 protected Widget m_ReturnToSector;
 protected TextWidget m_ReturnToSectorTitle;
 protected TextWidget m_ReturnToSectorTime;
 protected bool m_TriedLayout;

 void Close()
 {
  m_Chat.Close();
  if (m_Root) m_Root.RemoveFromHierarchy();
  m_Root = null;
  m_ResultRoot = null;
  m_ResultView = null;
  m_ReturnToSector = null;
  m_ReturnToSectorTitle = null;
  m_ReturnToSectorTime = null;
  m_Map = null;
  m_MapImage = null;
  m_MapDetail = null;
  m_Text.Clear();
  m_MapCommands.Clear();
  m_TrackCommands.Clear();
  m_HeadingCommands.Clear();
  m_HeadingLabels.Clear();
  m_StatusBadges.Clear();
  m_WaypointBadges.Clear();
  m_BadgeCommands.Clear();
  m_BadgeScales.Clear();
  m_BadgeColors.Clear();
 }

 void Tick(BLCA_RushGameModeComponent mode, float timeSlice)
 {
  if (!mode || !GetGame().GetPlayerController() || !GetGame().GetWorkspace()) return;
  float now = BLCA_RushGameModeComponent.Now();
  bool refresh = now >= m_NextUpdate;
  if (refresh)
  {
   m_NextUpdate = now + 200;
   m_Chat.Register();
  }
  if (!m_Root && !m_TriedLayout)
  {
   m_TriedLayout = true;
   m_Root = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
   if (!m_Root)
   {
    Print("[BLCA_RUSH][HUD] Unable to load Rush HUD.", LogLevel.ERROR);
    return;
   }
   m_Settings = BLCA_RushClientSettings.Load();
   m_Map = CanvasWidget.Cast(m_Root.FindAnyWidget("Minimap"));
   m_MapImage = ImageWidget.Cast(m_Root.FindAnyWidget("MinimapTerrain"));
   m_MapDetail = ImageWidget.Cast(m_Root.FindAnyWidget("MinimapDetail"));
   for (int compassIndex; compassIndex < 4; compassIndex++)
    m_CompassLabels.Insert(TextWidget.Cast(m_Root.FindAnyWidget("Compass" + compassIndex)));
   m_WaypointA = m_Root.FindAnyWidget("WaypointA");
   m_WaypointB = m_Root.FindAnyWidget("WaypointB");
   SetWaypointLayer(m_WaypointA);
   SetWaypointLayer(m_WaypointB);
   m_RushStatus = m_Root.FindAnyWidget("RushStatus");
   m_ReturnToSector = m_Root.FindAnyWidget("ReturnToSector");
   m_ReturnToSectorTitle = FindText("ReturnToSectorTitle");
   m_ReturnToSectorTime = FindText("ReturnToSectorTime");
   m_SectorTrack = CanvasWidget.Cast(m_Root.FindAnyWidget("SectorTrack"));
   m_HeadingTicks = CanvasWidget.Cast(m_Root.FindAnyWidget("CompassTicks"));
   for (int h = 0; h < 7; h++) m_HeadingLabels.Insert(FindText("Heading" + h));
   m_StatusBadges.Insert(CanvasWidget.Cast(m_Root.FindAnyWidget("ObjectiveABadge")));
   m_StatusBadges.Insert(CanvasWidget.Cast(m_Root.FindAnyWidget("ObjectiveBBadge")));
   CanvasWidget waypointA;
   if (m_WaypointA) waypointA = CanvasWidget.Cast(m_WaypointA.FindAnyWidget("Badge"));
   m_WaypointBadges.Insert(waypointA);
   CanvasWidget waypointB;
   if (m_WaypointB) waypointB = CanvasWidget.Cast(m_WaypointB.FindAnyWidget("Badge"));
   m_WaypointBadges.Insert(waypointB);
   for (int b = 0; b < 4; b++)
   {
    m_BadgeCommands.Insert(new array<ref CanvasWidgetCommand>());
    m_BadgeColors.Insert(0);
    m_BadgeScales.Insert(0);
   }
   m_EnemyName = TextWidget.Cast(m_Root.FindAnyWidget("EnemyAimName"));
   m_AimTrace = new TraceParam();
   m_AimTrace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
   m_AimTrace.LayerMask = EPhysicsLayerPresets.Projectile;
   m_AimTrace.ExcludeArray = m_AimExclude;
   if (m_MapImage)
   {
    m_MapImage.LoadImageTexture(0, MAP_TEXTURE);
    m_MapImage.SetImage(0);
    m_MapImage.SetColor(Color.FromRGBA(74, 89, 107, 255));
   }
   if (m_MapDetail)
   {
    m_MapDetail.LoadImageTexture(0, MAP_DETAIL);
    m_MapDetail.SetImage(0);
    m_MapDetail.SetColor(Color.FromRGBA(88, 101, 117, 255));
    m_MapDetail.SetOpacity(0.55);
   }
  }
  if (!m_Root) return;
  IEntity body = SCR_PlayerController.GetLocalControlledEntity();
  ChimeraCharacter character = ChimeraCharacter.Cast(body);
  bool deployed = character && character.GetCharacterController() && !character.GetCharacterController().IsDead();
  bool otherMenu = GetGame().GetMenuManager() && GetGame().GetMenuManager().GetTopMenu();
  SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
  bool editorOpen = editor && editor.IsOpened();
  m_Root.SetVisible(!editorOpen && !otherMenu);
  if (!m_Root.IsVisible())
  {
   m_MapBody = null;
   SetAimedPlayer(0, now);
   return;
  }

  bool summary = mode.Phase() == BLCA_RushPhase.SCOREBOARD;
  bool combat = deployed && !summary;
  float retreatSeconds = -1;
  if (combat) retreatSeconds = BLCA_RushCombatBoundary.Remaining(mode, body);
  bool enemySpawn;
  if (retreatSeconds >= 0)
  {
   BLCA_RushSectorComponent sector = mode.ActiveSector();
   if (sector) enemySpawn = BLCA_RushCombatBoundary.IsEnemySpawn(mode, sector, body);
  }
  UpdateRetreatNotice(retreatSeconds, enemySpawn);
  if (m_RushStatus) m_RushStatus.SetVisible(!summary);
  if (!summary)
  {
   vector camera[4];
   GetGame().GetWorld().GetCurrentCamera(camera);
   UpdateHeading(camera[2]);
  }
  if (combat)
  {
   if (refresh || m_MapRosterBody != body)
    UpdateMapRoster(mode, body);
   if (m_Settings.Minimap && m_Map) UpdateMap(mode, body, timeSlice);
   UpdateWaypoints(mode, body);
   UpdateAimedEnemy(mode, character, now);
  }
  else
  {
   m_MapBody = null;
   if (m_WaypointA) m_WaypointA.SetVisible(false);
   if (m_WaypointB) m_WaypointB.SetVisible(false);
   SetAimedPlayer(0, now);
  }
  if (!refresh) return;
  UpdateStatus(mode, now);
  UpdateResult(mode, summary);
  Show("WeaponPlate", combat);
  Show("WeaponRule", combat);
  Show("Weapon", combat);
  Show("Ammo", combat);
  Show("Equipment", combat);
  Show("Prompt", combat);
  Show("MinimapFrame", combat && m_Settings.Minimap);
  Show("MapRule", combat && m_Settings.Minimap);
  Show("MapHeading", combat && m_Settings.Minimap);
  Show("SquadLabel", combat && m_Settings.SquadNames);
  for (int j; j < 6; j++) Show("SquadRow" + j, combat && m_Settings.SquadNames);
  if (!combat) return;

  UpdateWeapon(character);
  UpdateSquad(m_Group);
  UpdatePrompt(mode, body);
 }

 protected void UpdateRetreatNotice(float seconds, bool enemySpawn)
 {
  if (!m_ReturnToSector) return;
  bool visible = seconds >= 0;
  if (m_ReturnToSector.IsVisible() != visible) m_ReturnToSector.SetVisible(visible);
  if (!visible || !m_ReturnToSectorTime) return;
  if (m_ReturnToSectorTitle)
  {
   string title = "RETURN TO THE COMBAT AREA";
   if (enemySpawn) title = "LEAVE THE ENEMY SPAWN";
   if (m_ReturnToSectorTitle.GetText() != title) m_ReturnToSectorTitle.SetText(title);
  }
  string countdown = Clock(Math.Ceil(seconds));
  if (m_ReturnToSectorTime.GetText() != countdown) m_ReturnToSectorTime.SetText(countdown);
 }

 protected void UpdateResult(BLCA_RushGameModeComponent mode, bool visible)
 {
  if (!m_ResultRoot && visible)
  {
   m_ResultRoot = GetGame().GetWorkspace().CreateWidgets(BLCA_RushResultView.LAYOUT, m_Root);
   if (m_ResultRoot)
   {
    FrameSlot.SetAnchorMin(m_ResultRoot, 0, 0);
    FrameSlot.SetAnchorMax(m_ResultRoot, 1, 1);
    FrameSlot.SetOffsets(m_ResultRoot, 0, 0, 0, 0);
    m_ResultView = new BLCA_RushResultView();
   }
  }
  if (!m_ResultRoot || !m_ResultView) return;
  m_ResultRoot.SetVisible(visible);
  if (visible) m_ResultView.Update(m_ResultRoot, mode);
 }

 protected void UpdateAimedEnemy(BLCA_RushGameModeComponent mode, ChimeraCharacter body, float now)
 {
  CharacterControllerComponent controller = body.GetCharacterController();
  if (!controller || !controller.IsWeaponRaised() || controller.IsFreeLookEnabled() || controller.IsUnconscious())
  {
   SetAimedPlayer(0, now);
   return;
  }
  if (now < m_NextAimTrace) return;
  m_NextAimTrace = now + 33;
  BaseWeaponManagerComponent manager = BaseWeaponManagerComponent.Cast(body.FindComponent(BaseWeaponManagerComponent));
  BaseWeaponComponent weapon;
  if (manager) weapon = manager.GetCurrentWeapon();
  int team = mode.TeamOfEntity(body);
  if (!weapon || team < 0)
  {
   SetAimedPlayer(0, now);
   return;
  }

  BaseWorld world = GetGame().GetWorld();
  vector camera[4];
  world.GetCurrentCamera(camera);
  m_AimExclude.Clear();
  m_AimExclude.Insert(body);
  m_AimExclude.Insert(weapon.GetOwner());
  m_AimTrace.Start = camera[3];
  m_AimTrace.End = camera[3] + camera[2] * 150;
  float fraction = world.TraceMove(m_AimTrace, null);
  IEntity target;
  if (fraction < 1) target = m_AimTrace.TraceEnt;
  while (target && !ChimeraCharacter.Cast(target)) target = target.GetParent();
  if (!BLCA_RushGameModeComponent.CanInteract(target) || mode.TeamOfEntity(target) != 1 - team)
  {
   SetAimedPlayer(0, now);
   return;
  }

  // Recheck from the character's eye so third-person cameras cannot reveal a target behind cover.
  vector hit = vector.Lerp(m_AimTrace.Start, m_AimTrace.End, fraction);
  m_AimTrace.Start = body.EyePosition();
  m_AimTrace.End = hit;
  if (world.TraceMove(m_AimTrace, null) < 1)
  {
   IEntity obstruction = m_AimTrace.TraceEnt;
   while (obstruction && obstruction != target) obstruction = obstruction.GetParent();
   if (!obstruction)
   {
    SetAimedPlayer(0, now);
    return;
   }
  }
  SetAimedPlayer(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(target), now);
 }

 protected void SetWaypointLayer(Widget marker)
 {
  if (!marker) return;
  Widget letter = marker.FindAnyWidget("Letter");
  Widget distance = marker.FindAnyWidget("Distance");
  if (letter) letter.SetZOrder(2);
  if (distance) distance.SetZOrder(2);
 }

 protected void SetAimedPlayer(int player, float now)
 {
  if (player != m_AimedPlayer)
  {
   m_AimedPlayer = player;
   m_AimedSince = now;
  }
  if (!m_EnemyName) return;
  bool show = player > 0 && now - m_AimedSince >= ENEMY_AIM_DELAY_MS;
  if (show && !m_EnemyName.IsVisible())
   m_EnemyName.SetText(GetGame().GetPlayerManager().GetPlayerName(player));
  m_EnemyName.SetVisible(show);
 }

 protected void UpdateWaypoints(BLCA_RushGameModeComponent mode, IEntity body)
 {
  if (!m_WaypointA || !m_WaypointB) return;
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  if (!sector || mode.Phase() != BLCA_RushPhase.ROUND_ACTIVE)
  {
   m_WaypointA.SetVisible(false);
   m_WaypointB.SetVisible(false);
   return;
  }

  BaseWorld world = GetGame().GetWorld();
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  vector camera[4];
  world.GetCurrentCamera(camera);
  float width = workspace.DPIUnscale(workspace.GetWidth());
  float height = workspace.DPIUnscale(workspace.GetHeight());
  vector positionA, positionB;
  int team = mode.TeamOfEntity(body);
  PlaceWaypoint(m_WaypointA, mode.ObjectiveState(0), sector.A, body, team, camera, width, height, positionA);
  PlaceWaypoint(m_WaypointB, mode.ObjectiveState(1), sector.B, body, team, camera, width, height, positionB);
  if (m_WaypointA.IsVisible() && m_WaypointB.IsVisible() && Math.AbsFloat(positionA[0] - positionB[0]) < 54 && Math.AbsFloat(positionA[1] - positionB[1]) < 49)
  {
   float separatedY = positionA[1] + 52;
   if (separatedY > height - 170) separatedY = positionA[1] - 52;
   FrameSlot.SetPos(m_WaypointB, positionB[0], separatedY);
  }
 }

 protected void PlaceWaypoint(Widget marker, BLCA_ObjectiveState state, BLCA_RushObjectiveComponent terminal, IEntity body, int team, vector camera[4], float width, float height, out vector position)
 {
  bool active = terminal && terminal.GetOwner() && state != BLCA_ObjectiveState.INACTIVE && state != BLCA_ObjectiveState.DESTROYED;
  marker.SetVisible(active);
  if (!active) return;

  vector origin = terminal.GetOwner().GetOrigin();
  float range = vector.DistanceXZ(body.GetOrigin(), origin);
  // Blend through the 90 m threshold to keep the marker from popping on screen.
  float proximity = Math.Clamp((110 - range) / OBJECTIVE_FADE_RANGE, 0, 1);
  float scale = 1 - 0.34 * proximity;
  float markerWidth = 76 * scale;
  float markerHeight = 52 * scale;
  vector target = origin + "0 1 0";
  vector relative = target - camera[3];
  vector projected;
  string direction;
  float edge = 38;
  float top = Math.Min(186, height * 0.27);
  float bottom = Math.Max(top, height - 170);
  if (vector.Dot(relative, camera[2]) <= 0.1)
  {
   bool right = vector.Dot(relative, camera[0]) >= 0;
   projected[0] = edge;
   direction = "<";
   if (right)
   {
    projected[0] = width - edge;
    direction = ">";
   }
   projected[1] = height * 0.48;
  }
  else
  {
   projected = GetGame().GetWorkspace().ProjWorldToScreen(target, GetGame().GetWorld());
   if (projected[0] < edge) direction = "<";
   else if (projected[0] > width - edge) direction = ">";
   else if (projected[1] - markerHeight < top) direction = "^";
   else if (projected[1] - markerHeight > bottom) direction = "v";
  }
  position[0] = Math.Clamp(projected[0] - markerWidth * 0.5, edge, width - edge - markerWidth);
  position[1] = Math.Clamp(projected[1] - markerHeight, top, bottom);
  FrameSlot.SetPos(marker, position[0], position[1]);
  FrameSlot.SetSize(marker, markerWidth, markerHeight);
  marker.SetOpacity(1 - 0.58 * proximity);

  CanvasWidget badge = m_WaypointBadges[terminal.Letter];
  if (badge)
  {
   FrameSlot.SetPos(badge, 23 * scale, 0);
   FrameSlot.SetSize(badge, 30 * scale, 30 * scale);
   DrawObjectiveBadge(badge, terminal.Letter + 2, state, scale);
  }
  TextWidget letter = TextWidget.Cast(marker.FindAnyWidget("Letter"));
  if (letter)
  {
   FrameSlot.SetPos(letter, 23 * scale, 0);
   FrameSlot.SetSize(letter, 30 * scale, 30 * scale);
   letter.SetDesiredFontSize(Math.Round(19 * scale));
  }
  TextWidget distance = TextWidget.Cast(marker.FindAnyWidget("Distance"));
  if (distance)
  {
   FrameSlot.SetPos(distance, 0, 31 * scale);
   FrameSlot.SetSize(distance, markerWidth, 21 * scale);
   distance.SetDesiredFontSize(Math.Round(15 * scale));
   string label = Math.Round(range).ToString() + " m";
   if (!direction.IsEmpty()) label = direction + " " + label;
   if (distance.GetText() != label) distance.SetText(label);
  }
  TextWidget order = TextWidget.Cast(marker.FindAnyWidget("Order"));
  if (order)
  {
   string action;
   bool focused = direction.IsEmpty() && Math.AbsFloat(projected[0] - width * 0.5) < 45 && Math.AbsFloat(projected[1] - height * 0.5) < 55;
   if (focused)
   {
    bool armed = state == BLCA_ObjectiveState.ARMED || state == BLCA_ObjectiveState.DEFUSING;
    if (team == 0 && !armed) action = "ATTACK";
    if (team == 1 && !armed) action = "DEFEND";
    if (team == 1 && armed) action = "DEFUSE";
   }
   order.SetVisible(!action.IsEmpty());
   if (order.GetText() != action) order.SetText(action);
   FrameSlot.SetPos(order, markerWidth * 0.5 - 56, -18);
  }
 }

 protected void UpdateStatus(BLCA_RushGameModeComponent mode, float now)
 {
  Text("Tickets", mode.AttackerTickets().ToString());
  UpdateSectorTrack(mode);
  bool overtime = mode.IsOvertime();
  bool showClock = mode.PhaseSeconds() > 0 || overtime;
  Show("Clock", showClock);
  if (showClock)
  {
   string clock = Clock(mode.PhaseSeconds());
   if (overtime) clock = "OT " + clock;
   else if (mode.Phase() == BLCA_RushPhase.ROUND_ACTIVE) clock = "ATK " + clock;
   Text("Clock", clock);
  }
  UpdateObjective(mode, 0, "ObjectiveA");
  UpdateObjective(mode, 1, "ObjectiveB");
  if (m_EventSerial != mode.EventSerial())
  {
   if (m_EventSerial >= 0 && m_Settings.AudioAlerts && SCR_UISoundEntity.GetInstance())
    SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_ITEM_CHANGE);
   m_EventSerial = mode.EventSerial();
   m_EventExpires = now + 4500;
  }
  string notice;
  if (mode.Phase() != BLCA_RushPhase.ROUND_ACTIVE || now < m_EventExpires) notice = mode.EventText();
  Text("Notice", notice);
  int armed = Math.Max(mode.ObjectiveSeconds(0), mode.ObjectiveSeconds(1));
  if (armed != m_ArmedSeconds && armed > 0 && armed <= 10 && m_Settings.AudioAlerts && SCR_UISoundEntity.GetInstance())
   SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_BUTTON_HOVER);
  m_ArmedSeconds = armed;
 }

 protected void UpdateObjective(BLCA_RushGameModeComponent mode, int index, string widget)
 {
  BLCA_ObjectiveState state = mode.ObjectiveState(index);
  bool armed = state == BLCA_ObjectiveState.ARMED || state == BLCA_ObjectiveState.DEFUSING;
  Show(widget, armed);
  if (armed) Text(widget, Clock(mode.ObjectiveSeconds(index)));
  DrawObjectiveBadge(m_StatusBadges[index], index, state, 32.0 / 30.0);
 }

 protected void UpdateHeading(vector forward)
 {
  if (!m_HeadingTicks || forward[0] * forward[0] + forward[2] * forward[2] <= 0.001) return;

  float bearing = Math.Atan2(forward[0], forward[2]) * Math.RAD2DEG;
  float scale = GetGame().GetWorkspace().DPIScale(1);
  if (Math.AbsFloat(bearing - m_LastCompassBearing) < 0.2 && scale == m_HeadingScale) return;
  m_LastCompassBearing = bearing;
  m_HeadingScale = scale;

  int centreLabel = Math.Round(bearing / 30);
  for (int i = 0; i < m_HeadingLabels.Count(); i++)
  {
   TextWidget label = m_HeadingLabels[i];
   if (!label) continue;
   int degrees = (centreLabel + i - 3) * 30;
   int heading = (degrees + 360) % 360;
   string text = heading.ToString();
   if (heading == 0) text = "N";
   else if (heading == 90) text = "E";
   else if (heading == 180) text = "S";
   else if (heading == 270) text = "W";
   float x = 173 + (degrees - bearing) * 2.5;
   bool inTape = x > 10 && x < 336;
   label.SetVisible(inTape);
   if (!inTape) continue;
   FrameSlot.SetPos(label, x - 21, 0);
   label.SetOpacity(Math.Clamp((173 - Math.AbsFloat(x - 173)) / 28, 0, 1));
   if (label.GetText() != text) label.SetText(text);
  }

  // Reuse the scale marks while the tape scrolls past the fixed centre notch.
  int tickCount = 27;
  if (m_HeadingCommands.IsEmpty())
  {
   for (int t = 0; t < tickCount; t++)
   {
    LineDrawCommand tick = new LineDrawCommand();
    tick.m_Vertices = {0, 0, 0, 0};
    m_HeadingCommands.Insert(tick);
   }
  }
  int startDegrees = Math.Floor(bearing / 5) * 5 - 65;
  for (int j = 0; j < tickCount; j++)
  {
   LineDrawCommand line = LineDrawCommand.Cast(m_HeadingCommands[j]);
   float position = 173 + (startDegrees + j * 5 - bearing) * 2.5;
   bool centre = j == tickCount - 1;
   if (centre) position = 173;
   line.m_iColor = 0x70566570;
   if (centre) line.m_iColor = 0xffa2b6c1;
   line.m_fWidth = Math.Max(1.2 * scale, 1);
   line.m_Vertices[0] = (position - 2.5) * scale;
   line.m_Vertices[1] = 28 * scale;
   line.m_Vertices[2] = (position + 2.5) * scale;
   line.m_Vertices[3] = 28 * scale;
   if (centre)
   {
    line.m_Vertices[0] = position * scale;
    line.m_Vertices[1] = 24 * scale;
    line.m_Vertices[2] = position * scale;
    line.m_Vertices[3] = 33 * scale;
   }
  }
  m_HeadingTicks.SetDrawCommands(m_HeadingCommands);
 }

 protected void UpdateSectorTrack(BLCA_RushGameModeComponent mode)
 {
  if (!m_SectorTrack) return;
  int count = mode.SectorCount();
  int index = mode.SectorIndex();
  int team = mode.TeamOfPlayer(SCR_PlayerController.GetLocalPlayerId());
  bool victory = mode.Phase() >= BLCA_RushPhase.ROUND_ENDING && mode.Winner() == 0;
  float scale = GetGame().GetWorkspace().DPIScale(1);
  if (count == m_TrackSectors && index == m_TrackSector && team == m_TrackTeam && victory == m_TrackVictory && scale == m_TrackScale) return;
  m_TrackSectors = count;
  m_TrackSector = index;
  m_TrackTeam = team;
  m_TrackVictory = victory;
  m_TrackScale = scale;
  m_TrackCommands.Clear();

  if (count > 0)
  {
   float step = 344.0 / count;
   float notch = Math.Min(6, step * 0.24);
   for (int i = 0; i < count; i++)
   {
    float left = 88 + step * i;
    float right = left + step - 3;
    float inset;
    if (i > 0) inset = notch;
    float tip = notch;
    if (i == count - 1) tip = 0;
    int color = 0xff22516b;
    if (i == index) color = 0xff4295ba;
    if (i < index || victory) color = 0xff172d3a;
    HudPolygon(m_TrackCommands, color, scale, {left, 10, right-tip, 10, right, 16, left+inset, 16});
    HudPolygon(m_TrackCommands, color, scale, {left+inset, 16, right, 16, right-tip, 22, left, 22});
   }
  }

  if (team == 1)
  {
   HudPolygon(m_TrackCommands, 0xff62c7d4, scale, {480, 5, 489, 9, 489, 19, 480, 26, 471, 19, 471, 9});
   HudPolygon(m_TrackCommands, 0xff102b3b, scale, {480, 7, 487, 10, 487, 18, 480, 23, 473, 18, 473, 10});
   HudPolygon(m_TrackCommands, 0xff62c7d4, scale, {480, 10, 484, 12, 484, 17, 480, 21, 476, 17, 476, 12});
  }
  else
  {
   HudPolygon(m_TrackCommands, 0xff62c7d4, scale, {480, 5, 490, 16, 480, 27, 470, 16});
   HudPolygon(m_TrackCommands, 0xff102b3b, scale, {480, 8, 487, 16, 480, 24, 473, 16});
   HudPolygon(m_TrackCommands, 0xff62c7d4, scale, {480, 12, 484, 16, 480, 20, 476, 16});
  }
  m_SectorTrack.SetDrawCommands(m_TrackCommands);
 }

 protected void DrawObjectiveBadge(CanvasWidget canvas, int badgeIndex, BLCA_ObjectiveState state, float size)
 {
  if (!canvas) return;
  float scale = GetGame().GetWorkspace().DPIScale(size);
  int color = 0xff728894;
  if (state == BLCA_ObjectiveState.ACTIVE || state == BLCA_ObjectiveState.DEFUSED) color = 0xff70cbd8;
  if (state == BLCA_ObjectiveState.ARMING || state == BLCA_ObjectiveState.DEFUSING) color = 0xffdfa56c;
  if (state == BLCA_ObjectiveState.ARMED) color = 0xffef866c;
  if (state == BLCA_ObjectiveState.DESTROYED) color = 0xff47515c;
  if (m_BadgeColors[badgeIndex] == color && m_BadgeScales[badgeIndex] == scale) return;
  m_BadgeColors[badgeIndex] = color;
  m_BadgeScales[badgeIndex] = scale;
  array<ref CanvasWidgetCommand> commands = m_BadgeCommands[badgeIndex];
  commands.Clear();
  HudPolygon(commands, color, scale, {15, 1, 27, 5, 27, 19, 15, 28, 3, 19, 3, 5});
  HudPolygon(commands, 0xee1d2b36, scale, {15, 3, 25, 6.5, 25, 18, 15, 25.5, 5, 18, 5, 6.5});
  canvas.SetDrawCommands(commands);
 }

 protected void HudPolygon(array<ref CanvasWidgetCommand> commands, int color, float scale, array<float> points)
 {
  PolygonDrawCommand shape = new PolygonDrawCommand();
  shape.m_Vertices = {};
  foreach (float point : points) shape.m_Vertices.Insert(point * scale);
  shape.m_iColor = color;
  commands.Insert(shape);
 }

 protected void UpdateWeapon(ChimeraCharacter body)
 {
  BaseWeaponManagerComponent weapons = BaseWeaponManagerComponent.Cast(body.FindComponent(BaseWeaponManagerComponent));
  BaseWeaponComponent weapon;
  if (weapons) weapon = weapons.GetCurrentWeapon();
  BaseMuzzleComponent muzzle;
  if (weapon) muzzle = weapon.GetCurrentMuzzle();
  if (!weapon || !muzzle)
  {
   Text("Weapon", "WEAPON HOLSTERED");
   Text("Ammo", "");
   Text("Equipment", "");
   return;
  }
  string name = "Weapon";
  if (weapon.GetUIInfo()) name = WidgetManager.Translate(weapon.GetUIInfo().GetName());
  string mode = weapon.GetCurrentFireModeName();
  if (!mode.IsEmpty()) name += "  /  " + WidgetManager.Translate(mode);
  if (body.GetCharacterController().IsReloading()) name += "  /  RELOADING";
  Text("Weapon", name);
  InventoryStorageManagerComponent storage = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
  array<IEntity> inventory = {};
  if (storage) storage.GetItems(inventory);
  BaseMagazineWell well = muzzle.GetMagazineWell();
  BaseMagazineComponent loaded = muzzle.GetMagazine();
  int reserves, grenades, dressings;
  foreach (IEntity item : inventory)
  {
   if (!item) continue;
   BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
   if (magazine && magazine != loaded && well && magazine.GetMagazineWell() && magazine.GetMagazineWell().IsInherited(well.Type()))
    reserves += magazine.GetAmmoCount();
   SCR_ConsumableItemComponent consumable = SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
   if (consumable && consumable.GetConsumableType() == SCR_EConsumableType.BANDAGE) dressings++;
   if (!item.GetPrefabData()) continue;
   string prefab = item.GetPrefabData().GetPrefabName();
   if (prefab.Contains("/Grenades/Grenade_M67.et") || prefab.Contains("/Grenades/Grenade_RGD5.et")) grenades++;
   if (!well && prefab == muzzle.GetDefaultMagazineOrProjectileName()) reserves++;
  }
  Text("Ammo", string.Format("%1  /  %2", muzzle.GetAmmoCount(), reserves));
  Text("Equipment", string.Format("FRAG  %1      BANDAGE  %2", grenades, dressings));
 }

 protected void UpdateSquad(SCR_AIGroup group)
 {
  int row;
  if (group)
  {
   PlayerManager players = GetGame().GetPlayerManager();
   foreach (int id : group.GetPlayerIDs())
   {
    if (row >= 6) break;
    ChimeraCharacter character = ChimeraCharacter.Cast(players.GetPlayerControlledEntity(id));
    CharacterControllerComponent controller;
    if (character) controller = character.GetCharacterController();
    bool deployed = controller && !controller.IsDead();
    SetSquadRow(row, players.GetPlayerName(id), deployed, deployed && controller.IsUnconscious(), group.IsPlayerLeader(id));
    row++;
   }
  }
  for (; row < 6; row++) SetSquadRow(row, "", false, false, false);
 }

 protected void SetSquadRow(int row, string name, bool deployed, bool down, bool leader)
 {
  TextWidget label = FindText("Squad" + row);
  if (label)
  {
   label.SetText(name);
   if (deployed) label.SetColorInt(0xff85ff85);
   else label.SetColorInt(0xffb7bfc7);
  }
  string status;
  if (down) status = "DOWN";
  Text("SquadStatus" + row, status);
  Show("SquadStatusSlot" + row, down);
  Show("SquadLeader" + row, leader && !name.IsEmpty());
 }

 protected void UpdatePrompt(BLCA_RushGameModeComponent mode, IEntity body)
 {
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  string prompt;
  if (sector && mode.Phase() == BLCA_RushPhase.ROUND_ACTIVE)
  {
   if (BLCA_RushCombatBoundary.RequiresReturn(mode, sector, body)) prompt = "";
   else if (BLCA_RushCombatBoundary.IsProtectedSpawn(body.GetOrigin())) prompt = "SPAWN PROTECTION / WEAPONS DISABLED";
   else
   {
    BLCA_RushObjectiveComponent terminal = sector.A;
    if (!terminal.InRange(body)) terminal = sector.B;
    if (terminal.InRange(body) && mode.ObjectiveSeconds(terminal.Letter) > 0)
     prompt = "M-COM " + BLCA_RushObjectiveComponent.Label(terminal.Letter) + "   /   " + Clock(mode.ObjectiveSeconds(terminal.Letter));
   }
  }
  Text("Prompt", prompt);
 }

 protected void UpdateMapRoster(BLCA_RushGameModeComponent mode, IEntity body)
 {
  int localId = SCR_PlayerController.GetLocalPlayerId();
  m_Group = BLCA_RushSquadSpawnPoint.PlayerGroup(localId);
  m_MapRosterBody = body;
  m_MapTeamBodies.Clear();
  m_MapSquadBodies.Clear();
  int localTeam = mode.TeamOfEntity(body);
  if (!m_Settings.Minimap || localTeam < 0) return;

  PlayerManager players = GetGame().GetPlayerManager();
  m_MapPlayerIds.Clear();
  players.GetPlayers(m_MapPlayerIds);
  foreach (int id : m_MapPlayerIds)
  {
   if (id == localId) continue;
   IEntity teammate = players.GetPlayerControlledEntity(id);
   if (!BLCA_RushGameModeComponent.CanInteract(teammate) || mode.TeamOfEntity(teammate) != localTeam) continue;
   if (m_Group && m_Group.IsPlayerInGroup(id))
    m_MapSquadBodies.Insert(teammate);
   else
    m_MapTeamBodies.Insert(teammate);
  }
 }

 protected void UpdateMap(BLCA_RushGameModeComponent mode, IEntity body, float timeSlice)
 {
  SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
  if (!mapEntity || !m_MapImage) return;
  vector size = mapEntity.Size();
  vector offset = mapEntity.Offset();
  float half = Math.Min(m_Settings.MapHalfRange, Math.Min(size[0], size[2]) * 0.5);
  if (half <= 0) return;

  vector player = body.GetOrigin();
  bool resetFollow = m_MapBody != body || vector.DistanceXZ(m_MapCentre, player) > half;
  float follow = Math.Clamp(timeSlice * 15, 0, 1);
  if (resetFollow)
   m_MapCentre = player;
  else
   m_MapCentre = vector.Lerp(m_MapCentre, player, follow);
  m_MapBody = body;
  vector centre = m_MapCentre;
  float mapHalf = MAP_SIZE * 0.5;
  float pixelsPerMeter = mapHalf / half;

  vector camera[4];
  GetGame().GetWorld().GetCurrentCamera(camera);
  float heading = m_MapYaw;
  if (camera[2][0] * camera[2][0] + camera[2][2] * camera[2][2] > 0.001)
   heading = Math.Atan2(camera[2][0], camera[2][2]) * Math.RAD2DEG;
  if (resetFollow) m_MapYaw = heading;
  else
  {
   float turn = heading - m_MapYaw;
   if (turn > 180) turn -= 360;
   if (turn < -180) turn += 360;
   m_MapYaw += turn * follow;
   if (m_MapYaw > 180) m_MapYaw -= 360;
   if (m_MapYaw < -180) m_MapYaw += 360;
  }
  float radians = m_MapYaw * Math.PI / 180;
  m_MapCos = Math.Cos(radians);
  m_MapSin = Math.Sin(radians);
  float sourceX = centre[0] - offset[0];
  float sourceY = offset[2] + size[2] - centre[2];
  vector extent = Vector(size[0] * pixelsPerMeter, size[2] * pixelsPerMeter, 0);
  if (extent != m_MapExtent)
  {
   m_MapExtent = extent;
   FrameSlot.SetSize(m_MapImage, extent[0], extent[1]);
   if (m_MapDetail) FrameSlot.SetSize(m_MapDetail, extent[0], extent[1]);
  }
  PlaceMapLayer(m_MapImage, sourceX, sourceY, size, pixelsPerMeter);
  if (m_MapDetail) PlaceMapLayer(m_MapDetail, sourceX, sourceY, size, pixelsPerMeter);
  float mapWidth, mapHeight;
  m_Map.GetScreenSize(mapWidth, mapHeight);
  m_MapDrawScale = Math.Min(mapWidth, mapHeight) / MAP_SIZE;
  if (m_MapDrawScale <= 0) return;
  m_MapCommands.Clear();
  m_RectangleCount = 0;
  m_DotCount = 0;
  m_LineCount = 0;

  // Cover the corners of the rotated viewport, then clip at the minimap frame.
  float span = half * 1.42;
  UpdateMapRoads(centre, span, resetFollow);
  for (int roadIndex = 0; roadIndex < m_MapRoadWidths.Count(); roadIndex++)
  {
   int startIndex = roadIndex * 2;
   float width = m_MapRoadWidths[roadIndex];
   int roadColor = 0xff535c69;
   if (width >= 4) roadColor = 0xff697789;
   DrawMapLine(m_MapRoadPoints[startIndex], m_MapRoadPoints[startIndex + 1], pixelsPerMeter, roadColor, Math.Clamp(width * pixelsPerMeter * 0.6, 1.0, 3.6));
  }
  float gridX = Math.Ceil((centre[0] - span) / MAP_GRID_METRES) * MAP_GRID_METRES;
  float gridZ = Math.Ceil((centre[2] - span) / MAP_GRID_METRES) * MAP_GRID_METRES;
  for (float east = gridX; east < centre[0] + span; east += MAP_GRID_METRES)
   MapGridLine(Vector(east, 0, centre[2] - span), Vector(east, 0, centre[2] + span), pixelsPerMeter);
  for (float north = gridZ; north < centre[2] + span; north += MAP_GRID_METRES)
   MapGridLine(Vector(centre[0] - span, 0, north), Vector(centre[0] + span, 0, north), pixelsPerMeter);

  BLCA_RushSectorComponent sector = BLCA_RushMapBoundary.VisibleSector(mode);
  if (sector)
  {
   if (sector != m_MapBoundarySector)
   {
    m_MapBoundarySector = sector;
    sector.BuildBoundaryOutline(m_MapBoundary);
   }
   m_MapBoundaryProjected.Clear();
   foreach (vector point : m_MapBoundary)
    m_MapBoundaryProjected.Insert(ProjectMapPoint(point, pixelsPerMeter) * m_MapDrawScale);
   vector sectorCenter = ProjectMapPoint(sector.GetOwner().GetOrigin(), pixelsPerMeter) * m_MapDrawScale;
   m_MapBoundaryShade.Draw(m_MapBoundaryProjected, sectorCenter, mapWidth, mapHeight, 27 * m_MapDrawScale, m_MapCommands);
   for (int edge = 0; edge < m_MapBoundary.Count(); edge++)
   {
    int nextEdge = (edge + 1) % m_MapBoundary.Count();
    DrawMapLine(m_MapBoundary[edge], m_MapBoundary[nextEdge], pixelsPerMeter, BLCA_RushMapBoundary.BORDER_SHADOW, 6.5);
    DrawMapLine(m_MapBoundary[edge], m_MapBoundary[nextEdge], pixelsPerMeter, BLCA_RushMapBoundary.BORDER_RED, 4);
   }
  }
  sector = mode.ActiveSector();
  if (sector)
  {
   DrawMapObjective("MapObjectiveA", sector.A.GetOwner().GetOrigin(), player, pixelsPerMeter, mode.ObjectiveState(0), mode.ObjectiveSeconds(0));
   DrawMapObjective("MapObjectiveB", sector.B.GetOwner().GetOrigin(), player, pixelsPerMeter, mode.ObjectiveState(1), mode.ObjectiveSeconds(1));
  }
  else
  {
   Show("MapObjectiveA", false);
   Show("MapObjectiveB", false);
  }

  foreach (IEntity teammate : m_MapTeamBodies)
   if (teammate && !teammate.IsDeleted()) MapDot(teammate.GetOrigin(), pixelsPerMeter, 3.3, TEAM_BLUE);
  foreach (IEntity squadmate : m_MapSquadBodies)
   if (squadmate && !squadmate.IsDeleted()) MapDot(squadmate.GetOrigin(), pixelsPerMeter, 3.8, SQUAD_GREEN);
  MapDot(player, pixelsPerMeter, 4.2, PLAYER_RED);
  m_Map.SetDrawCommands(m_MapCommands);
  UpdateCompass();
 }

 protected void PlaceMapLayer(ImageWidget layer, float sourceX, float sourceY, vector mapSize, float pixelsPerMeter)
 {
  float mapHalf = MAP_SIZE * 0.5;
  FrameSlot.SetPos(layer, mapHalf - sourceX * pixelsPerMeter, mapHalf - sourceY * pixelsPerMeter);
  layer.SetPivot(sourceX / mapSize[0], sourceY / mapSize[2]);
  layer.SetRotation(-m_MapYaw);
 }

 protected vector ProjectMapPoint(vector point, float pixelsPerMeter)
 {
  float mapHalf = MAP_SIZE * 0.5;
  float east = (point[0] - m_MapCentre[0]) * pixelsPerMeter;
  float south = (m_MapCentre[2] - point[2]) * pixelsPerMeter;
  return Vector(mapHalf + east * m_MapCos + south * m_MapSin, mapHalf - east * m_MapSin + south * m_MapCos, 0);
 }

 protected void MapGridLine(vector start, vector end, float pixelsPerMeter)
 {
  DrawMapLine(start, end, pixelsPerMeter, 0x38535f6b, 0.7);
 }

 protected void UpdateMapRoads(vector centre, float span, bool resetFollow)
 {
  float now = BLCA_RushGameModeComponent.Now();
  if (!resetFollow && now < m_MapRoadRefreshAt && span == m_MapRoadHalf && vector.DistanceSqXZ(centre, m_MapRoadCentre) < 20 * 20) return;
  m_MapRoadCentre = centre;
  m_MapRoadHalf = span;
  m_MapRoadRefreshAt = now + 5000;
  m_MapRoadPoints.Clear();
  m_MapRoadWidths.Clear();
  ChimeraAIWorld ai = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
  if (!ai) return;
  RoadNetworkManager roads = ai.GetRoadNetworkManager();
  if (!roads) return;
  float reach = span + 25;
  vector lower = centre - Vector(reach, 10000, reach);
  vector upper = centre + Vector(reach, 10000, reach);
  array<BaseRoad> nearby = {};
  roads.GetRoadsInAABB(lower, upper, nearby);
  array<vector> points = {};
  foreach (BaseRoad road : nearby)
  {
   if (!road) continue;
   float width = road.GetWidth();
   if (width <= 0) continue;
   points.Clear();
   road.GetPoints(points);
   for (int node = 1; node < points.Count(); node++)
   {
    vector a = points[node - 1];
    vector b = points[node];
    if (Math.Max(a[0], b[0]) < lower[0] || Math.Min(a[0], b[0]) > upper[0]) continue;
    if (Math.Max(a[2], b[2]) < lower[2] || Math.Min(a[2], b[2]) > upper[2]) continue;
    m_MapRoadPoints.Insert(a);
    m_MapRoadPoints.Insert(b);
    m_MapRoadWidths.Insert(width);
    if (m_MapRoadWidths.Count() >= 192) return;
   }
  }
 }

 protected void DrawMapLine(vector start, vector end, float pixelsPerMeter, int color, float width)
 {
  vector first = ProjectMapPoint(start, pixelsPerMeter);
  vector last = ProjectMapPoint(end, pixelsPerMeter);
  if (Math.Max(first[0], last[0]) < 0 || Math.Min(first[0], last[0]) > MAP_SIZE) return;
  if (Math.Max(first[1], last[1]) < 0 || Math.Min(first[1], last[1]) > MAP_SIZE) return;
  if (m_LineCount == m_MapLines.Count())
  {
   LineDrawCommand line = new LineDrawCommand();
   line.m_Vertices = {0, 0, 0, 0};
   m_MapLines.Insert(line);
  }
  LineDrawCommand command = m_MapLines[m_LineCount++];
  command.m_iColor = color;
  command.m_fWidth = Math.Max(width * m_MapDrawScale, 0.6);
  command.m_Vertices[0] = first[0] * m_MapDrawScale;
  command.m_Vertices[1] = first[1] * m_MapDrawScale;
  command.m_Vertices[2] = last[0] * m_MapDrawScale;
  command.m_Vertices[3] = last[1] * m_MapDrawScale;
  m_MapCommands.Insert(command);
 }

 protected void UpdateCompass()
 {
  int heading = Math.Round(m_MapYaw + 360);
  if (heading >= 360) heading -= 360;
  if (heading != m_LastHeading)
  {
   m_LastHeading = heading;
   string degrees = heading.ToString();
   if (heading < 10) degrees = "00" + degrees;
   else if (heading < 100) degrees = "0" + degrees;
   int direction = Math.Round(heading / 45.0);
   if (direction == m_CompassDirections.Count()) direction = 0;
   Text("MapHeading", string.Format("%1  %2°   ·   25 M GRID", m_CompassDirections[direction], degrees));
  }
  float centre = MAP_SIZE * 0.5;
  for (int i; i < m_CompassLabels.Count(); i++)
  {
   TextWidget label = m_CompassLabels[i];
   if (!label) continue;
   float bearing = (i * 90 - m_MapYaw) * Math.PI / 180;
   FrameSlot.SetPos(label, centre + Math.Sin(bearing) * (centre - 12) - 10, centre - Math.Cos(bearing) * (centre - 12) - 9);
  }
 }

 protected void DrawMapObjective(string name, vector point, vector player, float pixelsPerMeter, BLCA_ObjectiveState state, int seconds)
 {
  TextWidget label = FindText(name);
  if (!label) return;
  vector projected = ProjectMapPoint(point, pixelsPerMeter);
  float x = projected[0];
  float y = projected[1];
  bool visible = x > 9 && y > 9 && x < MAP_SIZE - 9 && y < MAP_SIZE - 9;
  label.SetVisible(visible);
  if (!visible) return;
  vector playerPoint = ProjectMapPoint(player, pixelsPerMeter);
  float dx = x - playerPoint[0];
  float dy = y - playerPoint[1];
  float distance = Math.Sqrt(dx * dx + dy * dy);
  // Keep the letter readable when the player is standing on the objective.
  if (distance < 17)
  {
   if (distance < 0.1)
   {
    dx = 0;
    dy = -1;
    distance = 1;
   }
   x = playerPoint[0] + 17 * dx / distance;
   y = playerPoint[1] + 17 * dy / distance;
  }
  int color = 0xfff6ca85;
  if (state == BLCA_ObjectiveState.DESTROYED) color = 0xffa6afb8;
  else if (seconds > 0) color = 0xffff7d72;
  Rectangle(x - 9, y - 9, 18, 18, DOT_OUTLINE);
  Rectangle(x - 7, y - 7, 14, 14, color);
  FrameSlot.SetPos(label, x - 7, y - 7);
 }

 protected void MapDot(vector point, float pixelsPerMeter, float radius, int color)
 {
  vector projected = ProjectMapPoint(point, pixelsPerMeter);
  float edge = radius + 1.3;
  if (projected[0] < edge || projected[1] < edge || projected[0] > MAP_SIZE - edge || projected[1] > MAP_SIZE - edge) return;
  DrawMapDot(projected, edge, DOT_OUTLINE);
  DrawMapDot(projected, radius, color);
 }

 protected void DrawMapDot(vector position, float radius, int color)
 {
  if (m_DotCount == m_MapDots.Count())
  {
   PolygonDrawCommand dot = new PolygonDrawCommand();
   dot.m_Vertices = {};
   dot.m_Vertices.Resize(DOT_VERTICES.Count());
   m_MapDots.Insert(dot);
  }
  PolygonDrawCommand command = m_MapDots[m_DotCount++];
  command.m_iColor = color;
  for (int vertex = 0; vertex < DOT_VERTICES.Count(); vertex += 2)
  {
   command.m_Vertices[vertex] = (position[0] + DOT_VERTICES[vertex] * radius) * m_MapDrawScale;
   command.m_Vertices[vertex + 1] = (position[1] + DOT_VERTICES[vertex + 1] * radius) * m_MapDrawScale;
  }
  m_MapCommands.Insert(command);
 }

 protected void Rectangle(float x, float y, float width, float height, int color)
 {
  if (m_RectangleCount == m_MapRectangles.Count())
  {
   PolygonDrawCommand rectangle = new PolygonDrawCommand();
   rectangle.m_Vertices = {0, 0, 0, 0, 0, 0, 0, 0};
   m_MapRectangles.Insert(rectangle);
  }
  PolygonDrawCommand command = m_MapRectangles[m_RectangleCount];
  m_RectangleCount++;
  command.m_iColor = color;
  x *= m_MapDrawScale;
  y *= m_MapDrawScale;
  width *= m_MapDrawScale;
  height *= m_MapDrawScale;
  command.m_Vertices[0] = x;
  command.m_Vertices[1] = y;
  command.m_Vertices[2] = x + width;
  command.m_Vertices[3] = y;
  command.m_Vertices[4] = x + width;
  command.m_Vertices[5] = y + height;
  command.m_Vertices[6] = x;
  command.m_Vertices[7] = y + height;
  m_MapCommands.Insert(command);
 }

 protected void Show(string name, bool visible)
 {
  Widget widget = m_Root.FindAnyWidget(name);
  if (widget) widget.SetVisible(visible);
 }

 protected TextWidget FindText(string name)
 {
  TextWidget widget;
  if (!m_Text.Find(name, widget))
  {
   widget = TextWidget.Cast(m_Root.FindAnyWidget(name));
   m_Text.Set(name, widget);
  }
  return widget;
 }

 protected void Text(string name, string value)
 {
  TextWidget widget = FindText(name);
  if (widget) widget.SetText(value);
 }

 protected static string Clock(int seconds)
 {
  int minutes = seconds / 60;
  int leftover = seconds % 60;
  string spacer = ":";
  if (leftover < 10) spacer = ":0";
  return minutes.ToString() + spacer + leftover.ToString();
 }
}

modded class SCR_WeaponInfo
{
 override void Show(bool show, float speed = UIConstants.FADE_RATE_INSTANT, EAnimationCurve curve = EAnimationCurve.LINEAR)
 {
  if (BLCA_RushGameModeComponent.Current()) show = false;
  super.Show(show, speed, curve);
 }
}

modded class SCR_NameTagDisplay
{
 protected override bool CanDisplayNameTag(notnull IEntity entity)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode) return super.CanDisplayNameTag(entity);
  IEntity player = SCR_PlayerController.GetLocalControlledEntity();
  int team = mode.TeamOfEntity(player);
  if (team < 0 || mode.TeamOfEntity(entity) != team) return false;
  return GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity) > 0;
 }
}

[BaseContainerProps(), SCR_NameTagZoneTitle()]
modded class SCR_NameTagZone
{
 override void DrawElements(SCR_NameTagData data)
 {
  super.DrawElements(data);
  if (!BLCA_RushGameModeComponent.Current() || !data || data.m_iPlayerID <= 0 || data.m_bIsCurrentPlayer) return;
  SCR_AIGroup squad = BLCA_RushSquadSpawnPoint.PlayerGroup(SCR_PlayerController.GetLocalPlayerId());
  SCR_AIGroup playerSquad = BLCA_RushSquadSpawnPoint.PlayerGroup(data.m_iPlayerID);
  int color = BLCA_RushHUDController.TEAM_BLUE;
  if (squad && squad == playerSquad) color = BLCA_RushHUDController.SQUAD_GREEN;
  for (int i; i < Math.Min(m_iElementsCount, 10); i++)
  {
   TextWidget label = TextWidget.Cast(data.m_aNametagElements[i]);
   if (!label) continue;
   label.SetColorInt(color);
   if (data.m_eType != ENameTagEntityType.PLAYER || m_aElements[i].m_sWidgetName != "PlayerName" || data.m_sName.IsEmpty()) continue;
   string name = data.m_sName;
   if (playerSquad && playerSquad.IsPlayerLeader(data.m_iPlayerID)) name = "* " + name;
   if (label.GetText() != name) label.SetText(name);
  }
 }
}

modded class ArmaReforgerScripted
{
 protected ref BLCA_RushHUDController m_BLCA_RushHUD;

 override void OnUpdate(BaseWorld world, float timeslice)
 {
  super.OnUpdate(world, timeslice);
  if (System.IsConsoleApp()) return;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (mode && world)
  {
   if (!m_BLCA_RushHUD) m_BLCA_RushHUD = new BLCA_RushHUDController();
   m_BLCA_RushHUD.Tick(mode, timeslice);
  }
  else if (m_BLCA_RushHUD)
  {
   m_BLCA_RushHUD.Close();
   m_BLCA_RushHUD = null;
  }
 }
}
