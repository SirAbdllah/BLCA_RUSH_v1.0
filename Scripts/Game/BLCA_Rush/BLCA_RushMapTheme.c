class BLCA_RushMapBoundary
{
 static const int OUTSIDE_SHADE = 0x9924272c;
 static const int BORDER_SHADOW = 0xe827161b;
 static const int BORDER_RED = 0xffe6464b;
 protected static const float HATCH_SLOPE = 0.46;
 protected ref array<vector> m_Outer = {};
 protected ref array<ref PolygonDrawCommand> m_Panels = {};
 protected ref array<ref LineDrawCommand> m_Hatches = {};
 protected ref array<vector> m_Quad = {vector.Zero, vector.Zero, vector.Zero, vector.Zero};
 protected int m_HatchCount;

 void Draw(notnull array<vector> outline, vector center, float width, float height, float hatchSpacing, notnull array<ref CanvasWidgetCommand> commands)
 {
  int count = outline.Count();
  if (count < 3 || width <= 0 || height <= 0) return;

  float reachX = Math.Max(Math.AbsFloat(center[0]), Math.AbsFloat(width - center[0]));
  float reachY = Math.Max(Math.AbsFloat(center[1]), Math.AbsFloat(height - center[1]));
  float radius = Math.Sqrt(reachX * reachX + reachY * reachY);
  foreach (vector point : outline)
   radius = Math.Max(radius, vector.Distance(center, point));
  radius = radius * 1.04 + 2;

  m_Outer.Clear();
  foreach (vector point : outline)
  {
   vector direction = point - center;
   direction.Normalize();
   m_Outer.Insert(center + direction * radius);
  }

  while (m_Panels.Count() < count)
  {
   PolygonDrawCommand panel = new PolygonDrawCommand();
   panel.m_iColor = OUTSIDE_SHADE;
   panel.m_Vertices = {0, 0, 0, 0, 0, 0, 0, 0};
   m_Panels.Insert(panel);
  }
  m_HatchCount = 0;
  for (int edge = 0; edge < count; edge++)
  {
   int next = (edge + 1) % count;
   vector a = outline[edge];
   vector b = outline[next];
   vector farA = m_Outer[edge];
   vector farB = m_Outer[next];
   if (Math.Max(Math.Max(a[0], b[0]), Math.Max(farA[0], farB[0])) < 0 || Math.Min(Math.Min(a[0], b[0]), Math.Min(farA[0], farB[0])) > width) continue;
   if (Math.Max(Math.Max(a[1], b[1]), Math.Max(farA[1], farB[1])) < 0 || Math.Min(Math.Min(a[1], b[1]), Math.Min(farA[1], farB[1])) > height) continue;
   PolygonDrawCommand panel = m_Panels[edge];
   panel.m_Vertices[0] = a[0];
   panel.m_Vertices[1] = a[1];
   panel.m_Vertices[2] = b[0];
   panel.m_Vertices[3] = b[1];
   panel.m_Vertices[4] = farB[0];
   panel.m_Vertices[5] = farB[1];
   panel.m_Vertices[6] = farA[0];
   panel.m_Vertices[7] = farA[1];
   commands.Insert(panel);
   m_Quad[0] = a;
   m_Quad[1] = b;
   m_Quad[2] = farB;
   m_Quad[3] = farA;
   DrawHatch(width, height, hatchSpacing, commands);
  }
 }

 protected void DrawHatch(float width, float height, float spacing, notnull array<ref CanvasWidgetCommand> commands)
 {
  float lowest = height;
  float highest = -HATCH_SLOPE * width;
  foreach (vector point : m_Quad)
  {
   float intercept = point[1] - HATCH_SLOPE * point[0];
   lowest = Math.Min(lowest, intercept);
   highest = Math.Max(highest, intercept);
  }
  lowest = Math.Max(lowest, -HATCH_SLOPE * width);
  highest = Math.Min(highest, height);

  // Intersect each stripe with a convex panel, so the lines stop exactly at the combat border.
  for (float intercept = Math.Ceil(lowest / spacing) * spacing; intercept < highest; intercept += spacing)
  {
   float firstX = width;
   float lastX = 0;
   for (int edge = 0; edge < 4; edge++)
   {
    vector a = m_Quad[edge];
    vector b = m_Quad[(edge + 1) % 4];
    float start = a[1] - HATCH_SLOPE * a[0];
    float end = b[1] - HATCH_SLOPE * b[0];
    if (intercept < Math.Min(start, end) || intercept >= Math.Max(start, end)) continue;
    float x = Math.Lerp(a[0], b[0], (intercept - start) / (end - start));
    firstX = Math.Min(firstX, x);
    lastX = Math.Max(lastX, x);
   }
   firstX = Math.Max(firstX, Math.Max(0, -intercept / HATCH_SLOPE));
   lastX = Math.Min(lastX, Math.Min(width, (height - intercept) / HATCH_SLOPE));
   if (lastX - firstX < 0.5) continue;

   if (m_HatchCount == m_Hatches.Count())
   {
    LineDrawCommand stripe = new LineDrawCommand();
    stripe.m_iColor = 0x99df4049;
    stripe.m_fWidth = 1.6;
    stripe.m_Vertices = {0, 0, 0, 0};
    m_Hatches.Insert(stripe);
   }
   LineDrawCommand stripe = m_Hatches[m_HatchCount++];
   stripe.m_Vertices[0] = firstX;
   stripe.m_Vertices[1] = firstX * HATCH_SLOPE + intercept;
   stripe.m_Vertices[2] = lastX;
   stripe.m_Vertices[3] = lastX * HATCH_SLOPE + intercept;
   commands.Insert(stripe);
  }
 }

 static BLCA_RushSectorComponent VisibleSector(BLCA_RushGameModeComponent mode)
 {
  if (!mode || mode.Phase() >= BLCA_RushPhase.ROUND_ENDING) return null;
  if (mode.Phase() == BLCA_RushPhase.SECTOR_TRANSITION) return mode.BLCA_SectorAt(mode.SectorIndex() + 1);
  return mode.ActiveSector();
 }
}

