#ifdef WORKBENCH
[WorkbenchPluginAttribute("Validate BLCA RUSH", "Check standalone resources and authored sector placement", "", "", {"WorldEditor"})]
class BLCA_RushSurvey : WorkbenchPlugin
{
 protected int m_Failures;
 protected bool m_ExitAfterValidation;

 override void RunCommandline()
 {
  m_ExitAfterValidation = true;
  Run();
 }

 override void Run()
 {
  WorldEditor editor = Workbench.GetModule(WorldEditor);
  if (!editor || !editor.SetOpenedResource("{672AAAA8E0FB5982}Worlds/BLCA_Rush_Arland.ent"))
  {
   Print("[BLCA_QA] WORLD_LOAD_FAILED", LogLevel.ERROR);
   if (m_ExitAfterValidation) Workbench.Exit(1);
   return;
  }
  BaseWorld world = editor.GetApi().GetWorld();
  Check(world != null, "ARLAND_WORLD");
  IEntity mode = world.FindEntityByName("BLCA_Mode");
  Check(mode && mode.FindComponent(BLCA_RushGameModeComponent), "RUSH_AUTHORITY");
  Check(world.FindEntityByName("BLCA_Factions") != null, "NATIVE_FACTIONS");
  Check(world.FindEntityByName("BLCA_Loadouts") != null, "NATIVE_LOADOUTS");
  CheckResource("{94AA9386CDB35E4F}Missions/BLCA_Rush_Arland.conf");
  CheckResource("{4DCA6B737E515B96}UI/layouts/BLCA_Rush/BLCA_HUD.layout");
  CheckResource("{B48C0B29FBEBA2A1}UI/Textures/Map/worlds/Arland/Arland.edds");
  CheckResource("{F98F3D2CBA523091}UI/Textures/Map/worlds/Arland/ArlandRasterized.edds");
  CheckResource("{5CC3A6A1318D523D}Prefabs/BLCA_Rush/BLCA_MCOM.et");
  CheckResource("{97550BAC861E518F}Prefabs/BLCA_Rush/BLCA_SquadSpawn.et");
  CheckResource("{44B3E655E2CF5369}Prefabs/BLCA_Rush/BLCA_VoicePublic.et");
  CheckResource("{F4CD71AA6B5E58E1}Prefabs/BLCA_Rush/BLCA_VoiceSquad.et");
  array<string> tags = {"A", "B", "Attack", "Defend", "DefendA", "DefendB"};
  for (int i = 0; i < 4; i++)
  {
   IEntity owner = world.FindEntityByName("BLCA_Sector_" + i);
   Check(owner && owner.FindComponent(BLCA_RushSectorComponent), "SECTOR_" + i);
   foreach (string tag : tags)
   {
    IEntity entity = world.FindEntityByName("BLCA_" + i + "_" + tag);
    Check(entity != null, "ENTITY_" + i + "_" + tag);
    if (!entity) continue;
    vector pos = entity.GetOrigin();
    float ground = editor.GetApi().GetTerrainSurfaceY(pos[0], pos[2]);
    Check(ground > 0.5 && pos[1] >= ground && pos[1] - ground < 1.1, "GROUND_" + i + "_" + tag);
    PrintFormat("[BLCA_POINT] name=%1 x=%2 y=%3 z=%4 ground=%5", entity.GetName(), pos[0], pos[1], pos[2], ground);
   }
  }
  PrintFormat("[BLCA_QA] SURVEY_COMPLETE failures=%1", m_Failures);
  if (m_ExitAfterValidation) Workbench.Exit(m_Failures);
 }

 protected void Check(bool passed, string label)
 {
  PrintFormat("[BLCA_QA] %1=%2", label, passed);
  if (!passed) m_Failures++;
 }

 protected void CheckResource(ResourceName name)
 {
  if (name.EndsWith(".edds"))
  {
   Widget root = GetGame().GetWorkspace().CreateWidgets("{4DCA6B737E515B96}UI/layouts/BLCA_Rush/BLCA_HUD.layout");
   ImageWidget image;
   if (root) image = ImageWidget.Cast(root.FindAnyWidget("MinimapTerrain"));
   Check(image && image.LoadImageTexture(0, name), name);
   if (root) root.RemoveFromHierarchy();
   return;
  }
  Resource resource = Resource.Load(name);
  Check(resource && resource.IsValid(), name);
 }
}
#endif
