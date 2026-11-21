class BLCA_RushMapMarkers
{
 static const int OBJECTIVE_A_MARKER = 31001;
 static const int OBJECTIVE_B_MARKER = 31002;

 protected ref array<ref SCR_MapMarkerBase> m_Markers = {};
 protected int m_LastSector = -1;
 protected BLCA_ObjectiveState m_LastA;
 protected BLCA_ObjectiveState m_LastB;

 void Update(BLCA_RushGameModeComponent mode)
 {
  if (!Replication.IsServer()) return;
  BLCA_RushSectorComponent sector = mode.ActiveSector();
  if (!sector || !mode.IsActiveSector(sector.Index))
  {
   Clear();
   return;
  }
  BLCA_ObjectiveState stateA = mode.ObjectiveState(0);
  BLCA_ObjectiveState stateB = mode.ObjectiveState(1);
  if (m_LastSector == sector.Index && stateA == m_LastA && stateB == m_LastB) return;
  SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
  if (!manager || !manager.GetMarkerConfig()) return;
  SCR_MapMarkerEntryPlaced placed = SCR_MapMarkerEntryPlaced.Cast(manager.GetMarkerConfig().GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
  if (!placed) return;
  Clear();
  Insert(manager, placed, sector.A, stateA);
  Insert(manager, placed, sector.B, stateB);
  m_LastSector = sector.Index;
  m_LastA = stateA;
  m_LastB = stateB;
 }

 protected void Insert(SCR_MapMarkerManagerComponent manager, SCR_MapMarkerEntryPlaced placed, BLCA_RushObjectiveComponent terminal, BLCA_ObjectiveState state)
 {
  if (!terminal || !terminal.GetOwner() || state == BLCA_ObjectiveState.DESTROYED) return;
  bool armed = state == BLCA_ObjectiveState.ARMED || state == BLCA_ObjectiveState.DEFUSING;
  string icon = "point-of-interest";
  string color = "ping";
  string title = "M-COM " + BLCA_RushObjectiveComponent.Label(terminal.Letter);
  if (armed)
  {
   icon = "attack";
   color = "red";
   title += " / ARMED";
  }
  int iconId = -1;
  foreach (int i, SCR_MarkerIconEntry entry : placed.GetIconEntries())
  {
   ResourceName image, glow;
   string quad;
   entry.GetIconResource(image, glow, quad);
   if (quad == icon) { iconId = i; break; }
  }
  if (iconId < 0) return;
  SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
  vector position = terminal.GetOwner().GetOrigin();
  marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
  int objectiveMarker = OBJECTIVE_A_MARKER;
  if (terminal.Letter != 0) objectiveMarker = OBJECTIVE_B_MARKER;
  marker.SetMarkerConfigID(objectiveMarker);
  marker.SetWorldPos(Math.Round(position[0]), Math.Round(position[2]));
  marker.SetIconEntry(iconId);
  marker.SetCustomText(title);
  marker.SetCanBeRemovedByOwner(false);
  foreach (int j, SCR_MarkerColorEntry entry : placed.GetColorEntries())
  {
   if (entry.GetName() == color) { marker.SetColorEntry(j); break; }
  }
  FactionManager factions = GetGame().GetFactionManager();
  Faction attacker = factions.GetFactionByKey("US");
  Faction defender = factions.GetFactionByKey("USSR");
  if (attacker) marker.AddMarkerFactionFlags(factions.GetFactionIndex(attacker));
  if (defender) marker.AddMarkerFactionFlags(factions.GetFactionIndex(defender));
  manager.InsertStaticMarker(marker, false, true);
  m_Markers.Insert(marker);
 }

 void Clear()
 {
  if (m_Markers.IsEmpty()) return;
  SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
  if (manager)
  {
   foreach (SCR_MapMarkerBase marker : m_Markers) manager.RemoveStaticMarker(marker);
  }
  m_Markers.Clear();
  m_LastSector = -1;
 }
}

modded class SCR_MapMarkerWidgetComponent
{
 protected Widget m_BLCA_RushMapBorder;
 protected ImageWidget m_BLCA_RushMapPin;

 override void SetImage(ResourceName icon, string quad, float aspectRatio = 1)
 {
  super.SetImage(icon, quad, aspectRatio);
  if (m_MarkerObject && m_MarkerObject.GetType() == SCR_EMapMarkerType.PLACED_CUSTOM && BLCA_RushGameModeComponent.Current())
   BLCA_StyleRushMarker(m_MarkerObject, icon, quad);
 }

 protected void BLCA_StyleRushMarker(SCR_MapMarkerBase marker, ResourceName imageset, string quad)
 {
  if (m_BLCA_RushMapBorder || !m_wRoot || !m_wMarkerIcon || !m_wMarkerGlowIcon) return;
  string letter;
  if (marker.GetMarkerOwnerID() < 0)
  {
   if (marker.GetMarkerConfigID() == BLCA_RushMapMarkers.OBJECTIVE_A_MARKER) letter = "A";
   else if (marker.GetMarkerConfigID() == BLCA_RushMapMarkers.OBJECTIVE_B_MARKER) letter = "B";
   else return;
  }
  else if (quad != "point-of-interest" && quad != "point-of-interest-2" && quad != "point-of-interest-3") return;

  Widget badge = GetGame().GetWorkspace().CreateWidgets("{82EE1B88AF3056B7}UI/layouts/BLCA_Rush/BLCA_MapBadge.layout", m_wRoot);
  if (!badge) return;

  TextWidget label = TextWidget.Cast(badge.FindAnyWidget("Letter"));
  ImageWidget pin = ImageWidget.Cast(badge.FindAnyWidget("Pin"));
  Widget border = badge.FindAnyWidget("Border");
  Widget fill = badge.FindAnyWidget("Badge");
  if (!label || !pin || !border || !fill)
  {
   badge.RemoveFromHierarchy();
   return;
  }
  if (letter.IsEmpty())
  {
   if (!pin.LoadImageFromSet(0, imageset, quad))
   {
    badge.RemoveFromHierarchy();
    return;
   }
   pin.SetImage(0);
   pin.SetRotation(marker.GetRotation());
   pin.SetVisible(true);
   m_BLCA_RushMapPin = pin;
  }
  else
  {
   label.SetText(letter);
   label.SetVisible(true);
   if (quad == "attack") fill.SetColorInt(0xffffad78);
  }

  // Keep native layout measurements and input hit targets while hiding its glyph.
  m_wMarkerIcon.SetOpacity(0);
  m_wMarkerGlowIcon.SetOpacity(0);
  m_BLCA_RushMapBorder = border;
 }

 override void SetRotation(float angle)
 {
  super.SetRotation(angle);
  if (m_BLCA_RushMapPin) m_BLCA_RushMapPin.SetRotation(angle);
 }

 override bool OnMouseEnter(Widget w, int x, int y)
 {
  bool handled = super.OnMouseEnter(w, x, y);
  if (m_BLCA_RushMapBorder) m_BLCA_RushMapBorder.SetColorInt(0xffe5f8ff);
  return handled;
 }

 override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
 {
  bool handled = super.OnMouseLeave(w, enterW, x, y);
  if (m_BLCA_RushMapBorder) m_BLCA_RushMapBorder.SetColorInt(0xe4193546);
  return handled;
 }
}
