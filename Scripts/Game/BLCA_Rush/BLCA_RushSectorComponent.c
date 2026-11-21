[ComponentEditorProps(category: "BLCA Rush", description: "Links a sector to its terminals and native spawn points.")]
class BLCA_RushSectorComponentClass : ScriptComponentClass {}

class BLCA_RushSectorComponent : ScriptComponent
{
 static ref array<BLCA_RushSectorComponent> s_Sectors = {};

 [Attribute("0")] int Index;
 [Attribute("Sector")] string Title;
 [Attribute("")] string ObjectiveA;
 [Attribute("")] string ObjectiveB;
 [Attribute("")] string AttackSpawn;
 [Attribute("")] string DefendSpawn;
 [Attribute("", desc: "Optional defender deployment point near M-COM A.")] string DefendSpawnA;
 [Attribute("", desc: "Optional defender deployment point near M-COM B.")] string DefendSpawnB;
 [Attribute("325")] float BoundaryRadius;

 BLCA_RushObjectiveComponent A;
 BLCA_RushObjectiveComponent B;
 SCR_SpawnPoint Attack;
 SCR_SpawnPoint Defend;
 SCR_SpawnPoint DefendA;
 SCR_SpawnPoint DefendB;
 protected static const float SPAWN_APPROACH_RADIUS = 100;
 protected static const float SPAWN_PROTECTION_RADIUS_SQ = 2500;
 protected vector m_AttackApproach;
 protected vector m_DefendApproach;

 override void OnPostInit(IEntity owner)
 {
  super.OnPostInit(owner);
  SetEventMask(owner, EntityEvent.INIT);
 }

 override void EOnInit(IEntity owner)
 {
  if (GetGame().InPlayMode() && !s_Sectors.Contains(this)) s_Sectors.Insert(this);
 }

 override void OnDelete(IEntity owner)
 {
  s_Sectors.RemoveItem(this);
  super.OnDelete(owner);
 }

 bool Resolve()
 {
  BaseWorld world = GetGame().GetWorld();
  IEntity first = world.FindEntityByName(ObjectiveA);
  IEntity second = world.FindEntityByName(ObjectiveB);
  if (!first || !second || first == second) return false;
  A = BLCA_RushObjectiveComponent.Cast(first.FindComponent(BLCA_RushObjectiveComponent));
  B = BLCA_RushObjectiveComponent.Cast(second.FindComponent(BLCA_RushObjectiveComponent));
  Attack = SCR_SpawnPoint.Cast(world.FindEntityByName(AttackSpawn));
  Defend = SCR_SpawnPoint.Cast(world.FindEntityByName(DefendSpawn));
  if (!A || !B || !Attack || !Defend || Attack == Defend || A.Sector != Index || B.Sector != Index || A.Letter != 0 || B.Letter != 1) return false;
  if (!DefendSpawnA.IsEmpty()) DefendA = SCR_SpawnPoint.Cast(world.FindEntityByName(DefendSpawnA));
  if (!DefendSpawnB.IsEmpty()) DefendB = SCR_SpawnPoint.Cast(world.FindEntityByName(DefendSpawnB));
  vector center = GetOwner().GetOrigin();
  m_AttackApproach = Attack.GetOrigin() - center;
  m_DefendApproach = Defend.GetOrigin() - center;
  m_AttackApproach[1] = 0;
  m_DefendApproach[1] = 0;
  return true;
 }

 bool ContainsSpawn(vector position, int team)
 {
  SCR_SpawnPoint spawn;
  if (team == 0) spawn = Attack;
  if (team == 1) spawn = Defend;
  return spawn && vector.DistanceSqXZ(position, spawn.GetOrigin()) <= SPAWN_PROTECTION_RADIUS_SQ;
 }

 bool Contains(vector position, float margin = 0)
 {
  vector delta = position - GetOwner().GetOrigin();
  delta[1] = 0;
  return ContainsOffset(delta, margin);
 }

 bool ContainsObjectiveArea(vector position)
 {
  return WithinObjectiveRadius(vector.DistanceSqXZ(position, GetOwner().GetOrigin()), 0);
 }

 protected bool WithinObjectiveRadius(float distanceSq, float margin)
 {
  float radius = Math.Max(0, BoundaryRadius - margin);
  return distanceSq <= radius * radius;
 }

 protected bool ContainsOffset(vector delta, float margin)
 {
  if (WithinObjectiveRadius(delta.LengthSq(), margin)) return true;
  float approachRadius = Math.Max(0, SPAWN_APPROACH_RADIUS - margin);
  float radiusSq = approachRadius * approachRadius;
  return WithinApproach(delta, m_AttackApproach, radiusSq) || WithinApproach(delta, m_DefendApproach, radiusSq);
 }

 protected bool WithinApproach(vector delta, vector approach, float radiusSq)
 {
  float lengthSq = approach.LengthSq();
  if (lengthSq <= 0.01) return false;
  float fraction = Math.Clamp(vector.Dot(delta, approach) / lengthSq, 0, 1);
  return vector.DistanceSq(delta, approach * fraction) <= radiusSq;
 }

 void BuildBoundaryOutline(notnull array<vector> outline)
 {
  outline.Clear();
  vector center = GetOwner().GetOrigin();
  float outerLimit = Math.Max(BoundaryRadius, Math.Max(m_AttackApproach.Length(), m_DefendApproach.Length()) + SPAWN_APPROACH_RADIUS);
  for (int i = 0; i < 48; i++)
  {
   float angle = i * Math.PI2 / 48;
   vector direction = Vector(Math.Cos(angle), 0, Math.Sin(angle));
   float inside = BoundaryRadius;
   float outside = outerLimit;
   // The objective circle and both approaches share the center, so each ray crosses the edge once.
   for (int step = 0; step < 10 && outside - inside > 0.5; step++)
   {
    float distance = (inside + outside) * 0.5;
    if (ContainsOffset(direction * distance, 0)) inside = distance;
    else outside = distance;
   }
   outline.Insert(center + direction * inside);
  }
 }
}