modded class SCR_MapEntity
{
 override protected void InitLayers(MapConfiguration mapConfig)
 {
  super.InitLayers(mapConfig);
  if (!BLCA_RushGameModeComponent.Current() || !mapConfig) return;
  if (mapConfig.MapEntityMode != EMapEntityMode.FULLSCREEN && mapConfig.MapEntityMode != EMapEntityMode.SPAWNSCREEN) return;

  for (int i = 0; i < mapConfig.LayerCount; i++)
  {
   MapLayer layer = GetLayer(i);
   if (layer) BLCA_StyleRushTerrain(layer, mapConfig);
  }
 }

 protected void BLCA_StyleRushTerrain(MapLayer layer, MapConfiguration mapConfig)
 {
  MapContourProps contour = layer.GetContourProps();
  contour.SetLandColor(Color.FromSRGBA(150, 182, 212, 255));
  contour.SetOceanColor(Color.FromSRGBA(67, 115, 155, 255));
  contour.SetCoastColor(Color.FromSRGBA(54, 96, 133, 255));
  contour.SetMajorColor(Color.FromSRGBA(64, 107, 144, 255));
  layer.GetWaterBodyProps().SetWaterColor(Color.FromSRGBA(78, 138, 180, 255));

  MapGridProps grid = layer.GetGridProps();
  grid.SetLineColor(Color.FromSRGBA(24, 54, 85, 112));
  grid.SetMainGridLineColor(Color.FromSRGBA(23, 53, 82, 205));
  grid.SetTextColor(Color.FromSRGBA(22, 50, 76, 255));

  if (!mapConfig.MapPropsConfig || !mapConfig.MapPropsConfig.m_aMapPropConfigs) return;
  foreach (SCR_MapPropsConfig props : mapConfig.MapPropsConfig.m_aMapPropConfigs)
  {
   SCR_RoadPropsConfig roads = SCR_RoadPropsConfig.Cast(props);
   if (roads && roads.m_aRoadTypes)
   {
    MapRoadProps roadLayer = layer.GetRoadProps();
    foreach (SCR_RoadTypeProps road : roads.m_aRoadTypes)
    {
     if (road.m_Color) roadLayer.SetColor(road.m_iType, BLCA_MapBlue(road.m_Color));
     if (road.m_fOutlineColor) roadLayer.SetBorderColor(road.m_iType, BLCA_MapBlue(road.m_fOutlineColor));
     if (road.m_iStyle == EMapLineType.LT_DASHED && road.m_DashedSecondaryColor)
      roadLayer.SetSecondaryColor(road.m_iType, BLCA_MapBlue(road.m_DashedSecondaryColor));
    }
   }

   SCR_AreaPropsConfig areas = SCR_AreaPropsConfig.Cast(props);
   if (areas && areas.m_aAreaTypes)
   {
    MapAreaProperties areaLayer = layer.GetAreaProps();
    foreach (SCR_AreaTypeProps area : areas.m_aAreaTypes)
    {
     if (area.m_Color) areaLayer.SetAreaColor(area.m_iType, BLCA_MapBlue(area.m_Color));
     if (area.m_ColorIndividual) areaLayer.SetIndividualColor(area.m_iType, BLCA_MapBlue(area.m_ColorIndividual));
    }
   }
  }
 }

 // Keep native brightness and opacity differences between road and terrain types.
 protected Color BLCA_MapBlue(Color source)
 {
  float value = source.R() * 0.21 + source.G() * 0.72 + source.B() * 0.07;
  return new Color(value * 0.61, value * 0.81, Math.Min(1, value * 1.06), source.A());
 }
}

