[ComponentEditorProps(category: "BLCA Rush", description: "Defines a replicated M-COM interaction target.")]
class BLCA_RushObjectiveComponentClass : ScriptComponentClass {}

[ComponentEditorProps(category: "BLCA Rush", description: "Stable prefab-local visual binding; entity names are not unique across terminals.")]
class BLCA_RushVisualPartClass : ScriptComponentClass {}
class BLCA_RushVisualPart : ScriptComponent
{
 [Attribute("0", desc: "0 console, 1 charge, 2 wreck, 3 indicator light")] int Kind;
}

class BLCA_RushObjectiveComponent : ScriptComponent
{
 [Attribute("0")] int Sector;
 [Attribute("0", desc: "0 is objective A. 1 is objective B.")] int Letter;
 [Attribute("3.25")] float InteractionRadius;
 [RplProp(onRplName: "RefreshVisual")] protected BLCA_ObjectiveState m_VisualState = BLCA_ObjectiveState.INACTIVE;
 protected IEntity m_Console;
 protected IEntity m_Charge;
 protected IEntity m_Wreck;
 protected LightEntity m_Light;
 protected bool m_Blink;

 override void OnPostInit(IEntity owner)
 {
  super.OnPostInit(owner);
  SetEventMask(owner, EntityEvent.INIT);
 }
 override void EOnInit(IEntity owner)
 {
  for (IEntity child = owner.GetChildren(); child; child = child.GetSibling())
  {
   BLCA_RushVisualPart part = BLCA_RushVisualPart.Cast(child.FindComponent(BLCA_RushVisualPart));
   if (!part) continue;
   if (part.Kind == 0) m_Console = child;
   else if (part.Kind == 1) m_Charge = child;
   else if (part.Kind == 2) m_Wreck = child;
   else if (part.Kind == 3) m_Light = LightEntity.Cast(child);
  }
  RefreshVisual();
 }
 void SetVisualState_S(BLCA_ObjectiveState state)
 {
  if (!Replication.IsServer() || state == m_VisualState) return;
  m_VisualState = state;
  RefreshVisual();
  Replication.BumpMe();
 }
 BLCA_ObjectiveState VisualState() { return m_VisualState; }
 protected void ShowPart(IEntity part, bool visible)
 {
  if (!part) return;
  if (visible) part.SetFlags(EntityFlags.VISIBLE, true);
  else part.ClearFlags(EntityFlags.VISIBLE, true);
 }
 protected void RefreshVisual()
 {
  if (!GetGame()) return;
  GetGame().GetCallqueue().Remove(Blink);
  bool destroyed = m_VisualState == BLCA_ObjectiveState.DESTROYED;
  bool armed = m_VisualState == BLCA_ObjectiveState.ARMED || m_VisualState == BLCA_ObjectiveState.DEFUSING;
  ShowPart(m_Console, !destroyed);
  ShowPart(m_Wreck, destroyed);
  ShowPart(m_Charge, armed || m_VisualState == BLCA_ObjectiveState.ARMING);
  if (m_Light)
  {
   m_Light.SetEnabled(!destroyed && m_VisualState != BLCA_ObjectiveState.INACTIVE && !System.IsConsoleApp());
   if (armed) m_Light.SetColor(Color.FromRGBA(255, 25, 10, 255), 1);
   else m_Light.SetColor(Color.FromRGBA(20, 200, 40, 255), 1);
  }
  if (armed && !System.IsConsoleApp()) GetGame().GetCallqueue().CallLater(Blink, 400, true);
 }
 protected void Blink()
 {
  m_Blink = !m_Blink;
  if (m_Light) m_Light.SetEnabled(m_Blink);
 }
 override void OnDelete(IEntity owner)
 {
  if (GetGame()) GetGame().GetCallqueue().Remove(Blink);
  super.OnDelete(owner);
 }




 bool InRange(IEntity player)
 {
  if (!player || !GetOwner()) return false;
  return vector.Distance(player.GetOrigin(), GetOwner().GetOrigin()) <= InteractionRadius;
 }

 static string Label(int letter)
 {
  if (letter == 0) return "A";
  return "B";
 }
}

class BLCA_RushObjectiveBlast
{
 static const float LETHAL_RADIUS = 5;
 static const float OUTER_RADIUS = 10;
 static const float OUTER_RADIUS_SQ = 100;
 static const ResourceName SOUND = "{15572A81FA5612BB}Sounds/Weapons/Explosives/DemoBlocks/_SharedData/Weapons_Explosives_DemoBlock_Generic.acp";
 static const ResourceName PARTICLE = "{9D8D204CCCC44A38}Particles/Weapon/Explosion_M112.ptc";

 // A clear blast can kill at close range. The outer ring falls to zero.
 static float DamageFraction(float distance)
 {
  if (distance <= LETHAL_RADIUS) return 1;
  if (distance >= OUTER_RADIUS) return 0;
  return 0.65 * (OUTER_RADIUS - distance) / (OUTER_RADIUS - LETHAL_RADIUS);
 }

 static void Damage(IEntity source, vector origin, IEntity victim)
 {
  if (!Replication.IsServer() || !source || !victim || victim.IsDeleted()) return;
  ChimeraCharacter character = ChimeraCharacter.Cast(victim);
  if (!character || !character.GetCharacterController() || character.GetCharacterController().IsDead()) return;
  SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.BLCA_RushCombatManager(victim);
  if (!damage || damage.IsDestroyed() || !damage.IsDamageHandlingEnabled()) return;

  float fraction = DamageFraction(vector.Distance(origin, victim.GetOrigin()));
  if (fraction <= 0) return;

  vector target = victim.GetOrigin() + "0 0.9 0";
  array<IEntity> ignored = {source, victim};
  TraceParam trace = new TraceParam();
  trace.Start = origin;
  trace.End = target;
  trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
  trace.LayerMask = EPhysicsLayerPresets.Projectile;
  trace.ExcludeArray = ignored;
  if (GetGame().GetWorld().TraceMove(trace, null) < 1) return;

  Instigator instigator = Instigator.CreateInstigator(null);
  if (fraction >= 1)
  {
   damage.Kill(instigator);
   return;
  }

  HitZone health = damage.GetDefaultHitZone();
  if (!health) return;
  BaseDamageContext context = new BaseDamageContext();
  context.damageType = EDamageType.EXPLOSIVE;
  context.damageValue = health.GetMaxHealth() * fraction;
  context.hitEntity = victim;
  context.struckHitZone = health;
  context.damageSource = source;
  context.instigator = instigator;
  context.hitPosition = target;
  vector direction = target - origin;
  direction.Normalize();
  context.hitDirection = direction;
  context.hitNormal = "0 1 0";
  damage.HandleDamage(context);
 }

 static void Play(vector position)
 {
  if (System.IsConsoleApp() || !GetGame() || !GetGame().GetWorld()) return;
  vector transform[4];
  Math3D.MatrixIdentity4(transform);
  transform[3] = position;
  AudioSystem.PlayEvent(SOUND, "SOUND_EXPLOSION", transform);

  // Audio has its own native attenuation. Only the local particle is culled.
  if (AudioSystem.GetDistance(position) > 250) return;
  ParticleEffectEntitySpawnParams params = new ParticleEffectEntitySpawnParams();
  params.TransformMode = ETransformMode.WORLD;
  params.Transform[3] = position;
  ParticleEffectEntity effect = ParticleEffectEntity.SpawnParticleEffect(PARTICLE, params);
  if (effect) GetGame().GetCallqueue().CallLater(DeleteEffect, 9000, false, effect);
 }

 protected static void DeleteEffect(ParticleEffectEntity effect)
 {
  if (effect && !effect.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(effect);
 }
}

class BLCA_RushObjectiveRound
{
 BLCA_ObjectiveState State;
 int Actor;
 IEntity Body;
 float StartTime;
 float ExplodesAt;
 string PlanterKey;

 void Cancel()
 {
  if (State == BLCA_ObjectiveState.ARMING) State = BLCA_ObjectiveState.ACTIVE;
  if (State == BLCA_ObjectiveState.DEFUSING) State = BLCA_ObjectiveState.ARMED;
  Actor = 0;
  Body = null;
  StartTime = 0;
 }