class BLCA_RushSectorMapOverlay
{
 protected CanvasWidget m_BLCA_SectorCanvas;
 protected BLCA_RushSectorComponent m_BLCA_MapSector;
 protected ref array<vector> m_BLCA_Outline = {};
 protected ref array<ref CanvasWidgetCommand> m_BLCA_SectorLines = {};
 protected ref array<ref CanvasWidgetCommand> m_BLCA_SectorCommands = {};
 protected ref array<vector> m_BLCA_ProjectedBoundary = {};
 protected ref BLCA_RushMapBoundary m_BLCA_BoundaryShade = new BLCA_RushMapBoundary();
 protected vector m_BLCA_LastSectorSize;
 protected float m_BLCA_LastSectorZoom = -1;
 protected vector m_BLCA_LastSectorPan;

 void BLCA_RushSectorMapOverlay(Widget frame)
 {
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  if (!frame || !workspace) return;
  m_BLCA_SectorCanvas = CanvasWidget.Cast(workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR, Color.White, 2, frame));
  if (!m_BLCA_SectorCanvas) return;
  FrameSlot.SetAnchorMin(m_BLCA_SectorCanvas, 0, 0);
  FrameSlot.SetAnchorMax(m_BLCA_SectorCanvas, 1, 1);
  FrameSlot.SetOffsets(m_BLCA_SectorCanvas, 0, 0, 0, 0);
 }

 void Close()
 {
  if (m_BLCA_SectorCanvas) m_BLCA_SectorCanvas.RemoveFromHierarchy();
  m_BLCA_SectorCanvas = null;
  m_BLCA_MapSector = null;
  m_BLCA_Outline.Clear();
  m_BLCA_SectorLines.Clear();
  m_BLCA_SectorCommands.Clear();
  m_BLCA_ProjectedBoundary.Clear();
  m_BLCA_LastSectorZoom = -1;
 }

 void Update(SCR_MapEntity mapEntity, BLCA_RushGameModeComponent mode)
 {
  if (!m_BLCA_SectorCanvas || !mapEntity || !mapEntity.IsOpen()) return;
  BLCA_RushSectorComponent sector = BLCA_RushMapBoundary.VisibleSector(mode);
  m_BLCA_SectorCanvas.SetVisible(sector != null);
  if (!sector) return;

  bool newSector = sector != m_BLCA_MapSector;
  if (newSector)
  {
   m_BLCA_MapSector = sector;
   sector.BuildBoundaryOutline(m_BLCA_Outline);
   m_BLCA_SectorLines.Clear();
   for (int i = 0; i < m_BLCA_Outline.Count() * 2; i++)
   {
    LineDrawCommand line = new LineDrawCommand();
    line.m_Vertices = {0, 0, 0, 0};
    if (i < m_BLCA_Outline.Count())
    {
     line.m_iColor = BLCA_RushMapBoundary.BORDER_SHADOW;
     line.m_fWidth = 9;
    }
    else
    {
     line.m_iColor = BLCA_RushMapBoundary.BORDER_RED;
     line.m_fWidth = 5.5;
    }
    m_BLCA_SectorLines.Insert(line);
   }
  }

  float width, height;
  m_BLCA_SectorCanvas.GetScreenSize(width, height);
  vector size = Vector(width, height, 0);
  float zoom = mapEntity.GetCurrentZoom();
  vector pan = mapEntity.GetCurrentPan();
  if (!newSector && zoom == m_BLCA_LastSectorZoom && pan == m_BLCA_LastSectorPan && size == m_BLCA_LastSectorSize) return;
  m_BLCA_LastSectorSize = size;
  m_BLCA_LastSectorZoom = zoom;
  m_BLCA_LastSectorPan = pan;

  m_BLCA_ProjectedBoundary.Clear();
  foreach (vector point : m_BLCA_Outline)
   m_BLCA_ProjectedBoundary.Insert(BLCA_ProjectSectorPoint(mapEntity, point));
  m_BLCA_SectorCommands.Clear();
  vector center = BLCA_ProjectSectorPoint(mapEntity, sector.GetOwner().GetOrigin());
  m_BLCA_BoundaryShade.Draw(m_BLCA_ProjectedBoundary, center, width, height, 60, m_BLCA_SectorCommands);

  int count = m_BLCA_ProjectedBoundary.Count();
  for (int edge = 0; edge < count; edge++)
  {
   vector start = m_BLCA_ProjectedBoundary[edge];
   vector end = m_BLCA_ProjectedBoundary[(edge + 1) % count];
   BLCA_ProjectSectorEdge(LineDrawCommand.Cast(m_BLCA_SectorLines[edge]), start, end);
   BLCA_ProjectSectorEdge(LineDrawCommand.Cast(m_BLCA_SectorLines[count + edge]), start, end);
  }
  foreach (CanvasWidgetCommand line : m_BLCA_SectorLines) m_BLCA_SectorCommands.Insert(line);
  m_BLCA_SectorCanvas.SetDrawCommands(m_BLCA_SectorCommands);
 }

 protected vector BLCA_ProjectSectorPoint(SCR_MapEntity mapEntity, vector point)
 {
  int x, y;
  mapEntity.WorldToScreen(point[0], point[2], x, y, true);
  return Vector(x, y, 0);
 }

 protected void BLCA_ProjectSectorEdge(LineDrawCommand line, vector start, vector end)
 {
  line.m_Vertices[0] = start[0];
  line.m_Vertices[1] = start[1];
  line.m_Vertices[2] = end[0];
  line.m_Vertices[3] = end[1];
 }

}