 void Reset(BLCA_ObjectiveState state)
 {
  Cancel();
  State = state;
  ExplodesAt = 0;
  PlanterKey = "";
 }

 bool HasCharge()
 {
  return IsArmedState(State);
 }

 static bool IsArmedState(BLCA_ObjectiveState state)
 {
  return state == BLCA_ObjectiveState.ARMED || state == BLCA_ObjectiveState.DEFUSING;
 }
}

class BLCA_RushObjectiveAction : ScriptedUserAction
{
 [Attribute("0")] protected bool m_Defuse;

 override bool HasLocalEffectOnlyScript() { return true; }

 override bool CanBeShownScript(IEntity user)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  BLCA_RushObjectiveComponent objective = Objective();
  if (!mode || !objective || !user || !mode.IsActiveSector(objective.Sector)) return false;
  int team = mode.TeamOfEntity(user);
  if (m_Defuse) return team == 1;
  return team == 0;
 }

 override bool CanBePerformedScript(IEntity user)
 {
  if (!CanBeShownScript(user)) return false;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  BLCA_RushObjectiveComponent objective = Objective();
  if (!objective.InRange(user) || !BLCA_RushGameModeComponent.CanInteract(user)) return false;
  BLCA_ObjectiveState state = mode.ObjectiveState(objective.Letter);
  if (m_Defuse) return state == BLCA_ObjectiveState.ARMED || (state == BLCA_ObjectiveState.DEFUSING && IsInProgress());
  if (mode.PhaseSeconds() == 0) return false;
  return state == BLCA_ObjectiveState.ACTIVE || state == BLCA_ObjectiveState.DEFUSED || (state == BLCA_ObjectiveState.ARMING && IsInProgress());
 }

 override bool GetActionNameScript(out string outName)
 {
  BLCA_RushObjectiveComponent objective = Objective();
  if (!objective) return false;
  outName = "Hold to plant M-COM " + BLCA_RushObjectiveComponent.Label(objective.Letter);
  if (m_Defuse) outName = "Hold to defuse M-COM " + BLCA_RushObjectiveComponent.Label(objective.Letter);
  return true;
 }

 override float GetActionProgressScript(float fProgress, float timeSlice)
 {
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode) return super.GetActionProgressScript(fProgress, timeSlice);
  return fProgress + timeSlice * GetActionDuration() / mode.HoldSeconds(m_Defuse);
 }

 override void OnActionStart(IEntity pUserEntity) { Send(pUserEntity, 0); }
 override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) { Send(pUserEntity, 1); }
 override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity) { Send(pUserEntity, 2); }

 protected BLCA_RushObjectiveComponent Objective()
 {
  if (!GetOwner()) return null;
  return BLCA_RushObjectiveComponent.Cast(GetOwner().FindComponent(BLCA_RushObjectiveComponent));
 }

 protected void Send(IEntity user, int step)
 {
  SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
  if (!controller || controller.GetControlledEntity() != user || !GetOwner()) return;
  RplComponent replication = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
  if (replication) controller.BLCA_RushRequestObjective(replication.Id(), m_Defuse, step);
 }
}

modded class SCR_PlayerController
{
 void BLCA_RushRequestObjective(RplId id, bool defuse, int step)
 {
  if (Replication.IsServer()) BLCA_RushRpcObjective(id, defuse, step);
  else Rpc(BLCA_RushRpcObjective, id, defuse, step);
 }

 [RplRpc(RplChannel.Reliable, RplRcver.Server)]
 protected void BLCA_RushRpcObjective(RplId id, bool defuse, int step)
 {
  if (!Replication.IsServer() || GetPlayerId() <= 0 || GetGame().GetPlayerManager().GetPlayerController(GetPlayerId()) != this) return;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode) return;
  RplComponent replication = RplComponent.Cast(Replication.FindItem(id));
  if (!replication || !replication.GetEntity()) return;
  BLCA_RushObjectiveComponent objective = BLCA_RushObjectiveComponent.Cast(replication.GetEntity().FindComponent(BLCA_RushObjectiveComponent));
  if (objective) mode.RequestObjective(GetPlayerId(), GetControlledEntity(), objective, defuse, step);
 }
}