modded class SCR_MapMenuUI
{
 protected bool m_BLCA_RushMapCloseBound;
 protected int m_BLCA_RushMapInputReadyAt;
 protected ref BLCA_RushSectorMapOverlay m_BLCA_SectorOverlay;

 override void OnMenuOpen()
 {
  super.OnMenuOpen();
  if (!BLCA_RushGameModeComponent.Current()) return;

  InputManager input = GetGame().GetInputManager();
  if (!input) return;
  Widget root = GetRootWidget();
  if (root) m_BLCA_SectorOverlay = new BLCA_RushSectorMapOverlay(root.FindAnyWidget("MapFrame"));
  m_BLCA_RushMapCloseBound = true;
  m_BLCA_RushMapInputReadyAt = System.GetTickCount() + 250;
  input.AddActionListener("GadgetMap", EActionTrigger.DOWN, BLCA_RequestRushMapClose);
  input.AddActionListener("MapEscape", EActionTrigger.DOWN, BLCA_RequestRushMapClose);
 }

 override void OnMenuUpdate(float tDelta)
 {
  super.OnMenuUpdate(tDelta);
  if (m_BLCA_RushMapCloseBound && GetGame().GetMenuManager().GetTopMenu() == this)
  {
   GetGame().GetInputManager().ActivateContext("GadgetMapContext");
   if (m_BLCA_SectorOverlay) m_BLCA_SectorOverlay.Update(m_MapEntity, BLCA_RushGameModeComponent.Current());
  }
 }

 override void OnMenuClose()
 {
  if (m_BLCA_RushMapCloseBound)
  {
   m_BLCA_RushMapCloseBound = false;
   InputManager input = GetGame().GetInputManager();
   if (input)
   {
    input.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, BLCA_RequestRushMapClose);
    input.RemoveActionListener("MapEscape", EActionTrigger.DOWN, BLCA_RequestRushMapClose);
   }
   GetGame().GetCallqueue().Remove(BLCA_CloseRushMap);
  }
  if (m_BLCA_SectorOverlay) m_BLCA_SectorOverlay.Close();
  m_BLCA_SectorOverlay = null;
  super.OnMenuClose();
 }

 protected void BLCA_RequestRushMapClose()
 {
  if (!m_BLCA_RushMapCloseBound || GetGame().GetMenuManager().GetTopMenu() != this || System.GetTickCount() < m_BLCA_RushMapInputReadyAt) return;

  SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(SCR_PlayerController.GetLocalControlledEntity());
  int delay;
  // A held map gets the stock stow animation first. The menu also works without a held gadget.
  if (gadgets && SCR_MapGadgetComponent.Cast(gadgets.GetHeldGadgetComponent())) delay = 850;
  GetGame().GetCallqueue().Remove(BLCA_CloseRushMap);
  GetGame().GetCallqueue().CallLater(BLCA_CloseRushMap, delay, false);
 }

 protected void BLCA_CloseRushMap()
 {
  if (!m_BLCA_RushMapCloseBound || GetGame().GetMenuManager().GetTopMenu() != this) return;
  SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(SCR_PlayerController.GetLocalControlledEntity());
  if (gadgets && SCR_MapGadgetComponent.Cast(gadgets.GetHeldGadgetComponent()))
   gadgets.SetGadgetMode(gadgets.GetHeldGadget(), EGadgetMode.IN_STORAGE);
  if (GetGame().GetMenuManager().GetTopMenu() == this) Close();
 }
}

modded class SCR_MapMarkersUI
{
 override protected void OnInputMapSelect(float value, EActionTrigger reason)
 {
  if (BLCA_RushGameModeComponent.Current())
  {
   SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
   if (!mapEntity || !mapEntity.IsOpen() || !m_CursorModule) return;
   array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
   if (!widgets) return;
   for (int i = widgets.Count() - 1; i >= 0; i--)
    if (!widgets[i]) widgets.RemoveOrdered(i);
  }
  super.OnInputMapSelect(value, reason);
 }
}
